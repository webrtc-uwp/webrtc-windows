/*
*  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
*
*  Use of this source code is governed by a BSD-style license
*  that can be found in the LICENSE file in the root of the source
*  tree. An additional intellectual property rights grant can be found
*  in the file PATENTS.  All contributing project authors may
*  be found in the AUTHORS file in the root of the source tree.
*/

#include "third_party/winuwp_h264/H264Encoder/H264Encoder.h"

#include <Windows.h>
#include <stdlib.h>
#include <ppltasks.h>
#include <mfapi.h>
#include <robuffer.h>
#include <wrl.h>
#include <mfidl.h>
#include <codecapi.h>
#include <mfreadwrite.h>
#include <wrl\implements.h>
#include <sstream>
#include <vector>
#include <iomanip>

#include "H264StreamSink.h"
#include "H264MediaSink.h"
#include "../Utils/Utils.h"
#include "modules/video_coding/include/video_codec_interface.h"
#include "rtc_base/timeutils.h"
#include "libyuv/convert.h"
#include "rtc_base/logging.h"
#include "rtc_base/win32.h"


#pragma comment(lib, "mfreadwrite")
#pragma comment(lib, "mfplat")
#pragma comment(lib, "mfuuid.lib")

namespace webrtc {

// QP scaling thresholds.
static const int kLowH264QpThreshold = 24;
static const int kHighH264QpThreshold = 37;
//////////////////////////////////////////
// H264 WinUWP Encoder Implementation
//////////////////////////////////////////

WinUWPH264EncoderImpl::WinUWPH264EncoderImpl()
{
}

WinUWPH264EncoderImpl::~WinUWPH264EncoderImpl() {
  Release();
}

int WinUWPH264EncoderImpl::InitEncode(const VideoCodec* codec_settings,
  int /*number_of_cores*/,
  size_t /*maxPayloadSize */) {

  if (!codec_settings || codec_settings->codecType != kVideoCodecH264) {
	  RTC_LOG(LS_ERROR) << "H264 UWP Encoder not registered as H264 codec";
	  return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  }
  if (codec_settings->maxFramerate == 0) {
	  RTC_LOG(LS_ERROR) << "H264 UWP Encoder has no framerate defined";
	  return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  }
  if (codec_settings->width < 1 || codec_settings->height < 1) {
	  RTC_LOG(LS_ERROR) << "H264 UWP Encoder has no valid frame size defined";
	  return WEBRTC_VIDEO_CODEC_ERR_PARAMETER;
  }

  width_ = codec_settings->width;
  height_ = codec_settings->height;
  target_bps_ = codec_settings->targetBitrate > 0 ? codec_settings->targetBitrate * 1000 : width_ * height_ * 2.0;
  max_frame_rate_ = codec_settings->maxFramerate;
  mode_ = codec_settings->mode;
  frame_dropping_on_ = codec_settings->H264().frameDroppingOn;
  key_frame_interval_ = codec_settings->H264().keyFrameInterval;
  // Codec_settings uses kbits/second; encoder uses bits/second.
  max_bitrate_ = codec_settings->maxBitrate * 1000;
  if (target_bps_ == 0)
	  target_bps_ = codec_settings->startBitrate * 1000;
  else
	  target_bps_ = codec_settings->targetBitrate * 1000;
  return InitEncoderWithSettings(codec_settings);
}

int WinUWPH264EncoderImpl::InitEncoderWithSettings(const VideoCodec* codec_settings) {
  HRESULT hr = S_OK;

  rtc::CritScope lock(&crit_);

  ON_SUCCEEDED(MFStartup(MF_VERSION));

  // output media type (h264)
  ComPtr<IMFMediaType> mediaTypeOut;
  ON_SUCCEEDED(MFCreateMediaType(&mediaTypeOut));
  ON_SUCCEEDED(mediaTypeOut->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
  ON_SUCCEEDED(mediaTypeOut->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264));
  // Lumia 635 and Lumia 1520 Windows phones don't work well
  // with constrained baseline profile.
  //ON_SUCCEEDED(mediaTypeOut->SetUINT32(MF_MT_MPEG2_PROFILE, eAVEncH264VProfile_ConstrainedBase));

  // Weight*Height*2 kbit represents a good balance between video quality and
  // the bandwidth that a 620 Windows phone can handle.
  ON_SUCCEEDED(mediaTypeOut->SetUINT32(
    MF_MT_AVG_BITRATE, target_bps_));
  ON_SUCCEEDED(mediaTypeOut->SetUINT32(
    MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive));
  ON_SUCCEEDED(MFSetAttributeSize(mediaTypeOut.Get(),
    MF_MT_FRAME_SIZE, width_, height_));
  ON_SUCCEEDED(MFSetAttributeRatio(mediaTypeOut.Get(),
    MF_MT_FRAME_RATE, max_frame_rate_, 1));

  // input media type (nv12)
  ComPtr<IMFMediaType> mediaTypeIn;
  ON_SUCCEEDED(MFCreateMediaType(&mediaTypeIn));
  ON_SUCCEEDED(mediaTypeIn->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
  ON_SUCCEEDED(mediaTypeIn->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12));
  ON_SUCCEEDED(mediaTypeIn->SetUINT32(
    MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive));
  ON_SUCCEEDED(mediaTypeIn->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE));
  ON_SUCCEEDED(MFSetAttributeSize(mediaTypeIn.Get(),
    MF_MT_FRAME_SIZE, width_, height_));
  ON_SUCCEEDED(MFSetAttributeRatio(mediaTypeIn.Get(),
    MF_MT_FRAME_RATE, max_frame_rate_, 1));

