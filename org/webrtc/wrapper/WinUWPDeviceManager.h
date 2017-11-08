// Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
//
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file in the root of the source
// tree. An additional intellectual property rights grant can be found
// in the file PATENTS.  All contributing project authors may
// be found in the AUTHORS file in the root of the source tree.

#ifndef ORG_WEBRTC_WINUWPDEVICEMANAGER_H_
#define ORG_WEBRTC_WINUWPDEVICEMANAGER_H_

#ifndef WINUWP
#error Invalid build configuration
#endif  // WINUWP

#include <map>
#include <string>
#include <vector>

#include "webrtc/media/base/device.h"
#include "webrtc/media/base/videocapturer.h"
#include "webrtc/media/base/videocapturerfactory.h"
#include "webrtc/rtc_base/sigslot.h"
#include "webrtc/rtc_base/stringencode.h"
#include "webrtc/rtc_base/logging.h"

namespace Org {
	namespace WebRtc {
		namespace Internal {

			class WinUWPDeviceManager {
			public:
				WinUWPDeviceManager();
				virtual ~WinUWPDeviceManager();

				sigslot::signal0<> SignalDevicesChange;
				static const char kDefaultDeviceName[];

				// Initialization
				bool Init();
				void Terminate();

				bool GetAudioInputDevices(std::vector<cricket::Device>* devices);
				bool GetAudioOutputDevices(std::vector<cricket::Device>* devices);

				bool GetVideoCaptureDevices(std::vector<cricket::Device>* devs);
				cricket::VideoCapturer* WinUWPDeviceManager::CreateVideoCapturer(const cricket::Device& device) const;

				virtual void SetVideoDeviceCapturerFactory(
					cricket::VideoDeviceCapturerFactory* video_device_capturer_factory) {
					video_device_capturer_factory_.reset(video_device_capturer_factory);
				}

			protected:
				bool IsInWhitelist(const std::string& key, cricket::VideoFormat* video_format) const;
				bool GetMaxFormat(const cricket::Device& device, cricket::VideoFormat* video_format) const;
			private:
				bool GetDefaultVideoCaptureDevice(cricket::Device* device);
				void OnDeviceChange();

				static const char* kUsbDevicePathPrefix;
				bool initialized_;
				std::unique_ptr<cricket::VideoDeviceCapturerFactory> video_device_capturer_factory_;
				std::map<std::string, cricket::VideoFormat> max_formats_;

				ref class WinUWPWatcher sealed {
				public:
					WinUWPWatcher();
					void Start();
					void Stop();

				private:
					friend WinUWPDeviceManager;
					WinUWPDeviceManager* deviceManager_;
					Windows::Devices::Enumeration::DeviceWatcher^ videoCaptureWatcher_;
					Windows::Devices::Enumeration::DeviceWatcher^ videoAudioInWatcher_;
					Windows::Devices::Enumeration::DeviceWatcher^ videoAudioOutWatcher_;

					void OnVideoCaptureAdded(Windows::Devices::Enumeration::DeviceWatcher
						^sender, Windows::Devices::Enumeration::DeviceInformation ^args);
					void OnVideoCaptureRemoved(Windows::Devices::Enumeration::DeviceWatcher
						^sender, Windows::Devices::Enumeration::DeviceInformationUpdate ^args);

					void OnAudioInAdded(Windows::Devices::Enumeration::DeviceWatcher
						^sender, Windows::Devices::Enumeration::DeviceInformation ^args);
					void OnAudioInRemoved(Windows::Devices::Enumeration::DeviceWatcher ^sender,
						Windows::Devices::Enumeration::DeviceInformationUpdate ^args);

					void OnAudioOutAdded(Windows::Devices::Enumeration::DeviceWatcher ^sender,
						Windows::Devices::Enumeration::DeviceInformation ^args);
					void OnAudioOutRemoved(Windows::Devices::Enumeration::DeviceWatcher
						^sender, Windows::Devices::Enumeration::DeviceInformationUpdate ^args);

					void OnDeviceChange();
				};

				WinUWPWatcher^ watcher_;
			};
			
			class DeviceManagerFactory {
			public:
				static WinUWPDeviceManager* Create();

			private:
				DeviceManagerFactory() {}
			};
		}
	}
}  // namespace Org.WebRtc.Internal

#endif  // ORG_WEBRTC_WINUWPDEVICEMANAGER_H_
