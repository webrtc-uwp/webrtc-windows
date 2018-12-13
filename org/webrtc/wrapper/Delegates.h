
// Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
//
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file in the root of the source
// tree. An additional intellectual property rights grant can be found
// in the file PATENTS.  All contributing project authors may
// be found in the AUTHORS file in the root of the source tree.

#ifndef ORG_WEBRTC_DELEGATES_H_
#define ORG_WEBRTC_DELEGATES_H_

namespace Org {
	namespace WebRtc {
		/// <summary>
		/// Generic delegate declaration.
		/// </summary>
		public delegate void EventDelegate();

		/// <summary>
		/// Delegate for receiving video frames from RawVideoSource.
		/// </summary>
		public delegate void RawVideoSourceDelegate(
			uint32, uint32,
			const Platform::Array<uint8>^, uint32,
			const Platform::Array<uint8>^, uint32,
			const Platform::Array<uint8>^, uint32);

		/// <summary>
		/// Delegate for receiving video frames from EncodedVideoSource.
		/// </summary>
		public delegate void EncodedVideoSourceDelegate(
			uint32, uint32, const Platform::Array<uint8>^);

		/// <summary>
		/// Delegate for receiving video frames from MFSampleVideoSource.
		/// </summary>
		public delegate void MFSampleVideoSourceDelegate(uint64 samplePointer);

		// ------------------
		ref class RTCPeerConnectionIceEvent;
		/// <summary>
		/// Delegate for receiving ICE connections events for ICE candidates.
		/// </summary>
		public delegate void RTCPeerConnectionIceEventDelegate(
			RTCPeerConnectionIceEvent^);

		// ------------------
		ref class RTCPeerConnectionIceStateChangeEvent;
		/// <summary>
		/// Delegate for receiving ICE connection state changes.
		/// </summary>
		public delegate void RTCPeerConnectionIceStateChangeEventDelegate(
			RTCPeerConnectionIceStateChangeEvent^);

		ref class RTCPeerConnectionHealthStats;
		/// <summary>
		/// Delegate for receiving ICE connection health update. This receives a connection state.
		/// </summary>
		public delegate void RTCPeerConnectionHealthStatsDelegate(
			RTCPeerConnectionHealthStats^);

		ref class RTCStatsReportsReadyEvent;
		/// <summary>
		/// Delegate for receiving a list of statistics.
		/// </summary>
		public delegate void RTCStatsReportsReadyEventDelegate(
			RTCStatsReportsReadyEvent^);

		// ------------------
		ref class MediaStreamEvent;
		/// <summary>
		/// Delegate for receiving new media stream events.
		/// </summary>
		public delegate void MediaStreamEventEventDelegate(
			MediaStreamEvent^);

		ref class RTCDataChannelMessageEvent;
		/// <summary>
		/// Delegate for receiving raw data from a data channel.
		/// </summary>
		public delegate void RTCDataChannelMessageEventDelegate(
			RTCDataChannelMessageEvent^);


		public enum class MediaDeviceType {
			MediaDeviceType_AudioCapture,
			MediaDeviceType_AudioPlayout,
			MediaDeviceType_VideoCapture
		};

		/// <summary>
		/// Delegate for receiving audio/video device change notification.
		/// </summary>
		public delegate void MediaDevicesChanged(MediaDeviceType);

	}
}  // namespace Org.WebRtc

#endif  // ORG_WEBRTC_DELEGATES_H_

