# 3D Jelly Physics 1.9.2

This release adopts the name **3D Jelly Physics** and advances the application version to **1.9.2** (mobile build number 192). Window titles, launcher labels, desktop/mobile build targets, distributable filenames, workflow artifacts and documentation use the new name. The fixed-rate 120 Hz physics and the 1.9.1 fixes for high Softness and dragging at 10–15 FPS are retained.

Existing mobile app identifiers and development signing keys are preserved for updates. Settings from the previous build are read as a fallback; new settings use a version-independent file. Desktop installations reuse an existing project cache and graphics preference instead of discarding them during the rename.

## Android test build

- Native C++20 NativeActivity app for Android 8.0+, ARM64 and x86_64, using OpenGL ES 3.0.
- Touch controls: single-finger grab/orbit, two-finger pinch/orbit and three-finger pan, with stable finger ownership and cancellation during gesture changes.
- Adaptive Physics, Jelly, Scene, Ground and Tools settings, translucent controls, embedded font and shaders, and offline operation without account or device permissions.
- Physics, appearance and quality preferences are saved in the app's private directory. Ground/background/world state is not persisted.
- Mobile ES 3 linking and private-file saving fixes, smaller packaged native libraries, and 16 KB ELF/archive alignment.
- Test certificate and debugging enabled; intended for personal testing.

The cloud-built APK installs and passes the API 26 AOSP x86_64/SwiftShader smoke: **120 frames, 240 physics steps, zero OpenGL errors, all bodies finite, positive tetrahedron ratios and a visually verified screenshot**. Portable touch regressions also pass. Physical Android devices and GPU drivers remain untested.

## iPad preparation

The native arm64/iPadOS 16+ source, SDL/UIKit integration, EAGL framebuffer handling and macOS device/simulator workflow are prepared. A one-command Mac builder validates device metadata and arm64 architecture before packaging an unsigned IPA.

**No IPA has been built or tested yet.** GitHub Actions will compile the device and simulator targets on macOS. After compilation, Apple signing and provisioning will be required for device installation. M-series iPad hardware, App Store and TestFlight distribution are not validated.

## Publication status

Version 1.9.2 introduces the new name, native mobile source and mobile build workflows. APK and IPA assets will be attached after their respective build checks pass. Earlier 1.9.1/1.9.0 releases are retained.
