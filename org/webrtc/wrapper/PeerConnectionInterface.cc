
// Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
//
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file in the root of the source
// tree. An additional intellectual property rights grant can be found
// in the file PATENTS.  All contributing project authors may
// be found in the AUTHORS file in the root of the source tree.
#include "PeerConnectionInterface.h"

#include <ppltasks.h>
#include <map>
#include <string>
#include <functional>
#include <codecvt>

#include "GlobalObserver.h"
#include "Marshalling.h"
#include "DataChannel.h"
#include "Media.h"
#include "rtc_base/ssladapter.h"
#include "rtc_base/win32socketinit.h"
#include "rtc_base/thread.h"
#include "rtc_base/bind.h"
#include "rtc_base/event_tracer.h"
#include "rtc_base/loggingserver.h"
#include "rtc_base/stream.h"
#include "test/field_trial.h"
#include "api/test/fakeconstraints.h"
#include "pc/channelmanager.h"
#include "rtc_base/win32.h"
#include "rtc_base/timeutils.h"
#include "third_party/winuwp_h264/winuwp_h264_factory.h"
#include "common_video/video_common_winuwp.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"

#include "webrtc/modules/audio_device/win/audio_device_wasapi_win.h"

using Org::WebRtc::Internal::FromCx;
using Org::WebRtc::Internal::ToCx;
using Platform::Collections::Vector;
using Windows::Media::Capture::MediaCapture;
using Windows::Media::Capture::MediaCaptureInitializationSettings;
using rtc::FileStream;


// Any globals we need to keep around.
namespace Org {
	namespace WebRtc {

		namespace globals {

			bool certificateVerifyCallBack(void* cert) {
				return true;
			}

			static const std::string logFileName = "_webrtc_logging.log";
			double gCurrentCPUUsage = 0.0;
			uint64 gCurrentMEMUsage = 0;

			// helper function to get default output path for the app
			std::string OutputPath() {
				auto folder = Windows::Storage::ApplicationData::Current->LocalFolder;
				wchar_t buffer[255];
				wcsncpy_s(buffer, 255, folder->Path->Data(), _TRUNCATE);
				return rtc::ToUtf8(buffer) + "\\";
			}

			// helper function to convert a std string to Platform string
			String^ toPlatformString(std::string aString) {
				std::wstring wide_str = std::wstring(aString.begin(), aString.end());
				Platform::String^ p_string = ref new Platform::String(wide_str.c_str());
				return p_string;
			}

			/**
			 * a private class only used in this file, which implements LogSink for logging to file
			 */
			class FileLogSink
				: public rtc::LogSink {
			public:
				explicit FileLogSink(rtc::FileStream* fStream) {
					fileStream_.reset(fStream);
				}
				rtc::FileStream* file() { return fileStream_.get(); }
			private:
				void OnLogMessage(const std::string& message) override {
					fileStream_->WriteAll(
						message.data(), message.size(), nullptr, nullptr);
				}

				std::unique_ptr<rtc::FileStream> fileStream_;
			};

			static bool isInitialized = false;

			rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>
				gPeerConnectionFactory;
			bool gIsTracing = false;
			std::unique_ptr<FileLogSink> gLoggingFile;
			std::unique_ptr<rtc::LoggingServer> gLoggingServer;
			// The worker thread for webrtc.
			rtc::Thread gThread;
			std::unique_ptr<rtc::Thread> gNetworkThread;
			std::unique_ptr<rtc::Thread> gWorkerThread;
			std::unique_ptr<rtc::Thread> gSignalingThread;
			// Default resolution. If no preferred video capture format is specified,
			// this is the resolution we will use.
			cricket::VideoFormat gPreferredVideoCaptureFormat = cricket::VideoFormat(640, 480,
				cricket::VideoFormat::FpsToInterval(30), cricket::FOURCC_ANY);
		}  // namespace globals

		RTCIceCandidate::RTCIceCandidate() {
		}

		RTCIceCandidate::RTCIceCandidate(
			String^ candidate, String^ sdpMid, uint16 sdpMLineIndex) {
			Candidate = candidate;
			SdpMid = sdpMid;
			SdpMLineIndex = sdpMLineIndex;
		}

		RTCSessionDescription::RTCSessionDescription() {
		}

		RTCSessionDescription::RTCSessionDescription(RTCSdpType type, String^ sdp) {
			Type = type;
			Sdp = sdp;
		}

