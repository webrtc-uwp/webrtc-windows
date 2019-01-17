
// Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
//
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file in the root of the source
// tree. An additional intellectual property rights grant can be found
// in the file PATENTS.  All contributing project authors may
// be found in the AUTHORS file in the root of the source tree.

#include "Media.h"
#include <stdio.h>
#include <ppltasks.h>
#include <mfapi.h>
#include <vector>
#include <string>
#include <set>
#include "PeerConnectionInterface.h"
#include "Marshalling.h"
#include "rtc_base/logging.h"
#include "api/videosourceinterface.h"
#include "pc/channelmanager.h"
#include "media/base/mediaengine.h"
#include "api/test/fakeconstraints.h"
#include "rtc_base/criticalsection.h"
#include "common_video/video_common_winuwp.h"
#include "third_party/winuwp_h264/H264Decoder/H264Decoder.h"

using Platform::Collections::Vector;
using Org::WebRtc::Internal::ToCx;
using Org::WebRtc::Internal::FromCx;
using Windows::Media::IMediaExtension;
using Windows::Media::Capture::MediaStreamType;
using Windows::Devices::Enumeration::DeviceClass;
using Windows::Devices::Enumeration::DeviceInformation;
using Windows::Devices::Enumeration::DeviceWatcherStatus;
using Windows::Devices::Enumeration::DeviceInformationCollection;
using Windows::Devices::Enumeration::EnclosureLocation;
using Windows::Foundation::TypedEventHandler;
using Windows::UI::Core::DispatchedHandler;
using Windows::UI::Core::CoreDispatcherPriority;

namespace {
  IVector<Org::WebRtc::MediaDevice^>^ g_videoDevices = ref new Vector<Org::WebRtc::MediaDevice^>();

  rtc::CriticalSection g_videoDevicesCritSect;
}

namespace Org {
	namespace WebRtc {

		// = MediaVideoTrack =========================================================

		MediaVideoTrack::MediaVideoTrack(
			rtc::scoped_refptr<webrtc::VideoTrackInterface> impl) :
			_impl(impl) {
		}

		MediaVideoTrack::~MediaVideoTrack() {
			_impl = nullptr;
		}

		String^ MediaVideoTrack::Kind::get() {
			if (_impl == nullptr)
				THROW_WEBRTC_NULL_REFERENCE_EXCEPTION("Invalid video track object");

			return ToCx(_impl->kind());
		}

		String^ MediaVideoTrack::Id::get() {
			if (_impl == nullptr)
				THROW_WEBRTC_NULL_REFERENCE_EXCEPTION("Invalid video track object");

			return ToCx(_impl->id());
		}

		bool MediaVideoTrack::Enabled::get() {
			if (_impl == nullptr)
				return false;

			return _impl->enabled();
		}

		void MediaVideoTrack::Enabled::set(bool value) {
			if (_impl == nullptr)
				THROW_WEBRTC_NULL_REFERENCE_EXCEPTION("Invalid video track object");

				_impl->set_enabled(value);
		}

		void MediaVideoTrack::Stop() {
			_impl = nullptr;
		}

		void MediaVideoTrack::SetRenderer(rtc::VideoSinkInterface<webrtc::VideoFrame>* renderer) {
			if (_impl == nullptr)
				THROW_WEBRTC_NULL_REFERENCE_EXCEPTION("Invalid video track object");

			_impl->AddOrUpdateSink(renderer, rtc::VideoSinkWants());
		}

		void MediaVideoTrack::UnsetRenderer(rtc::VideoSinkInterface<webrtc::VideoFrame>* renderer) {
			if (_impl == nullptr)
				THROW_WEBRTC_NULL_REFERENCE_EXCEPTION("Invalid video track object");

			_impl->RemoveSink(renderer);
		}

		// = MediaAudioTrack =========================================================

		MediaAudioTrack::MediaAudioTrack(
			rtc::scoped_refptr<webrtc::AudioTrackInterface> impl) :
			_impl(impl) {
		}