  // Create the media sink
  ON_SUCCEEDED(Microsoft::WRL::MakeAndInitialize<H264MediaSink>(&mediaSink_));

  // SinkWriter creation attributes
  ON_SUCCEEDED(MFCreateAttributes(&sinkWriterCreationAttributes_, 1));
  ON_SUCCEEDED(sinkWriterCreationAttributes_->SetUINT32(
    MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE));
  ON_SUCCEEDED(sinkWriterCreationAttributes_->SetUINT32(
    MF_SINK_WRITER_DISABLE_THROTTLING, TRUE));
  ON_SUCCEEDED(sinkWriterCreationAttributes_->SetUINT32(
    MF_LOW_LATENCY, TRUE));

  // Create the sink writer
  ON_SUCCEEDED(MFCreateSinkWriterFromMediaSink(mediaSink_.Get(),
    sinkWriterCreationAttributes_.Get(), &sinkWriter_));

  // Add the h264 output stream to the writer
  ON_SUCCEEDED(sinkWriter_->AddStream(mediaTypeOut.Get(), &streamIndex_));

  // SinkWriter encoder properties
  ON_SUCCEEDED(MFCreateAttributes(&sinkWriterEncoderAttributes_, 1));
  ON_SUCCEEDED(sinkWriter_->SetInputMediaType(streamIndex_, mediaTypeIn.Get(), nullptr));

  // Register this as the callback for encoded samples.
  ON_SUCCEEDED(mediaSink_->RegisterEncodingCallback(this));

  ON_SUCCEEDED(sinkWriter_->BeginWriting());

  codec_ = *codec_settings;

  if (SUCCEEDED(hr)) {
    inited_ = true;
    lastTimeSettingsChanged_ = rtc::TimeMillis();
    return WEBRTC_VIDEO_CODEC_OK;
  } else {
    return hr;
  }
}

int WinUWPH264EncoderImpl::RegisterEncodeCompleteCallback(
  EncodedImageCallback* callback) {
  rtc::CritScope lock(&callbackCrit_);
  encodedCompleteCallback_ = callback;
  return WEBRTC_VIDEO_CODEC_OK;
}

int WinUWPH264EncoderImpl::Release() {
  // Use a temporary sink variable to prevent lock inversion
  // between the shutdown call and OnH264Encoded() callback.
  ComPtr<H264MediaSink> tmpMediaSink;

  {
    rtc::CritScope lock(&crit_);
    sinkWriter_.Reset();
    if (mediaSink_ != nullptr) {
      tmpMediaSink = mediaSink_;
    }
    sinkWriterCreationAttributes_.Reset();
    sinkWriterEncoderAttributes_.Reset();
    mediaSink_.Reset();
    startTime_ = 0;
    lastTimestampHns_ = 0;
    firstFrame_ = true;
    inited_ = false;
    framePendingCount_ = 0;
    _sampleAttributeQueue.clear();
    rtc::CritScope callbackLock(&callbackCrit_);
    encodedCompleteCallback_ = nullptr;
  }

  if (tmpMediaSink != nullptr) {
    tmpMediaSink->Shutdown();
  }
  return WEBRTC_VIDEO_CODEC_OK;
}

