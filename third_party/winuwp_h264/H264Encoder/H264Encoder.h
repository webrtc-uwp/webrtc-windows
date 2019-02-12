/*
*  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
*
*  Use of this source code is governed by a BSD-style license
*  that can be found in the LICENSE file in the root of the source
*  tree. An additional intellectual property rights grant can be found
*  in the file PATENTS.  All contributing project authors may
*  be found in the AUTHORS file in the root of the source tree.
*/

#ifndef THIRD_PARTY_H264_WINUWP_H264ENCODER_H264ENCODER_H_
#define THIRD_PARTY_H264_WINUWP_H264ENCODER_H264ENCODER_H_

#include <mfapi.h>
#include <mfidl.h>
#include <Mfreadwrite.h>
#include <mferror.h>
#include <vector>
#include "H264MediaSink.h"
#include "IH264EncodingCallback.h"
#include "../Utils/SampleAttributeQueue.h"
#include "api/video_codecs/video_encoder.h"
#include "rtc_base/criticalsection.h"
#include "modules/video_coding/utility/quality_scaler.h"
#include "common_video/h264/h264_bitstream_parser.h"

#pragma comment(lib, "mfreadwrite")
#pragma comment(lib, "mfplat")
#pragma comment(lib, "mfuuid")

namespace webrtc {

class H264MediaSink;

class WinUWPH264EncoderImpl : public VideoEncoder, public IH264EncodingCallback {
 public:
  WinUWPH264EncoderImpl();

  ~WinUWPH264EncoderImpl();

  // === VideoEncoder overrides ===
  int InitEncode(const VideoCodec* codec_settings,
    int number_of_cores, size_t max_payload_size) override;
  int RegisterEncodeCompleteCallback(EncodedImageCallback* callback) override;
  int Release() override;
  int Encode(const VideoFrame& input_image,
    const CodecSpecificInfo* codec_specific_info,
    const std::vector<FrameType>* frame_types) override;
  int SetChannelParameters(uint32_t packet_loss, int64_t rtt) override;
  int SetRates(uint32_t new_bitrate_kbit, uint32_t frame_rate) override;
  ScalingSettings GetScalingSettings() const override;
  const char* ImplementationName() const override;

  // === IH264EncodingCallback overrides ===
  void OnH264Encoded(ComPtr<IMFSample> sample) override;

 private:
  ComPtr<IMFSample> FromVideoFrame(const VideoFrame& frame);
  int InitEncoderWithSettings(const VideoCodec* codec_settings);

 private:
  rtc::CriticalSection crit_;
  rtc::CriticalSection callbackCrit_;
  bool inited_ {};
  const CodecSpecificInfo* codecSpecificInfo_ {};
  ComPtr<IMFSinkWriter> sinkWriter_;
  ComPtr<IMFAttributes> sinkWriterCreationAttributes_;
  ComPtr<IMFAttributes> sinkWriterEncoderAttributes_;
  ComPtr<H264MediaSink> mediaSink_;
  EncodedImageCallback* encodedCompleteCallback_ {};
  DWORD streamIndex_ {};
  LONGLONG startTime_ {};
  LONGLONG lastTimestampHns_ {};
  bool firstFrame_ {true};
  int framePendingCount_ {};
  DWORD frameCount_ {};
  bool lastFrameDropped_ {};
  UINT32 currentWidth_ {};
  UINT32 currentHeight_ {};
  UINT32 currentBitrateBps_ {};
  UINT32 currentFps_ {};
  int64_t lastTimeSettingsChanged_ {};

  struct CachedFrameAttributes {
    uint32_t timestamp;
    uint64_t ntpTime;
    uint64_t captureRenderTime;
    uint32_t frameWidth;
    uint32_t frameHeight;
  };
  SampleAttributeQueue<CachedFrameAttributes> _sampleAttributeQueue;

  // Caching the codec received in InitEncode().
  VideoCodec codec_;
};  // end of WinUWPH264EncoderImpl class

}  // namespace webrtc
#endif  // THIRD_PARTY_H264_WINUWP_H264ENCODER_H264ENCODER_H_

