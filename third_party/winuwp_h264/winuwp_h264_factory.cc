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

  CodecInfo QueryVideoEncoder(const SdpVideoFormat& format) const
  {
    return {};
  }

  std::unique_ptr<VideoEncoder> CreateVideoEncoder(
    const SdpVideoFormat& format)
  {
    return {};
  }

#if 0
  webrtc::VideoEncoder* WinUWPH264EncoderFactory::CreateVideoEncoder(
    const cricket::VideoCodec& codec) {
    if (codec.name == "H264") {
      return new WinUWPH264EncoderImpl();
    } else {
      return nullptr;
    }
  }

  const std::vector<cricket::VideoCodec>&
    WinUWPH264EncoderFactory::supported_codecs() const {
    return codecList_;
  }

  void WinUWPH264EncoderFactory::DestroyVideoEncoder(
    webrtc::VideoEncoder* encoder) {
      encoder->Release();
      delete encoder;
  }

#endif //0  


  webrtc::VideoDecoder* WinUWPH264DecoderFactory::CreateVideoDecoder(
    webrtc::VideoCodecType type) {
    if (type == kVideoCodecH264) {
      return new WinUWPH264DecoderImpl();
    }
    return nullptr;
  }

  std::vector<SdpVideoFormat> WinUWPH264DecoderFactory::GetSupportedFormats() const
  {
    return {};
  }

  // Creates a VideoDecoder for the specified format.
  std::unique_ptr<VideoDecoder> WinUWPH264DecoderFactory::CreateVideoDecoder(
    const SdpVideoFormat& format)
  {
    return {};
  }

#if 0
  void WinUWPH264DecoderFactory::DestroyVideoDecoder(
    webrtc::VideoDecoder* decoder) {
    decoder->Release();
    delete decoder;
  }
#endif //0

}  // namespace webrtc

