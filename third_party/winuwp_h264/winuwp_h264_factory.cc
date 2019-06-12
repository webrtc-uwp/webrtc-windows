/*
*  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
*
*  Use of this source code is governed by a BSD-style license
*  that can be found in the LICENSE file in the root of the source
*  tree. An additional intellectual property rights grant can be found
*  in the file PATENTS.  All contributing project authors may
*  be found in the AUTHORS file in the root of the source tree.
*/

#include <vector>
#include "third_party/winuwp_h264/winuwp_h264_factory.h"
#include "third_party/winuwp_h264/H264Encoder/H264Encoder.h"
#include "third_party/winuwp_h264/H264Decoder/H264Decoder.h"


namespace webrtc {

  WinUWPH264EncoderFactory::WinUWPH264EncoderFactory() {
    codecList_ =
      std::vector<cricket::VideoCodec> {
        cricket::VideoCodec("H264")
    };
  }

  std::vector<SdpVideoFormat> WinUWPH264EncoderFactory::GetSupportedFormats() const
  {
    return {};
  }

  VideoEncoderFactory::CodecInfo WinUWPH264EncoderFactory::QueryVideoEncoder(
    const SdpVideoFormat& format) const
  {
    return {};
  }

  std::unique_ptr<VideoEncoder> WinUWPH264EncoderFactory::CreateVideoEncoder(
    const SdpVideoFormat& format)
  {
    return std::unique_ptr<VideoEncoder>(new WinUWPH264EncoderImpl());
  }

  std::vector<SdpVideoFormat> WinUWPH264DecoderFactory::GetSupportedFormats() const
  {
    return {};
  }

  std::unique_ptr<VideoDecoder> WinUWPH264DecoderFactory::CreateVideoDecoder(
    const SdpVideoFormat& format)
  {
    return std::unique_ptr<VideoDecoder>(new WinUWPH264DecoderImpl());
  }

  std::unique_ptr<VideoDecoder>
  WinUWPH264DecoderFactory::LegacyCreateVideoDecoder(
      const SdpVideoFormat& format,
      const std::string& receive_stream_id) {
    return {};
  }
}  // namespace webrtc

