/*
*  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
*
*  Use of this source code is governed by a BSD-style license
*  that can be found in the LICENSE file in the root of the source
*  tree. An additional intellectual property rights grant can be found
*  in the file PATENTS.  All contributing project authors may
*  be found in the AUTHORS file in the root of the source tree.
*/

#ifndef THIRD_PARTY_H264_WINUWP_H264DECODER_H264DECODER_H_
#define THIRD_PARTY_H264_WINUWP_H264DECODER_H264DECODER_H_

#include <mfapi.h>
#include <mfidl.h>
#include <Mfreadwrite.h>
#include <mferror.h>
#include <wrl.h>
#include "../Utils/SampleAttributeQueue.h"
#include "api/video_codecs/video_decoder.h"
#include "common_video/include/i420_buffer_pool.h"
#include "modules/video_coding/codecs/h264/include/h264.h"
#include "rtc_base/criticalsection.h"

#pragma comment(lib, "mfreadwrite")
#pragma comment(lib, "mfplat")
#pragma comment(lib, "mfuuid")

using Microsoft::WRL::ComPtr;

namespace webrtc {

class WinUWPH264DecoderImpl : public H264Decoder {
 public:
  WinUWPH264DecoderImpl();

  virtual ~WinUWPH264DecoderImpl();

  int InitDecode(const VideoCodec* codec_settings, int number_of_cores) override;

  int Decode(const EncodedImage& input_image,
    bool missing_frames,
    const CodecSpecificInfo* codec_specific_info,
    int64_t /*render_time_ms*/) override;

  int RegisterDecodeCompleteCallback(DecodedImageCallback* callback) override;

  int Release() override;

  const char* ImplementationName() const override;

 private:
  HRESULT FlushFrames(uint32_t timestamp, uint64_t ntp_time_ms);
  HRESULT EnqueueFrame(const EncodedImage& input_image, bool missing_frames);

 private:
  ComPtr<IMFTransform> decoder_;
  I420BufferPool buffer_pool_;

  bool inited_ = false;
  bool require_keyframe_ = true;
  uint32_t first_frame_rtp_ = 0;
  absl::optional<uint32_t> width_;
  absl::optional<uint32_t> height_;
  rtc::CriticalSection crit_;
  DecodedImageCallback* decode_complete_callback_;
};  // end of WinUWPH264DecoderImpl class

}  // namespace webrtc

#endif  // THIRD_PARTY_H264_WINUWP_H264DECODER_H264DECODER_H_
