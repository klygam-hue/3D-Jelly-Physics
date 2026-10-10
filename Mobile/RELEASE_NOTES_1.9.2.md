# 3D Jelly Physics 1.9.2

Version **1.9.2** (mobile build **192**) adopts the name **3D Jelly Physics** for window/launcher labels, build targets, packages, workflow artifacts and documentation. Earlier releases remain available. The 120 Hz solver, high-Softness elastic hardening and 10–15 FPS dragging fixes are retained.

## Soft jelly and rapid dragging

- Give the physical grab handle finite acceleration and braking near its target. Abrupt mouse/touch reversals no longer inject an instantaneous 12 m/s velocity change into a small surface patch.
- Add XPBD viscous damping relative to the moving hand, preserving body inertia, elastic stretching and throws after release.
- Keep the existing Softness 0–100% mapping, nonlinear elastic hardening, mass distribution and volume solver. The 0% endpoint remains rigid.
- Add corner-grip reversal, floor-lift and throw-recovery regressions at 80–100% Softness on both mesh resolutions. All **43** headless regression groups pass locally.
- Match mobile Detailed quality to the desktop's ten solver iterations; Balanced retains eight.

## Android

Native C++20 NativeActivity app for Android 8.0+, ARM64/x86_64 and OpenGL ES 3.0. Includes adaptive Physics/Jelly/Scene/Ground/Tools controls, translucent UI, embedded font/shaders, local preferences, 16 KB native/archive alignment and offline operation without account, Internet, camera, microphone or storage permissions.

The APK uses a development certificate and enables debugging. CI builds generate test certificates; if Android reports a signature mismatch, uninstall the older test build before installing this one. Uninstalling clears saved settings. No private keystore is tracked.

## iPad

Native arm64/iPadOS 16+ port for M-series iPads, with SDL/UIKit integration, ES 3 shaders, correct EAGL framebuffer/renderbuffer restoration, a launch screen and embedded license notices. The Mac builder validates the device platform, iPad family, version/build and arm64 executable, then packages an **unsigned IPA** and SHA-256.

**Apple signing/provisioning are required for installation.** This is not an App Store or TestFlight release. Emulator/simulator smoke reports and captures are shipped with the packages; physical Android GPU drivers and M-series iPad hardware are not validated.

One finger grabs/orbits; two fingers pinch/orbit; three fingers pan. Mobile app identifiers remain stable. Preferences use version-independent storage with a legacy fallback; desktop installations reuse existing project caches.

See [validation records](VALIDATION.md) for the final CI runs and numerical comparison. This real-time simulation approximates jelly; its material parameters are not calibrated against a measured real gel.
