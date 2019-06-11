/*
*  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
*
*  Use of this source code is governed by a BSD-style license
*  that can be found in the LICENSE file in the root of the source
*  tree. An additional intellectual property rights grant can be found
*  in the file PATENTS.  All contributing project authors may
*  be found in the AUTHORS file in the root of the source tree.
*/

#ifndef THIRD_PARTY_H264_WINUWP_H264_WINUWP_FACTORY_H_
#define THIRD_PARTY_H264_WINUWP_H264_WINUWP_FACTORY_H_

#include <vector>
#include "api/video_codecs/video_encoder_factory.h"
#include "api/video_codecs/video_decoder_factory.h"
#include "media/base/codec.h"

namespace webrtc {

class WinUWPH264EncoderFactory : public webrtc::VideoEncoderFactory {
 public:
  WinUWPH264EncoderFactory();

  std::vector<SdpVideoFormat> GetSupportedFormats() const override;

  CodecInfo QueryVideoEncoder(const SdpVideoFormat& format) const override;

  std::unique_ptr<VideoEncoder> CreateVideoEncoder(
    const SdpVideoFormat& format) override;

 private:
  std::vector<cricket::VideoCodec> codecList_;
};

class WinUWPH264DecoderFactory : public webrtc::VideoDecoderFactory {

  std::vector<SdpVideoFormat> GetSupportedFormats() const override;

  std::unique_ptr<VideoDecoder> CreateVideoDecoder(
      const SdpVideoFormat& format) override;

  std::unique_ptr<VideoDecoder> LegacyCreateVideoDecoder(
      const SdpVideoFormat& format,
      const std::string& receive_stream_id) override;
};

}  // namespace webrtc

#endif  // THIRD_PARTY_H264_WINUWP_H264_WINUWP_FACTORY_H_