		String^ MediaAudioTrack::Kind::get() {
			if (_impl == nullptr)
				THROW_WEBRTC_NULL_REFERENCE_EXCEPTION("Invalid audio track object");

			return ToCx(_impl->kind());
		}

		String^ MediaAudioTrack::Id::get() {
			if (_impl == nullptr)
				THROW_WEBRTC_NULL_REFERENCE_EXCEPTION("Invalid audio track object");

			return ToCx(_impl->id());
		}

		bool MediaAudioTrack::Enabled::get() {
			if (_impl == nullptr)
				return false;

			return _impl->enabled();
		}

		void MediaAudioTrack::Enabled::set(bool value) {
			if (_impl == nullptr)
				THROW_WEBRTC_NULL_REFERENCE_EXCEPTION("Invalid audio track object");

			_impl->set_enabled(value);
		}

		void MediaAudioTrack::Stop() {
			_impl = nullptr;
		}

		// = MediaStream =============================================================

		MediaStream::MediaStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> impl)
			: _impl(impl) {

		}

		MediaStream::~MediaStream() {
			RTC_LOG(LS_INFO) << "MediaStream::~MediaStream";

		}

		rtc::scoped_refptr<webrtc::MediaStreamInterface> MediaStream::GetImpl() {
			return _impl;
		}

		IVector<MediaAudioTrack^>^ MediaStream::GetAudioTracks() {
			if (_impl == nullptr)
				return nullptr;

			auto ret = ref new Vector<MediaAudioTrack^>();
			for (auto track : _impl->GetAudioTracks()) {
				ret->Append(ref new MediaAudioTrack(track));
			}
			return ret;
		}

		String^ MediaStream::Id::get() {
			if (_impl == nullptr)
				return nullptr;
			return ToCx(_impl->label());
		}

		IVector<MediaVideoTrack^>^ MediaStream::GetVideoTracks() {
			if (_impl == nullptr)
				return nullptr;

			auto ret = ref new Vector<MediaVideoTrack^>();
			for (auto track : _impl->GetVideoTracks()) {
				ret->Append(ref new MediaVideoTrack(track));
			}
			return ret;
		}

		IVector<IMediaStreamTrack^>^ MediaStream::GetTracks() {
			if (_impl == nullptr)
				return nullptr;

			auto ret = ref new Vector<IMediaStreamTrack^>();
			for (auto track : _impl->GetAudioTracks()) {
				ret->Append(ref new MediaAudioTrack(track));
			}
			for (auto track : _impl->GetVideoTracks()) {
				ret->Append(ref new MediaVideoTrack(track));
			}
			return ret;
		}

		IMediaStreamTrack^ MediaStream::GetTrackById(String^ trackId) {
			if (_impl == nullptr)
				return nullptr;
			IMediaStreamTrack^ ret = nullptr;
			std::string trackIdStr = FromCx(trackId);
			// Search the audio tracks.
			auto audioTrack = _impl->FindAudioTrack(trackIdStr);
			if (audioTrack != nullptr) {
				ret = ref new MediaAudioTrack(audioTrack);
			}
			else {
				// Search the video tracks.
				auto videoTrack = _impl->FindVideoTrack(trackIdStr);
				if (videoTrack != nullptr) {
					ret = ref new MediaVideoTrack(videoTrack);
				}
			}
			return ret;
		}

		void MediaStream::AddTrack(IMediaStreamTrack^ track) {
			if (_impl == nullptr)
				return;

			std::string kind = FromCx(track->Kind);
			if (kind == "audio") {
				auto audioTrack = static_cast<MediaAudioTrack^>(track);
				_impl->AddTrack(audioTrack->GetImpl());

			}
			else if (kind == "video") {
				auto videoTrack = static_cast<MediaVideoTrack^>(track);
				_impl->AddTrack(videoTrack->GetImpl());

			}
			else {
				throw "Unknown track kind";
			}
		}

