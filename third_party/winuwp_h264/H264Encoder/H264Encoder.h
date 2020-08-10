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

#include <atomic>
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

 public:
  // Determines whether to pad or crop frames whose height is not multiple
  // of 16. User code should extern-declare and set this. There seems to be no
  // easy way to pass arbitrary parameters to the encoder so this is the best
  // (easy) option.
  enum class FrameHeightRoundMode { kNoChange = 0, kCrop = 1, kPad = 2 };
  static std::atomic<FrameHeightRoundMode> global_frame_height_round_mode;

  // H.264 profile to use for encoding.
  // Once a value is set, it will be used by any instance of
  // WinUWPH264EncoderImpl created after that.
  // Note : by default we should use what's passed by WebRTC on codec
  // initialization (which seems to be always ConstrainedBaseline), but we use
  // Baseline to avoid changing behavior compared to earlier versions.
  static std::atomic<webrtc::H264::Profile> global_profile;

  // Rate control mode. See
  // https://docs.microsoft.com/en-us/windows/win32/medfound/h-264-video-encoder
  // for details. If kUnset, the default for the encoder implementation will be
  // used.
  // Once a value is set, it will be used by any instance of
  // WinUWPH264EncoderImpl created after that.
  enum class RcMode { kUnset = -1, kCBR = 0, kVBR = 1, kQuality = 2 };
  static std::atomic<RcMode> global_rc_mode;

  // If set to a value between 0 and 51, determines the max QP to use for
  // encoding.
  // Once a value is set, it will be used by any instance of
  // WinUWPH264EncoderImpl created after that.
  static std::atomic<int> global_max_qp;

  // If set to a value between 0 and 100, determines the target quality value.
  // The effect of this depends on the encoder and on the rate control mode
  // chosen. In the Quality RC mode this will be the target for the whole
  // stream, while in VBR it might be used as a target for individual frames
  // while the average quality of the stream is determined by the target
  // bitrate.
  // Once a value is set, it will be used by any instance of
  // WinUWPH264EncoderImpl created after that.
  static std::atomic<int> global_quality;

 private:
  ComPtr<IMFSample> FromVideoFrame(const VideoFrame& frame);
  int InitWriter();
  int ReleaseWriter();
  LONGLONG GetFrameTimestampHns(const VideoFrame& frame) const;
  int ReconfigureSinkWriter(UINT32 new_width,
                            UINT32 new_height,
                            UINT32 new_target_bps,
                            UINT32 new_frame_rate);

 private:
  rtc::CriticalSection crit_;
  rtc::CriticalSection callbackCrit_;
  bool inited_ {};
  const CodecSpecificInfo* codecSpecificInfo_ {};
  ComPtr<IMFSinkWriter> sinkWriter_;
  ComPtr<H264MediaSink> mediaSink_;
  EncodedImageCallback* encodedCompleteCallback_ {};
  DWORD streamIndex_ {};
  LONGLONG startTime_ {};
  LONGLONG lastTimestampHns_ {};
  bool firstFrame_ {true};
  int framePendingCount_ {};
  DWORD frameCount_ {};
  bool lastFrameDropped_ {};
  //These fields are never used
  /*
  UINT32 currentWidth_ {};
  UINT32 currentHeight_ {};
  UINT32 currentBitrateBps_ {};
  UINT32 currentFps_ {};
  */
  UINT32 max_bitrate_;

  UINT32 width_;
  UINT32 height_;
  UINT32 frame_rate_;
  UINT32 target_bps_;
  VideoCodecMode mode_;
  // H.264 specifc parameters
  bool frame_dropping_on_;
  int key_frame_interval_;

  int64_t last_rate_change_time_rtc_ms {};
  bool rate_change_requested_ {};

  // Values to use as soon as the min interval between rate changes has passed
  UINT32 next_frame_rate_;
  UINT32 next_target_bps_;

  struct CachedFrameAttributes {
    uint32_t timestamp;
    uint64_t ntpTime;
    uint64_t captureRenderTime;
    uint32_t frameWidth;
    uint32_t frameHeight;
  };
  SampleAttributeQueue<CachedFrameAttributes> _sampleAttributeQueue;

  H264::Profile profile_;
  RcMode rc_mode_;
  int max_qp_;
  int quality_;
};  // end of WinUWPH264EncoderImpl class

}  // namespace webrtc
#endif  // THIRD_PARTY_H264_WINUWP_H264ENCODER_H264ENCODER_H_

