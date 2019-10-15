using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace win32test
{
    class Program
    {
        static void Main(string[] args)
        {
            var configuration = new Org.WebRtc.WebRtcLibConfiguration
            {
                Queue = Org.WebRtc.EventQueue.GetOrCreateThreadQueueByName("rtc")
            };
            //configuration.CustomAudioQueue = Org.WebRtc.EventQueue.GetOrCreateThreadQueueByName("audio");
            //configuration.CustomVideoQueue = Org.WebRtc.EventQueue.GetOrCreateThreadQueueByName("video");
            //configuration.AudioCaptureFrameProcessingQueue = Org.WebRtc.EventQueue.GetOrCreateThreadQueueByName("audiocap");
            //configuration.VideoFrameProcessingQueue = Org.WebRtc.EventQueue.GetOrCreateThreadQueueByName("videocap");
            //configuration.AudioRenderFrameProcessingQueue = Org.WebRtc.EventQueue.GetOrCreateThreadQueueByName("audioren");

            Org.WebRtc.WebRtcLib.Setup(configuration);

            WebRtcManaged m = new WebRtcManaged();
            m.InitializePeerConnection();
        }
    }
}
