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
#include "media/engine/webrtcvideoencoderfactory.h"
#include "media/engine/webrtcvideodecoderfactory.h"
#include "rtc_base/logging.h"

namespace webrtc {

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

  std::vector<SdpVideoFormat> WinUWPH264EncoderFactory::GetSupportedFormats()
    const {
    std::vector<SdpVideoFormat> formats = { 
      SdpVideoFormat(cricket::kH264CodecName, 
      {
        //copy-pasted from h264.cc
        {cricket::kH264FmtpProfileLevelId, "42100b"},
        {cricket::kH264FmtpLevelAsymmetryAllowed, "1"},
        {cricket::kH264FmtpPacketizationMode, "0"}
      }),
      SdpVideoFormat(cricket::kH264CodecName, 
      {
        {cricket::kH264FmtpProfileLevelId, "42100b"},
        {cricket::kH264FmtpLevelAsymmetryAllowed, "1"},
        {cricket::kH264FmtpPacketizationMode, "1"}
      }) 
    };
    return formats;
  }

  VideoEncoderFactory::CodecInfo WinUWPH264EncoderFactory::QueryVideoEncoder(
    const SdpVideoFormat& format) const {
    CodecInfo info;
    info.is_hardware_accelerated = true;
    info.has_internal_source = false;
    
    return info;
  }

  std::unique_ptr<VideoEncoder> WinUWPH264EncoderFactory::CreateVideoEncoder(
    const SdpVideoFormat& format) {
    if (cricket::CodecNamesEq(format.name, cricket::kH264CodecName)) {
      return std::make_unique<WinUWPH264EncoderImpl>();
    }

    RTC_LOG(LS_ERROR) << "Trying to create encoder of unsupported format "
                    << format.name;
    return nullptr;
  }

}  // namespace webrtc

