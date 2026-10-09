# Mobile 1.9.2 validation — 2026-10-09

## Android build

Built on cloud Linux using Android platform 35, Build Tools 35.0.0, NDK r28c (28.2.13676358), CMake 3.31.6, Ninja 1.13.0 and Java 17. Both ARM64 and x86_64 Release native builds pass. Package ID: `org.jely.mobile`; version name/code: `1.9.2`/`192`; minimum Android API: 26; target API: 35.

- Test APK: `3D-Jelly-Physics-1.9.2-android-test.apk`, 6,017,828 bytes.
- SHA-256: `6ca11dd87727aa71c0a4a1798365c9185937ba82c87f632c33468da2bba1a2ab`.
- `apksigner verify --verbose`: v2 and v3 signatures pass.
- `zipalign -c -P 16 4`: pass.
- All LOAD segments in both stripped, packaged native libraries have alignment of at least 16,384 bytes.
- Portable touch regressions pass: capture, cancellation, pinch, pan, stable identity and handover.

The APK uses a development certificate and is debuggable. It is a personal test build. Only packaged copies of native libraries are stripped; original build outputs retain debugging information. No private keystore is tracked in the repository.

Actual SDK linking exposed raylib's ES 2 `glDrawBuffersEXT` reference. The mobile build copies raylib into its build directory and switches this reference and its color-attachment constants to ES 3 core symbols. The pinned vendored desktop source is unchanged.

Android runtime validation passes using an API 26 x86_64 AOSP emulator and SwiftShader ES 3.0. The 1.9.2 APK installs as an update over the existing 1.9.1 test build and launches successfully. The 120-frame smoke completes 240 physics steps, reports zero OpenGL errors, writes its framebuffer capture and reports three finite bodies with positive minimum tetrahedron ratios. The screenshot was inspected: scene geometry, translucent settings panel, labels and controls render correctly. Hardware virtualization is unavailable in this cloud environment, so CPU emulation is used. Compilation, signing and alignment are not physical-device or GPU compatibility tests.

## iPad boundary

No IPA has been compiled or validated yet. The local environment is Linux. The GitHub macOS workflow builds an unsigned arm64 device IPA and runs the iPad simulator smoke; its result is pending. An unsigned IPA will also need Apple signing/provisioning before installation on an iPad.

`build_ipad.py` now provides the same device build locally and in CI. It checks Xcode/iPhoneOS SDK availability, version 1.9.2/192, the iPad device family, the device platform and the arm64 executable before packaging an IPA and SHA-256. Python syntax, help output and the Linux preflight refusal have been checked. macOS compilation and packaging remain unverified; no IPA is claimed from these checks.

## Publication boundary

The repository is renamed to `klygam-hue/3D-Jelly-Physics`. Source and workflows are being published on `feat/mobile-192`; version 1.9.2 adds mobile ports and the new product name. Existing 1.9.1 and 1.9.0 releases remain available.

The first emulator run exercised rendering through the final screenshot stage, where it exposed an absolute-path bug in raylib Android file saving. The build-directory wrapper now preserves absolute paths, and the native app ensures its private files directory exists. The report check uses `ls`, which is available on Android 8, rather than assuming a standalone `test` executable exists. The corrected APK passes the complete emulator smoke and capture checks.

## Final emulator report

```text
version=1.9.2
frames=120
physics_steps=240
shader_api=OpenGL ES 3
driver=OpenGL ES 3.0 (OpenGL ES 3.0 SwiftShader 4.0.0.1) / Android Emulator OpenGL ES Translator (Google SwiftShader)
graphics_errors=0
captured=1
error=
finite=1 min_tet=0.994455 volume=0.999928
finite=1 min_tet=0.983799 volume=0.999968
finite=1 min_tet=1 volume=1
```
