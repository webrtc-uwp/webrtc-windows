
// Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
//
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file in the root of the source
// tree. An additional intellectual property rights grant can be found
// in the file PATENTS.  All contributing project authors may
// be found in the AUTHORS file in the root of the source tree.

#include "WebRtcMediaSource.h"
#include "Media.h"
#include <mferror.h>
#include "common_video/video_common_winuwp.h"

namespace Org {
	namespace WebRtc {
		void Org::WebRtc::FrameCounterHelper::FireEvent(String^ id,
			Platform::String^ str) {
			Windows::UI::Core::CoreDispatcher^ _windowDispatcher = webrtc::VideoCommonWinUWP::GetCoreDispatcher();
			if (_windowDispatcher != nullptr) {
				_windowDispatcher->RunAsync(
					Windows::UI::Core::CoreDispatcherPriority::Normal,
					ref new Windows::UI::Core::DispatchedHandler([id, str] {
					FramesPerSecondChanged(id, str);
				}));
			} else {
				FramesPerSecondChanged(id, str);
			}
		}

		void Org::WebRtc::ResolutionHelper::FireEvent(String^ id,
			unsigned int width, unsigned int heigth) {
			Windows::UI::Core::CoreDispatcher^ _windowDispatcher = webrtc::VideoCommonWinUWP::GetCoreDispatcher();
			if (_windowDispatcher != nullptr) {
				_windowDispatcher->RunAsync(
					Windows::UI::Core::CoreDispatcherPriority::Normal,
					ref new Windows::UI::Core::DispatchedHandler([id, width, heigth] {
					ResolutionChanged(id, width, heigth);
				}));
			} else {
				ResolutionChanged(id, width, heigth);
			}
		}
	}
}

namespace Org {
	namespace WebRtc {
		namespace Internal {
			using Microsoft::WRL::MakeAndInitialize;

#define RETURN_ON_FAIL(code) { HRESULT hr = code; if (FAILED(hr)) {OutputDebugString(L"Failed\r\n"); return hr;} }

			WebRtcMediaSource::WebRtcVideoSink::WebRtcVideoSink(
				VideoFrameType frameType,
				ComPtr<WebRtcMediaStream> i420Stream,
				ComPtr<WebRtcMediaStream> h264Stream,
				WebRtcVideoSinkObserver *videoSinkObserver) :
				_i420Stream(i420Stream),
				_h264Stream(h264Stream),
				_videoSinkObserver(videoSinkObserver),
				_frameType(frameType) {
			}

			void WebRtcMediaSource::WebRtcVideoSink::OnFrame(const webrtc::VideoFrame& frame) {
				if (frame.video_frame_buffer()->ToI420() != nullptr) {
					if (_frameType == FrameTypeH264) {
						_videoSinkObserver->OnVideoFormatChanged(FrameTypeI420);
						_frameType = FrameTypeI420;
					}
					_i420Stream->RenderFrame(&frame);
				} else {
					if (_frameType == FrameTypeI420) {
						_videoSinkObserver->OnVideoFormatChanged(FrameTypeH264);
						_frameType = FrameTypeH264;
					}
					_h264Stream->RenderFrame(&frame);
				}
			}

			WebRtcMediaSource::WebRtcMediaSource() :
				_i420FirstStart(true),
				_h264FirstStart(true) {
			}

			WebRtcMediaSource::~WebRtcMediaSource() {
				_webRtcVideoSink.reset();
			}

			HRESULT WebRtcMediaSource::CreateMediaSource(
				WebRtcMediaSource** source,
				VideoFrameType frameType,
				String^ id) {
				*source = nullptr;
				ComPtr<WebRtcMediaSource> comSource;
				RETURN_ON_FAIL(MakeAndInitialize<WebRtcMediaSource>(
					&comSource, frameType, id));
				*source = comSource.Detach();
				return S_OK;
			}