		void MediaStream::RemoveTrack(IMediaStreamTrack^ track) {
			if (_impl == nullptr)
				return;

			std::string kind = FromCx(track->Kind);
			if (kind == "audio") {
				auto audioTrack = static_cast<MediaAudioTrack^>(track);
				_impl->RemoveTrack(audioTrack->GetImpl());
			}
			else if (kind == "video") {
				auto videoTrack = static_cast<MediaVideoTrack^>(track);
				_impl->RemoveTrack(videoTrack->GetImpl());
			}
			else {
				throw "Unknown track kind";
			}
		}

		bool MediaStream::Active::get() {
			if (_impl == nullptr)
				return false;
			bool ret = false;
			for (auto track : _impl->GetAudioTracks()) {
				if (track->state() < webrtc::MediaStreamTrackInterface::kEnded) {
					ret = true;
				}
			}
			for (auto track : _impl->GetVideoTracks()) {
				if (track->state() < webrtc::MediaStreamTrackInterface::kEnded) {
					ret = true;
				}
			}
			return ret;
		}

		// = RawVideoStream =============================================================

		RawVideoStream::RawVideoStream(RawVideoSource^ videoSource) :
			_videoSource(videoSource) {
		}

		void RawVideoStream::RenderFrame(const webrtc::VideoFrame* frame) {
			rtc::scoped_refptr<webrtc::PlanarYuvBuffer> frameBuffer =
				static_cast<webrtc::PlanarYuvBuffer*>(frame->video_frame_buffer().get());
			_videoSource->RawVideoFrame((uint32)frame->width(), (uint32)frame->height(),
				Platform::ArrayReference<uint8>((uint8*)frameBuffer->DataY(),
				(unsigned int)(frameBuffer->StrideY() * frame->height())),
				frameBuffer->StrideY(),
				Platform::ArrayReference<uint8>((uint8*)frameBuffer->DataU(),
				(unsigned int)(frameBuffer->StrideU() * ((frame->height() + 1) / 2))),
				frameBuffer->StrideU(),
				Platform::ArrayReference<uint8>((uint8*)frameBuffer->DataV(),
				(unsigned int)(frameBuffer->StrideV() * ((frame->height() + 1) / 2))),
				frameBuffer->StrideV());
		}

		// = RawVideoSource =============================================================

		RawVideoSource::RawVideoSource(MediaVideoTrack^ track) :
			_videoStream(new RawVideoStream(this)),
			_track(track) {
			_track->SetRenderer(_videoStream.get());
		}

		void RawVideoSource::RawVideoFrame(uint32 width, uint32 height,
			const Platform::Array<uint8>^ yPlane, uint32 yPitch,
			const Platform::Array<uint8>^ vPlane, uint32 vPitch,
			const Platform::Array<uint8>^ uPlane, uint32 uPitch) {
			OnRawVideoFrame(width, height, yPlane, yPitch, vPlane, vPitch, uPlane, uPitch);
		}

		RawVideoSource::~RawVideoSource() {
			_track->UnsetRenderer(_videoStream.get());
		}

		// = EncodedVideoStream =============================================================

		EncodedVideoStream::EncodedVideoStream(EncodedVideoSource^ videoSource) :
			_videoSource(videoSource) {
		}

		void EncodedVideoStream::RenderFrame(const webrtc::VideoFrame* frame) {
			rtc::scoped_refptr<webrtc::NativeHandleBuffer> frameBuffer =
				static_cast<webrtc::NativeHandleBuffer*>(frame->video_frame_buffer().get());
			ComPtr<IMFSample> pSample = (IMFSample*)frameBuffer->native_handle();
			if (pSample == nullptr)
				return;
			ComPtr<IMFMediaBuffer> pBuffer;
			if (FAILED(pSample->GetBufferByIndex(0, &pBuffer))) {
				RTC_LOG(LS_ERROR) << "Failed to retrieve buffer.";
				return;
			}
			BYTE* pBytes;
			DWORD maxLength, curLength;
			if (FAILED(pBuffer->Lock(&pBytes, &maxLength, &curLength))) {
				RTC_LOG(LS_ERROR) << "Failed to lock buffer.";
				return;
			}
			_videoSource->EncodedVideoFrame((uint32)frame->width(), (uint32)frame->height(),
			Platform::ArrayReference<uint8>((uint8*)pBytes, curLength));
			if (FAILED(pBuffer->Unlock())) {
				RTC_LOG(LS_ERROR) << "Failed to unlock buffer";
				return;
			}
		}

