# 3D Jelly Physics — Android and iPadOS 1.9.2

Native mobile ports of the same C++20 XPBD simulation and renderer. These are development ports intended for GitHub Actions builds, not Google Play/App Store releases. The desktop version and earlier release tags are retained.

**Packages:** version **1.9.2**, mobile build **192**. [Release downloads](https://github.com/klygam-hue/3D-Jelly-Physics/releases/tag/v1.9.2) provide an Android test APK and an unsigned arm64 iPad IPA. Android requires Android 8.0+/OpenGL ES 3.0; iPad requires iPadOS 16+ and Apple signing/provisioning. See [validation records](VALIDATION.md) for exact tested builds, emulator/simulator results and hardware limits.

## Android

Android 8.0+ (API 26), ARM64 or x86_64, and OpenGL ES 3.0 are required. Download `3D-Jelly-Physics-1.9.2-android-test.apk` from the [release](https://github.com/klygam-hue/3D-Jelly-Physics/releases/tag/v1.9.2) and open it on your device. Allow installation from the file manager you use. The app runs offline and requests no Internet, storage, camera, microphone, or account permissions.

The APK is signed with a development/test certificate and permits debugging. It is intended for personal testing, not production distribution. A fresh CI run may use a different test certificate; uninstall an earlier test APK before installing it if Android reports a signature mismatch. Uninstalling clears saved settings.

Build with the official Android SDK, Build Tools 35.0.0, platform android-35, NDK r28c (28.2.13676358), CMake 3.25+, Ninja, Python 3, and JDK 17:

```sh
python3 Mobile/build_android.py --sdk "$ANDROID_SDK_ROOT"
```

The APK contains ARM64 and x86_64 native libraries, statically linked C++ runtime, embedded font/shaders, and license notices. NDK r28c plus explicit linker flags align native libraries for 16 KB pages; packaging uses `zipalign -P 16`. This is a native app, not a browser/WebView wrapper.

## iPad with an M-series chip

The iPad-only arm64 build targets iPadOS 16+. It is intended for M1 and later iPads; Detailed physics is the default. It also has no processor allowlist that would unnecessarily block an otherwise compatible iPad. ARM64 alone does not identify an M-series chip.

Download `3D-Jelly-Physics-1.9.2-ipad-unsigned.ipa` and its checksum from the [release](https://github.com/klygam-hue/3D-Jelly-Physics/releases/tag/v1.9.2). It is an **unsigned device IPA**. It cannot be installed by opening its download link: iPadOS requires Apple signing and provisioning. Sign it with your own valid Apple identity/profile using your normal development or sideloading workflow, or build and install from Xcode. No signing certificates, Apple IDs, provisioning profiles, or paid-account access are included. This is not a TestFlight or App Store release.

On a Mac with Xcode, CMake 3.25+, Git and Python 3, build the unsigned device IPA with one command:

```sh
python3 Mobile/build_ipad.py
```

The builder checks the iPhoneOS SDK, bundle version, iPad device family, device platform and arm64 executable before writing `Mobile/package/3D-Jelly-Physics-1.9.2-ipad-unsigned.ipa`, its SHA-256 and installation/release notes. It does not request signing credentials or create an IPA on Linux. Device compilation and packaging pass on the GitHub macOS runner. CMake's [Apple cross-compilation instructions](https://cmake.org/cmake/help/latest/manual/cmake-toolchains.7.html#cross-compiling-for-ios-tvos-visionos-or-watchos) describe the Xcode/SDK setup.

To open a project in Xcode for your own signed device build:

```sh
cmake -S Mobile -B Mobile/build/ipad -G Xcode \
  -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphoneos \
  -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=16.0
open Mobile/build/ipad/3D_Jelly_Physics_Mobile.xcodeproj
```

In Xcode, select `3D_Jelly_Physics_Mobile`, choose your signing Team, enable signing for your local build (the CI default is `CODE_SIGNING_ALLOWED=NO`), select your connected iPad, and Run. CMake fetches the pinned official SDL2 2.32.10 commit `5d249570393f7a37e037abf22cd6012a4cc56a71`. Dependency fetch/build needs network access; the installed app does not.

The iPad port uses SDL/UIKit and OpenGL ES 3.0/EAGL. It does not claim to be a Metal renderer. Apple's OpenGL ES API is deprecated. The port handles EAGL's nonzero default framebuffer, uses the same packed-depth/refraction shaders with an ES 3 header, and runs frames through the UIKit animation callback.

## Touch controls and features

| Gesture/control | Action |
|---|---|
| One finger on jelly | Grab, stretch, drag; release to throw |
| One finger on empty space | Orbit camera |
| Two fingers | Pinch to zoom; move together to orbit |
| Three fingers | Pan camera |
| Settings | Open/close the adaptive control panel |
| Spawn / Pause / Reset / Launch | Same world actions as desktop |
| Back / Next inside Settings | Page through controls on smaller screens |

Touch ownership remains captured until release. A second finger cancels an active grab; returning from two fingers to one cannot unexpectedly press a slider or start another grab. Backgrounding cancels grabs and discards pending catch-up time. The fixed physics rate remains 120 Hz, including the twelve-tick low-FPS correction. Rapid cursor/touch reversals use finite handle acceleration and moving-hand XPBD damping, reducing local spikes while retaining elastic stretching and throw momentum. Balanced uses eight solver iterations and Detailed uses ten, matching desktop quality.

Physics, Jelly, Scene, Ground and Tools tabs expose Softness 0–100%, gravity/friction/damping, Balanced/Detailed quality, scene presets, transparency/refraction/gloss/tint/HSV color, time of day, background, ground size/material/patterns, debug geometry, opaque controls, camera reset, and 60/120 FPS targets. Actual FPS depends on the display, OS and workload; selecting 120 does not guarantee 120 FPS. Maximum simultaneous bodies remains 16. Render quality is preserved; no physics stiffness or mesh quality is silently reduced.

Material, basic physics, quality, frame target and opaque-control preferences are saved locally. Ground/background/session world state is not persisted. Landscape is supported; iPad split-screen is not enabled. Desktop mouse/keyboard behavior is unchanged.

## Validation

The CI workflow builds ARM64/x86_64 APKs, the arm64 iPad device app, and a simulator app matching the runner architecture. The portable touch regressions verify capture, release, pinch cancellation, stable finger identity/order and three-finger pan. The existing desktop physics suite remains the shared solver regression boundary.

Mobile smoke mode exercises transparent/opaque/invisible materials, scene rendering, spawning, 100% Softness, ground resize, grabbing/release and framebuffer capture. Reports and screenshots are included with CI artifacts. Emulator/simulator success is not a physical Android GPU or M-series iPad hardware test. APK/IPA compilation alone is not presented as a runtime test.