			HRESULT WebRtcMediaSource::RuntimeClassInitialize(
				VideoFrameType frameType, String^ id) {
				rtc::CritScope lock(&_critSect);
				if (_eventQueue != nullptr)
					return S_OK;

				RETURN_ON_FAIL(MFCreateEventQueue(&_eventQueue));

				RETURN_ON_FAIL(MakeAndInitialize<WebRtcMediaStream>(
					&_i420Stream, this, id, FrameTypeI420));
				RETURN_ON_FAIL(MakeAndInitialize<WebRtcMediaStream>(
					&_h264Stream, this, id, FrameTypeH264));

				if (frameType == FrameTypeI420)
					_selectedStream = 0;
				else if (frameType == FrameTypeH264)
					_selectedStream = 1;

				_webRtcVideoSink.reset(new WebRtcVideoSink(
					frameType, _i420Stream, _h264Stream, this));

				ComPtr<IMFStreamDescriptor> i420StreamDescriptor;
				ComPtr<IMFStreamDescriptor> h264streamDescriptor;
				RETURN_ON_FAIL(_i420Stream->GetStreamDescriptor(&i420StreamDescriptor));
				RETURN_ON_FAIL(_h264Stream->GetStreamDescriptor(&h264streamDescriptor));
				IMFStreamDescriptor *streamDescriptors[2];
				streamDescriptors[0] = *i420StreamDescriptor.GetAddressOf();
				streamDescriptors[1] = *h264streamDescriptor.GetAddressOf();
				RETURN_ON_FAIL(MFCreatePresentationDescriptor(
					2, streamDescriptors, &_presDescriptor));
        
				RETURN_ON_FAIL(_presDescriptor->SelectStream(_selectedStream));

				return S_OK;
			}

			// WebRtcVideoSinkObserver
			void WebRtcMediaSource::OnVideoFormatChanged(VideoFrameType frameType) {
				// The MediaElement rendering component doesn't support video format
				// changes on the fly (I420<->H264). This case is not handled for now.
				return;
				rtc::CritScope lock(&_critSect);
				if (frameType == FrameTypeI420) {
					_selectedStream = 0;
					if (_presDescriptor != nullptr) {
						Stop();
						_presDescriptor->SelectStream(0);
						_presDescriptor->DeselectStream(1);
						PROPVARIANT startPosition;
						PropVariantInit(&startPosition);
						startPosition.scode = VT_EMPTY;
						Start(_presDescriptor.Get(), NULL, &startPosition);
					}
				} else if (frameType == FrameTypeH264) {
					_selectedStream = 1;
					if (_presDescriptor != nullptr) {
						Stop();
						_presDescriptor->SelectStream(1);
						_presDescriptor->DeselectStream(0);
						PROPVARIANT startPosition;
						PropVariantInit(&startPosition);
						startPosition.scode = VT_EMPTY;
						Start(_presDescriptor.Get(), NULL, &startPosition);
					}
				}
			}

			// IMFMediaEventGenerator
			IFACEMETHODIMP WebRtcMediaSource::GetEvent(
				DWORD dwFlags, IMFMediaEvent **ppEvent) {
				rtc::CritScope lock(&_critSect);
				if (_eventQueue == nullptr) {
					return MF_E_SHUTDOWN;
				}
				return _eventQueue->GetEvent(dwFlags, ppEvent);
			}

			IFACEMETHODIMP WebRtcMediaSource::BeginGetEvent(
				IMFAsyncCallback *pCallback, IUnknown *punkState) {
				rtc::CritScope lock(&_critSect);
				if (_eventQueue == nullptr) {
					return MF_E_SHUTDOWN;
				}
				return _eventQueue->BeginGetEvent(pCallback, punkState);
			}

			IFACEMETHODIMP WebRtcMediaSource::EndGetEvent(
				IMFAsyncResult *pResult, IMFMediaEvent **ppEvent) {
				rtc::CritScope lock(&_critSect);
				if (_eventQueue == nullptr) {
					return MF_E_SHUTDOWN;
				}
				return _eventQueue->EndGetEvent(pResult, ppEvent);
			}

			IFACEMETHODIMP WebRtcMediaSource::QueueEvent(
				MediaEventType met, const GUID& guidExtendedType,
				HRESULT hrStatus, const PROPVARIANT *pvValue) {
				rtc::CritScope lock(&_critSect);
				if (_eventQueue == nullptr) {
					return MF_E_SHUTDOWN;
				}
				return _eventQueue->QueueEventParamVar(
					met, guidExtendedType, hrStatus, pvValue);
			}

