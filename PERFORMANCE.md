# Axion performance

## Method

The benchmark runs the built-in `benchmark_quit` frame recorder in a fresh
temporary storage directory. It uses a 1280×720 window, the Jungle menu
theme, VSync off, and client and graphics refresh caps disabled. The
benchmark discards the first interval and then warms up for three seconds.
Average FPS is the reciprocal of the mean interval. P99 uses the nearest
rank. The 1% low is the reciprocal of the mean of the slowest
`ceil(frame_count / 100)` intervals. Diagnostics are off for timed runs.

The frame intervals include client pacing and scheduling. The menu exercise
has a different workload from online gameplay and map loading. Windows
denied the Windows Performance Recorder CPU profile (0xc5585011).

The upstream Release baseline uses TaterClient commit
`087d4ae5ab7266f3afc4fe6d46a579eff83234f6`, MSVC 19.44 and its default
CMake IPO setting. Axion used MSVC IPO. Both builds selected Intel UHD
Graphics with Vulkan driver 101.7092. This is a build-to-build comparison.

The first upstream capture measured 70.16 average FPS with another client
running. The table uses the three later runs for each build. The client
diagnostics were off. Axion runs were captured before the final change to
the opt-in CPU timing clock.

| Build | Run | Samples | Mean frame (ms) | P99 (ms) | Mean FPS | 1% low FPS |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Upstream TClient, Vulkan | 1 | 2053 | 5.85 | 8.75 | 171.04 | 88.41 |
| Upstream TClient, Vulkan | 2 | 2155 | 5.57 | 8.93 | 179.60 | 101.00 |
| Upstream TClient, Vulkan | 3 | 2053 | 5.84 | 8.86 | 171.16 | 95.73 |
| Upstream TClient, Vulkan | Median | | 5.84 | 8.86 | 171.16 | 95.73 |
| Axion, C++ presentation | 1 | 2047 | 5.86 | 8.51 | 170.52 | 86.84 |
| Axion, C++ presentation | 2 | 1612 | 7.44 | 23.85 | 134.41 | 31.38 |
| Axion, C++ presentation | 3 | 1452 | 8.26 | 28.69 | 121.01 | 29.00 |
| Axion, C++ presentation | Median | | 7.44 | 23.85 | 134.41 | 31.38 |
| Axion, Rust presentation | 1 | 2064 | 5.81 | 12.78 | 172.14 | 55.48 |
| Axion, Rust presentation | 2 | 1630 | 7.37 | 13.01 | 135.64 | 54.53 |
| Axion, Rust presentation | 3 | 2071 | 5.79 | 21.43 | 172.68 | 33.07 |
| Axion, Rust presentation | Median | | 5.81 | 13.01 | 172.14 | 54.53 |

Rust presentation had similar median average FPS to upstream and worse
median P99 and 1% low. The Axion runs varied substantially. These captures
establish no performance improvement. C++ Vulkan remains the default.

To repeat a menu run from PowerShell:

```powershell
python scripts/benchmark_axion.py build-axion/DDNet.exe --seconds 15 --warmup 3 --command 'gfx_rust_vulkan 0'
python scripts/benchmark_axion.py build-axion/DDNet.exe --seconds 15 --warmup 3 --command 'gfx_rust_vulkan 1' --require-log 'Using Rust Vulkan presentation.'
```

## Final packaged executable

A further menu run on the packaged executable after the diagnostics fix
produced these results.

| Presentation | Samples | Mean frame (ms) | P99 (ms) | Mean FPS | 1% low FPS |
| --- | ---: | ---: | ---: | ---: | ---: |
| C++ Vulkan | 2129 | 5.64 | 10.02 | 177.43 | 84.29 |
| Rust Vulkan | 2116 | 5.67 | 9.97 | 176.37 | 95.88 |

The upstream client executable is 6,655,512 bytes. The Axion executable is
6,904,832 bytes. Both figures exclude assets, DLLs, and symbols. The FPS
graph now reuses the frame timestamp captured by the main loop.

## Compatibility checks

