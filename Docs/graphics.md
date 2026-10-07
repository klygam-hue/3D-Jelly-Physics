# 3D Jely Physiks — OpenGL / Vulkan

Version 1.9 retains Windows/Linux ANGLE hardware Vulkan and macOS native OpenGL. Windows/Radeon runtime is checked on both APIs. Remote CI now builds Windows/MSVC, Linux x64 and macOS universal packages, runs native headless tests (also Linux ARM64), and passes Linux OpenGL smoke on software Mesa/Xvfb. Physical foreign GPU runs and NVIDIA/Intel remain unverified; Mac Vulkan is not implemented. See compatibility.md/validation.md for exact evidence. Historical runtime measurements below remain Windows/Radeon evidence, not proof for other platforms/vendors.

## What the modes actually do

OpenGL: native WGL/desktop GL 3.3 through raylib/GLFW. Vulkan (ANGLE): GLFW explicitly selects EGL's ANGLE Vulkan platform, then ANGLE translates the existing GLES3-compatible geometry, shadow, exit-depth, refraction, blur and UI commands/shaders into hardware Vulkan. No whole OpenGL-rendered frame is uploaded to a Vulkan presenter; no software SwiftShader mode is advertised. This is not a handwritten native Vulkan renderer or a full arbitrary-OpenGL compatibility layer.

Activation checks context version and requires all three: an EGL/GLES client context, a real Vulkan hardware driver string, and nonzero native VkDevice/VkPhysicalDevice handles queried through EGL_ANGLE_device_vulkan. Reports expose these checks. Native OpenGL remains a separate choice and safe default. Vulkan implementation is Windows x64 and Linux x64/arm64; headless physics remains independent of either graphics API. SwiftShader, llvmpipe, lavapipe, softpipe and software-rasterizer descriptions cannot masquerade as hardware Vulkan. No AMD/NVIDIA/Intel allowlist is used.