		// = EncodedVideoSource =============================================================

		EncodedVideoSource::EncodedVideoSource(MediaVideoTrack^ track) :
			_videoStream(new EncodedVideoStream(this)),
			_track(track) {
			_track->SetRenderer(_videoStream.get());
		}

		void EncodedVideoSource::EncodedVideoFrame(uint32 width, uint32 height,
		const Platform::Array<uint8>^ frameData) {
			OnEncodedVideoFrame(width, height, frameData);
		}

		EncodedVideoSource::~EncodedVideoSource() {
			_track->UnsetRenderer(_videoStream.get());
		}
		
		// = MFSampleVideoStream =============================================================

		MFSampleVideoStream::MFSampleVideoStream(MFSampleVideoSource^ videoSource) :
			_videoSource(videoSource) {
		}

		void MFSampleVideoStream::VideoFrameReceived(void* pSample) {
			IMFSample* mfSample = static_cast<IMFSample*>(pSample);
			_videoSource->MFSampleVideoFrame(mfSample);
		}

		// = MFSampleVideoSource =============================================================

		MFSampleVideoSource::MFSampleVideoSource() :
			_videoStream(new MFSampleVideoStream(this)) {
			webrtc::videocapturemodule::AppStateDispatcher::Instance()->AddObserver(_videoStream.get());
		}

		void MFSampleVideoSource::MFSampleVideoFrame(void* pSample) {
			OnMFSampleVideoFrame(reinterpret_cast<uint64>(pSample));
		}

		MFSampleVideoSource::~MFSampleVideoSource() {
			webrtc::videocapturemodule::AppStateDispatcher::Instance()->RemoveObserver(_videoStream.get());
		}

		// = Media ===================================================================

		const char kAudioLabel[] = "audio_label_%llx";
		const char kVideoLabel[] = "video_label_%llx";
		const char kStreamLabel[] = "stream_label_%llx";
		// we will append current time (uint32 in Hex, e.g.:
		// 8chars to the end to generate a unique string)

		Media::VideoFrameSink::VideoFrameSink(MediaElement^ mediaElement, String^ id) :
			_firstFrameReceived(false),
			_mediaElement(mediaElement),
			_id(id) { }

		void Media::VideoFrameSink::OnFrame(const webrtc::VideoFrame& frame) {
			if (!_firstFrameReceived) {
				_firstFrameReceived = true;
				if (frame.video_frame_buffer()->ToI420() != nullptr)
					_frameType = Internal::FrameTypeI420;
				else
					_frameType = Internal::FrameTypeH264;

				auto handler = ref new DispatchedHandler([this]() {
					std::unique_lock<std::mutex> lock(_mutex);
					_mediaSource = Internal::RTMediaStreamSource::CreateMediaSource(nullptr, _frameType, _id);
					_mediaElement->SetMediaStreamSource(_mediaSource->GetMediaStreamSource());
				});

				Windows::UI::Core::CoreDispatcher^ windowDispatcher =
					webrtc::VideoCommonWinUWP::GetCoreDispatcher();
				if (windowDispatcher != nullptr) {
					auto dispatcher_action = windowDispatcher->RunAsync(
						CoreDispatcherPriority::Normal, handler);
					Concurrency::create_task(dispatcher_action);
				}
				else {
					handler->Invoke();
				}
			}

			{
				std::unique_lock<std::mutex> lock(_mutex);
				if (_mediaSource) {
					while (!_receivedFrames.empty()) {
						_mediaSource->RenderFrame(&_receivedFrames.front());
						_receivedFrames.pop();
					}
					_mediaSource->RenderFrame(&frame);
				}
				else {
					_receivedFrames.push(frame);
				}
			}
		}

