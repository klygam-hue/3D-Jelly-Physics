# 3D Jelly Physics

A C++20 interactive 3D soft-body laboratory with translucent jelly, elastic dragging,
random shapes, customizable lighting and ground, and an animated glass-style interface.


## Download and run

**1.9.3** fixes jelly becoming stuck during high-Softness floor dragging and recovering after release. Desktop and mobile ports share this fix and retain 10–15 FPS physics catch-up. Earlier releases remain available as fallbacks.
The Windows x64 executable embeds shaders, fonts and the optional graphics runtime;
no browser, compiler or asset folder is required. A working OpenGL 3.3 graphics driver is required. Vulkan mode uses
real hardware Vulkan through ANGLE and needs a compatible Vulkan driver.

## Features

- Real simulated tetrahedral XPBD bodies, not predefined deformation animations.
- Softness from a rigid stone endpoint to stretching, compressing, spring-like jelly, with smooth elastic hardening under large deformation.
- Left-click dragging and throwing, gravity, friction, damping, collisions and sleep.
- A new random physical shape and independent color with each **Spawn jelly** / **B** press.
- Adjustable transparency, refraction, tint and highlights.
- A 24-hour lighting cycle, contrast and independent background/ground colors.
- Ground width/depth, grid/checker patterns, roughness, outline and ring controls.
- Animated Liquid Glass-inspired controls with collapse/reopen, reduced motion and solid UI.
- Monitor-aware rendering/VSync, independent of the fixed 120 Hz physics solver.
- Native OpenGL or verified ANGLE/hardware Vulkan on supported platforms.

## Controls

Left mouse grabs jelly; right mouse orbits; middle mouse pans; the wheel zooms.
**B** spawns, **Space** pauses, **N** single-steps while paused, **R** resets,
**J** launches, **Tab** folds/unfolds controls, **F1** shows debug data,
**F11** toggles borderless fullscreen and **Esc** exits.

## Build

Desktop source and build instructions are in [`Code/`](Code/README.md); mobile source and validation records are in [`Mobile/`](Mobile/README.md).

```sh
cmake -S Code -B Code/build/headless -DJELY_BUILD_APP=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build Code/build/headless --parallel 2
ctest --test-dir Code/build/headless --output-on-failure
```

For Windows, open `Code` with Visual Studio 2022 C++/CMake, or run
`Code/bootstrap_tools.ps1` followed by `Code/build_release.ps1` for the pinned portable
toolchain. Detailed controls, native Linux/macOS builds, CLI tests and switches are in [`Code/README.md`](Code/README.md).

## Compatibility and scope

Windows AMD hardware is locally tested with both APIs. Linux has OpenGL and pinned
x64/arm64 ANGLE/Vulkan support; macOS has a native OpenGL universal app. Native CI
builds/tests pass Windows/MSVC, Linux x64, Linux ARM64 headless and macOS universal;
Linux x64 graphics smoke uses software Mesa/Xvfb, not a physical GPU.
**macOS Vulkan is not implemented.** Neither source portability nor CI compilation
proves runtime behavior on all AMD/NVIDIA/Intel drivers; see the recorded test boundary.

The physics is a real-time numerical approximation, not a calibrated model of a
particular real jelly. Contacts are particle-based, optical effects are screen-space,
and extreme deformation is safeguarded by dissipative backtracking. The simulation
does not claim exact real-material fidelity, self-collision or continuous surface
collision. Maximum simultaneous bodies is 16; this bounds resource growth, not a
guaranteed frame rate. The ground is a bounded arena, not an unsupported ledge.

## License

Original project code and documentation are under the [MIT License](LICENSE).
Third-party components retain their own licenses and copyright notices in [`Code/vendor/`](Code/vendor/) and [`Code/assets/`](Code/assets/). The app's `--licenses` command prints
embedded font/graphics notices. No dependency is relicensed under MIT.

## Desktop downloads — 1.9.3

Download Windows x64, Linux x64 or macOS universal packages from the [1.9.3 release](https://github.com/klygam-hue/3D-Jelly-Physics/releases/tag/v1.9.3). Extract and run `3D_Jelly_Physics.exe` on Windows, `3D_Jelly_Physics` on Linux, or `3D_Jelly_Physics.app` on macOS. Desktop CI runs the shared physics regressions on Windows, Linux x64/ARM64 and macOS, plus Linux software-Mesa graphics checks.

## Android and M-series iPad — 1.9.3

Native mobile ports use the same C++20 renderer and 120 Hz physics, including the low-FPS fix and smoother high-Softness dragging. The application version is **1.9.3** (mobile build 193).

| Platform | Requirements | Package |
|---|---|---|
| Android | Android 8.0+, ARM64/x86_64, OpenGL ES 3.0 | [Download test APK](https://github.com/klygam-hue/3D-Jelly-Physics/releases/download/v1.9.3/3D-Jelly-Physics-1.9.3-android-test.apk) |
| M-series iPad | iPadOS 16+, arm64, landscape | [Download unsigned IPA](https://github.com/klygam-hue/3D-Jelly-Physics/releases/download/v1.9.3/3D-Jelly-Physics-1.9.3-ipad-unsigned.ipa) |

The Android APK uses a development certificate. An older test installation may need to be uninstalled if its signature differs. **The unsigned IPA requires Apple signing/provisioning before installation on an iPad.** These are personal development builds, not Google Play/App Store/TestFlight releases.

One finger grabs/orbits; two fingers pinch/orbit; three fingers pan. Adaptive Physics, Jelly, Scene, Ground and Tools controls retain elastic stretching and throwing after release. Balanced/Detailed use eight/ten solver iterations. The app runs offline with embedded fonts/shaders and local preferences.

Mobile CI checks native builds, Android signatures/16 KB alignment, 120-frame/240-step ES 3 smoke tests and framebuffer captures. Physical Android GPU drivers and M-series iPad hardware are not tested. See [validation records](Mobile/VALIDATION.md), [installation/build instructions](Mobile/README.md) and [1.9.3 changes](Mobile/RELEASE_NOTES_1.9.3.md).

For an unsigned device IPA on a Mac with Xcode/CMake/Python: `python3 Mobile/build_ipad.py`. Sign and install with your own Apple development identity/profile as described in the mobile instructions.
