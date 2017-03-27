
// Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
//
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file in the root of the source
// tree. An additional intellectual property rights grant can be found
// in the file PATENTS.  All contributing project authors may
// be found in the AUTHORS file in the root of the source tree.

#include "WebRtcMediaSource.h"
#include <mferror.h>

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

			void WebRtcMediaSource::WebRtcVideoSink::OnFrame(const cricket::VideoFrame& frame) {
				if (frame.video_frame_buffer()->native_handle() == nullptr) {
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
				_h264FirstStart(true),
				_lock(webrtc::CriticalSectionWrapper::CreateCriticalSection()) {
			}

			WebRtcMediaSource::~WebRtcMediaSource() {
				_webRtcVideoSink.reset();
			}

			HRESULT WebRtcMediaSource::CreateMediaSource(
				IMediaSource** source,
				Org::WebRtc::MediaVideoTrack^ track,
				String^ id) {
				*source = nullptr;
				ComPtr<WebRtcMediaSource> internalRet;
				RETURN_ON_FAIL(MakeAndInitialize<WebRtcMediaSource>(
					&internalRet, track, id));
				ComPtr<IMediaSource> ret;
				internalRet.As(&ret);
				*source = ret.Detach();
				return S_OK;
			}

			HRESULT WebRtcMediaSource::RuntimeClassInitialize(
				Org::WebRtc::MediaVideoTrack^ track, String^ id) {
				webrtc::CriticalSectionScoped csLock(_lock.get());
				if (_eventQueue != nullptr)
					return S_OK;

				RETURN_ON_FAIL(MFCreateEventQueue(&_eventQueue));

				RETURN_ON_FAIL(MakeAndInitialize<WebRtcMediaStream>(
					&_i420Stream, this, id, FrameTypeI420));
				RETURN_ON_FAIL(MakeAndInitialize<WebRtcMediaStream>(
					&_h264Stream, this, id, FrameTypeH264));

				if (!track->GetImpl()->GetSource()->IsH264Source())
					_selectedStream = 0;
				else
					_selectedStream = 1;

				_webRtcVideoSink.reset(new WebRtcVideoSink(
				_selectedStream == 0 ? FrameTypeI420 : FrameTypeH264,
				_i420Stream, _h264Stream, this));
				_track = track;
				_track->SetRenderer(_webRtcVideoSink.get());

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
				webrtc::CriticalSectionScoped csLock(_lock.get());
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
				webrtc::CriticalSectionScoped csLock(_lock.get());
				if (_eventQueue == nullptr) {
					return MF_E_SHUTDOWN;
				}
				return _eventQueue->GetEvent(dwFlags, ppEvent);
			}

			IFACEMETHODIMP WebRtcMediaSource::BeginGetEvent(
				IMFAsyncCallback *pCallback, IUnknown *punkState) {
				webrtc::CriticalSectionScoped csLock(_lock.get());
				if (_eventQueue == nullptr) {
					return MF_E_SHUTDOWN;
				}
				return _eventQueue->BeginGetEvent(pCallback, punkState);
			}

			IFACEMETHODIMP WebRtcMediaSource::EndGetEvent(
				IMFAsyncResult *pResult, IMFMediaEvent **ppEvent) {
				webrtc::CriticalSectionScoped csLock(_lock.get());
				if (_eventQueue == nullptr) {
					return MF_E_SHUTDOWN;
				}
				return _eventQueue->EndGetEvent(pResult, ppEvent);
			}

			IFACEMETHODIMP WebRtcMediaSource::QueueEvent(
				MediaEventType met, const GUID& guidExtendedType,
				HRESULT hrStatus, const PROPVARIANT *pvValue) {
				webrtc::CriticalSectionScoped csLock(_lock.get());
				if (_eventQueue == nullptr) {
					return MF_E_SHUTDOWN;
				}
				return _eventQueue->QueueEventParamVar(
					met, guidExtendedType, hrStatus, pvValue);
			}

			// IMFMediaSource
			IFACEMETHODIMP WebRtcMediaSource::GetCharacteristics(
				DWORD *pdwCharacteristics) {
				webrtc::CriticalSectionScoped csLock(_lock.get());
				if (_eventQueue == nullptr) {
					return MF_E_SHUTDOWN;
				}
				*pdwCharacteristics = MFMEDIASOURCE_IS_LIVE;
				return S_OK;
			}

			IFACEMETHODIMP WebRtcMediaSource::CreatePresentationDescriptor(
				IMFPresentationDescriptor **ppPresentationDescriptor) {
				webrtc::CriticalSectionScoped csLock(_lock.get());
				if (_eventQueue == nullptr) {
					return MF_E_SHUTDOWN;
				}
				return _presDescriptor->Clone(ppPresentationDescriptor);
			}

			IFACEMETHODIMP WebRtcMediaSource::Start(
				IMFPresentationDescriptor *pPresentationDescriptor,
				const GUID *pguidTimeFormat, const PROPVARIANT *pvarStartPosition) {
				webrtc::CriticalSectionScoped csLock(_lock.get());
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
				webrtc::CriticalSectionScoped csLock(_lock.get());
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
				webrtc::CriticalSectionScoped csLock(_lock.get());
				OutputDebugString(L"WebRtcMediaSource::Shutdown\r\n");
				if (_eventQueue != nullptr) {
					_eventQueue->Shutdown();
				}

				_track->UnsetRenderer(_webRtcVideoSink.get());

				RETURN_ON_FAIL(_i420Stream->Shutdown());
				RETURN_ON_FAIL(_h264Stream->Shutdown());

				_presDescriptor = nullptr;
				_deviceManager = nullptr;
				_track = nullptr;
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
				webrtc::CriticalSectionScoped csLock(_lock.get());
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
				webrtc::CriticalSectionScoped csLock(_lock.get());
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
				webrtc::CriticalSectionScoped csLock(_lock.get());
				if (_eventQueue == nullptr) {
					return MF_E_SHUTDOWN;
				}
				*pfThin = FALSE;
				*pflRate = 1.0f;
				return S_OK;
			}

			IFACEMETHODIMP WebRtcMediaSource::GetSlowestRate(
				MFRATE_DIRECTION eDirection, BOOL fThin, float *pflRate) {
				webrtc::CriticalSectionScoped csLock(_lock.get());
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
				webrtc::CriticalSectionScoped csLock(_lock.get());
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
				webrtc::CriticalSectionScoped csLock(_lock.get());
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
		}
	}
}  // namespace Org.WebRtc.Internal
