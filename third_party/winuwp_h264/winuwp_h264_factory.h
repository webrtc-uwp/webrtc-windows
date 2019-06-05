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
#include "media/engine/webrtc_video_encoder_factory.h"
#include "media/engine/webrtc_video_decoder_factory.h"
#include "media/base/codec.h"

namespace webrtc {

class WinUWPH264EncoderFactory : public cricket::WebRtcVideoEncoderFactory {
 public:
  WinUWPH264EncoderFactory();

  webrtc::VideoEncoder* CreateVideoEncoder(const cricket::VideoCodec& codec)
    override;

  const std::vector<cricket::VideoCodec>& supported_codecs()
    const override;

  void DestroyVideoEncoder(webrtc::VideoEncoder* encoder) override;

 private:
  std::vector<cricket::VideoCodec> codecList_;
};

class WinUWPH264DecoderFactory : public cricket::WebRtcVideoDecoderFactory {
  webrtc::VideoDecoder* CreateVideoDecoder(webrtc::VideoCodecType type)
    override;

  void DestroyVideoDecoder(webrtc::VideoDecoder* decoder) override;
};
}  // namespace webrtc

#endif  // THIRD_PARTY_H264_WINUWP_H264_WINUWP_FACTORY_H_
