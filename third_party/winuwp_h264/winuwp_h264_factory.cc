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
#include "absl/strings/match.h"
#include "rtc_base/logging.h"
#include "third_party/winuwp_h264/winuwp_h264_factory.h"
#include "third_party/winuwp_h264/H264Encoder/H264Encoder.h"
#include "third_party/winuwp_h264/H264Decoder/H264Decoder.h"
#include "modules/video_coding/codecs/vp8/include/vp8.h"
#include "modules/video_coding/codecs/vp9/include/vp9.h"

namespace webrtc {

namespace {

  bool IsFormatSupported(
      const std::vector<webrtc::SdpVideoFormat>& supported_formats,
      const webrtc::SdpVideoFormat& format) {
    for (const webrtc::SdpVideoFormat& supported_format : supported_formats) {
      if (cricket::IsSameCodec(format.name, format.parameters,
                               supported_format.name,
                               supported_format.parameters)) {
        return true;
      }
    }
    return false;
  }
} // namespace

  WinUWPH264EncoderFactory::WinUWPH264EncoderFactory() {
  }

  std::vector<SdpVideoFormat> WinUWPH264EncoderFactory::GetSupportedFormats() const
  {
    std::vector<SdpVideoFormat> supported_codecs;
    supported_codecs.push_back(SdpVideoFormat(cricket::kVp8CodecName));
    for (const webrtc::SdpVideoFormat& format : webrtc::SupportedVP9Codecs())
      supported_codecs.push_back(format);
    supported_codecs.push_back(SdpVideoFormat(cricket::kH264CodecName));
    return supported_codecs;
  }

  VideoEncoderFactory::CodecInfo WinUWPH264EncoderFactory::QueryVideoEncoder(
    const SdpVideoFormat& format) const
  {
    CodecInfo info;
    if (absl::EqualsIgnoreCase(format.name, cricket::kH264CodecName))
      info.is_hardware_accelerated = true;
    else
      info.is_hardware_accelerated = false;
    info.has_internal_source = false;
    return info;
  }

  std::unique_ptr<VideoEncoder> WinUWPH264EncoderFactory::CreateVideoEncoder(
    const SdpVideoFormat& format)
  {
    if (absl::EqualsIgnoreCase(format.name, cricket::kVp8CodecName))
      return VP8Encoder::Create();
    if (absl::EqualsIgnoreCase(format.name, cricket::kVp9CodecName))
      return VP9Encoder::Create(cricket::VideoCodec(format));
    if (absl::EqualsIgnoreCase(format.name, cricket::kH264CodecName))
      return std::unique_ptr<VideoEncoder>(new WinUWPH264EncoderImpl());
    RTC_LOG(LS_ERROR) << "Trying to created encoder of unsupported format "
                      << format.name;
    return nullptr;
  }

  std::vector<SdpVideoFormat> WinUWPH264DecoderFactory::GetSupportedFormats() const
  {
    std::vector<SdpVideoFormat> formats;
    formats.push_back(SdpVideoFormat(cricket::kVp8CodecName));
    for (const SdpVideoFormat& format : SupportedVP9Codecs())
      formats.push_back(format);
    formats.push_back(SdpVideoFormat(cricket::kH264CodecName));
    return formats;
  }

  std::unique_ptr<VideoDecoder> WinUWPH264DecoderFactory::CreateVideoDecoder(
    const SdpVideoFormat& format)
  {
    if (!IsFormatSupported(GetSupportedFormats(), format)) {
      RTC_LOG(LS_ERROR) << "Trying to create decoder for unsupported format";
      return nullptr;
    }

    if (absl::EqualsIgnoreCase(format.name, cricket::kVp8CodecName))
      return VP8Decoder::Create();
    if (absl::EqualsIgnoreCase(format.name, cricket::kVp9CodecName))
      return VP9Decoder::Create();
    if (absl::EqualsIgnoreCase(format.name, cricket::kH264CodecName))
      return std::unique_ptr<VideoDecoder>(new WinUWPH264DecoderImpl());

    RTC_NOTREACHED();
    return nullptr;
  }
}  // namespace webrtc

