# 3D Jelly Physics 1.9.3 validation — 2026-10-10

## Floor dragging regression

A recorded report showed the grab point/line moving while the body stayed against the floor. The user confirmed 1.9.2 at 80–100% Softness. Headless Detailed-mesh floor reversals reproduce a local compression reaching the inversion boundary and cancelling the entire body's movement. The patch replaces that global fallback with bounded individual-node backtracking against adjacent tetrahedra. The valid-pose fast path remains unchanged. Cached adjacency and scratch arrays avoid per-step allocation; already accepted nodes are skipped on later fallback passes.

A deterministic Detailed probe settles 480 ticks, reverses the floor cursor between x=±5 and z=±3 for 720 ticks, holds the last cursor for 480 ticks, then releases for 960 ticks. The candidate follows the actual requested cursor and recovers:

| Softness / grip | Requested cursor-to-node gap after hold | Minimum tetrahedron/rest ratio after release | Maximum edge strain after release |
|---|---:|---:|---:|
| 95% / face | 0.024 | 0.995 | 4.90% |
| 95% / corner | 0.210 | 0.995 | 4.92% |
| 100% / face | 0.028 | 0.993 | 9.79% |
| 100% / corner | 0.203 | 0.993 | 9.80% |

All four released bodies have volume/rest approximately 1.000. Remaining deformation under gravity is included in the reported strain. This is numerical validation, not real-gel calibration or a test on the user's physical laptop.

The final suite contains 45 regression groups. Full local and native CI verification and final package checks are in progress; results are recorded before publication. New tests independently check that one threatened tetrahedron cannot cancel unrelated-node movement, and that floor pulls at 10/15/43 FPS follow the requested cursor, move the body, and recover after release. Existing rapid-air pulls, throws, rigid stone, drops, obstacles, random shapes, timing and graphics checks remain required.

## Final packages

Version/build 1.9.3/193. Desktop, APK and unsigned IPA are rebuilt from the shared patched source. Final CI run links and package checksums will be recorded after those builds complete. The same native mobile packaging/runtime checks described below are required. Hardware limits and signing requirements remain applicable.

---

## Historical 1.9.2 validation

### 3D Jelly Physics 1.9.2 validation — 2026-10-10

## Shared physics: rapid dragging at high Softness

All **43 headless regression groups pass locally**, including new rapid corner reversals, floor-to-air corner pulls and throw recovery. Existing tests still verify 10–15 FPS fixed-step catch-up, the 0–100% Softness mapping, a rigid 0% endpoint, high-drop compression/recovery, weighted surface grabbing, volume preservation and deterministic input batching.

The handle retains its 12-unit/s speed limit and adds finite acceleration/braking. The grip uses viscous XPBD damping relative to the moving hand ([Macklin et al., 2016, equation 26](https://mmacklin.com/xpbd.pdf)). Release removes the grip force; it does not zero body velocity. Material compliance, finite-strain hardening and volume constraints remain unchanged. Mobile Balanced/Detailed use eight/ten solver iterations, matching desktop.

The same deterministic corner-jitter probe was run before and after the change: resolution 7, 95% Softness, eight solver iterations, zero gravity, 360 physics ticks, ±5-unit cursor reversals every ten ticks and alternating ±2-unit depth offsets every sixty ticks.

| Quantity | Previous grab | Smooth/damped grab |
|---|---:|---:|
| Maximum local edge strain | 205.3% | 30.0% |
| Worst relative volume error | 1.60% | 0.065% |
| Minimum tetrahedron volume/rest ratio | 0.100 | 0.918 |
| Maximum handle-to-grabbed-node gap | 1.973 | 0.329 |

The broader probe also tests meshes 5/7 at 80/95/100% Softness, 83/500 ms reversals and loaded floor lifts. Maximum strain across those cases drops from 205.3% to 49.9%. Tests retain visible elastic stretching, finite positive tetrahedra, shape recovery and forward throw momentum. These are numerical test results, not measured real-gel calibration.

## Final package validation

The updated source is checked in [mobile CI](https://github.com/klygam-hue/3D-Jelly-Physics/actions/runs/38032047740) and [desktop CI](https://github.com/klygam-hue/3D-Jelly-Physics/actions/runs/38032047780). Android and desktop rebuilds pass. Windows, Linux x64, Linux ARM64 headless and macOS universal regressions pass, including Linux software-Mesa graphics checks. The final iPad device build, simulator runtime check and release staging also pass.

## Android

Native C++20 NativeActivity package `org.jely.mobile`, version/build `1.9.2`/`192`. Minimum API 26; target API 35; ARM64 and x86_64; ES 3.0. Build tools: Android platform/Build Tools 35, NDK r28c (28.2.13676358), JDK 17, CMake/Ninja/Python.

The final rapid-grab APK passes API 35 x86_64 emulator validation: 120 frames, 240 physics steps, zero OpenGL errors and framebuffer capture. Its version/build and launcher label are checked as `1.9.2`/`192` and `3D Jelly Physics`. SHA-256: `e0b33d1107b88f083c1f686d1c15be4ef89441f14d7240a9725ee6ad27deb429`.

CI verifies APK v2/v3 signatures, `zipalign -P 16`, and at least 16,384-byte alignment of every packaged native LOAD segment. The APK uses a development certificate and permits debugging. Fresh CI runs generate different test certificates; a signature mismatch may require uninstalling an older test build, which clears its settings. No private keystore is tracked.

Earlier 1.9.2 port validation passed an API 26 AOSP x86_64/SwiftShader emulator run with 120 frames, 240 physics steps, zero graphics errors, framebuffer capture and three finite bodies with positive tetrahedra. That earlier APK also installed over the corresponding local 1.9.1 test build using its retained certificate. This is historical port validation; the final rapid-grab APK is rebuilt and checked in the API 35 CI run linked above.

## iPad

Native arm64, iPad-only device IPA targeting iPadOS 16+. The Mac builder checks `org.jely.mobile`, version/build `1.9.2`/`192`, device family `[2]`, device platform `iphoneos`, arm64 executable and ZIP integrity, then writes an IPA and SHA-256.

The final [1.9.2 mobile run](https://github.com/klygam-hue/3D-Jelly-Physics/actions/runs/38032047740), including the improved rapid-grab physics, passes device compilation/packaging and a genuine iPad simulator ES 3 runtime smoke: 120 frames, 240 physics steps, zero graphics errors and framebuffer capture. The mobile raylib build copy uses Apple ES 3 core format names, restores the EAGL default framebuffer and rebinds its drawable renderbuffer before SDL presents. The launch storyboard has the required Interface Builder metadata. Vendored desktop source remains unchanged.

The IPA is **unsigned**. Apple signing/provisioning are required before device installation. Physical Android GPU drivers, M-series iPads, real-device performance, App Store and TestFlight distribution are not validated. Simulator success is not a physical-device test.

## Packages and publication

The repository is `klygam-hue/3D-Jelly-Physics`. Version 1.9.2 includes the new name, all mobile source/workflows and improved grabbing. The release contains Windows x64, Linux x64, macOS universal, Android test APK and unsigned iPad IPA packages, plus mobile reports/captures and APK/IPA checksum files. Earlier 1.9.1/1.9.0 releases are retained.

Download checksums alongside the matching package from the [1.9.2 release](https://github.com/klygam-hue/3D-Jelly-Physics/releases/tag/v1.9.2). CI stages assets only after the required build/runtime checks pass. No signing identity or provisioning profile is distributed.