		RTCPeerConnection::RTCPeerConnection(RTCConfiguration^ configuration)
			: _observer(new GlobalObserver()) {
			webrtc::PeerConnectionInterface::RTCConfiguration cc_configuration;
			FromCx(configuration, &cc_configuration);
			globals::RunOnGlobalThread<void>([this, cc_configuration] {
				webrtc::FakeConstraints constraints;
				constraints.SetAllowDtlsSctpDataChannels();
				constraints.AddOptional(
					webrtc::MediaConstraintsInterface::kCombinedAudioVideoBwe, "true");
				_observer->SetPeerConnection(this);
				RTC_LOG(LS_INFO) << "Creating PeerConnection native.";
				_impl = globals::gPeerConnectionFactory->CreatePeerConnection(
					cc_configuration, &constraints, nullptr, nullptr, _observer.get());
			});
		}

		RTCPeerConnection::~RTCPeerConnection() {
			RTC_LOG(LS_INFO) << "RTCPeerConnection::~RTCPeerConnection";
			for (typename std::vector<DataChannelObserver*>::iterator it = _dataChannelObservers.begin();
				it != _dataChannelObservers.end(); ++it) {
				delete *it;
			}
		}

		// Utility function to create an async operation
		// which wraps a callback based async function.
		// Use std::tuple<> for callbacks with more than one argument.
		// Different types T1 and T2 where additional processing
		// needs to be done in the callback.
		template <typename T1, typename T2>
		IAsyncOperation<T2>^ CreateCallbackBridge(
			std::function<void(Concurrency::task_completion_event<T1>)> init,
			std::function<T2(T1)> onCallback) {
			Concurrency::task_completion_event<T1> tce;

			// Start the initial async operation
			Concurrency::create_async([tce, init] {
				globals::RunOnGlobalThread<void>([tce, init] {
					init(tce);
				});
			});

			// Create the task that waits on the completion event.
			auto tceTask = Concurrency::task<T1>(tce)
				.then([onCallback](T1 arg) {
				// Then calls the callback with the return value.
				return onCallback(arg);
			});

			// Return an async operation that waits on the return value
			// of the callback and returns it.
			return Concurrency::create_async([tceTask] {
				try {
					return tceTask.get();
				}
				catch (...) {
					return (T2)nullptr;
				}
			});
		}

		// Specialized version for void callbacks.
		IAsyncAction^ CreateCallbackBridge(
			std::function<void(Concurrency::task_completion_event<void>)> init) {
			Concurrency::task_completion_event<void> tce;

			// Start the initial async operation
			Concurrency::create_async([tce, init] {
				globals::RunOnGlobalThread<void>([tce, init] {
					init(tce);
				});
			});

			// Create the task that waits on the completion event.
			auto tceTask = Concurrency::task<void>(tce);

			// Return an async operation that waits on the
			// task completetion event.
			return Concurrency::create_async([tceTask] {
				return tceTask.get();
			});
		}

		IAsyncOperation<RTCSessionDescription^>^ RTCPeerConnection::CreateOffer() {
			return CreateCallbackBridge
				<webrtc::SessionDescriptionInterface*, RTCSessionDescription^>(
					[this](Concurrency::task_completion_event
						<webrtc::SessionDescriptionInterface*> tce) {
				webrtc::PeerConnectionInterface::RTCOfferAnswerOptions options;

				rtc::CritScope lock(&_critSect);
				if (_impl == nullptr) {
					tce.set(nullptr);
					return;
				}

				rtc::scoped_refptr<CreateSdpObserver> observer(
					new rtc::RefCountedObject<CreateSdpObserver>(tce));
				// The callback is kept for the lifetime of the RTCPeerConnection.
				_createSdpObservers.push_back(observer);

				_impl->CreateOffer(observer, nullptr);
			}, [](webrtc::SessionDescriptionInterface* sdi) {
				RTCSessionDescription^ ret = nullptr;
				ToCx(sdi, &ret);
				return ret;
			});
		}