References: [ANGLE's official architecture/platform support](https://chromium.googlesource.com/angle/angle/+/main/README.md), [GLFW context/ANGLE configuration](https://www.glfw.org/docs/latest/window_guide.html), [ANGLE Vulkan device extension](https://github.com/google/angle/blob/main/include/EGL/eglext_angle.h). API translation is explicitly identified in UI/documentation.

## Lifetime and compatibility

GraphicsSession owns window/context, renderer and panel; Application owns the same PhysicsWorld, camera and UiState across a change. GPU owners are destroyed before closing their old context. New graphics bind the current world; simulation pose/velocity, settings, pause and camera are not reset. Smoke switch tests compare exact before/after positions/velocities, settings, appearance, step count and camera. The GUI menu/tab layout is restored. Recreation discards stale window input and skips an accumulated wall-time spike.

Raylib changes are guarded by JELY_DUAL_GRAPHICS: GLFW context/ANGLE hints, fresh input state, procedure gateway, dead rlgl-state reset, valid GLES capability use and sized D32F depth attachments. Shader #version 330 becomes ESSL300 with high-precision qualifiers; shader bodies remain identical. GLES texture swizzle uses individual component enums and clear-depth uses float ABI. The app's current paths are covered; unused arbitrary desktop-only raylib APIs are not claimed portable to GLES.

MSAA x4 remains requested in both modes. Transparent composition remains single-sample as before. Framebuffers are checked for completeness; shader/resource failure rolls back to the prior backend. Screenshots flush/capture before swap because EGL/Vulkan can discard the presented back buffer. Resize recreates existing transactional targets. Minimized windows skip rendering/accumulated time. API changes are rejected in fullscreen with a visible F11 instruction rather than changing window mode behind the user.

An observed crash in AMD's optional swapchain/surface-maintenance presentation path was reproduced in a debugger during the first swap, not hidden as a working mode. Process-local ANGLE overrides disable those optional maintenance features and use standard synchronization; pixels/MSAA/solver quality are not reduced. The override is restored at lifetime end. Vulkan driver startup is additionally tried in a hidden, isolated copy of this EXE (two actual rendered frames, strict API/device checks). A crash/nonzero exit or 25-second timeout prevents activation in the main application. The owned hung helper is terminated; no unrelated process is affected. A successful probe is reused for subsequent switches during that session. This contains startup faults, not all conceivable future GPU device-loss/driver bugs.

No driver update, registry edit, system environment change, or external layer disable is performed. Existing native OpenGL and physics behavior are preserved. Fullscreen switching, manual hardware input, sustained minimize/restore/high-DPI/multi-GPU use and other GPU vendors have not been verified and are not promised bug-free.

## Runtime, persistence and packaging

The single EXE embeds unmodified graphics-only assets from official Electron v41.0.0 (Chromium 146, ANGLE 2.1.27037 / 1d3190bf5633): libEGL.dll, libGLESv2.dll, vulkan-1.dll and full LICENSES.chromium.html. No Electron/Node/browser process/framework is used. Asset/archive hashes and source are in Code/vendor/angle/README.md; official archive hash matches SHASUMS256.txt. bootstrap_angle.ps1 restores only those fixed entries after verifying the pinned archive. CMake checks individual SHA256 and tracks all RC dependencies.

On Vulkan activation, embedded and cached DLL bytes are checked with Windows SHA256, atomically written to a version/hash-scoped private cache and loaded by absolute paths with restricted dependency search. The pinned ANGLE build needs its Vulkan loader next to its own DLL. The normal cache is LocalAppData/3D_Jely/runtime; an unwritable profile uses executable-side 3D_JelyData. If neither can be written, activation fails safely to OpenGL. These generated files are not manually required distribution sidecars. GPU ICD/driver must still be installed by the operating system/vendor.

Only API choice persists, in graphics.txt under the application data root. Normal UI selection saves it atomically; missing/corrupt/oversized preference defaults to OpenGL. Appearance/physics remain session-only. CLI --backend overrides preference for that launch. Smoke starts OpenGL unless explicitly selected and never saves user choice. --licenses prints embedded Inter plus upstream graphics notices.

## Verified checks (2026-10-07)

Windows x64 / GCC 16.2 / raylib 5.5; AMD Radeon RX 7600 XT, AMD driver 26.8.1 / 2.0.395. Vulkan reports 1.4.349. Twenty-two headless groups passed in Release/Debug, including API parsing/lossless shader adaptation. See validation.md for final build time/artifact hash.

- Both APIs: 480-frame UI input-path automation, 21 assertions, all tabs/opacity endpoints, menu collapse/reopen/reversal, topology resets, accessibility and two resizes. Exit 0 / graphics_errors=0.
- Both APIs: 300-frame scene/action/camera/picking/resize checks. Five camera checks, one picking check, two finite bodies, no obstacles or error. All shader/font/framebuffer initialization logs checked.
- Release 1500-frame continuous-motion/resting stress: 12 OpenGL/Vulkan round trips, 24 actual button-driven changes and 24 exact state-preservation checks. No fallback or graphics error; 150 framebuffer allocations and 150 releases. Debug four-change paused smoke also passed. This is bounded lifecycle evidence, not a formal leak audit.
- Deterministic two-body side-view opacity 0/65/100 captures: 3D crop (x=350..1179, y=260..814) differs between APIs by at most one 8-bit channel level; no pixel differs by more than two levels. Mean absolute channel differences 5.79e-6 / 6.51e-6 / 2.17e-6 respectively. Settings/physical outputs match. No image-quality reduction was used.
- Simulated unavailable Vulkan: normal startup returns 0 with active OpenGL and a visible failure explanation; strict startup returns 1 without a blocking error dialog. Strict Vulkan successful runs confirm hardware/native handles, so fallback cannot masquerade as a pass.
- Final source-free launches started from a directory containing only the EXE, passed both strict APIs' 300-frame scene/resize/camera/picking checks with zero errors, and generated the verified runtime cache automatically. Embedded font/ANGLE/Vulkan-loader license output was checked. Final artifact hash/imports are in validation.md.

Khronos Vulkan validation layers are not installed on this host and were not run. ANGLE frontend validation, GL error counters, framebuffer completeness checks, real handles, visual/physical comparisons and lifecycle reports are evidence, not a guarantee that every driver/device/workload is glitch-free.

## Performance

1440 x 900, Balanced resolution, three substeps/eight iterations, default 65% jelly and glass UI. Three alternating 600-frame runs per API/case, first 60 frames excluded; medians of run means. No concurrent compilation/test app. Initialization/probe is outside measured frames. Normal 120 FPS/VSync cap is disabled only for benchmark. Ten simulated seconds (two physics ticks per rendered smoke frame).

| Case | OpenGL frame mean, ms | Vulkan (ANGLE) frame mean, ms |
|---|---:|---:|
| Duet settling + sleep | 1.79887 | 2.33540 |
| Duet continuous no-sleep solving | 2.95547 | 3.61374 |
| Paused single body | 0.479307 | 0.821648 |

Vulkan is functional but not faster on this setup/workload; ANGLE translation/presentation adds overhead. OpenGL stays the recommended/default mode here. Sleeping, exact mesh/shadow/scene caching and quarter-resolution blur remain enabled in both; paused blur updates once per 600-frame run. Values are local CPU/frame-wall measurements, not isolated GPU timestamps or guaranteed FPS. GPU-side/native-Vulkan optimization remains future work, not hidden parity loss.

Reproduce (existing absolute output directory):

```text
3D_Jely.exe --backend vulkan --strict-backend --smoke 480 --ui-automate --report C:\temp\vk-ui.txt
3D_Jely.exe --backend opengl --strict-backend --smoke 1500 --switch-cycles 12 --report C:\temp\switch.txt
3D_Jely.exe --backend vulkan --strict-backend --smoke 600 --benchmark --scene 2 --no-sleep --report C:\temp\vk-cost.txt
3D_Jely.exe --backend vulkan --simulate-vulkan-failure --smoke 120 --report C:\temp\fallback.txt
```