			// IMFMediaSource
			IFACEMETHODIMP WebRtcMediaSource::GetCharacteristics(
				DWORD *pdwCharacteristics) {
				rtc::CritScope lock(&_critSect);
				if (_eventQueue == nullptr) {
					return MF_E_SHUTDOWN;
				}
				*pdwCharacteristics = MFMEDIASOURCE_IS_LIVE;
				return S_OK;
			}

			IFACEMETHODIMP WebRtcMediaSource::CreatePresentationDescriptor(
				IMFPresentationDescriptor **ppPresentationDescriptor) {
				rtc::CritScope lock(&_critSect);
				if (_eventQueue == nullptr) {
					return MF_E_SHUTDOWN;
				}
				return _presDescriptor->Clone(ppPresentationDescriptor);
			}

			IFACEMETHODIMP WebRtcMediaSource::Start(
				IMFPresentationDescriptor *pPresentationDescriptor,
				const GUID *pguidTimeFormat, const PROPVARIANT *pvarStartPosition) {
				rtc::CritScope lock(&_critSect);
				OutputDebugString(L"WebRtcMediaSource::Start\r\n");
				if (_eventQueue == nullptr) {
					return MF_E_SHUTDOWN;
				}
				if (_selectedStream == 0 && _i420FirstStart) {
					ComPtr<IUnknown> unk;
					_i420Stream.As(&unk);
					RETURN_ON_FAIL(_eventQueue->QueueEventParamUnk(
						MENewStream, GUID_NULL, S_OK, unk.Get()));
					_i420FirstStart = false;
				}
				if (_selectedStream == 1 && _h264FirstStart) {
					ComPtr<IUnknown> unk;
					_h264Stream.As(&unk);
					RETURN_ON_FAIL(_eventQueue->QueueEventParamUnk(
						MENewStream, GUID_NULL, S_OK, unk.Get()));
					_h264FirstStart = false;
				}
				if (_selectedStream == 0) {
					RETURN_ON_FAIL(_i420Stream->Start(pPresentationDescriptor,
						pguidTimeFormat, pvarStartPosition));
				}
				if (_selectedStream == 1) {
					RETURN_ON_FAIL(_h264Stream->Start(pPresentationDescriptor,
						pguidTimeFormat, pvarStartPosition));
				}
				RETURN_ON_FAIL(QueueEvent(MESourceStarted,
				GUID_NULL, S_OK, pvarStartPosition));
				return S_OK;
			}

			IFACEMETHODIMP WebRtcMediaSource::Stop() {
				rtc::CritScope lock(&_critSect);
				OutputDebugString(L"WebRtcMediaSource::Stop\r\n");
				if (_eventQueue == nullptr) {
					return MF_E_SHUTDOWN;
				}
				if (_selectedStream == 0)
					RETURN_ON_FAIL(_i420Stream->Stop());
				if (_selectedStream == 1)
					RETURN_ON_FAIL(_h264Stream->Stop());
				RETURN_ON_FAIL(QueueEvent(MESourceStopped, GUID_NULL, S_OK, nullptr));
				return S_OK;
			}

			IFACEMETHODIMP WebRtcMediaSource::Pause() {
				return MF_E_INVALID_STATE_TRANSITION;
			}

			IFACEMETHODIMP WebRtcMediaSource::Shutdown() {
				rtc::CritScope lock(&_critSect);
				OutputDebugString(L"WebRtcMediaSource::Shutdown\r\n");
				if (_eventQueue != nullptr) {
					_eventQueue->Shutdown();
				}

				RETURN_ON_FAIL(_i420Stream->Shutdown());
				RETURN_ON_FAIL(_h264Stream->Shutdown());

				_presDescriptor = nullptr;
				_deviceManager = nullptr;
				_eventQueue = nullptr;
				_webRtcVideoSink.reset();
				_i420Stream.Reset();
				_h264Stream.Reset();
				return S_OK;
			}

			// IMFMediaSourceEx
			IFACEMETHODIMP WebRtcMediaSource::GetSourceAttributes(
				IMFAttributes **ppAttributes) {
				return E_NOTIMPL;
			}

			IFACEMETHODIMP WebRtcMediaSource::GetStreamAttributes(
				DWORD dwStreamIdentifier, IMFAttributes **ppAttributes) {
				return E_NOTIMPL;
			}