		IAsyncOperation<RTCSessionDescription^>^ RTCPeerConnection::CreateAnswer() {
			return CreateCallbackBridge
				<webrtc::SessionDescriptionInterface*, RTCSessionDescription^>(
					[this](Concurrency::task_completion_event
						<webrtc::SessionDescriptionInterface*> tce) {
				webrtc::PeerConnectionInterface::RTCOfferAnswerOptions options;

				rtc::CritScope lock(&_critSect);
				if (_impl == nullptr) {
					tce.set(nullptr);
					return;
				}

				rtc::scoped_refptr<CreateSdpObserver> observer(
					new rtc::RefCountedObject<CreateSdpObserver>(tce));
				// The callback is kept for the lifetime of the RTCPeerConnection.
				_createSdpObservers.push_back(observer);

				_impl->CreateAnswer(observer, nullptr);
			}, [](webrtc::SessionDescriptionInterface* sdi) {
				RTCSessionDescription^ ret = nullptr;
				ToCx(sdi, &ret);
				return ret;
			});
		}

		IAsyncAction^ RTCPeerConnection::SetLocalDescription(
			RTCSessionDescription^ description) {
			return CreateCallbackBridge(
				[this, description](Concurrency::task_completion_event<void> tce) {
				webrtc::PeerConnectionInterface::RTCOfferAnswerOptions options;

				rtc::CritScope lock(&_critSect);
				if (_impl == nullptr) {
					tce.set();
					return;
				}

				rtc::scoped_refptr<SetSdpObserver> observer(
					new rtc::RefCountedObject<SetSdpObserver>(tce));
				// The callback is kept for the lifetime of the RTCPeerConnection.
				_setSdpObservers.push_back(observer);

				std::unique_ptr<webrtc::SessionDescriptionInterface> nativeDescription;
				FromCx(description, &nativeDescription);

				_impl->SetLocalDescription(observer, nativeDescription.release());
			});
		}

		IAsyncAction^ RTCPeerConnection::SetRemoteDescription(
			RTCSessionDescription^ description) {
			return CreateCallbackBridge(
				[this, description](Concurrency::task_completion_event<void> tce) {
				webrtc::PeerConnectionInterface::RTCOfferAnswerOptions options;

				rtc::CritScope lock(&_critSect);
				if (_impl == nullptr) {
					tce.set();
					return;
				}

				rtc::scoped_refptr<SetSdpObserver> observer(
					new rtc::RefCountedObject<SetSdpObserver>(tce));
				// The callback is kept for the lifetime of the RTCPeerConnection.
				_setSdpObservers.push_back(observer);

				std::unique_ptr<webrtc::SessionDescriptionInterface> nativeDescription;
				FromCx(description, &nativeDescription);

				_impl->SetRemoteDescription(observer, nativeDescription.release());
			});
		}

		RTCConfiguration^ RTCPeerConnection::GetConfiguration() {
			// The WebRtc api doesn't allow us to get the configuration back.
			return nullptr;
		}

		IVector<MediaStream^>^ RTCPeerConnection::GetLocalStreams() {
			auto ret = ref new Vector<MediaStream^>();
			globals::RunOnGlobalThread<void>([this, ret] {
				rtc::CritScope lock(&_critSect);
				if (_impl == nullptr) {
					return;
				}

				auto streams = _impl->local_streams();
				for (size_t i = 0; i < streams->count(); ++i) {
					ret->Append(ref new MediaStream(streams->at(i)));
				}
			});
			return ret;
		}

		IVector<MediaStream^>^ RTCPeerConnection::GetRemoteStreams() {
			auto ret = ref new Vector<MediaStream^>();
			globals::RunOnGlobalThread<void>([this, ret] {
				rtc::CritScope lock(&_critSect);
				if (_impl == nullptr) {
					return;
				}

				auto streams = _impl->remote_streams();
				for (size_t i = 0; i < streams->count(); ++i) {
					ret->Append(ref new MediaStream(streams->at(i)));
				}
			});
			return ret;
		}

		MediaStream^ RTCPeerConnection::GetStreamById(String^ streamId) {
			MediaStream^ ret = nullptr;
			globals::RunOnGlobalThread<void>([this, streamId, &ret] {
				rtc::CritScope lock(&_critSect);
				if (_impl == nullptr) {
					return;
				}

				std::string streamIdStr = FromCx(streamId);
				// Look through the local streams.
				auto streams = _impl->local_streams();
				for (size_t i = 0; i < streams->count(); ++i) {
					auto stream = streams->at(i);
					if (stream->label() == streamIdStr) {
						ret = ref new MediaStream(stream);
						return;
					}
				}
				// Look through the remote streams.
				streams = _impl->remote_streams();
				for (size_t i = 0; i < streams->count(); ++i) {
					auto stream = streams->at(i);
					if (stream->label() == streamIdStr) {
						ret = ref new MediaStream(stream);
						return;
					}
				}
			});
			return ret;
		}

