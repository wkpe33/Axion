# Axion architecture

Axion is a Windows-tested derivative of TaterClient. The C++ game client owns
the main loop, input, prediction, networking, UI, graphics command buffering,
render worker threads, and SDL window. The C++ Vulkan command processor still
records draws, batches map layers and sprites, manages vertex and texture
uploads, submits graphics work, and owns the logical device.

## Rust Vulkan presentation

Set `gfx_backend Vulkan` and `gfx_rust_vulkan 1`, then restart. The switch
is also under Settings → Graphics. The default remains C++ Vulkan
presentation, with upstream OpenGL selection available.

CMake adds the `ddnet-engine-shared/vulkan-presentation` Cargo feature for
Vulkan client builds. It reuses the existing locked Cargo invocation and Rust
static library. Ash uses the Vulkan loader already linked by TClient.

The render thread owns an opaque Rust context. The context dispatches
swapchain creation, destruction, image enumeration, image acquisition, and
queue presentation. It owns an in-memory Vulkan pipeline cache used when
C++ creates graphics pipelines. The Rust context is released before device
destruction. C++ retains swapchain lifecycle coordination with its image
views, framebuffers, synchronization objects, and window events.

The boundary passes Vulkan ABI handles, pointers, and result codes directly.
Steady-state acquire and present each cross it once per frame. The C++ side submits the existing Vulkan data directly to Rust.
If Rust cannot resolve the required swapchain entry points, the renderer
selects C++ presentation at initialization. Driver errors flow through the
C++ Vulkan error path. Swapchain acquisition retries once after an
out-of-date result. Device loss is reported through the existing error
path and live device recovery remains unimplemented.

## Settings and diagnostics

Settings → Graphics has three presets. Ultra Lite disables high-detail map
layers and background quads. Competitive enables high-detail layers and
disables background quads. Balanced enables both. All three disable tee
trails. The existing searchable Configs tab and Controls tab expose detailed
settings and keybinds.

`cl_performance_hud 1` displays frame intervals, the slowest 1% of the last
up to 4096 frames, main-thread render and submission wall time, and the
process working set on Windows. Vulkan GPUs with queue timestamp support
also show the most recent draw command buffer duration. GPU queries are
allocated on demand. The backend reads results after the existing per-image
fence, with no profiling-specific wait. Graphics uploads submitted in a
separate command buffer and presentation are outside the GPU timing interval.
Diagnostics default to off. The `performance_status` console command prints the current snapshot.

This implementation covers Vulkan presentation and cache lifetime in Rust.
Draw recording, resource uploads, and the remainder of the renderer still
use upstream C++. The cache lasts for the lifetime of the logical device.

## Windows x64 update

The Axion page is inserted after TClient in the tab order. It reuses the
former Credits page number so existing Profiles and Configs selections
keep their values. Attribution notices remain in the shipped license files.

TClient's input section selects the legacy TClient path or one of four
BestClient prediction modes. The imported modes calculate their own
offsets and use the existing prediction history, input sampler, and
dummy routing. Best Input changes the interpolation curve. Cloud uses
live sampling and enables prediction for other tees only after server
pre-input arrives. The regular simulation and wire protocol remain in
C++.

The server browser owns the UDP controls. The client loop polls isolated
sockets and stores preferences per numeric endpoint in the DDNet config.
The main connection consumes its preference only when connecting. See
[UDP_PORT_TESTING.md](UDP_PORT_TESTING.md).

Fast startup caps progress rendering at 24 FPS and uses the simpler
background during initial loading. The interactive menu still loads the
user's theme. The opt-in startup log times graphics initialization,
initial clear, fonts, components, assets, initialization completion and
the first interactive frame.

UDP probes now run on a background worker that owns their sockets and
publishes progress snapshots. The main loop consumes snapshots without
waiting for network I/O. Cancellation requests stop the worker, and
connection or shutdown waits for socket cleanup. Network traffic counters
use atomics so gameplay and probe traffic can update them concurrently.

## Visuals, Game and Misc

Axion uses the standard TClient panel colors and scroll region. Visuals
contains the short-name option and aspect presets/custom width and
height. The custom aspect applies in gameplay and demo playback when
the game permits zoom. Game links to the existing TClient input section.
Misc discovers sound packs from assets/audio and audio directories.
Sound-pack changes are captured into an immutable loading job, and sound
playback pauses while that job replaces the samples. Missing files use
the original game sound. Pack files retain DDNet's WavPack format.

The Release client enforces the low-detail visual settings, enables
threaded sound loading and reduced startup progress rendering, and hides
the previous preset and diagnostics controls.

Rust Vulkan now reads and writes a pipeline cache per GPU and driver in
the user's cache directory. It validates the Vulkan header, vendor,
device and UUID before using stored data. An invalid cache is discarded.
This remains a hybrid renderer. C++ records draws, uploads resources and
submits command buffers. Rust owns presentation dispatch and the pipeline
cache.
