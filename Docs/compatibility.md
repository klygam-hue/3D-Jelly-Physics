# 3D Jely 1.6 — platform / GPU compatibility

Version 1.8 Ground retains these platform restrictions. Its C++ rectangle/resize/UI code was cross-compiled for all four targets; new ground shader features were executed on Windows/Radeon through both native GL and hardware Vulkan. Linux/Mac full graphics builds/runs and other physical vendors remain unverified. GLSL uses GLES3-compatible derivatives/math, without vendor-specific texture extensions or per-frame texture generation.

Version 1.7 random spawning retains this platform boundary. The new shape/RNG/color/GUI code has also been cross-compiled with all 14 project C++ units for the same four target architectures; Linux headless binaries linked, not executed. Current Windows Radeon GL/Vulkan spawn tests are in validation.md. This does not add macOS Vulkan or foreign GPU runtime evidence.

## Implementation versus measured support

There is no AMD/NVIDIA/Intel allowlist or vendor-specific shader fork. Actual desktop OpenGL 3.3+ or GLES 3.0+ backed by hardware Vulkan is required. Context versions are checked. SwiftShader/llvmpipe/lavapipe/softpipe is rejected when hardware Vulkan is requested; native OpenGL may use Mesa software for CI, explicitly not hardware validation. Physics retains portable C++20/double precision, without fast-math or machine-specific instructions.

| Platform target | OpenGL | Vulkan selection | Validation boundary |
|---|---|---|---|
| Windows 10/11 x64 | Native WGL | Embedded ANGLE → Vulkan | Radeon RX 7600 XT runtime verified; NVIDIA/Intel hardware not tested here |
| Linux x64 / arm64 | Native GLX | Packaged ANGLE → Vulkan / X11 | Cross-compile/link evidence recorded below; no Linux GPU run here |
| macOS 11+ Intel / Apple Silicon | Native NSGL/core GL | Unavailable, with explanation | Universal `.app` profile; full SDK build and graphics run still need a Mac |

AMD, NVIDIA and Intel are design targets where the OS/driver exposes these capabilities, not a guarantee for every old GPU/driver, laptop routing setup or OS release. Apple Silicon is also a macOS build target. Modern macOS NVIDIA availability is constrained by system/driver support; an API abstraction cannot add a missing GPU driver.

