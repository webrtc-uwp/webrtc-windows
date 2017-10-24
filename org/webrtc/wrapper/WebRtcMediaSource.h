
// Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
//
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file in the root of the source
// tree. An additional intellectual property rights grant can be found
// in the file PATENTS.  All contributing project authors may
// be found in the AUTHORS file in the root of the source tree.

#ifndef WEBRTC_BUILD_WINUWP_GYP_API_WEBRTCMEDIASOURCE_H_
#define WEBRTC_BUILD_WINUWP_GYP_API_WEBRTCMEDIASOURCE_H_

#include "WebRtcMediaStream.h"
#include <wrl.h>
#include <mfapi.h>
#include <mfidl.h>
#include <windows.media.h>
#include <windows.media.core.h>
#include "webrtc/system_wrappers/include/critical_section_wrapper.h"

interface IFrameSource;

using Microsoft::WRL::ComPtr;
using Microsoft::WRL::RuntimeClass;
using Microsoft::WRL::RuntimeClassFlags;
using Microsoft::WRL::RuntimeClassType;

namespace Org {
	namespace WebRtc {
		ref class MediaVideoTrack;
	}
}

namespace Org {
	namespace WebRtc {
		namespace Internal {

			class WebRtcMediaSource;

			class WebRtcVideoSinkObserver {
			public:
				virtual void OnVideoFormatChanged(VideoFrameType frameType) = 0;
			};

			class DECLSPEC_UUID("E306B192-18F8-49BF-BCA5-E675540E91D1") WebRtcMediaSource :
				public RuntimeClass<RuntimeClassFlags<RuntimeClassType::WinRtClassicComMix>,
					IMFMediaSourceEx, IMFMediaSource, IMFMediaEventGenerator,
					IMFGetService,
					IMFRateControl,
					IMFRateSupport,
					ABI::Windows::Media::Core::IMediaSource, IInspectable>,
				public WebRtcVideoSinkObserver {
				InspectableClass(L"WebRtcMediaSource", BaseTrust)
			public:
				class WebRtcVideoSink : public rtc::VideoSinkInterface<webrtc::VideoFrame> {
				public:
					WebRtcVideoSink(VideoFrameType frameType, 
						ComPtr<WebRtcMediaStream> i420Stream,
						ComPtr<WebRtcMediaStream> h264Stream,
						WebRtcVideoSinkObserver* videoSinkObserver);
					virtual void OnFrame(const webrtc::VideoFrame& frame) override;
				private:
					VideoFrameType _frameType;
					ComPtr<WebRtcMediaStream> _i420Stream;
					ComPtr<WebRtcMediaStream> _h264Stream;
					WebRtcVideoSinkObserver* _videoSinkObserver;
				};

				static HRESULT CreateMediaSource(
					WebRtcMediaSource** source,
					VideoFrameType frameType, String^ id);

				WebRtcMediaSource();
				virtual ~WebRtcMediaSource();
				HRESULT RuntimeClassInitialize(VideoFrameType frameType, String^ id);

				// WebRtcVideoSinkObserver
				virtual void OnVideoFormatChanged(VideoFrameType frameType) override;

				// IMFMediaEventGenerator
				IFACEMETHOD(GetEvent)(DWORD dwFlags, IMFMediaEvent **ppEvent);
				IFACEMETHOD(BeginGetEvent)(IMFAsyncCallback *pCallback, IUnknown *punkState);
				IFACEMETHOD(EndGetEvent)(IMFAsyncResult *pResult, IMFMediaEvent **ppEvent);
				IFACEMETHOD(QueueEvent)(MediaEventType met, const GUID& guidExtendedType,
					HRESULT hrStatus, const PROPVARIANT *pvValue);
				// IMFMediaSource
				IFACEMETHOD(GetCharacteristics)(DWORD *pdwCharacteristics);
				IFACEMETHOD(CreatePresentationDescriptor)(
					IMFPresentationDescriptor **ppPresentationDescriptor);
				IFACEMETHOD(Start)(IMFPresentationDescriptor *pPresentationDescriptor,
					const GUID *pguidTimeFormat, const PROPVARIANT *pvarStartPosition);
				IFACEMETHOD(Stop)();
				IFACEMETHOD(Pause)();
				IFACEMETHOD(Shutdown)();
				// IMFMediaSourceEx
				IFACEMETHOD(GetSourceAttributes)(IMFAttributes **ppAttributes);
				IFACEMETHOD(GetStreamAttributes)(DWORD dwStreamIdentifier,
					IMFAttributes **ppAttributes);
				IFACEMETHOD(SetD3DManager)(IUnknown *pManager);
				// IMFGetService
				IFACEMETHOD(GetService)(REFGUID guidService, REFIID riid, LPVOID *ppvObject);
				// IMFRateControl
				IFACEMETHOD(SetRate)(BOOL fThin, float flRate);
				IFACEMETHOD(GetRate)(BOOL *pfThin, float *pflRate);
				// IMFRateSupport
				IFACEMETHOD(GetSlowestRate)(MFRATE_DIRECTION eDirection, BOOL fThin, float *pflRate);
				IFACEMETHOD(GetFastestRate)(MFRATE_DIRECTION eDirection, BOOL fThin, float *pflRate);
				IFACEMETHOD(IsRateSupported)(BOOL fThin, float flRate, float *pflNearestSupportedRate);

				void RenderFrame(const webrtc::VideoFrame *frame);

			private:
				std::unique_ptr<webrtc::CriticalSectionWrapper> _lock;
				std::unique_ptr<WebRtcVideoSink> _webRtcVideoSink;
				ComPtr<WebRtcMediaStream> _i420Stream;
				ComPtr<WebRtcMediaStream> _h264Stream;
				ComPtr<IMFMediaEventQueue> _eventQueue;
				ComPtr<IMFPresentationDescriptor> _presDescriptor;
				ComPtr<IMFDXGIDeviceManager> _deviceManager;
				bool _i420FirstStart;
				bool _h264FirstStart;
				int _selectedStream;
			};
		}
	}
}  // namespace Org.WebRtc.Internal

#endif  // WEBRTC_BUILD_WINUWP_GYP_API_WEBRTCMEDIASOURCE_H_
