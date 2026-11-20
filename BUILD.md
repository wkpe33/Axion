# Build Axion on Windows x64

Axion is an altered build of TaterClient/TClient. The upstream reference is
`087d4ae5ab7266f3afc4fe6d46a579eff83234f6`. The original license and asset
notices are in `license.txt`.

## Prerequisites

- Git and the pinned `ddnet-libs` submodule
- Visual Studio 2022 Build Tools with Desktop development with C++ and a
  Windows SDK
- CMake, Ninja, and Python 3
- Rust and Cargo for `x86_64-pc-windows-msvc`
- LunarG Vulkan SDK with `glslang` or `glslangValidator` and `spirv-opt`

The verified build used MSVC 19.44, Windows SDK 10.0.26100.0, CMake 4.4.4,
Ninja 1.13.2, Python 3.12.10, Rust 1.99.1, and Vulkan SDK 1.4.363.0.
CMake uses the Windows dependencies from the pinned submodule.

## Release build

Run in an x64 Native Tools Command Prompt for VS 2022. Point `VULKAN_SDK`
at the installed SDK before configuration. Use a fresh build directory when
changing generators or configuring before the submodule is ready.

```bat
git submodule update --init --recursive
set "VULKAN_SDK=C:\VulkanSDK\1.4.363.0"
set "PATH=%VULKAN_SDK%\Bin;%PATH%"
chcp 65001 > nul
cmake -S . -B build-next -G Ninja -DCMAKE_BUILD_TYPE=Release -DVULKAN=ON -DIPO=OFF -DDOWNLOAD_GTEST=ON
cmake --build build-next --target game-client game-server testrunner --parallel 8
build-next\testrunner.exe
python scripts\smoke_axion.py build-next
python scripts\smoke_udp_axion.py build-next
cmake --build build-next --target run_rust_tests
```

`build-next\DDNet.exe` is the client. The upstream executable name is
used by client restart and storage discovery. This Windows x64 update uses the MSVC Release settings and the Rust
Release profile uses thin LTO. The distributed executable is
accompanied by runtime DLLs and the data directory. Keep these next to the
executable and omit PDB files when distributing the binary.

`-DVULKAN=OFF` builds with OpenGL and leaves the optional Ash dependency
out of the Cargo build. Use `-DDOWNLOAD_GTEST=OFF` for a client-only build
without downloading GoogleTest.

On Windows the `register_shell` console command registers this installation
for DDNet links and files. Run it only when you want Windows to associate
those links and files with this installation.

Use the same UTF-8 code page for incremental builds with localized MSVC.
This keeps the Ninja include prefix consistent with compiler output.

Sound packs are discovered in assets/audio/<pack> and audio/<pack> inside
the DDNet storage paths. The pack mirrors the original audio filenames
and uses WavPack files. Axion can change packs while running.

The current aspect controls are in Axion → Visuals. Sound packs are in
Axion → Misc. Input controls remain in TClient.
