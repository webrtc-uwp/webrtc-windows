/*
*  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
*
*  Use of this source code is governed by a BSD-style license
*  that can be found in the LICENSE file in the root of the source
*  tree. An additional intellectual property rights grant can be found
*  in the file PATENTS.  All contributing project authors may
*  be found in the AUTHORS file in the root of the source tree.
*/

#ifndef THIRD_PARTY_H264_WINUWP_NATIVE_FRAME_H_
#define THIRD_PARTY_H264_WINUWP_NATIVE_FRAME_H_

#include "api/video/video_frame_buffer.h"
#include "media/base/videocommon.h"

namespace webrtc {

class NativeHandleBuffer : public VideoFrameBuffer {
 public:
  NativeHandleBuffer(void* native_handle, int width, int height)
    : native_handle_(native_handle),
    width_(width),
    height_(height) { }

  Type type() const override {
    return Type::kNative;
  }

  int width() const override {
    return width_;
  }
  int height() const override {
    return height_;
  }

  void* native_handle() const {
    return native_handle_;
  }

  virtual cricket::FourCC fourCC() const = 0;

 protected:
  void* native_handle_;
  const int width_;
  const int height_;
};
}  // namespace webrtc

#endif  // THIRD_PARTY_H264_WINUWP_NATIVE_FRAME_H_