		void RTCPeerConnection::AddStream(MediaStream^ stream) {
			globals::RunOnGlobalThread<void>([this, stream] {
				rtc::CritScope lock(&_critSect);
				if (_impl == nullptr) {
					return;
				}

				_impl->AddStream(stream->GetImpl());
			});
		}

		void RTCPeerConnection::RemoveStream(MediaStream^ stream) {
			globals::RunOnGlobalThread<void>([this, stream] {
				rtc::CritScope lock(&_critSect);
				if (_impl == nullptr) {
					return;
				}

				_impl->RemoveStream(stream->GetImpl());
			});
		}

		RTCDataChannel^ RTCPeerConnection::CreateDataChannel(
			String^ label, RTCDataChannelInit^ init) {
			rtc::CritScope lock(&_critSect);
			if (_impl == nullptr) {
				return nullptr;
			}

			webrtc::DataChannelInit nativeInit;
			if (init != nullptr) {
				FromCx(init, &nativeInit);
			}

			auto channel = _impl->CreateDataChannel(
				FromCx(label), init != nullptr ? &nativeInit : nullptr);
			auto ret = ref new RTCDataChannel(channel);

			auto observer = new Org::WebRtc::Internal::DataChannelObserver(ret);
			// The callback is kept for the lifetime of the RTCPeerConnection.
			_dataChannelObservers.push_back(observer);
			channel->RegisterObserver(observer);
			return ret;
		}

		IAsyncAction^ RTCPeerConnection::AddIceCandidate(RTCIceCandidate^ candidate) {
			return Concurrency::create_async([this, candidate] {
				globals::RunOnGlobalThread<void>([this, candidate] {
					rtc::CritScope lock(&_critSect);
					if (_impl == nullptr) {
						return;
					}

					std::unique_ptr<webrtc::IceCandidateInterface> nativeCandidate;
					FromCx(candidate, &nativeCandidate);
					_impl->AddIceCandidate(nativeCandidate.get());
				});
			});
		}

		void RTCPeerConnection::Close() {
			globals::RunOnGlobalThread<void>([this] {
				rtc::CritScope lock(&_critSect);

				if (_impl.get()) {
					_impl->Close();
				}

				// Needed to remove the circular references and allow
				// this object to be garbage collected.
				_observer->SetPeerConnection(nullptr);
				_impl = nullptr;
			});
		}

		bool RTCPeerConnection::EtwStatsEnabled::get() {
			return globals::RunOnGlobalThread<bool>([this] {
				return _observer->AreETWStatsEnabled();
			});
		}

		void RTCPeerConnection::EtwStatsEnabled::set(bool value) {
			globals::RunOnGlobalThread<void>([this, value] {
				_observer->EnableETWStats(value);
			});
		}

		bool RTCPeerConnection::ConnectionHealthStatsEnabled::get() {
			return globals::RunOnGlobalThread<bool>([this] {
				return _observer->AreConnectionHealthStatsEnabled();
			});
		}

		void RTCPeerConnection::ConnectionHealthStatsEnabled::set(bool value) {
			globals::RunOnGlobalThread<void>([this, value] {
				_observer->EnableConnectionHealthStats(value);
			});
		}

		bool RTCPeerConnection::RtcStatsEnabled::get() {
			return globals::RunOnGlobalThread<bool>([this] {
				return _observer->AreRTCStatsEnabled();
			});
		}

		void RTCPeerConnection::RtcStatsEnabled::set(bool value) {
			globals::RunOnGlobalThread<void>([this, value] {
				_observer->EnableRTCStats(value);
			});
		}

		bool RTCPeerConnection::SendRtcStatsToRemoteHostEnabled::get() {
			return globals::RunOnGlobalThread<bool>([this] {
				return _observer->IsSendRtcStatsToRemoteHostEnabled();
			});
		}

		void RTCPeerConnection::SendRtcStatsToRemoteHostEnabled::set(bool value) {
			globals::RunOnGlobalThread<void>([this, value] {
				_observer->EnableSendRtcStatsToRemoteHost(value);
			});
		}

