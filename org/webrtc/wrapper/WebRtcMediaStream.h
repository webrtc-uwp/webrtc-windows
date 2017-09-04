
// Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
//
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file in the root of the source
// tree. An additional intellectual property rights grant can be found
// in the file PATENTS.  All contributing project authors may
// be found in the AUTHORS file in the root of the source tree.

#ifndef WEBRTC_BUILD_WINUWP_GYP_API_WEBRTCMEDIASTREAM_H_
#define WEBRTC_BUILD_WINUWP_GYP_API_WEBRTCMEDIASTREAM_H_

#include <wrl.h>
#include <mfidl.h>
#include <vector>
#include "webrtc/api/mediastreaminterface.h"
#include "webrtc/system_wrappers/include/critical_section_wrapper.h"
#include "MediaSourceHelper.h"

using Microsoft::WRL::ComPtr;
using Microsoft::WRL::RuntimeClass;
using Microsoft::WRL::RuntimeClassFlags;
using Microsoft::WRL::RuntimeClassType;
using Windows::System::Threading::ThreadPoolTimer;
using Platform::String;

namespace Org {
	namespace WebRtc {
		/// <summary>
		/// Delegate used to notify an update of the frame per second on a video stream.
		/// </summary>
		public delegate void FramesPerSecondChangedEventHandler(String^ id,
			Platform::String^ fps);

		/// <summary>
		/// Delegate used to notify an update of the frame resolutions.
		/// </summary>
		public delegate void ResolutionChangedEventHandler(String^ id,
			unsigned int width, unsigned int height);

		/// <summary>
		/// Class used to get frame rate events from renderer.
		/// </summary>
		public ref class FrameCounterHelper sealed {
		public:
			/// <summary>
			/// Event fires when the frame rate changes.
			/// </summary>
			static event FramesPerSecondChangedEventHandler^ FramesPerSecondChanged;
		internal:
			static void FireEvent(String^ id, Platform::String^ str);
		};

		/// <summary>
		/// Class used to get frame size change events from renderer.
		/// </summary>
		public ref class ResolutionHelper sealed {
		public:
			/// <summary>
			/// Event fires when the resolution changes.
			/// </summary>
			static event ResolutionChangedEventHandler^ ResolutionChanged;
		internal:
			static void FireEvent(String^ id, unsigned int width, unsigned int height);
		};
	}
}

namespace Org {
	namespace WebRtc {
		namespace Internal {

			class WebRtcMediaSource;

			class WebRtcMediaStream :
				public RuntimeClass<RuntimeClassFlags<RuntimeClassType::WinRtClassicComMix>,
				IMFMediaStream, IMFMediaEventGenerator,
				IMFGetService> {
				InspectableClass(L"WebRtcMediaStream", BaseTrust)
			public:
				WebRtcMediaStream();
				virtual ~WebRtcMediaStream();
				HRESULT RuntimeClassInitialize(
					WebRtcMediaSource* source,
					String^ id,
					VideoFrameType frameType);

				// IMFMediaEventGenerator
				IFACEMETHOD(GetEvent)(DWORD dwFlags, IMFMediaEvent **ppEvent);
				IFACEMETHOD(BeginGetEvent)(IMFAsyncCallback *pCallback, IUnknown *punkState);
				IFACEMETHOD(EndGetEvent)(IMFAsyncResult *pResult, IMFMediaEvent **ppEvent);
				IFACEMETHOD(QueueEvent)(MediaEventType met, const GUID& guidExtendedType,
					HRESULT hrStatus, const PROPVARIANT *pvValue);
				// IMFMediaStream
				IFACEMETHOD(GetMediaSource)(IMFMediaSource **ppMediaSource);
				IFACEMETHOD(GetStreamDescriptor)(IMFStreamDescriptor **ppStreamDescriptor);
				IFACEMETHOD(RequestSample)(IUnknown *pToken);
				// IMFGetService
				IFACEMETHOD(GetService)(REFGUID guidService, REFIID riid, LPVOID *ppvObject);

				void RenderFrame(const webrtc::VideoFrame *frame);

				STDMETHOD(Start)(IMFPresentationDescriptor *pPresentationDescriptor,
					const GUID *pguidTimeFormat, const PROPVARIANT *pvarStartPosition);
				STDMETHOD(Stop)();
				STDMETHOD(Shutdown)();
				STDMETHOD(SetD3DManager)(ComPtr<IMFDXGIDeviceManager> manager);

			private:
				std::unique_ptr<webrtc::CriticalSectionWrapper> _lock;

				ComPtr<IMFMediaEventQueue> _eventQueue;

				WebRtcMediaSource* _source;
				String^ _id;
				VideoFrameType _frameType;

				static HRESULT CreateMediaType(unsigned int width, unsigned int height,
					unsigned int rotation, bool isH264, IMFMediaType** ppType);
				HRESULT MakeSampleCallback(const webrtc::VideoFrame* frame, IMFSample** sample);
				void FpsCallback(int fps);

				HRESULT ReplyToSampleRequest();

				std::unique_ptr<MediaSourceHelper> _helper;

				ComPtr<IMFMediaType> _mediaType;
				ComPtr<IMFDXGIDeviceManager> _deviceManager;
				ComPtr<IMFStreamDescriptor> _streamDescriptor;
				ULONGLONG _startTickCount;
				ULONGLONG _frameCount;
				int _index;
				ULONG _frameReady;
				ULONG _frameBeingQueued;

				// From the sample manager.
				HRESULT ResetMediaBuffers();
				static const int BufferCount = 3;
				std::vector<ComPtr<IMFMediaBuffer>> _mediaBuffers;
				int _frameBufferIndex;

				bool _gpuVideoBuffer;
				bool _started;
				bool _isShutdown;
			};
		}
	}
}  // namespace Org.WebRtc.Internal

#endif  // WEBRTC_BUILD_WINUWP_GYP_API_WEBRTCMEDIASTREAM_H_