		Media::Media() :
			_videoCaptureDeviceChanged(true) {
			_dev_manager = std::unique_ptr<Internal::WinUWPDeviceManager>
				(Internal::DeviceManagerFactory::Create());

			if (!_dev_manager->Init()) {
				RTC_LOG(LS_ERROR) << "Can't create device manager";
				return;
			}
			SubscribeToMediaDeviceChanges();

			// Warning, do not perform time consuming operation in this constructor
			// such as audio/video device enumeration, which might cause threading issue
			// for WinJS app on windows8.1
		}

		Media::~Media() {
			UnsubscribeFromMediaDeviceChanges();
		}

		Media^ Media::CreateMedia() {
			return ref new Media();
		}

		namespace globals {
			extern cricket::VideoFormat gPreferredVideoCaptureFormat;
		}

		IAsyncOperation<MediaStream^>^ Media::GetUserMedia(
			RTCMediaStreamConstraints^ mediaStreamConstraints) {

			IAsyncOperation<MediaStream^>^ asyncOp = Concurrency::create_async(
				[this, mediaStreamConstraints]() -> MediaStream^ {
				return globals::RunOnGlobalThread<MediaStream^>([this,
					mediaStreamConstraints]()->MediaStream^ {
					// This is the stream returned.
					char streamLabel[32];
					_snprintf(streamLabel, sizeof(streamLabel), kStreamLabel,
						rtc::CreateRandomId64());
					rtc::scoped_refptr<webrtc::MediaStreamInterface> stream =
						globals::gPeerConnectionFactory->CreateLocalMediaStream(streamLabel);

					auto ret = ref new MediaStream(stream);

					if (mediaStreamConstraints->audioEnabled) {
						RTC_LOG(LS_INFO) << "Creating audio track.";
						char audioLabel[32];
						_snprintf(audioLabel, sizeof(audioLabel), kAudioLabel,
							rtc::CreateRandomId64());
						rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track(
							globals::gPeerConnectionFactory->CreateAudioTrack(
								audioLabel,
								globals::gPeerConnectionFactory->CreateAudioSource(NULL)));
						RTC_LOG(LS_INFO) << "Adding audio track to stream.";
						auto audioTrack = ref new MediaAudioTrack(audio_track);
						ret->AddTrack(audioTrack);
					}

					if (mediaStreamConstraints->videoEnabled) {
						cricket::VideoCapturer* videoCapturer = NULL;
						cricket::Device* videoCaptureDevice = nullptr;

						std::vector<cricket::Device> videoDevices;
						globals::RunOnGlobalThread<void>([this, &videoDevices] {
							if (!_dev_manager->GetVideoCaptureDevices(&videoDevices)) {
								RTC_LOG(LS_ERROR) << "Can't get video capture devices list";
							}
						});
						if (_selectedVideoDevice.id == "") {
							// No device selected by app, try to use the first video device as the capturer.
							videoCaptureDevice = videoDevices.size() ? &(videoDevices[0])
								: nullptr;
						}
						else {
							rtc::CritScope lock(&g_videoDevicesCritSect);
							// Make sure the selected video device is still connected
							for (auto& capturer : videoDevices) {
								if (capturer.id == _selectedVideoDevice.id) {
									videoCaptureDevice = &capturer;
									break;
								}
							}
							if (videoCaptureDevice == nullptr) {
								// Selected device not connected anymore, try to use the first video device as the capturer.
								RTC_LOG(LS_WARNING) << "Selected video capturer ("
									<< _selectedVideoDevice.name << ") not found. ";
								videoCaptureDevice = videoDevices.size() ? &(videoDevices[0])
									: nullptr;
								if (videoCaptureDevice != nullptr) {
									RTC_LOG(LS_WARNING) << "Using video capturer "
										<< videoCaptureDevice->name;
								}
							}
						}
						if (videoCaptureDevice != nullptr) {							
							videoCapturer = _dev_manager->CreateVideoCapturer(
								*videoCaptureDevice);
						}

						char videoLabel[32];
						_snprintf(videoLabel, sizeof(videoLabel), kVideoLabel,
							rtc::CreateRandomId64());

						// Add a video track
						if (videoCapturer != nullptr) {
							webrtc::FakeConstraints constraints;
							constraints.SetMandatory(webrtc::MediaConstraintsInterface::kMinWidth, globals::gPreferredVideoCaptureFormat.width);
							constraints.SetMandatory(webrtc::MediaConstraintsInterface::kMinHeight, globals::gPreferredVideoCaptureFormat.height);
							constraints.SetMandatory(webrtc::MediaConstraintsInterface::kMaxWidth, globals::gPreferredVideoCaptureFormat.width);
							constraints.SetMandatory(webrtc::MediaConstraintsInterface::kMaxHeight, globals::gPreferredVideoCaptureFormat.height);
							constraints.SetMandatoryMaxFrameRate(cricket::VideoFormat::IntervalToFps(globals::gPreferredVideoCaptureFormat.interval));
							constraints.SetMandatory(webrtc::MediaConstraintsInterface::kEnableMrc, globals::gPreferredVideoCaptureFormat.mrcEnabled);

							RTC_LOG(LS_INFO) << "Creating video track.";
							rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track(
								globals::gPeerConnectionFactory->CreateVideoTrack(
									videoLabel,
									globals::gPeerConnectionFactory->CreateVideoSource(
										videoCapturer, &constraints)));
							RTC_LOG(LS_INFO) << "Adding video track to stream.";
							auto videoTrack = ref new MediaVideoTrack(video_track);
							ret->AddTrack(videoTrack);
						}
					}

					return ret;
				});
			});

			return asyncOp;
		}