		String^ RTCPeerConnection::RtcStatsDestinationHost::get() {
			return globals::RunOnGlobalThread<String^>([this] {
				return ToCx(_observer->GetRtcStatsDestinationHost());
			});
		}

		void RTCPeerConnection::RtcStatsDestinationHost::set(String^ value) {
			globals::RunOnGlobalThread<void>([this, value] {
				_observer->SetRtcStatsDestinationHost(FromCx(value));
			});
		}

		int RTCPeerConnection::RtcStatsDestinationPort::get() {
			return globals::RunOnGlobalThread<int>([this] {
				return _observer->GetRtcStatsDestinationPort();
			});
		}

		void RTCPeerConnection::RtcStatsDestinationPort::set(int value) {
			globals::RunOnGlobalThread<void>([this, value] {
				_observer->SetRtcStatsDestinationPort(value);
			});
		}

		RTCSessionDescription^ RTCPeerConnection::LocalDescription::get() {
			RTCSessionDescription^ ret;
			globals::RunOnGlobalThread<void>([this, &ret] {
				rtc::CritScope lock(&_critSect);
				if (_impl == nullptr) {
					return;
				}

				if (_impl->local_description() != nullptr) {
					ToCx(_impl->local_description(), &ret);
				}
			});
			return ret;
		}

		RTCSessionDescription^ RTCPeerConnection::RemoteDescription::get() {
			RTCSessionDescription^ ret;
			globals::RunOnGlobalThread<void>([this, &ret] {
				rtc::CritScope lock(&_critSect);
				if (_impl == nullptr) {
					return;
				}

				if (_impl->remote_description() != nullptr) {
					ToCx(_impl->remote_description(), &ret);
				}
			});
			return ret;
		}

		RTCSignalingState RTCPeerConnection::SignalingState::get() {
			RTCSignalingState ret;
			globals::RunOnGlobalThread<void>([this, &ret] {
				rtc::CritScope lock(&_critSect);
				if (_impl == nullptr) {
					ret = RTCSignalingState::Closed;
					return;
				}

				ToCx(_impl->signaling_state(), &ret);
			});
			return ret;
		}

		RTCIceGatheringState RTCPeerConnection::IceGatheringState::get() {
			RTCIceGatheringState ret;
			globals::RunOnGlobalThread<void>([this, &ret] {
				rtc::CritScope lock(&_critSect);
				if (_impl == nullptr) {
					ret = RTCIceGatheringState::Complete;
					return;
				}

				ToCx(_impl->ice_gathering_state(), &ret);
			});
			return ret;
		}

		RTCIceConnectionState RTCPeerConnection::IceConnectionState::get() {
			RTCIceConnectionState ret;
			globals::RunOnGlobalThread<void>([this, &ret] {
				rtc::CritScope lock(&_critSect);
				if (_impl == nullptr) {
					ret = RTCIceConnectionState::Closed;
					return;
				}

				ToCx(_impl->ice_connection_state(), &ret);
			});
			return ret;
		}

		IAsyncOperation<bool>^  WebRTC::RequestAccessForMediaCapture() {
			// On some platforms, two calls of InitializeAsync on two diferent
			// instances causes exception to be thrown from the second call to
			// InitializeAsync. The second InitializeAsync
			// is called in MediaCaptureDevicesWinUWP::GetMediaCapture
			// Behavior present on Lumia620, OS version 8.10.14219.341.
			Platform::Agile<MediaCapture> mediaAccessRequester(
				ref new MediaCapture());
			MediaCaptureInitializationSettings^ mediaSettings =
				ref new MediaCaptureInitializationSettings();
			mediaSettings->AudioDeviceId = "";
			mediaSettings->VideoDeviceId = "";
			mediaSettings->StreamingCaptureMode =
				Windows::Media::Capture::StreamingCaptureMode::AudioAndVideo;
			mediaSettings->PhotoCaptureSource =
				Windows::Media::Capture::PhotoCaptureSource::VideoPreview;
			Concurrency::task<void> initTask = Concurrency::create_task(
				mediaAccessRequester->InitializeAsync(mediaSettings));
			return Concurrency::create_async([initTask] {
				bool accessRequestAccepted = true;
				try {
					initTask.get();
				}
				catch (Platform::Exception^ e) {
					RTC_LOG(LS_ERROR) << "Failed to obtain media access permission: "
						<< rtc::ToUtf8(e->Message->Data()).c_str();
					accessRequestAccepted = false;
				}
				return accessRequestAccepted;
			});
		}

