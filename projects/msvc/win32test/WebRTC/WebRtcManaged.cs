using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Org.WebRtc;
//using UseConstraint = Org.WebRtc.IConstraint;
//using UseMediaConstraints = Org.WebRtc.IMediaConstraints;
using UseMediaStreamTrack = Org.WebRtc.IMediaStreamTrack;

namespace win32test
{
    public class WebRtcManaged : IDisposable
    {
        private MediaStreamTrack _selfVideoTrack;
        private MediaStreamTrack _selfAudioTrack;
        private bool _disposed;
        private VideoData _vd = null;
        private Size _lastSize = Size.Empty;
        private volatile bool _closing;
        private AutoResetEvent[] _frameEvents = new AutoResetEvent[2];

        private readonly object PeerConnectionLock = new object();
        RTCPeerConnection PeerConnection_DoNotUse;
        private RTCPeerConnection PeerConnection
        {
            get
            {
                lock (PeerConnectionLock)
                {
                    return PeerConnection_DoNotUse;
                }
            }
            set
            {
                lock (PeerConnectionLock)
                {
                    if (null == value)
                    {
                        if (null != PeerConnection_DoNotUse)
                        {
                            (PeerConnection_DoNotUse as IDisposable)?.Dispose();
                        }
                    }
                    PeerConnection_DoNotUse = value;
                }
            }
        }

        //private RTCSession _session;
        private List<RTCDataChannel> channels = new List<RTCDataChannel>();
        public WebRtcManaged()//RTCSession session)
        {
//_session = session;
            _frameEvents[0] = new AutoResetEvent(false);
            _frameEvents[1] = new AutoResetEvent(false);
        }


        public void Dispose()
        {
            //_factory.Dispose();
            if (!_disposed)
            {
                _disposed = true;
                Close();
            }
            //GC.SuppressFinalize(this);
        }

        public void Close()
        {
            ClosePeerConnection();
        }

        //public event OnCallbackLogMessage OnLogMessage;
        //public event OnCallbackIceCandidate OnIceCandidate;
        //public event OnCallbackDataMessage OnDataMessage;
        //public event OnCallbackBinaryMessage OnBinaryMessage;
        //public event OnCallbackBufferedAmountChange OnBufferedAmountChange;
        //public event OnCallbackDataChannelStateChange OnDataChannelStateChange;
        //public event OnCallbackIceConnectionChange OnIceConnectionChange;
        //public event OnCallbackRenderAudioRemote OnRenderAudioRemote;
        //public event OnCallbackSdp OnSuccessAnswer;
        //public event OnCallbackSdp OnFailure;
        //public event OnCallbackSdp OnError;
        public event EventHandler OnClosed;

       
        public void DataChannelSendText(string channel, string text)
        {
            var chan = channels.FirstOrDefault(p => p.Label == channel);
            if (chan != null)
            {
                chan.Send(text);
            }
        }

        public ulong DataChannelSendData(string channel, byte[] data, int len)
        {
            var chan = channels.FirstOrDefault(p => p.Label == channel);
            if (chan != null)
            {
                chan.Send(data);
                return chan.BufferedAmount;
            }
            return 0;
        }
        
        public async Task AddIceCandidate(string sdpMid, int sdpMLineIndex, string candidate)
        {
            if (!string.IsNullOrEmpty(candidate))
            {
                var ice = new RTCIceCandidate(new RTCIceCandidateInit() { Candidate = candidate, SdpMid = sdpMid, SdpMLineIndex = (ushort)sdpMLineIndex });
                await PeerConnection.AddIceCandidate(ice);
            }
        }

        public void AddServerConfig(string url, string username, string password)
        {
            var ic = new RTCIceServer {Urls = new List<string> {url}};
            if (!string.IsNullOrEmpty(username))
            {
                ic.Credential = password;
                ic.Username = username;
            }
            _iceServers.Add(ic);
        }

        public class CaptureCapability
        {
            public uint Width { get; set; }
            public uint Height { get; set; }
            public uint FrameRate { get; set; }
            public bool MrcEnabled { get; set; }
            public string ResolutionDescription { get; set; }
            public string FrameRateDescription { get; set; }
        }

        public class CodecInfo
        {
            public byte PreferredPayloadType { get; set; }
            public string Name { get; set; }
            public int ClockRate { get; set; }
        }

        public class MediaDevice
        {
            public string Id { get; set; }
            public string Name { get; set; }
        }

        /// <summary>
        /// Video codec used in WebRTC session.
        /// </summary>
        public CodecInfo VideoCodec { get; set; }

        /// <summary>
        /// Audio codec used in WebRTC session.
        /// </summary>
        public CodecInfo AudioCodec { get; set; }

        /// <summary>
        /// Video capture details (frame rate, resolution)
        /// </summary>
        public CaptureCapability VideoCaptureProfile;

        private readonly List<RTCIceServer> _iceServers = new List<RTCIceServer>();
        WebRtcFactory _factory;
        private CustomVideoCapturer videocapturer;
        private CustomAudioDevice audioDevice = null;