ComPtr<IMFSample> WinUWPH264EncoderImpl::FromVideoFrame(const VideoFrame& frame) {
  HRESULT hr = S_OK;
  ComPtr<IMFSample> sample;
  ON_SUCCEEDED(MFCreateSample(sample.GetAddressOf()));

  ComPtr<IMFAttributes> sampleAttributes;
  ON_SUCCEEDED(sample.As(&sampleAttributes));

  rtc::scoped_refptr<I420BufferInterface> frameBuffer =
      static_cast<I420BufferInterface*>(frame.video_frame_buffer().get());

  if (SUCCEEDED(hr)) {
    auto totalSize = frameBuffer->StrideY() * frameBuffer->height() +
      frameBuffer->StrideU() * (frameBuffer->height() + 1) / 2 +
      frameBuffer->StrideV() * (frameBuffer->height() + 1) / 2;

    ComPtr<IMFMediaBuffer> mediaBuffer;
    ON_SUCCEEDED(MFCreateMemoryBuffer(totalSize, mediaBuffer.GetAddressOf()));

    BYTE* destBuffer = nullptr;
    if (SUCCEEDED(hr)) {
      DWORD cbMaxLength;
      DWORD cbCurrentLength;
      ON_SUCCEEDED(mediaBuffer->Lock(
        &destBuffer, &cbMaxLength, &cbCurrentLength));
    }

    if (SUCCEEDED(hr)) {
      BYTE* destUV = destBuffer +
        (frameBuffer->StrideY() * frameBuffer->height());
      libyuv::I420ToNV12(
        frameBuffer->DataY(), frameBuffer->StrideY(),
        frameBuffer->DataU(), frameBuffer->StrideU(),
        frameBuffer->DataV(), frameBuffer->StrideV(),
        destBuffer, frameBuffer->StrideY(),
        destUV, frameBuffer->StrideY(),
        frameBuffer->width(),
        frameBuffer->height());
    }

    {
      if (frameBuffer->width() != (int)width_ || frameBuffer->height() != (int)height_) {
        EncodedImageCallback* tempCallback = encodedCompleteCallback_;
        Release();
        {
          rtc::CritScope lock(&callbackCrit_);
          encodedCompleteCallback_ = tempCallback;
        }

        width_ = frameBuffer->width();
        height_ = frameBuffer->height();
        InitEncoderWithSettings(&codec_);
        RTC_LOG(LS_WARNING) << "Resolution changed to: " << frameBuffer->width() << "x" << frameBuffer->height();
      }
    }

    if (firstFrame_) {
      firstFrame_ = false;
      startTime_ = frame.timestamp();
    }

    auto timestampHns = ((frame.timestamp() - startTime_) / 90) * 1000 * 10;
    ON_SUCCEEDED(sample->SetSampleTime(timestampHns));

    if (SUCCEEDED(hr)) {
      auto durationHns = timestampHns - lastTimestampHns_;
      hr = sample->SetSampleDuration(durationHns);
    }

    if (SUCCEEDED(hr)) {
      lastTimestampHns_ = timestampHns;

      // Cache the frame attributes to get them back after the encoding.
      CachedFrameAttributes frameAttributes;
      frameAttributes.timestamp = frame.timestamp();
      frameAttributes.ntpTime = frame.ntp_time_ms();
      frameAttributes.captureRenderTime = frame.render_time_ms();
      frameAttributes.frameWidth = frame.width();
      frameAttributes.frameHeight = frame.height();
      _sampleAttributeQueue.push(timestampHns, frameAttributes);
    }

    ON_SUCCEEDED(mediaBuffer->SetCurrentLength(
      frameBuffer->width() * frameBuffer->height() * 3 / 2));

    if (destBuffer != nullptr) {
      mediaBuffer->Unlock();
    }

    ON_SUCCEEDED(sample->AddBuffer(mediaBuffer.Get()));

    if (lastFrameDropped_) {
      lastFrameDropped_ = false;
      sampleAttributes->SetUINT32(MFSampleExtension_Discontinuity, TRUE);
    }
  }

  return sample;
}