- The C++ test runner passed all 374 enabled tests. Three upstream tests
  are disabled. The Rust test target passed 18 doctests.
- The loopback smoke test exercised Rust Vulkan startup, resize and
  swapchain recreation, a direct connection on Tutorial, dummy join,
  clean shutdown, and configuration reload.
- With diagnostics enabled, a sample showed 0.09 ms main-thread render
  and submission, 1.57 ms GPU draw time, and 550 MiB working set.
- An invalid Vulkan driver manifest triggered OpenGL fallback and
  rendered benchmark frames through clean exit.
- The packaged Rust path joined the public DDNet GER Novice Tutorial
  server and received a pong. The client also fetched the master server
  list.
- Visual inspection and interactive movement and hook verification remain
  open. The Windows UI control helper failed to initialize, and TClient
  suppresses in-game screenshots when its window is inactive.

## Windows x64 update

An earlier build of this update completed ten launches per mode with
fresh user profiles. Median time to the first interactive frame was
2.20 s with the original loading presentation and 1.04 s with reduced
loading redraws. P95 was 2.83 s and 1.35 s respectively. Fresh profiles
reuse the operating system's file cache. The final-package repeat was
stopped at the user's request.

The final executable passed 377 C++ tests and 18 Rust doctests. Loopback
checks exercised all four input modes while connected, input settings
and keybind persistence, the UDP test and cancellation, occupied-port
handling, and automatic and manual source-port binding. Two usable ports
each received all six replies. The server observed the configured main
source port and a separate dummy port. Rust Vulkan resizing, saved
configuration reload, and OpenGL fallback passed. One earlier reload
exited with an access violation in the unloaded Steam tier0 module. A
subsequent run completed cleanly. Visual inspection remains unverified.

## Background UDP tests

Individual tests, Test All, and the 64-port search now execute on a
background worker. The browser receives progress snapshots and provides
an explicit Select button for each completed result with replies. The
automatic search presents a confirmed candidate with a Select port
button. Selection saves the port for the next connection.

The updated C++ suite passed all 379 enabled tests. The loopback responder
verified that all six requests are sent and answered while the client
loop does not call Update. Cancellation and restart also passed.
The client UI was not launched for this update.

## Visuals and sound update

The current build forces reduced startup progress rendering, threaded
sound loading and the low-detail visual settings. Rust persists pipeline
caches for both Vulkan presentation paths. No new startup or frame-rate
measurements were collected for this update.

The earlier benchmark tables describe their stated builds and workloads.
They do not establish the performance of the updated aspect-ratio,
sound-pack or persistent-cache code.

The updated source passed 379 C++ tests and 19 Rust checks, including
rejection of truncated or incompatible pipeline cache headers. The
client was not launched for visual or audio inspection. Full-interface
aspect changes currently have no timed confirmation/revert overlay.

## Vulkan startup crash fix

Moved pipeline-cache context creation from Vulkan pre-initialization to
the graphics initialization command, after the storage interface is
assigned and before pipelines are created. The crash address in the
Windows event log corresponded to the premature storage access.

One controlled launch of the fixed executable initialized Rust Vulkan,
rendered 543 measured frames, and exited with code 0. The executable in
Desktop/Axion-client was replaced and its checksum matches the build.

## Texture-memory pressure protection

Asset browsers scan names first, decode/upload only visible thumbnails,
limit thumbnails to 256x256 with aspect preserved and release off-screen
preview textures. Applied game assets retain their original resolution.
Skin loading bounds HD sheets to 512x256 while preserving sprite-grid
alignment before computing metrics and grayscale copies. Files are unchanged.
At 768 MiB of total texture allocations, skin loading pauses and unused
skin entries become eligible for eviction after 250 ms; recently requested
player skins stay protected. Uploads are limited to two skins per update
so asynchronous graphics commands cannot enqueue the entire cache at once.
Vulkan counters now count successful allocations and use atomic increments
and decrements. These changes reduce texture pressure; actual allocation
failure on the user's GPU has not been reproduced in this session.
