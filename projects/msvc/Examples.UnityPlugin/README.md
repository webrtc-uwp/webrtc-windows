## Examples.UnityPlugin

Provides a vcxproj to enable building the `unityplugin` webrtc example source, with support for `winuwp`.

## What is this?

A wrapper project that facilitates two things:
  + Building `webrtc\xplatform\webrtc\examples\unityplugin` code for the `WINUWP` target
  + Consuming the `webrtc-uwp` wrapper project to enable `winuwp` runtime support for webrtc

## How does it work?

+ Open `..\..\solutions\WebRtc.Universal.sln` Solution file
+ Restore nuget packages
+ Build the `Examples.UnityPlugin` project
+ Find the build output in `..\..\solutions\Output`
+ Copy the `webrtc_unity_plugin` to a compatible Unity application

More information about compatible Unity applications can be found at `webrtc\xplatform\webrtc\examples\unityplugin\README`