int WinUWPH264EncoderImpl::Encode(
  const VideoFrame& frame,
  const CodecSpecificInfo* codec_specific_info,
  const std::vector<FrameType>* frame_types) {
  {
      rtc::CritScope lock(&crit_);
      if (!inited_) {
      return -1;
    }
  }


  if (frame_types != nullptr) {
    for (auto frameType : *frame_types) {
      if (frameType == kVideoFrameKey) {
        RTC_LOG(LS_INFO) << "Key frame requested in H264 encoder.";
        ComPtr<IMFSinkWriterEncoderConfig> encoderConfig;
        sinkWriter_.As(&encoderConfig);
        ComPtr<IMFAttributes> encoderAttributes;
        MFCreateAttributes(&encoderAttributes, 1);
        encoderAttributes->SetUINT32(CODECAPI_AVEncVideoForceKeyFrame, TRUE);
        encoderConfig->PlaceEncodingParameters(streamIndex_, encoderAttributes.Get());
        break;
      }
    }
  }

  HRESULT hr = S_OK;

  codecSpecificInfo_ = codec_specific_info;

  ComPtr<IMFSample> sample;
  {
    rtc::CritScope lock(&crit_);
    if (_sampleAttributeQueue.size() > 2) {
      return WEBRTC_VIDEO_CODEC_OK;
    }
    sample = FromVideoFrame(frame);
  }

  ON_SUCCEEDED(sinkWriter_->WriteSample(streamIndex_, sample.Get()));

  rtc::CritScope lock(&crit_);
  // Some threads online mention this is useful to do regularly.
  ++frameCount_;
  if (frameCount_ % 30 == 0) {
    ON_SUCCEEDED(sinkWriter_->NotifyEndOfSegment(streamIndex_));
  }

  ++framePendingCount_;
  return WEBRTC_VIDEO_CODEC_OK;
}

void WinUWPH264EncoderImpl::OnH264Encoded(ComPtr<IMFSample> sample) {
  DWORD totalLength;
  HRESULT hr = S_OK;
  ON_SUCCEEDED(sample->GetTotalLength(&totalLength));

  ComPtr<IMFMediaBuffer> buffer;
  hr = sample->GetBufferByIndex(0, &buffer);

  if (SUCCEEDED(hr)) {
    BYTE* byteBuffer;
    DWORD maxLength;
    DWORD curLength;
    hr = buffer->Lock(&byteBuffer, &maxLength, &curLength);
    if (FAILED(hr)) {
      return;
    }
    if (curLength == 0) {
      RTC_LOG(LS_WARNING) << "Got empty sample.";
      buffer->Unlock();
      return;
    }
    std::vector<byte> sendBuffer;
    sendBuffer.resize(curLength);
    memcpy(sendBuffer.data(), byteBuffer, curLength);
    hr = buffer->Unlock();
    if (FAILED(hr)) {
      return;
    }

    // sendBuffer is not copied here.
    EncodedImage encodedImage(sendBuffer.data(), curLength, curLength);

    ComPtr<IMFAttributes> sampleAttributes;
    hr = sample.As(&sampleAttributes);
    if (SUCCEEDED(hr)) {
      UINT32 cleanPoint;
      hr = sampleAttributes->GetUINT32(
        MFSampleExtension_CleanPoint, &cleanPoint);
      if (SUCCEEDED(hr) && cleanPoint) {
        encodedImage._completeFrame = true;
        encodedImage._frameType = kVideoFrameKey;
      }
    }

    // Scan for and create mark all fragments.
    RTPFragmentationHeader fragmentationHeader;
    uint32_t fragIdx = 0;
    for (uint32_t i = 0; i < sendBuffer.size() - 5; ++i) {
      byte* ptr = sendBuffer.data() + i;
      int prefixLengthFound = 0;
      if (ptr[0] == 0x00 && ptr[1] == 0x00 && ptr[2] == 0x00 && ptr[3] == 0x01
        && ((ptr[4] & 0x1f) != 0x09 /* ignore access unit delimiters */)) {
        prefixLengthFound = 4;
      } else if (ptr[0] == 0x00 && ptr[1] == 0x00 && ptr[2] == 0x01
        && ((ptr[3] & 0x1f) != 0x09 /* ignore access unit delimiters */)) {
        prefixLengthFound = 3;
      }

      // Found a key frame, mark is as such in case
      // MFSampleExtension_CleanPoint wasn't set on the sample.
      if (prefixLengthFound > 0 && (ptr[prefixLengthFound] & 0x1f) == 0x05) {
        encodedImage._completeFrame = true;
        encodedImage._frameType = kVideoFrameKey;
      }

      if (prefixLengthFound > 0) {
        fragmentationHeader.VerifyAndAllocateFragmentationHeader(fragIdx + 1);
        fragmentationHeader.fragmentationOffset[fragIdx] = i + prefixLengthFound;
        fragmentationHeader.fragmentationLength[fragIdx] = 0;  // We'll set that later
        // Set the length of the previous fragment.
        if (fragIdx > 0) {
          fragmentationHeader.fragmentationLength[fragIdx - 1] =
            i - fragmentationHeader.fragmentationOffset[fragIdx - 1];
        }
        fragmentationHeader.fragmentationPlType[fragIdx] = 0;
        fragmentationHeader.fragmentationTimeDiff[fragIdx] = 0;
        ++fragIdx;
        i += 5;
      }
    }
    // Set the length of the last fragment.
    if (fragIdx > 0) {
      fragmentationHeader.fragmentationLength[fragIdx - 1] =
        sendBuffer.size() -
        fragmentationHeader.fragmentationOffset[fragIdx - 1];
    }

    {
      rtc::CritScope lock(&callbackCrit_);
      --framePendingCount_;
      if (encodedCompleteCallback_ == nullptr) {
        return;
      }

      LONGLONG sampleTimestamp;
      sample->GetSampleTime(&sampleTimestamp);

      CachedFrameAttributes frameAttributes;
      if (_sampleAttributeQueue.pop(sampleTimestamp, frameAttributes)) {
        encodedImage.SetTimestamp(frameAttributes.timestamp);
        encodedImage.ntp_time_ms_ = frameAttributes.ntpTime;
        encodedImage.capture_time_ms_ = frameAttributes.captureRenderTime;
        encodedImage._encodedWidth = frameAttributes.frameWidth;
        encodedImage._encodedHeight = frameAttributes.frameHeight;
      }
      else {
        // No point in confusing the callback with a frame that doesn't
        // have correct attributes.
        return;
      }

	  if (encodedCompleteCallback_ != nullptr) {
		CodecSpecificInfo codecSpecificInfo;
		codecSpecificInfo.codecType = webrtc::kVideoCodecH264;
		codecSpecificInfo.codecSpecific.H264.packetization_mode = H264PacketizationMode::NonInterleaved;
		encodedCompleteCallback_->OnEncodedImage(
		  encodedImage, &codecSpecificInfo, &fragmentationHeader);
	  }
    }
  }
}