		Platform::IntPtr Media::CreateMediaStreamSource(MediaVideoTrack^ track, String^ type, String^ id) {
			Internal::VideoFrameType frameType;
			if (_wcsicmp(type->Data(), L"i420") == 0)
				frameType = Internal::VideoFrameType::FrameTypeI420;
			else if (_wcsicmp(type->Data(), L"h264") == 0)
				frameType = Internal::VideoFrameType::FrameTypeH264;
			else
				return nullptr;
			return globals::RunOnGlobalThread<void*>([track, frameType, id]()->void* {
				Internal::RTMediaStreamSource^ mediaSource =
					Internal::RTMediaStreamSource::CreateMediaSource(track, frameType, id);
				ComPtr<IInspectable> inspectable = reinterpret_cast<IInspectable*>(mediaSource->GetMediaStreamSource());
				return inspectable.Detach();
			});
		}

		void Media::AddVideoTrackMediaElementPair(MediaVideoTrack^ track, MediaElement^ mediaElement, String^ id) {
			std::list<std::unique_ptr<VideoTrackMediaElementPair>>::iterator iter =
				_videoTrackMediaElementPairList.begin();
			while (iter != _videoTrackMediaElementPairList.end()) {
				if ((*iter)->_videoTrack == track) {
					(*iter)->_videoSink.reset(new VideoFrameSink(mediaElement, id));
					(*iter)->_mediaElement = mediaElement;
					track->SetRenderer((*iter)->_videoSink.get());
					return;
				}
				iter++;
			}
			_videoTrackMediaElementPairList.push_back(
				std::unique_ptr<VideoTrackMediaElementPair>(new VideoTrackMediaElementPair()));
			_videoTrackMediaElementPairList.back()->_videoTrack = track;
			_videoTrackMediaElementPairList.back()->_videoSink.reset(new VideoFrameSink(mediaElement, id));
			_videoTrackMediaElementPairList.back()->_mediaElement = mediaElement;
			track->SetRenderer(_videoTrackMediaElementPairList.back()->_videoSink.get());
		}