        public bool InitializePeerConnection()
        {
            var factoryConfig = new WebRtcFactoryConfiguration
            {
                AudioRenderingEnabled = false,
                AudioCapturingEnabled = true
            };

            var videoCapturerFactory = CustomVideoCapturerFactory.Create();
            videoCapturerFactory.OnCreateCustomVideoCapturer += WebRtcManaged_OnCreateCustomVideoCapturer;
            factoryConfig.CustomVideoFactory = videoCapturerFactory;
            
            var cadp = new CustomAudioDeviceParameters();
            audioDevice = (CustomAudioDevice) CustomAudioDevice.Create(cadp);
            var capdi = new CustomAudioPlayoutDeviceInfo();
            var cardi = new CustomAudioRecordingDeviceInfo
            {
                Name = "dummy",
                StereoRecordingIsAvailable = false
            };
            cadp.PlayoutDevices = new List<CustomAudioPlayoutDeviceInfo>() { capdi };
            cadp.RecordingDevices = new List<CustomAudioRecordingDeviceInfo>() { cardi };

            if (audioDevice != null)
            {
                audioDevice.OnRequestDeviceState += WebRtcManaged_OnRequestDeviceState;
                audioDevice.PlayoutChannels = 1;
                audioDevice.PlaybackSampleRateFsHz =(uint) 16000;
                audioDevice.PlayoutCurrentState = CustomAudioState.Stopped;
                audioDevice.OnSelectDevice += AudioDevice_OnSelectDevice;
                audioDevice.RecordingChannels = 1;
                //audioDevice.UpdateVqeData
                audioDevice.RecordingSampleRateFsHz = (uint)16000;
                audioDevice.RecordingCurrentState = CustomAudioState.Stopped;
            }
            

            factoryConfig.CustomAudioDevice = audioDevice;

            _factory = new WebRtcFactory(factoryConfig);
            var config = new RTCConfiguration
                         {
                             Factory = _factory,
                             BundlePolicy = RTCBundlePolicy.Balanced,
                             IceTransportPolicy = RTCIceTransportPolicy.All,
                             IceServers = _iceServers
                         };
            
            PeerConnection = new RTCPeerConnection(config);
            PeerConnection.OnIceCandidate += PeerConnection_OnIceCandidate;
            PeerConnection.OnIceConnectionStateChange+=  delegate()
                                                    {
                                                        if (PeerConnection != null)
                                                        {
                                                            var cs = (uint)PeerConnection.IceConnectionState;
                                                            //OnIceConnectionChange?.Invoke(cs);
                                                            switch (PeerConnection.IceConnectionState)
                                                            {
                                                                case RTCIceConnectionState.Failed:
                                                                    ClosePeerConnection();
                                                                    break;
                                                            }
                                                            
                                                        }
                                                    };

            PeerConnection.OnDataChannel += delegate (RTCDataChannelEvent ev)
                                            {
                                                var chan = ev.Channel as RTCDataChannel;
                                                if (chan != null)
                                                {
                                                    channels.Add(chan);
                                                    chan.OnMessage += s =>
                                                                      {
                                                                          if (!string.IsNullOrEmpty(s.Text))
                                                                          {
                                                                              //OnDataMessage?.Invoke(chan.Label, s.Text);
                                                                          }

                                                                          if (s.Binary != null)
                                                                          {
                                                                              //OnBinaryMessage?.Invoke(chan.Label, s.Binary);
                                                                          }
                                                                          
                                                                      };
                                                    chan.OnClose+= delegate
                                                                   {
                                                                       //OnDataChannelStateChange?.Invoke(chan.Label,3);//3=closed
                                                                   };
                                                }
                                            };
            PeerConnection.OnTrack += PeerConnection_OnTrack;
            PeerConnection.OnRemoveTrack += PeerConnection_OnRemoveTrack;
            VideoFormat vf = new VideoFormat
            {
                Fourcc = 0,
                Interval = TimeSpan.FromMilliseconds(50),
                Height = 1280,
                Width = 720
            };

            var vcparams = new VideoCapturerCreationParameters() {
                EnableMrc = false,
                Factory = _factory,
                Name="camera",
                Id = "dummy",
                Format = vf

            };

            var videoCapturer = VideoCapturer.Create(vcparams);
            _selfVideoTrack = MediaStreamTrack.CreateVideoTrack(_factory,"SELF_VIDEO", videoCapturer);
            

            AudioOptions audioOptions = new AudioOptions
            {
                Factory = _factory
            };

            var audioTrackSource = AudioTrackSource.Create(audioOptions);
            _selfAudioTrack = MediaStreamTrack.CreateAudioTrack(_factory,"SELF_AUDIO", audioTrackSource);

            //Debug.WriteLine("Conductor: Adding local media tracks.");
            PeerConnection.AddTrack(_selfVideoTrack);
            PeerConnection.AddTrack(_selfAudioTrack);

            return true;
        }

        private void PeerConnection_OnRemoveTrack(IRTCTrackEvent Event)
        {
            
        }

