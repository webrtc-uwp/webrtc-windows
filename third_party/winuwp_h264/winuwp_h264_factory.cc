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
#include "webrtc/media/engine/webrtcvideoencoderfactory.h"
#include "webrtc/media/engine/webrtcvideodecoderfactory.h"


namespace webrtc {

  WinUWPH264EncoderFactory::WinUWPH264EncoderFactory() {
    codecList_ =
      std::vector<cricket::VideoCodec> {
        cricket::VideoCodec("H264")
    };
  }

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


  webrtc::VideoDecoder* WinUWPH264DecoderFactory::CreateVideoDecoder(
    webrtc::VideoCodecType type) {
    if (type == kVideoCodecH264) {
      return new WinUWPH264DecoderImpl();
    } else {
      return nullptr;
    }
  }

  void WinUWPH264DecoderFactory::DestroyVideoDecoder(
    webrtc::VideoDecoder* decoder) {
    decoder->Release();
    delete decoder;
  }

}  // namespace webrtc