			IFACEMETHODIMP WebRtcMediaSource::SetD3DManager(IUnknown *pManager) {
				rtc::CritScope lock(&_critSect);
				if (_eventQueue == nullptr) {
					return MF_E_SHUTDOWN;
				}
				RETURN_ON_FAIL(pManager->QueryInterface(
					IID_IMFDXGIDeviceManager,
					reinterpret_cast<void**>(_deviceManager.ReleaseAndGetAddressOf())));

				RETURN_ON_FAIL(_i420Stream->SetD3DManager(_deviceManager));

				return S_OK;
			}

			IFACEMETHODIMP WebRtcMediaSource::GetService(
				REFGUID guidService, REFIID riid, LPVOID *ppvObject) {
				if (guidService == MF_RATE_CONTROL_SERVICE || guidService == MF_MEDIASOURCE_SERVICE) {
					HRESULT hr = QueryInterface(riid, ppvObject);
					return hr;
				}
				return MF_E_UNSUPPORTED_SERVICE;
			}

			IFACEMETHODIMP WebRtcMediaSource::SetRate(BOOL fThin, float flRate) {
				rtc::CritScope lock(&_critSect);
				if (_eventQueue == nullptr) {
					return MF_E_SHUTDOWN;
				}
				if (fThin) {
					return MF_E_THINNING_UNSUPPORTED;
				}

				PROPVARIANT pv;
				PropVariantInit(&pv);
				pv.vt = VT_R4;
				pv.fltVal = 1.0f;
				RETURN_ON_FAIL(QueueEvent(MESourceRateChanged, GUID_NULL, S_OK, &pv));

				return S_OK;
			}

			IFACEMETHODIMP WebRtcMediaSource::GetRate(BOOL *pfThin, float *pflRate) {
				rtc::CritScope lock(&_critSect);
				if (_eventQueue == nullptr) {
					return MF_E_SHUTDOWN;
				}
				*pfThin = FALSE;
				*pflRate = 1.0f;
				return S_OK;
			}

			IFACEMETHODIMP WebRtcMediaSource::GetSlowestRate(
				MFRATE_DIRECTION eDirection, BOOL fThin, float *pflRate) {
				rtc::CritScope lock(&_critSect);
				if (_eventQueue == nullptr) {
					return MF_E_SHUTDOWN;
				}
				if (eDirection == MFRATE_REVERSE) {
					return MF_E_REVERSE_UNSUPPORTED;
				}

				if (fThin) {
					return MF_E_THINNING_UNSUPPORTED;
				}

				if (!pflRate) {
					return E_POINTER;
				}

				*pflRate = 1.0f;

				return S_OK;
			}

			IFACEMETHODIMP WebRtcMediaSource::GetFastestRate(
				MFRATE_DIRECTION eDirection, BOOL fThin, float *pflRate) {
				rtc::CritScope lock(&_critSect);
				if (_eventQueue == nullptr) {
					return MF_E_SHUTDOWN;
				}
				if (eDirection == MFRATE_REVERSE) {
					return MF_E_REVERSE_UNSUPPORTED;
				}

				if (fThin) {
					return MF_E_THINNING_UNSUPPORTED;
				}

				if (!pflRate) {
					return E_POINTER;
				}

				*pflRate = 1.0f;

				return S_OK;
			}

			IFACEMETHODIMP WebRtcMediaSource::IsRateSupported(
				BOOL fThin, float flRate, float *pflNearestSupportedRate) {
				rtc::CritScope lock(&_critSect);
				if (_eventQueue == nullptr) {
					return MF_E_SHUTDOWN;
				}
				if (fThin) {
					return MF_E_THINNING_UNSUPPORTED;
				}

				if (pflNearestSupportedRate) {
					*pflNearestSupportedRate = 1.0f;
				}

				if (flRate != 1.0f) {
					return MF_E_UNSUPPORTED_RATE;
				}

				return S_OK;
			}

			void WebRtcMediaSource::RenderFrame(const webrtc::VideoFrame *frame) {
				rtc::CritScope lock(&_critSect);
				if (_selectedStream == 0) {
					if (_i420Stream != nullptr)
						_i420Stream->RenderFrame(frame);
				}
				else if (_selectedStream == 1) {
					if (_h264Stream != nullptr)
						_h264Stream->RenderFrame(frame);
				}
			}
		}
	}
}  // namespace Org.WebRtc.Internal
