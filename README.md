# This project has been deprecated

We are currently focusing our efforts on getting out of the fork business. This effort is happening in the [WinRTC GitHub repo](https://github.com/microsoft/winrtc). Keeping WebRTC-UWP fork doesn't allow us to move fast enough. We are contributing back the changes needed to build WebRTC.org code base for UWP. Here are some of the changes we're contributing back:

- https://webrtc-review.googlesource.com/c/src/+/167021
- https://boringssl-review.googlesource.com/c/boringssl/+/39584
- https://chromium-review.googlesource.com/c/chromium/src/+/1962509
- [abseil/abseil-cpp#594](https://github.com/abseil/abseil-cpp/pull/594)
- [abseil/abseil-cpp#596](https://github.com/abseil/abseil-cpp/pull/596)

Besides the new video capturing module that is being reviewed, we're also creating a new audio capturing module. There are more changes in the pipeline to be contributed back and more changes required for finishing the port. After having WebRTC.org code base compatible with UWP, we're going to work on a WinRT abstraction layer allowing easy consumption of WebRTC capabilities by WinRT projections.

That said, keep in mind we are contributing back the changes and we have no control over when/if the changes will be accepted by their teams.
# webrtc-wrapper-uwp
WebRTC wrapper API for exposing API to UWP platform (C# / WinJS)