		void WinJSHooks::initialize() {
			webrtc::VideoCommonWinUWP::SetCoreDispatcher(Windows::UI::Core::CoreWindow::GetForCurrentThread()->Dispatcher);
			Org::WebRtc::WebRTC::Initialize(Windows::UI::Core::CoreWindow::GetForCurrentThread()->Dispatcher);
		}

		IAsyncOperation<bool>^ WinJSHooks::requestAccessForMediaCapture() {
			return  Org::WebRtc::WebRTC::RequestAccessForMediaCapture();
		}

		bool WinJSHooks::IsTracing() {
			return  Org::WebRtc::WebRTC::IsTracing();
		}

		void WinJSHooks::StartTracing(Platform::String^ filename) {
			Org::WebRtc::WebRTC::StartTracing(filename);
		}

		void WinJSHooks::StopTracing() {
			Org::WebRtc::WebRTC::StopTracing();
		}

		webrtc::AudioDeviceWindowsWasapi* WebRTC::m_NativeAudioDevice = nullptr;		

		void WebRTC::Initialize(Windows::UI::Core::CoreDispatcher^ dispatcher) {
			if (globals::isInitialized)
				return;

			webrtc::VideoCommonWinUWP::SetCoreDispatcher(dispatcher);

			// Create a worker thread
			globals::gThread.SetName("WinUWPApiWorker", nullptr);
			globals::gThread.Start();
			globals::RunOnGlobalThread<void>([] {
				rtc::EnsureWinsockInit();
				rtc::InitializeSSL(globals::certificateVerifyCallBack);

				globals::gNetworkThread = rtc::Thread::CreateWithSocketServer();
				globals::gNetworkThread->Start();

				globals::gWorkerThread = rtc::Thread::Create();
				globals::gWorkerThread->Start();

				globals::gSignalingThread = rtc::Thread::Create();
				globals::gSignalingThread->Start();

				auto encoderFactory = new webrtc::WinUWPH264EncoderFactory();
				auto decoderFactory = new webrtc::WinUWPH264DecoderFactory();

				RTC_LOG(LS_INFO) << "Creating PeerConnectionFactory.";
				globals::gPeerConnectionFactory =
					webrtc::CreatePeerConnectionFactory(
						globals::gNetworkThread.get(), globals::gWorkerThread.get(),
						globals::gSignalingThread.get(),
						nullptr, webrtc::CreateBuiltinAudioEncoderFactory(), webrtc::CreateBuiltinAudioDecoderFactory(),
						encoderFactory, decoderFactory);

				rtc::tracing::SetupInternalTracer();
			});
			m_NativeAudioDevice = webrtc::AudioInterfaceActivator::m_AudioDevice;
			const size_t getDecibelFullScalePeriod = 25; // 250 ms
			m_NativeAudioDevice->RegisterGetDecibelFullScaleCallback(WebRTC::DecibellFullScaleCallBack, getDecibelFullScalePeriod);
			globals::isInitialized = true;
		}

		void WebRTC::SetAudioManualAudioRecordingControl(bool manualControl) {
			if (globals::isInitialized && m_NativeAudioDevice) {
				m_NativeAudioDevice->SetManualRecordingControl(manualControl);
			}
		}

		void WebRTC::StartAudioRecordingManual() {
			if (globals::isInitialized && globals::gWorkerThread && m_NativeAudioDevice) {
				globals::gWorkerThread->Invoke<void, std::function<void()>>(RTC_FROM_HERE, [] () {
					m_NativeAudioDevice->InitRecording();
					m_NativeAudioDevice->StartRecordingManual();
				});
			}
		}

		void WebRTC::StopAudioRecordingManual() {
			if (globals::isInitialized && globals::gWorkerThread && m_NativeAudioDevice) {
				globals::gWorkerThread->Invoke<void, std::function<void()>>(RTC_FROM_HERE, []() {
					m_NativeAudioDevice->StopRecordingManual();
				});
			}
		}

		void WebRTC::DecibellFullScaleCallBack(double decibel) {
			Concurrency::create_async([decibel] {
				Org::WebRtc::DecibelFullScaleHelper::FireEvent(decibel);
			});
		}

		bool WebRTC::IsTracing() {
			return globals::gIsTracing;
		}

