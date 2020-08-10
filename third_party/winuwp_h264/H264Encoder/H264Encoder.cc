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

std::atomic<WinUWPH264EncoderImpl::FrameHeightRoundMode>
    WinUWPH264EncoderImpl::global_frame_height_round_mode =
        WinUWPH264EncoderImpl::FrameHeightRoundMode::kNoChange;
std::atomic<H264::Profile> WinUWPH264EncoderImpl::global_profile =
    webrtc::H264::kProfileBaseline;
std::atomic<WinUWPH264EncoderImpl::RcMode>
    WinUWPH264EncoderImpl::global_rc_mode = RcMode::kUnset;
std::atomic<int> WinUWPH264EncoderImpl::global_max_qp = -1;
std::atomic<int> WinUWPH264EncoderImpl::global_quality = -1;

// QP scaling thresholds.
static constexpr int kLowH264QpThreshold = 24;
static constexpr int kHighH264QpThreshold = 37;

static constexpr unsigned kMaxH264Qp = 51;

// On some encoders (e.g. Hololens) changing rates is slow and will cause
// visible stuttering, so we don't want to do it too often.
// todo(fibann): we are ignoring small variations which means the rates might
// end up being off the requested value by a small amount in the long term. We
// should not ignore small variations but possibly use a longer min interval so
// they are eventually applied.
static constexpr int kMinIntervalBetweenRateChangesMs = 5000;
static constexpr float kMinRateVariation = 0.1f;

//////////////////////////////////////////
// H264 WinUWP Encoder Implementation
//////////////////////////////////////////

WinUWPH264EncoderImpl::WinUWPH264EncoderImpl()
{
}

WinUWPH264EncoderImpl::~WinUWPH264EncoderImpl() {
  Release();
}

namespace {

UINT32 HeightToEncode(UINT32 height) {
  switch (WinUWPH264EncoderImpl::global_frame_height_round_mode) {
    case WinUWPH264EncoderImpl::FrameHeightRoundMode::kNoChange:
      return height;
    case WinUWPH264EncoderImpl::FrameHeightRoundMode::kCrop:
      return height & ~15;
    case WinUWPH264EncoderImpl::FrameHeightRoundMode::kPad:
      return (height + 15) & ~15;
  }
}
}  // namespace

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

  // WebRTC only passes the max frame rate so use it as the initial value for
  // the desired frame rate too.
  frame_rate_ = codec_settings->maxFramerate;

  mode_ = codec_settings->mode;
  frame_dropping_on_ = codec_settings->H264().frameDroppingOn;
  key_frame_interval_ = codec_settings->H264().keyFrameInterval;
  // Codec_settings uses kbits/second; encoder uses bits/second.
  max_bitrate_ = codec_settings->maxBitrate * 1000;

  if (codec_settings->targetBitrate > 0) {
    target_bps_ = codec_settings->targetBitrate * 1000;
  } else if (codec_settings->startBitrate > 0) {
    target_bps_ = codec_settings->startBitrate * 1000;
  } else {
    // Weight*Height*2 kbit represents a good balance between video quality and
    // the bandwidth that a 620 Windows phone can handle.
    target_bps_ = width_ * height_ * 2;
  }

  // Initialize the configuration for the track encoded by this object.
  profile_ = global_profile.load();
  rc_mode_ = global_rc_mode.load();
  quality_ = global_quality.load();

  max_qp_ = std::min(codec_settings->qpMax, kMaxH264Qp);

  // Manual value overrides the value passed by WebRTC.
  int curr_global_max_qp = global_max_qp.load();
  if (curr_global_max_qp >= 0 && (unsigned)curr_global_max_qp < kMaxH264Qp) {
    max_qp_ = curr_global_max_qp;
  }


  // Configure the encoder.
  HRESULT hr = S_OK;
  ON_SUCCEEDED(MFStartup(MF_VERSION));

  ON_SUCCEEDED(InitWriter());

  return hr;
}

