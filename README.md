# Intro

Some racing games have a feature called "UDP Data Out", which consists of sending the car telemetry in real time to external devices (e.g. motion rigs, dashboard displays, leaderboard screens etc).

In addition to real-time display, the telemetry is useful for analysis of the car and driver performance.

This program binds to the UDP port and logs the raw traffic to disk so it can be used for analysis offline.

# How to build

The build is supported in Windows only at this time. Follow the steps below:

1. Clone the repo including the submodules:

```
git clone https://github.com/cmello/udp-logger.git --recurse-submodules
```

2. Bootstrap vcpkg included as submodule. From a terminal prompt at the root of the repo type:

```
.\vcpkg\bootstrap-vcpkg.bat
```

3. Open the solution `vs/udp-logger.sln` in Visual Studio and build it. It integrates with vcpkg and downloads the dependencies as part of the build (e.g. ASIO library).