		void WebRTC::StartTracing(Platform::String^ filename) {
			globals::gIsTracing = true;
			std::string filenameStr = FromCx(filename);
			rtc::tracing::StartInternalCapture(filenameStr.c_str());
		}

		void WebRTC::StopTracing() {
			globals::gIsTracing = false;
			rtc::tracing::StopInternalCapture();
		}

		void WebRTC::EnableLogging(LogLevel level) {
			if (globals::gLoggingFile.get() != nullptr ||
				globals::gLoggingServer.get() != nullptr) {
				// already logging
				return;
			}

			// setup logging to network
			rtc::SocketAddress sa(INADDR_ANY, 47003);
			globals::gLoggingServer = std::unique_ptr<rtc::LoggingServer>(
				new rtc::LoggingServer());
			globals::gLoggingServer->Listen(sa, static_cast<rtc::LoggingSeverity>(level));

			// setup logging to a file
			rtc::FileStream* fileStream = new rtc::FileStream();
			fileStream->Open(globals::OutputPath() + globals::logFileName, "wb", NULL);
			fileStream->DisableBuffering();
			globals::gLoggingFile = std::unique_ptr<globals::FileLogSink>(
				new globals::FileLogSink(fileStream));
			rtc::LogMessage::AddLogToStream(globals::gLoggingFile.get(),
				static_cast<rtc::LoggingSeverity>(level));

			RTC_LOG(LS_INFO) << "WebRTC logging enabled";
		}

		void WebRTC::DisableLogging() {
			RTC_LOG(LS_INFO) << "WebRTC logging disabled";
			rtc::LogMessage::RemoveLogToStream(globals::gLoggingFile.get());
			globals::gLoggingFile.get()->file()->Close();
			globals::gLoggingFile.reset();
			globals::gLoggingServer.reset();
		}

		Windows::Storage::StorageFolder^ WebRTC::LogFolder::get() {
			return Windows::Storage::ApplicationData::Current->LocalFolder;
		}

		String^  WebRTC::LogFileName::get() {
			return globals::toPlatformString(globals::logFileName);
		}

		IVector<CodecInfo^>^ WebRTC::GetAudioCodecs() {
			auto ret = ref new Vector<CodecInfo^>();
			globals::RunOnGlobalThread<void>([ret] {
				ret->Append(ref new CodecInfo(111, 48000, "opus"));
				ret->Append(ref new CodecInfo(103, 16000, "ISAC"));
				ret->Append(ref new CodecInfo(104, 32000, "ISAC"));
				ret->Append(ref new CodecInfo(9, 8000, "G722"));
				ret->Append(ref new CodecInfo(102, 8000, "ILBC"));
				ret->Append(ref new CodecInfo(0, 8000, "PCMU"));
				ret->Append(ref new CodecInfo(8, 8000, "PCMA"));
			});
			return ret;
		}

		IVector<CodecInfo^>^ WebRTC::GetVideoCodecs() {
			auto ret = ref new Vector<CodecInfo^>();
			globals::RunOnGlobalThread<void>([ret] {
				ret->Append(ref new CodecInfo(96, 90000, "VP8"));
				ret->Append(ref new CodecInfo(98, 90000, "VP9"));
				ret->Append(ref new CodecInfo(125, 90000, "H264"));
			});
			return ret;
		}

		void WebRTC::SynNTPTime(int64 currentNtpTime) {
			rtc::SyncWithNtp(currentNtpTime);
		}

		double WebRTC::CpuUsage::get() {
			return globals::gCurrentCPUUsage;
		}

		void WebRTC::CpuUsage::set(double value) {
			globals::gCurrentCPUUsage = value;
		}

		INT64 WebRTC::MemoryUsage::get() {
			return globals::gCurrentMEMUsage;
		}

		void WebRTC::MemoryUsage::set(INT64 value) {
			globals::gCurrentMEMUsage = value;
		}

		void WebRTC::SetPreferredVideoCaptureFormat(int frameWidth,
			int frameHeight, int fps) {
			globals::gPreferredVideoCaptureFormat.interval =
				cricket::VideoFormat::FpsToInterval(fps);
			globals::gPreferredVideoCaptureFormat.width = frameWidth;
			globals::gPreferredVideoCaptureFormat.height = frameHeight;
		}
	}
}  // namespace Org.WebRtc