int WinUWPH264EncoderImpl::SetChannelParameters(
  uint32_t packetLoss, int64_t rtt) {
  return WEBRTC_VIDEO_CODEC_OK;
}

#define DYNAMIC_FPS
#define DYNAMIC_BITRATE

int WinUWPH264EncoderImpl::SetRates(
  uint32_t new_bitrate_kbit, uint32_t new_framerate) {
  RTC_LOG(LS_INFO) << "WinUWPH264EncoderImpl::SetRates("
    << new_bitrate_kbit << "kbit " << new_framerate << "fps)";

  // This may happen.  Ignore it.
  if (new_framerate == 0) {
    return WEBRTC_VIDEO_CODEC_OK;
  }

  rtc::CritScope lock(&crit_);
  if (sinkWriter_ == nullptr) {
    return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  }

  bool bitrateUpdated = false;
  bool fpsUpdated = false;

#ifdef DYNAMIC_BITRATE
  if (target_bps_ != (new_bitrate_kbit * 1000)) {
    target_bps_ = new_bitrate_kbit * 1000;
    bitrateUpdated = true;
  }
#endif

#ifdef DYNAMIC_FPS
  // Fps changes seems to be expensive, make it granular to several frames per second.
  if (max_frame_rate_ != new_framerate && std::abs((int)max_frame_rate_ - (int)new_framerate) > 5) {
    max_frame_rate_ = new_framerate;
    fpsUpdated = true;
  }
#endif

  if (bitrateUpdated || fpsUpdated) {
    if ((rtc::TimeMillis() - lastTimeSettingsChanged_) < 15000) {
      RTC_LOG(LS_INFO) << "Last time settings changed was too soon, skipping this SetRates().\n";
      return WEBRTC_VIDEO_CODEC_OK;
    }

    EncodedImageCallback* tempCallback = encodedCompleteCallback_;
    Release();
    {
      rtc::CritScope lock(&callbackCrit_);
      encodedCompleteCallback_ = tempCallback;
    }
    InitEncoderWithSettings(&codec_);
  }

  return WEBRTC_VIDEO_CODEC_OK;
}

VideoEncoder::ScalingSettings WinUWPH264EncoderImpl::GetScalingSettings() const {
  return ScalingSettings(kLowH264QpThreshold, kHighH264QpThreshold);
}

const char* WinUWPH264EncoderImpl::ImplementationName() const {
  return "H264_MediaFoundation";
}

}  // namespace webrtc