[ANGLE's platform matrix](https://chromium.googlesource.com/angle/angle/+/main/README.md) supports Vulkan on Windows/Linux and Metal on macOS. Metal is not renamed Vulkan. [MoltenVK](https://github.com/KhronosGroup/MoltenVK) provides Vulkan over Metal, but shipping that library alone cannot adapt this ANGLE renderer. A separate MoltenVK-capable native renderer remains planned. OpenGL is deprecated on macOS; indefinite future Apple OS support is not promised.

## Linux build

CMake 3.25+, C++20 compiler, Ninja and X11/GL development files. Ubuntu 22.04+/Debian example:

```sh
sudo apt-get install build-essential cmake ninja-build libx11-dev libxrandr-dev libxi-dev libxcursor-dev libxinerama-dev libgl1-mesa-dev
cd Code
cmake -DJELY_ANGLE_PLATFORM=linux-x64 -P bootstrap_angle.cmake
# An arm64 host uses linux-arm64 instead.
cmake --preset native-release
cmake --build --preset native-release --parallel 2
ctest --preset native-release
cmake --install build/native-release --prefix package
./package/Release/3D_Jely --backend opengl
./package/Release/3D_Jely --backend vulkan --strict-backend
```

Use `native-gl-release` to configure/build/test without ANGLE/Vulkan assets. Linux Vulkan is X11, including XWayland on a Wayland desktop, not native Wayland WSI. `DISPLAY` and a working hardware Vulkan ICD are required. AMD/Intel Mesa Vulkan drivers and NVIDIA proprietary drivers are supplied by the OS/user, never installed by the app. Missing driver/XWayland or incompatible runtime dependencies cause a visible activation failure and GL restoration, not a fake success.

Distribution is **not a single ELF file**: retain `Release/3D_Jely` and `Release/runtime/angle/` together. The runtime contains `libEGL.so`, `libGLESv2.so`, `libvulkan.so.1` and full notices from official Electron 41.0.0, not Electron itself. Bootstrap verifies the archive, CMake verifies every file's SHA256, then post-build/install copy those exact assets. Startup uses absolute installation-relative paths; guarded GLFW EGL/GLES loaders cannot accidentally select Mesa instead of ANGLE. Linux runtime files are trusted installation files, not Windows-style self-extracted/hash-rechecked cache bytes. Native GL continues using the system driver.

## macOS build

Install Apple's Xcode command-line tools, CMake 3.25+ and Ninja. From `Code`:

```sh
cmake --preset macos-universal
cmake --build --preset macos-universal --parallel 2
ctest --preset macos-universal
cmake --install build/macos-universal --prefix package
open package/3D_Jely.app
./package/3D_Jely.app/Contents/MacOS/3D_Jely --backend opengl --strict-backend --smoke 300 --automate --report /tmp/jely-gl.txt
```

This profile builds x86_64 + arm64, targeting macOS 11+, into a high-resolution-capable `.app` using the Apple SDK. Native single-architecture builds use `native-gl-release`. CMake rejects `JELY_ENABLE_VULKAN=ON` on Mac rather than making a fake Vulkan option. The unavailable UI button explains this without destroying the working context. Strict CLI Vulkan fails; non-strict startup restores GL with an explanation. No Metal renderer, notarization, signing certificate, `.dmg`, or prebuilt/tested Mac executable is supplied here. Build locally or properly sign/notarize a future distribution; do not globally disable Gatekeeper.

## Preferences / startup protection

| OS | Preferred graphics.txt location |
|---|---|
| Windows | `%LOCALAPPDATA%/3D_Jely/graphics.txt` |
| Linux | `$XDG_CONFIG_HOME/3D_Jely/graphics.txt`, else `$HOME/.config/3D_Jely/graphics.txt` |
| macOS | `$HOME/Library/Application Support/3D_Jely/graphics.txt` |

Relative XDG/HOME paths are not accepted as standard roots; POSIX can resolve home from the user database. If user storage is unavailable, executable-side `3D_JelyData` is tried. Small preferences are atomically replaced; POSIX temporaries are unique/0600, fully written and fsynced before rename. Corrupt/oversized preferences default to GL. Linux Vulkan startup probes two strict actual frames in an isolated owned process using `posix_spawn`/`waitpid`, with a 25-second timeout and termination of only its own hung child. Hardware/client API/nonzero VkDevice and VkPhysicalDevice checks match Windows. The Windows-specific maintenance-presentation workaround is not applied to Linux drivers or system settings.

## CI and evidence

Local 2026-10-07 cross-check: official SHA256-verified Zig 0.15.2/Clang compiled all 14 project C++ units (including the real POSIX backend) for x86_64-linux-gnu, aarch64-linux-gnu, x86_64-macos and aarch64-macos. Linux x64/arm64 graphics-free regression executables also linked (ELF, not executed). Final cross-check emitted no project warnings with raylib system includes, matching CMake. This is **not** a complete raylib/GLFW/X11/AppKit/Apple-SDK link, .app build, Linux process run or GPU validation. The Apple header check found and fixed missing signal.h; both Mac architecture checks then succeeded.

`.github/workflows/desktop.yml` now resides at the actual repository root, with run steps rooted at `Code`. Windows MSVC, Linux x64 and macOS universal build/package jobs plus Linux arm64 headless regression are defined. Version 1.9 adds source publication at the owner's request. Remote CI results must be read separately from local cross-compilation; a compiled package or software-Mesa Xvfb smoke run is not AMD/NVIDIA/Intel hardware validation. The latest observed status is recorded in validation.md.

`JELY_GRAPHICS_TESTS=ON` registers strict 300-frame GL scene and 480-frame/21-assertion UI tests, requiring a desktop session. Linux CI runs them via Xvfb/software Mesa and does not claim vendor GPU validation. Mac GUI tests and hardware Vulkan cannot be inferred from a CPU build. Separately test each physical vendor/OS machine; preserve GPU/driver details, exact binary hash, reports, captures and active/idle frame measurements:

```text
3D_Jely --backend opengl --strict-backend --smoke 300 --automate --report gl-scene.txt
3D_Jely --backend opengl --strict-backend --smoke 480 --ui-automate --report gl-ui.txt
3D_Jely --backend vulkan --strict-backend --smoke 300 --automate --report vk-scene.txt
3D_Jely --backend opengl --strict-backend --smoke 780 --switch-cycles 6 --report switches.txt
```

Vulkan commands apply only to enabled Windows/Linux builds. Switch stress must report 12 switches/12 exact state checks, zero fallback/errors. Manually check opacity endpoints, overlap views, time presets, fullscreen exit/switch, resize, high-DPI, minimize/restore, real input and multi-GPU routing. Prior AMD/Windows timings are not performance claims for Linux/Mac/NVIDIA/Intel. Exact-real-material physics and universal bug freedom remain unclaimed.