		void Media::RemoveVideoTrackMediaElementPair(MediaVideoTrack^ track) {
			std::list<std::unique_ptr<VideoTrackMediaElementPair>>::iterator iter =
				_videoTrackMediaElementPairList.begin();
			while (iter != _videoTrackMediaElementPairList.end()) {
				if ((*iter)->_videoTrack == track) {
					(*iter)->_videoTrack->UnsetRenderer((*iter)->_videoSink.get());
					(*iter)->_mediaElement->Stop();
					(*iter)->_mediaElement->Source = nullptr;
					_videoTrackMediaElementPairList.erase(iter);
					return;
				}
				iter++;
			}
		}

		RawVideoSource^ Media::CreateRawVideoSource(MediaVideoTrack^ track) {
			return ref new RawVideoSource(track);
		}

		EncodedVideoSource^ Media::CreateEncodedVideoSource(MediaVideoTrack^ track) {
			return ref new EncodedVideoSource(track);
		}

		MFSampleVideoSource^ Media::CreateMFSampleVideoSource() {
			return ref new MFSampleVideoSource();
		}

		IVector<MediaDevice^>^ Media::GetVideoCaptureDevices() {
			rtc::CritScope lock(&g_videoDevicesCritSect);
			std::vector<cricket::Device> videoDevices;
			DeviceInformationCollection^ dev_info_collection = nullptr;

			if (_videoCaptureDeviceChanged) {
				// Get list of devices from device manager.
				globals::RunOnGlobalThread<void>([this, &videoDevices, &dev_info_collection] {
					if (!_dev_manager->GetVideoCaptureDevices(&videoDevices)) {
						RTC_LOG(LS_ERROR) << "Can't enumerate video capture devices";
					}

					// Obtain also the list of devices directly from the OS API.
					// Only device location will be used from this list.
					Concurrency::create_task(DeviceInformation::FindAllAsync(
						DeviceClass::VideoCapture)).then([this, &dev_info_collection](
							Concurrency::task<DeviceInformationCollection^> find_task) {
						try {
							dev_info_collection = find_task.get();
						}
						catch (Platform::Exception^ e) {
							RTC_LOG(LS_ERROR)
								<< "Failed to retrieve device info collection. "
								<< rtc::ToUtf8(e->Message->Data());
						}
					}).wait();
				});


				g_videoDevices->Clear();
				for (auto videoDev : videoDevices) {
					EnclosureLocation^ location = nullptr;
					if (dev_info_collection != nullptr) {
						for (unsigned int i = 0; i < dev_info_collection->Size; i++) {
							auto dev_info = dev_info_collection->GetAt(i);
							if (rtc::ToUtf8(dev_info->Id->Data()) == videoDev.id) {
								location = dev_info->EnclosureLocation;
								break;
							}
						}
					}
					g_videoDevices->Append(ref new MediaDevice(ToCx(videoDev.id), ToCx(videoDev.name), location));
				}
				_videoCaptureDeviceChanged = false;
			}
			return g_videoDevices;
		}

		void Media::SelectVideoDevice(MediaDevice^ device) {
			rtc::CritScope lock(&g_videoDevicesCritSect);
			_selectedVideoDevice.id = "";
			_selectedVideoDevice.name = "";
			for (auto videoDev : g_videoDevices) {
				if (videoDev->Id == device->Id) {
					_selectedVideoDevice.id = FromCx(videoDev->Id);
					_selectedVideoDevice.name = FromCx(videoDev->Name);
					break;
				}
			}
		}

		void Media::OnAppSuspending() {
			// https://msdn.microsoft.com/library/windows/apps/br241124
			// Note  For Windows Phone Store apps, music and media apps should clean up
			// the MediaCapture object and associated resources in the Suspending event
			// handler and recreate them in the Resuming event handler.
			webrtc::videocapturemodule::MediaCaptureDevicesWinUWP::Instance()->
				ClearCaptureDevicesCache();
		}

		void Media::SetDisplayOrientation(
			Windows::Graphics::Display::DisplayOrientations display_orientation) {
			webrtc::videocapturemodule::AppStateDispatcher::Instance()->
				DisplayOrientationChanged(display_orientation);
		}