        private void PeerConnection_OnIceCandidate(IRTCPeerConnectionIceEvent evt)
        {
            if (evt.Candidate == null) // relevant: GlobalObserver::OnIceComplete in Org.WebRtc
            {
                return;
            }

            //OnIceCandidate?.Invoke(evt.Candidate.SdpMid, (int)evt.Candidate.SdpMLineIndex, evt.Candidate.Candidate);
        }


    private void ClosePeerConnection()
        {
            var pc = PeerConnection;
            if (pc != null)
            {
                _closing = true;

                _frameEvents[0].WaitOne(500);
                _frameEvents[1].WaitOne(500);

                pc.OnIceCandidate -= PeerConnection_OnIceCandidate;
                pc.OnTrack -= PeerConnection_OnTrack;
                pc.OnRemoveTrack -= PeerConnection_OnRemoveTrack;

                if (null != _selfVideoTrack) _selfVideoTrack.Element = null; // Org.WebRtc.MediaElementMaker.Bind(obj);
                (_selfVideoTrack as IDisposable)?.Dispose();
                (_selfAudioTrack as IDisposable)?.Dispose();
                _selfVideoTrack = null;
                _selfAudioTrack = null;
                audioBuffRemaining = null;

                if (audioDevice != null)
                {
                    audioDevice.OnRequestDeviceState -= WebRtcManaged_OnRequestDeviceState;
                    audioDevice.OnSelectDevice -= AudioDevice_OnSelectDevice;
                }

                _vd?.Dispose();
                _vd = null;
                PeerConnection = null;
                _frameEvents[0].Close();
                _frameEvents[1].Close();
                GC.Collect(); // Ensure all references are truly dropped.
            }
            
        }

        private void PeerConnection_OnTrack(IRTCTrackEvent Event)
        {
            
        }

        private void AudioDevice_OnSelectDevice(ICustomAudioDeviceSelectEvent Evt)
        {
            
        }

        private void WebRtcManaged_OnRequestDeviceState(ICustomAudioDeviceRequestStateEvent Evt)
        {
            switch(Evt.RequestedState)
            {
                case CustomAudioRequestState.Initialize:
                    audioDevice.RecordingCurrentState = CustomAudioState.Initialized;
                    break;
                case CustomAudioRequestState.Start:
                    audioDevice.RecordingCurrentState = CustomAudioState.Started;
                    break;
                case CustomAudioRequestState.Stop:
                    audioDevice.RecordingCurrentState = CustomAudioState.Stopped;
                    break;
                case CustomAudioRequestState.Terminate:
                    audioDevice.RecordingCurrentState = CustomAudioState.Terminated;
                    break;
            }
            //audioDevice.RecordingCurrentState = Evt.RequestedState;
        }

        private void WebRtcManaged_OnCreateCustomVideoCapturer(ICustomVideoCapturerCreateEvent Evt)
        {
            var parameters = new CustomVideoCapturerParameters();
            videocapturer = CustomVideoCapturer.Create(parameters);
            Evt.CreatedCapturer = videocapturer;
        }

        public async Task OnOfferRequest(string sdp)
        {
            var ini = new RTCSessionDescriptionInit {Type = RTCSdpType.Offer, Sdp = sdp};
            var rtcDescription = new RTCSessionDescription(ini);
            await PeerConnection.SetRemoteDescription(rtcDescription);

            RTCAnswerOptions answerOptions = new RTCAnswerOptions();

            var answer = await PeerConnection.CreateAnswer(answerOptions);
            await PeerConnection.SetLocalDescription(answer);
            //OnSuccessAnswer?.Invoke(answer.Sdp);
        }

        byte[] audioBuffRemaining = new byte[0];
        public void AddAudioSamples(byte[] samples, int len)
        {
            if (_closing)
                return;
            var adev = audioDevice;
            if (adev != null)
            {
                if (adev.RecordingCurrentState != CustomAudioState.Started)
                    return;
                if (audioBuffRemaining.Length > 0)
                {
                    len += audioBuffRemaining.Length;
                    int idx = 0;
                    byte[] samplesnew = new byte[len];
                    for (int i = 0; i < audioBuffRemaining.Length; i++)
                        samplesnew[idx++] = audioBuffRemaining[i];
                    for (int j = 0; j < samples.Length; j++)
                        samplesnew[idx++] = samples[j];

                    samples = samplesnew;
                    audioBuffRemaining = new byte[0];
                    
                }
                var pktSize = (16000 / 100);
                var bytSize = pktSize * 2;

                int tot = 0;
                var pkt = new short[pktSize];

                while (tot+ bytSize < len && !_closing) {
                    Buffer.BlockCopy(samples, tot, pkt, 0, bytSize);

                    using (var ad = new AudioData())
                    {
                        ad.SetData(pkt);
                        adev.SetRecordedBuffer(ad);
                        if (!adev.DeliverRecordedData())
                        {
                            
                        }
                    }
                    
                    tot += bytSize;
                }
                if (tot < len)
                {
                    audioBuffRemaining = new byte[len - tot];
                    Buffer.BlockCopy(samples, tot, audioBuffRemaining, 0, len - tot);
                }
            }
            _frameEvents[1].Set();
        }
    }
}