int WinUWPH264EncoderImpl::InitWriter() {
  HRESULT hr = S_OK;

  rtc::CritScope lock(&crit_);

  UINT32 encoded_height = HeightToEncode(height_);

  // output media type (h264)
  ComPtr<IMFMediaType> mediaTypeOut;
  ON_SUCCEEDED(MFCreateMediaType(&mediaTypeOut));
  ON_SUCCEEDED(mediaTypeOut->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
  ON_SUCCEEDED(mediaTypeOut->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264));

  eAVEncH264VProfile mf_profile;
  switch (profile_) {
    case H264::kProfileConstrainedBaseline:
      mf_profile = eAVEncH264VProfile_ConstrainedBase;
      RTC_LOG_F(LS_INFO) << "Using Constrained Baseline profile";
      break;
    case H264::kProfileBaseline:
      mf_profile = eAVEncH264VProfile_Base;
      RTC_LOG_F(LS_INFO) << "Using Baseline profile";
      break;
    case H264::kProfileMain:
      mf_profile = eAVEncH264VProfile_Main;
      RTC_LOG_F(LS_INFO) << "Using Main profile";
      break;
    case H264::kProfileConstrainedHigh:
      mf_profile = eAVEncH264VProfile_ConstrainedHigh;
      RTC_LOG_F(LS_INFO) << "Using Constrained High profile";
      break;
    case H264::kProfileHigh:
      mf_profile = eAVEncH264VProfile_High;
      RTC_LOG_F(LS_INFO) << "Using High profile";
      break;
    default:
      return WEBRTC_VIDEO_CODEC_ERROR;
      break;
  }
  ON_SUCCEEDED(mediaTypeOut->SetUINT32(MF_MT_MPEG2_PROFILE, mf_profile));


  ON_SUCCEEDED(mediaTypeOut->SetUINT32(
    MF_MT_AVG_BITRATE, target_bps_));
  ON_SUCCEEDED(mediaTypeOut->SetUINT32(
    MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive));
  ON_SUCCEEDED(MFSetAttributeSize(mediaTypeOut.Get(), MF_MT_FRAME_SIZE, width_,
                                  encoded_height));
  ON_SUCCEEDED(MFSetAttributeRatio(mediaTypeOut.Get(), MF_MT_FRAME_RATE,
                                   frame_rate_, 1));

  // input media type (nv12)
  ComPtr<IMFMediaType> mediaTypeIn;
  ON_SUCCEEDED(MFCreateMediaType(&mediaTypeIn));
  ON_SUCCEEDED(mediaTypeIn->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
  ON_SUCCEEDED(mediaTypeIn->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12));
  ON_SUCCEEDED(mediaTypeIn->SetUINT32(
    MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive));
  ON_SUCCEEDED(mediaTypeIn->SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE));
  ON_SUCCEEDED(MFSetAttributeSize(mediaTypeIn.Get(), MF_MT_FRAME_SIZE, width_,
                                  encoded_height));

  ON_SUCCEEDED(MFSetAttributeRatio(mediaTypeIn.Get(),
    MF_MT_FRAME_RATE, frame_rate_, 1));

  // Create the media sink
  ON_SUCCEEDED(Microsoft::WRL::MakeAndInitialize<H264MediaSink>(&mediaSink_));

  // SinkWriter creation attributes
  ComPtr<IMFAttributes> sinkWriterCreationAttributes;
  ON_SUCCEEDED(MFCreateAttributes(&sinkWriterCreationAttributes, 1));
  ON_SUCCEEDED(sinkWriterCreationAttributes->SetUINT32(
    MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE));
  ON_SUCCEEDED(sinkWriterCreationAttributes->SetUINT32(
    MF_SINK_WRITER_DISABLE_THROTTLING, TRUE));
  ON_SUCCEEDED(sinkWriterCreationAttributes->SetUINT32(
    MF_LOW_LATENCY, TRUE));

  // Create the sink writer
  ON_SUCCEEDED(MFCreateSinkWriterFromMediaSink(mediaSink_.Get(),
    sinkWriterCreationAttributes.Get(), &sinkWriter_));

  // Add the h264 output stream to the writer
  ON_SUCCEEDED(sinkWriter_->AddStream(mediaTypeOut.Get(), &streamIndex_));

  // SinkWriter encoder properties
  ComPtr<IMFAttributes> encodingAttributes;
  ON_SUCCEEDED(MFCreateAttributes(&encodingAttributes, 1));

  // Set mode explicitly if specified.
  if (rc_mode_ != RcMode::kUnset) {
    eAVEncCommonRateControlMode mode;
    switch (rc_mode_) {
      case RcMode::kCBR:
        RTC_LOG_F(LS_INFO) << "Using CBR mode";
        mode = eAVEncCommonRateControlMode_CBR;
        break;
      case RcMode::kVBR:
        RTC_LOG_F(LS_INFO) << "Using Unconstrained VBR mode";
        mode = eAVEncCommonRateControlMode_UnconstrainedVBR;
        break;
      case RcMode::kQuality:
        RTC_LOG_F(LS_INFO) << "Using Quality mode";
        mode = eAVEncCommonRateControlMode_Quality;
        break;
      default:
        RTC_NOTREACHED();
    }

    ON_SUCCEEDED(encodingAttributes->SetUINT32(
        CODECAPI_AVEncCommonRateControlMode, mode));
  }

  // Enable CABAC. This will only have effect in profiles that support it.
  ON_SUCCEEDED(encodingAttributes->SetUINT32(CODECAPI_AVEncH264CABACEnable,
                                             VARIANT_TRUE));

  // kMaxH264Qp is the default.
  if (max_qp_ < kMaxH264Qp) {
    RTC_LOG(LS_INFO) << "Set max QP to " << max_qp_;
    ON_SUCCEEDED(
        encodingAttributes->SetUINT32(CODECAPI_AVEncVideoMaxQP, max_qp_));
  }

  if (quality_ >= 0 && quality_ <= 100) {
    RTC_LOG(LS_INFO) << "Set quality to " << quality_;
    encodingAttributes->SetUINT32(CODECAPI_AVEncCommonQuality,
                                  quality_);
  }

  ON_SUCCEEDED(sinkWriter_->SetInputMediaType(streamIndex_, mediaTypeIn.Get(),
                                              encodingAttributes.Get()));

  // Register this as the callback for encoded samples.
  ON_SUCCEEDED(mediaSink_->RegisterEncodingCallback(this));

  ON_SUCCEEDED(sinkWriter_->BeginWriting());

  if (SUCCEEDED(hr)) {
    inited_ = true;
    last_rate_change_time_rtc_ms = rtc::TimeMillis();
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

int WinUWPH264EncoderImpl::ReleaseWriter() {
  // Use a temporary sink variable to prevent lock inversion
  // between the shutdown call and OnH264Encoded() callback.
  ComPtr<H264MediaSink> tmpMediaSink;

  {
    rtc::CritScope lock(&crit_);
    sinkWriter_.Reset();
    if (mediaSink_ != nullptr) {
      tmpMediaSink = mediaSink_;
    }
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

int WinUWPH264EncoderImpl::Release() {
  ReleaseWriter();
  HRESULT hr = S_OK;
  ON_SUCCEEDED(MFShutdown());
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

  assert(frameBuffer->width() == width_);
  assert(frameBuffer->height() == height_);
  int encoded_height = HeightToEncode(height_);

  int dst_height_uv = (height_ + 1) / 2;
  int dst_stride_y = frameBuffer->StrideY();
  int dst_stride_uv = dst_stride_y;

  auto totalSize =
      dst_stride_y * encoded_height + dst_stride_uv * encoded_height / 2;

  // Will be negative when cropping.
  int padding_top_y = (encoded_height - (int)height_) / 2;
  int padding_bottom_y = encoded_height - (int)height_ - padding_top_y;
  int padding_top_uv = padding_top_y / 2;
  int padding_bottom_uv = encoded_height / 2 - dst_height_uv - padding_top_uv;

  if (SUCCEEDED(hr)) {
    ComPtr<IMFMediaBuffer> mediaBuffer;
    ON_SUCCEEDED(MFCreateMemoryBuffer(totalSize, mediaBuffer.GetAddressOf()));

    BYTE* destBuffer = nullptr;
    if (SUCCEEDED(hr)) {
      DWORD cbMaxLength;
      DWORD cbCurrentLength;
      ON_SUCCEEDED(mediaBuffer->Lock(
        &destBuffer, &cbMaxLength, &cbCurrentLength));
    }
    const uint8_t* src_y = frameBuffer->DataY();
    const uint8_t* src_u = frameBuffer->DataU();
    const uint8_t* src_v = frameBuffer->DataV();
    int src_height = height_;

    BYTE* dst_y = destBuffer;
    BYTE* dst_uv = dst_y + dst_stride_y * encoded_height;

	if (encoded_height > (int)height_) {
      // Pad the destination.
      dst_y += dst_stride_y * padding_top_y;
      dst_uv += dst_stride_uv * padding_top_uv;
    } else {
      // Crop the source.
      src_y += frameBuffer->StrideY() * -padding_top_y;
      src_u += frameBuffer->StrideU() * -padding_top_uv;
      src_v += frameBuffer->StrideV() * -padding_top_uv;
      src_height = encoded_height;
    }
    if (SUCCEEDED(hr)) {
      libyuv::I420ToNV12(src_y, frameBuffer->StrideY(), src_u,
                         frameBuffer->StrideU(), src_v, frameBuffer->StrideV(),
                         dst_y, dst_stride_y, dst_uv, dst_stride_uv, width_, src_height);

      if (padding_top_y > 0) {
        libyuv::CopyPlane(dst_y, dst_stride_y,
                          destBuffer, dst_stride_y,
                          width_, -padding_top_y);
        libyuv::CopyPlane(dst_uv, dst_stride_uv,
                          dst_uv - (dst_stride_uv * padding_top_uv), dst_stride_uv,
                          width_, -padding_top_uv);
      }
      if (padding_bottom_y > 0) {
        BYTE* dst_end_y = dst_y + (dst_stride_y * height_);
        BYTE* dst_end_uv = dst_uv + (dst_stride_uv * dst_height_uv);
        libyuv::CopyPlane(dst_end_y - (dst_stride_y * padding_bottom_y),
                          dst_stride_y, dst_end_y,
                          dst_stride_y, width_, -padding_bottom_y);
        libyuv::CopyPlane(dst_end_uv - (dst_stride_uv * padding_bottom_uv),
                          dst_stride_uv, dst_end_uv, dst_stride_uv, width_,
                          -padding_bottom_uv);
      }
    }

    if (firstFrame_) {
      firstFrame_ = false;
      startTime_ = frame.timestamp();
    }

    auto timestampHns = GetFrameTimestampHns(frame);
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
      frameAttributes.frameHeight = encoded_height;
      _sampleAttributeQueue.push(timestampHns, frameAttributes);
    }

    ON_SUCCEEDED(
        mediaBuffer->SetCurrentLength(totalSize));

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

// Returns the timestamp in hundreds of nanoseconds (Media Foundation unit)
LONGLONG WinUWPH264EncoderImpl::GetFrameTimestampHns(const VideoFrame& frame) const {
  // H.264 clock rate is 90kHz (https://tools.ietf.org/html/rfc6184#page-11).
  // timestamp_100ns = timestamp_90kHz / {90'000 Hz} * {10'000'000 hns/sec}
  return (frame.timestamp() - startTime_) * 10'000 / 90;
}

int WinUWPH264EncoderImpl::Encode(
  const VideoFrame& frame,
  const CodecSpecificInfo* codec_specific_info,
  const std::vector<FrameType>* frame_types) {
  if (!inited_) {
    return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  }

  {
    rtc::scoped_refptr<webrtc::VideoFrameBuffer> frame_buffer =
        frame.video_frame_buffer();
    int cur_width = frame_buffer->width();
    int cur_height = frame_buffer->height();
    int64_t now = rtc::TimeMillis();

    rtc::CritScope lock(&crit_);
    // Reset the encoder configuration if necessary.
    bool res_changed = cur_width != (int)width_ || cur_height != (int)height_;
    bool should_change_rate_now =
        rate_change_requested_ &&
        (now - last_rate_change_time_rtc_ms) > kMinIntervalBetweenRateChangesMs;
    if (res_changed || should_change_rate_now) {
      int res = ReconfigureSinkWriter(cur_width, cur_height, next_target_bps_,
                                      next_frame_rate_);
      if (FAILED(res)) {
        return res;
      }
      rate_change_requested_ = false;
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
    // Only encode the frame if the encoder pipeline is not full.
    if (_sampleAttributeQueue.size() <= 2) {
      sample = FromVideoFrame(frame);
    }
  }

  if (!sample) {
    // Drop the frame. Send a tick to keep the encoder going.
    lastFrameDropped_ = true;
    auto timestampHns = GetFrameTimestampHns(frame);
    ON_SUCCEEDED(sinkWriter_->SendStreamTick(streamIndex_, timestampHns));
    lastTimestampHns_ = timestampHns;
    return WEBRTC_VIDEO_CODEC_OK;
  }

  ON_SUCCEEDED(sinkWriter_->WriteSample(streamIndex_, sample.Get()));

  ++framePendingCount_;
  return WEBRTC_VIDEO_CODEC_OK;
}

void WinUWPH264EncoderImpl::OnH264Encoded(ComPtr<IMFSample> sample) {
  DWORD totalLength;
  HRESULT hr = S_OK;
  ON_SUCCEEDED(sample->GetTotalLength(&totalLength));

  LONGLONG sampleTimestamp = 0;
  ON_SUCCEEDED(sample->GetSampleTime(&sampleTimestamp));

  // Pop the attributes for this frame. This must be done even if the
  // frame is discarded later, or the queue will clog.
  CachedFrameAttributes frameAttributes;
  if (!_sampleAttributeQueue.pop(sampleTimestamp, frameAttributes)) {
    // No point in processing a frame that doesn't have correct attributes.
    return;
  }

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

    encodedImage.SetTimestamp(frameAttributes.timestamp);
    encodedImage.ntp_time_ms_ = frameAttributes.ntpTime;
    encodedImage.capture_time_ms_ = frameAttributes.captureRenderTime;
    encodedImage._encodedWidth = frameAttributes.frameWidth;
    encodedImage._encodedHeight = frameAttributes.frameHeight;

    {
      rtc::CritScope lock(&callbackCrit_);
      --framePendingCount_;

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

int WinUWPH264EncoderImpl::SetRates(uint32_t new_bitrate_kbit,
                                    uint32_t new_framerate) {
  if (sinkWriter_ == nullptr) {
    return WEBRTC_VIDEO_CODEC_UNINITIALIZED;
  }
  RTC_LOG(LS_INFO) << "WinUWPH264EncoderImpl::SetRates(" << new_bitrate_kbit
                   << "kbit " << new_framerate << "fps)";

  // This may happen. Ignore it.
  if (new_framerate == 0) {
    return WEBRTC_VIDEO_CODEC_OK;
  }

  int64_t now = rtc::TimeMillis();
  rtc::CritScope lock(&crit_);
  int64_t time_to_wait_before_rate_change =
      kMinIntervalBetweenRateChangesMs - (now - last_rate_change_time_rtc_ms);
  if (time_to_wait_before_rate_change > 0) {
    // Delay rate update until the interval has passed.
    RTC_LOG(LS_INFO) << "Postponing this SetRates() in "
                     << time_to_wait_before_rate_change << " ms.\n";
    next_target_bps_ = new_bitrate_kbit * 1000;
    next_frame_rate_ = new_framerate;
    rate_change_requested_ = true;
    return WEBRTC_VIDEO_CODEC_OK;
  }
  // Update the configuration.
  return ReconfigureSinkWriter(width_, height_, new_bitrate_kbit * 1000,
                               new_framerate);
}

int WinUWPH264EncoderImpl::ReconfigureSinkWriter(UINT32 new_width,
                                                 UINT32 new_height,
                                                 UINT32 new_target_bps,
                                                 UINT32 new_frame_rate) {
  // NOTE: must be called under crit_ lock.
  RTC_LOG(LS_INFO) << "WinUWPH264EncoderImpl::ResetSinkWriter() " << new_width
                   << "x" << new_height << "@" << new_frame_rate << " "
                   << new_target_bps / 1000 << "kbps";
  bool resUpdated = false;
  bool bitrateUpdated = false;
  bool fpsUpdated = false;

  if (width_ != new_width || height_ != new_height) {
    resUpdated = true;
    width_ = new_width;
    height_ = new_height;
  }

#ifdef DYNAMIC_BITRATE
  // Ignore small changes.
  if (std::abs((int)target_bps_ - (int)new_target_bps) >
      target_bps_ * kMinRateVariation) {
    bitrateUpdated = true;
    target_bps_ = new_target_bps;
  }
#endif

#ifdef DYNAMIC_FPS
  // Ignore small changes.
  if (std::abs((int)frame_rate_ - (int)new_frame_rate) >
      frame_rate_ * kMinRateVariation) {
    fpsUpdated = true;
    frame_rate_ = new_frame_rate;
  }
#endif

  if (resUpdated || bitrateUpdated || fpsUpdated) {
    EncodedImageCallback* tempCallback = encodedCompleteCallback_;
    ReleaseWriter();
    {
      rtc::CritScope lock(&callbackCrit_);
      encodedCompleteCallback_ = tempCallback;
    }
    InitWriter();

    last_rate_change_time_rtc_ms = rtc::TimeMillis();
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