		IAsyncOperation<IVector<CaptureCapability^>^>^
			MediaDevice::GetVideoCaptureCapabilities() {
			auto op = concurrency::create_async([this]() -> IVector<CaptureCapability^>^ {
				auto mediaCapture =
					webrtc::videocapturemodule::MediaCaptureDevicesWinUWP::Instance()->
					GetMediaCapture(_id);
				if (mediaCapture == nullptr) {
					return nullptr;
				}

				auto streamProperties =
					mediaCapture->VideoDeviceController->GetAvailableMediaStreamProperties(
						MediaStreamType::VideoRecord);
				if (streamProperties == nullptr) {
					return nullptr;
				}
				auto ret = ref new Vector<CaptureCapability^>();
				std::set<std::wstring> descSet;
				for (auto prop : streamProperties) {
					if (prop->Type != L"Video") {
						continue;
					}
					auto videoProp =
						static_cast<Windows::Media::MediaProperties::
						IVideoEncodingProperties^>(prop);
					if ((videoProp->FrameRate == nullptr) ||
						(videoProp->FrameRate->Numerator == 0) ||
						(videoProp->FrameRate->Denominator == 0) ||
						(videoProp->Width == 0) || (videoProp->Height == 0)) {
						continue;
					}
					auto cap = ref new CaptureCapability(videoProp->Width, videoProp->Height,
						videoProp->FrameRate->Numerator / videoProp->FrameRate->Denominator,
						false, videoProp->PixelAspectRatio);
					if (descSet.find(cap->FullDescription->Data()) == descSet.end()) {
						ret->Append(cap);
						descSet.insert(cap->FullDescription->Data());
					}
				}
				return ret;
			});
			return op;
		}

		void Media::SubscribeToMediaDeviceChanges() {
			_videoCaptureWatcher = DeviceInformation::CreateWatcher(
				DeviceClass::VideoCapture);

			_videoCaptureWatcher->Added += ref new TypedEventHandler<DeviceWatcher^,
				DeviceInformation^>(this, &Media::OnMediaDeviceAdded);
			_videoCaptureWatcher->Removed += ref new TypedEventHandler<DeviceWatcher^,
				DeviceInformationUpdate^>(this, &Media::OnMediaDeviceRemoved);

			_videoCaptureWatcher->Start();
		}

		void Media::UnsubscribeFromMediaDeviceChanges() {
			if (_videoCaptureWatcher != nullptr) {
				_videoCaptureWatcher->Stop();
			}
		}

		void Media::OnMediaDeviceAdded(DeviceWatcher^ sender,
			DeviceInformation^ args) {
			// Do not send notifications while DeviceWatcher automatically
			// enumerates devices.
			if (sender->Status != DeviceWatcherStatus::EnumerationCompleted)
				return;
			if (sender == _videoCaptureWatcher) {
				RTC_LOG(LS_INFO) << "OnVideoCaptureAdded";
				_videoCaptureDeviceChanged = true;
				OnMediaDevicesChanged(MediaDeviceType::MediaDeviceType_VideoCapture);
				RTC_LOG(LS_INFO) << "OnVideoCaptureAdded END";
			}
		}

		void Media::OnMediaDeviceRemoved(DeviceWatcher^ sender,
			DeviceInformationUpdate^ updateInfo) {
			// Do not send notifs while DeviceWatcher automaticall enumerates devices
			if (sender->Status != DeviceWatcherStatus::EnumerationCompleted)
				return;
			if (sender == _videoCaptureWatcher) {
				// Need to remove the cached MediaCapture intance if device removed,
				// otherwise, DeviceWatchers stops working properly
				// (event handlers are not called each time)
				webrtc::videocapturemodule::MediaCaptureDevicesWinUWP::Instance()->
					RemoveMediaCapture(updateInfo->Id);
				_videoCaptureDeviceChanged = true;
				OnMediaDevicesChanged(MediaDeviceType::MediaDeviceType_VideoCapture);
			}
		}
	}
}  // namespace Org.WebRtc
