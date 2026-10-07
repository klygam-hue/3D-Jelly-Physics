# 3D Jely — development plan

Project root: `C:\Users\User\Code Projekts\3D_Physiks`. The direct user instruction supersedes the older `VS Projekts` path in Prompt.txt. Source/build files belong in Code; mandatory documentation belongs in Docs. Existing idea.md and Prompt.txt are retained.

## Version 1.9 — softness, monitor pacing and publication (in progress)

- [x] Re-map Softness from true rigid 0% to visibly compliant jelly; use a weighted surface grab, validate whole-body stretching, spring compression/recovery and rigid rotation/contacts. Fixed-step handle pacing and bounded volume backtracking address extreme pulls.
- [x] Follow the current monitor's reported refresh rate/VSync; expose monitor Hz separately from fixed 120 Hz simulation and measured FPS. 280 Hz is verified here; physical multi-monitor moves/mode changes remain a manual hardware check.
- [x] Final regression/graphics/portability/security/license review and source publication passed for 3D Jely Physiks; original MIT and dependency notices are intact. All four native CI jobs pass; Linux software graphics checks are separate from hardware evidence.
- [~] Publish and verify final Windows/Linux/macOS assets, complete source archive and SHA256 checksums.

## Version 1.8 — ground customization

- [x] Dedicated Ground subpage/navigation, independent width/depth, color, plain/grid/tiles, pattern scale/strength, visual roughness, outline and ring size; independent reset.
- [x] Match rectangular render/physics/spawn/drag bounds; transactional non-deforming resize, conservative rejection of unfit/worsening contacts, fixed unit plane/shadow projection, no GPU mesh reallocation.
- [x] 29 headless groups; GL/Vulkan Release/Debug Ground UI/style/spawn/rectangular-physics checks, API preservation, source-free captures and measured small rendering cost. Platform/hardware limitations remain explicit.

## Version 1.7 — random jelly spawn

- [x] Always-visible animated Spawn jelly button and B shortcut; additive spawn, including while paused/menu collapsed.
- [x] Random physical rounded-box/ellipsoid/capsule-like/pillow rest shapes with varied scale/rotation and independent RGB color; positive tetrahedra and rebuilt rest constraints/masses.
- [x] Clear-column placement with particle-radius margin, explicit 16-body budget, wake on spawn and reuse of existing GPU meshes.
- [x] Seeded invariants/determinism/drop/wake tests (26 total groups), real button-driven paused/collapsed-menu spawning and picking; GL/Vulkan multi-body tests. Validation.md separates platforms/builds and outstanding manual checks.

## Version 1.6 — cross-platform implementation and explicit validation boundary

- [x] Native OpenGL Windows/Linux/macOS build profiles; AMD/NVIDIA/Intel selected by actual capabilities, not vendor IDs; Apple Silicon macOS target included. Cross-compilation is recorded separately from full builds and hardware runs.
- [x] Linux x64/arm64 pinned ANGLE runtime, explicit library selection, native Vulkan-device checks, isolated POSIX driver probe and per-user preferences. Linux hardware validation remains unchecked below.
- [x] Package layouts, reproducible dependency restore, prepared (not dispatched) CI build/test matrix and compatibility instructions.
- [ ] Run on physical AMD/NVIDIA/Intel Linux and macOS machines. This Windows host cannot supply those hardware checks; build profiles/CI are not runtime evidence.
- [ ] Native macOS Vulkan via a separate MoltenVK-capable renderer. ANGLE's macOS backend is Metal, not Vulkan; this release offers native OpenGL on Mac and labels Vulkan unavailable honestly.

## Version 1.5 — completed Vulkan/OpenGL selection

- [x] Explicit hardware Vulkan via ANGLE/EGL; separate native OpenGL, all current scene/UI effects and verified native VkDevice/physical-device handles. No presentation-only or renamed OpenGL mode.
- [x] Pinned graphics-only assets from official Electron 41.0.0/Chromium, full notices, SHA256-checked embedded/cache bytes and safe absolute loading. No browser/SDK/manual DLL package needed at launch.
- [x] Graphics settings, actual driver/API reporting, persisted choice, graceful unavailability rollback and exact state-preserving recreation.
- [x] Isolated bounded startup probe and workaround for the observed AMD optional maintenance-presentation crash, without quality/MSAA reduction or system changes.
- [x] Both APIs' Release/Debug UI/scene/resize checks, 24 live switches with exact state checks, 0/65/100 visual parity, twenty-two headless groups and source-free strict launches. See graphics.md.
- [x] Measure both: native OpenGL is faster on this host and stays default; no Vulkan speedup or universal bug-free claim.

## Version 1.4 — completed Liquid Glass-inspired UI

- [x] Original OpenGL glass material (shared blurred scene, edge refraction, rim highlights), floating rounded controls and hover/press/tab animations; embedded Inter/OFL.
- [x] Reversible animated menu collapse with always-visible Controls launcher, Tab shortcut and matching input geometry through transitions.
- [x] Quarter-resolution cached GPU blur, reduced motion/opaque UI, OS animation preference, transactional resize and endpoint checks.
- [x] Twenty-one headless groups and twenty-one synthetic UI assertions; Debug/Release graphics/camera checks, source-free Release and measured rendering cost. This is an Apple-inspired Windows UI, not Apple's native framework.
- [x] Preserve exact physical softness on tab changes; no log/pow round-trip writes without slider input. Solver unchanged.

## Version 1.3 — completed optimization pass

- [x] Capture reproducible uncapped stage timings before optimization (60 warm-up frames; three alternating before/after runs).
- [x] Cache edge denominators, damping, surface indices/bindings and flat smoothing adjacency; reject separated body pairs early.
- [x] Conservative supported-body sleep and immediate wake on input/settings/state changes; retain continuous no-sleep regression.
- [x] Skip unchanged GPU uploads, shadows and transparent composition; direct opaque/invisible paths and cached shader locations.
- [x] Cache ASCII glyph lookup and share font atlas with UI shapes; sample volume telemetry at 10 Hz.
- [x] Compare timings/pixels and nineteen regression groups; rebuild Debug/Release and verify source-free Release.
- [x] Publish performance.md with active-versus-resting results. No resolution, precision or solver iteration reduction.

## Version 1.2 — completed material update

- [x] Adjustable transmission: 0% opaque, 100% invisible, 65% transparent default.
- [x] Jelly tab with refraction, gloss, tint strength and HSV color/presets.
- [x] Per-body back-face eye-depth capture, thickness-driven absorption and screen-space refraction.
- [x] Sorted composition with separate color targets and depth/color copies; no framebuffer feedback.
- [x] Shadows fade with transparency; automatic 120-second day/night cycle.
- [x] Compile and validate endpoints, overlapping bodies from opposite sides, day/night, shader controls and framebuffer resize.
- [x] Rebuild Debug/Release, verify a source-free launch and update evidence/delivery artifacts.
- [x] Separate material/environment resets and smooth the automatic light orbit across sunrise/sunset.

## Completed appearance update

- [x] Remove sphere and block from all playable presets and their contacts; replace Obstacle with High drop.
- [x] Add independent Appearance/LightingState and HSV background/platform controls.
- [x] Add time-of-day slider and presets, changing key-light position, color, intensity and ambient light.
- [x] Add contrast, filmic tone mapping, higher-precision packed depth, grid/ring toggles and palette presets.
- [x] Build and inspect day/night/customized scenes; all fourteen test groups pass in Release and Debug.
- [x] Rebuild/install latest Release, verify a source-free launch, synchronize validation evidence.

## Vision and current phase

Version 1.9 now adds true rigid/soft endpoints, distributed elastic grabbing, inversion backtracking and current-monitor render pacing. Windows Release/Debug physics: 35 regression groups pass. Final graphics/performance/release publication evidence is being recorded in validation.md. The complete editable source remains in Code; publication includes source, MIT original-code licensing and intact dependency notices. Older milestone paragraphs below retain historical state, not current feature claims.

A maintainable C++20 desktop soft-body laboratory with rounded jelly, elastic whole-body interaction, customizable lighting/ground, glass controls and a portable Windows executable. Current phase: version 1.9 validated rigid/soft endpoints, distributed grab, inversion safeguards, monitor-aware rendering and source/release publication. Native GL and Windows/Linux ANGLE/Vulkan paths coexist; macOS remains native GL. Windows/Radeon runtime and cross-compilation are separate from remote build and foreign/vendor hardware evidence. Reference-video fidelity and exact real-material calibration remain unverified.

## Milestones and priorities

- [x] Inspect existing source and documentation: Code was empty; structure.md was empty.
- [x] Choose CMake, static raylib 5.5, graphics-independent physics.
- [x] Implement tetrahedral topology, lumped masses, XPBD distance and volume constraints, inversion barrier.
- [x] Implement fixed 120 Hz stepping, substeps, contacts, friction, damping, bounded impulses and compliant dragging.
- [x] Implement separate welded render surface, trilinear binding, Taubin smoothing and normals.
- [x] Implement window, camera, embedded shaders/font, shadows, scene and controls.
- [x] Build Windows Debug/Release and no-Vulkan/headless boundaries; twenty-three regression groups pass. Both APIs' graphics/UI checks are recorded separately from manual input and cross-compilation.
- [x] Run rendering/action/camera/picking smoke checks; inspect generated frame. Physical mouse/keyboard testing remains separate and unperformed.
- [x] Verify the executable in a source-free directory and inspect imported DLLs.
- [x] Synchronize documentation with verified implementation and results.

## Architecture and technical decisions

Physics owns particles, unique distance edges, positive rest-volume tetrahedra and constraint multipliers. Freudenthal triangulation keeps shared faces consistent. Double precision, exponential damping and fixed substeps reduce numerical drift. Per-tetrahedron XPBD volume constraints preserve local volume; a unilateral hard barrier discourages inversion. Compliance is scaled by substep duration squared. Particle masses derive from tetrahedral volumes.

Rendering consumes interpolated physics positions; no graphics calls occur in the solver. A welded, higher-resolution cube surface uses pre-existing topology and fixed GPU buffers. Four Taubin passes and area-weighted normals smooth the surface independently of the solver. Surface smoothing is visual and never alters the simulation.

raylib 5.5 remains the source-built window/input/render frontend with guarded patches/licenses. Windows embeds the optional pinned ANGLE runtime/notices in one EXE and extracts verified cache bytes. Linux ships exact runtime SO/notices next to the executable; Mac is a native GL .app profile. Shaders/font are compiled into each app, physics remains independent. Platform system libraries and a working GPU driver are required. No Electron/browser framework is used.

## Physics roadmap

- [ ] Calibrate a physically meaningful material/size/density and continuum/viscoelastic model against measured compression/drop/oscillation data. User requested real-life fidelity; the actual reference material/conditions remain unspecified. Version 1.4 does not claim exact real-world physics.

- [x] Gravity, inertia, elastic compression/stretch, local volume, floor/wall contacts. Sphere/AABB support is retained for explicit test fixtures; all playable scenes are obstacle-free.
- [x] Restitution threshold, Coulomb-style contact friction, energy limiting safeguards.
- [x] Two-body particle contact with sweep-and-prune.
- [ ] Continuous surface collision / robust tetrahedral inversion handling for extreme manipulations.
- [x] Conservative global sleep for supported, settled bodies with explicit wake/invalidation checks.
- [ ] Optional self collision, arbitrary triangle obstacles and per-body contact-island sleeping.

## Rendering, input, performance roadmap

- [x] Liquid Glass-inspired UI, animated reversible collapse/launcher, pill/tab feedback and reduced-motion/opaque presentation. New UI adds a measured 0.127..0.204 ms median frame cost on this machine; cached paused blur refreshes once in 600 frames (interface.md).

- [x] Smooth lit materials, directional PCF packed-depth shadow map and configurable procedural floor. Sphere/block render meshes removed by user request.
- [x] Appearance tab: time of day, sun/moon light direction and temperature, contrast, independent HSV background/platform colors, three themes, grid/ring toggles.
- [x] Jelly tab: opacity endpoints, thickness-based transmission/refraction, highlights/tint/custom HSV, presets and independent reset.
- [x] Auto day cycles in 120 seconds, pauses with the simulation and moves lights continuously through the horizon.
- [x] Orbit, zoom, pan, mouse drag/throw, impulses, scene presets, live parameter controls, debug views.
- [x] Measure stage/frame costs including active solving separately from sleep: single 1.513 -> 0.656 ms, duet 2.993 -> 1.560 ms, opaque duet 3.024 -> 1.635 ms; active no-sleep duet 2.979 -> 2.742 ms. See performance.md; no isolated GPU or cross-machine profile.
- [ ] Future improvements: GPU deformation, contact-aware surface smoothing, higher-quality optical transport, SSAO, higher-resolution meshes and saved settings.

## Limitations and known risks

Vulkan is an ANGLE-translated hardware backend, not a handwritten native renderer/full arbitrary GL port. OpenGL is faster in measured workloads. API changes require leaving fullscreen; high-DPI/multi-GPU/minimize/manual interaction and other vendors are not comprehensively verified. Khronos Vulkan validation layers are unavailable on this host. The isolated probe contains startup crashes/timeouts, not all future device loss. Profile/cache write failure disables Vulkan safely; preference remains separate from session-only appearance. Optional maintenance presentation is disabled only within the process for the observed AMD fault. No claim of universally bug-free drivers is made.

No continuous collision detection, self collision or exact surface-to-surface contact. Interbody collision approximates surfaces by particles; large deformation can expose sampling gaps. Wall constraints are axis-aligned. A volume barrier reduces inversions but does not mathematically guarantee an inversion-free mesh under arbitrary forces. Velocity/drag bounds are numerical safeguards. Transmission/refraction is screen-space and thickness-driven, without ray-traced optical transport. Center-sorted whole bodies can show ordering artifacts for deeply interleaved/non-convex shapes. Exit-depth capture covers 128 world units. Shadows attenuate uniformly rather than transmitting colored volumetric light. Composition buffers are single-sample; the window retains the MSAA hint, but no new postprocess AA is implemented. No SSAO. Visual smoothing can slightly cross contact surfaces. Dropped backlog slows simulation under sustained frame overload. Appearance is session-only; no persistence between launches. Physical mouse/keyboard use, Visual Studio builds and non-Windows builds have not been manually verified. No observed failure remains in the covered tests; this does not imply exhaustive validation or formal leak analysis.

## Validation evidence (2026-10-07)

Current 1.9: 35 Release/Debug numerical groups; both APIs' 9 new Softness assertions plus existing UI/spawn/Ground/scene and six-body endpoint checks. Fifteen total real API recreations across GL/Vulkan starts preserve tracked state; source-free/Debug endpoint checks and simulated failure handling pass. Host monitor/cap independently confirmed 280 Hz, with physics kept at 120. Median paired active/settling frame-wall change stays within about 1.2% without quality cuts; performance.md contains exact data. Original MIT is embedded, dependency bytes/notices preserved. Latest Windows EXE SHA256: `80B35985351EF00D4B85B9654B3C7A5CF42193E52784DB1152FBF2463D4D6BC4`, 35,047,790 bytes. Publication/remote CI status is recorded in validation.md.

Current 1.8: 29 groups passed Windows Release (12.67 s)/Debug (138.87 s); final stricter contact-rejection cases also passed Debug. Both APIs/Release/Debug passed 17 Ground assertions, 21 existing UI assertions, spawn/picking and six-body 6 x 32 tests. Final source-free GL/Vulkan Ground checks and a 24 x 10 checker capture passed; 4 API changes preserved ground/scene state. All 14 C++ units cross-compiled for Linux x64/arm64/Mac x64/arm64; Linux headless ELFs linked, not executed. Two mesh uploads only (floor/body), one composition generation during Ground changes. Median duet frame wall 1.80155 -> 1.82885 ms GL and 2.35816 -> 2.39574 ms Vulkan, a small measured feature cost without precision/iteration cuts. Current EXE: 35,019,342 bytes, SHA256 `ED30191E9D1123366DB3FBEAC2AD799AB339EFD332104D106BAC6FF4518AB7B9`. See validation.md/performance.md; foreign graphics/hardware/manual checks remain outstanding.

Historical 1.7: 26 groups passed Release (12.75 s)/Debug (139.13 s); final focused spawn assertions also passed Debug. GL/Vulkan actual-button spawning/picking passed 7 assertions, including paused old-body preservation and menu collapse; 8-body 600-frame checks, original 21-assertion UI/scene checks, 4 API changes with 7 random bodies and source-free launches passed without errors. All 14 C++ modules cross-compiled for Linux x64/arm64 and Mac x64/arm64; Linux headless binaries linked, not executed. Windows EXE: 35,000,217 bytes, SHA256 `26ECD8CBE441E49F0CDEDCB2EBE1553321D2E26EA93716EF36F87B8E1AED0B53`. 16-body UI budget, explicit random color ownership and retained GPU mesh allocations documented; performance is not guaranteed at maximum moving/Detailed count. Existing platform/physics/optical limitations remain. See validation.md for scope and previous results.

Historical 1.6: 23 groups pass in Windows Release (8.94 s, 8.96 total)/Debug (105.59 s, 105.62 total). A no-Vulkan build also passed 23 groups plus scene/UI graphics CTests. Both APIs passed strict 300-frame scene/480-frame 21-assertion UI checks, 12 switches/12 exact state checks and source-free launches, zero graphics errors. All 14 project C++ units cross-compiled for Linux x64/arm64 and macOS x64/arm64; Linux headless binaries linked, not executed. Complete foreign graphics builds/SDK linkage/hardware and NVIDIA/Intel remain untested; CI is prepared only. Historical EXE: 34,981,167 bytes, SHA256 `60F6EAD8A2B35776747DAAA2182EA49988C91BF4633A85F242D63DD9D382DB25`. GL/Vulkan median mean frame wall: 1.80596/2.30645 ms on this AMD/Windows host. See compatibility.md and validation.md, including the initially early-closed UI attempt and successful serial repeat. Historical evidence follows.

Historical 1.5: twenty-two groups pass in Release (8.85 s, 8.86 total) and Debug (89.60 s, 89.62 total), plus a separate no-graphics build. Both APIs pass Release/Debug 21-assertion UI and 300-frame scene checks; Vulkan confirms native devices and no graphics errors. Release stress switched 24 times with 24 exact pose/velocity/settings/camera checks and 150 FBO allocations/releases; Debug switch smoke also passed. Opacity 0/65/100 side-view crops differ by at most one channel level. Source-free strict Vulkan/OpenGL runs passed with automatic cache creation. Historical EXE: 34,979,987 bytes, SHA256 `A2FE6E3E93574593B6DC2288A35B1760C99D3E80B1C00D018D0542B5D66CB154`. Only Windows system DLLs are statically imported; embedded dependencies load dynamically. Historical evidence follows. graphics.md records methodology, restrictions and slower Vulkan cost honestly.

Historical 1.4: twenty-one groups passed in Release (8.87 s, 8.88 s total) and Debug (87.20 s, 87.22 s total), with twenty-one UI assertions and source-free runs. The 1.4 artifact was 5,274,490 bytes, SHA256 `A9C74C406FD2788BE8170314A9C37B8B80467A7701A162798FE246C0E14AB04B`. Interface.md retains those earlier measurements; it is not the current artifact. Historical 1.3/1.2 evidence follows below.

Historical version 1.3: nineteen groups passed in Release (8.90 s, 8.92 s total) and Debug (89.91 s, 89.94 s total). Final graphics-only changes rebuilt afterward and passed 300-frame action/resize smoke checks in both configurations. A source-free Release run returned zero with five camera checks, one pick check, two finite bodies, unit volume and no error/obstacles. Render target generation reached three after two resizes. Deterministic one/two-body 3D crops match the pre-optimization screenshots pixel-for-pixel; paused cache reuse was checked over 600 frames. Numerical stability remains tested with sleep disabled. Detailed evidence is in validation.md and performance.md.

Historical 1.3 Release: 4,637,197 bytes; SHA256 `0758A0D645375DC64F2E6B6BEBA699E4AA56901278171723B47F3B0153B092FE`. Same six Windows system DLLs only. The following paragraphs retain historical version 1.2 evidence, not the current artifact:

See validation.md for commands, environment and limitations. GCC 16.2.0 / w64devkit 2.10.0 and local CMake produced both configurations. Version 1.2 final Release CTest: 10.37 s, fifteen groups, zero failures. Full Debug CTest passed in 131.01 s; subsequent small reset/orbit refinements passed focused material and appearance groups in Debug. The 20-second resting test ended at volume ratio 0.999966 and maximum node speed 3.42855e-6 m/s. The suite covers physics, geometry, empty presets, material bounds and reset independence, HSV, cycle wrap and horizon continuity.

The final version 1.2 300-frame Release smoke run from a directory containing only the EXE completed with exit 0 and no simulation error: camera checks 5/5, surface picking 1/1, both bodies finite, volume ratios 1.0 / 1.0, zero sphere/block obstacles. Target generation reached three after two automated window resizes. Runtime logs recorded ten framebuffer allocations and ten unloads. Separate 0/65/100-percent, day/night/sunset, opposing overlap views and shader-control captures were inspected. Pixel comparisons confirmed visible effects for refraction, gloss and tint. A final Debug graphics/action/resize smoke also returned zero. Embedded font and all three custom shaders initialized on Radeon RX 7600 XT / OpenGL 3.3.

PE imports: GDI32, KERNEL32, msvcrt, SHELL32, USER32, WINMM only. OpenGL is resolved through the operating system/driver. No external raylib/libstdc++/libgcc DLL or runtime font/shader asset. Version 1.2 Release size: 4,615,862 bytes; SHA256 2562F08ED4E0A687C5930ADC5883CB9DF4A4C83B4FA1D87913B1D04282472E87. Installed release is copied from the canonical latest build; editable source and licenses are retained. GPU cleanup is recorded in smoke logs; no formal allocator/leak tool was run.

## Validation strategy

Headless tests must cover topology and positive rest volumes, mass, rigid translation invariance, free fall, drop/resting stability, impulses, obstacle contacts, drag/release, multiple-body contact, determinism, invalid inputs and render binding/normals. Build both configurations. Automated graphics smoke mode must load shaders and render offscreen capture with bounded lifetime. Single-file release must run outside the source directory. Manual interaction coverage is recorded separately from synthetic checks.

## Dependencies and ownership

Performance remains an acceptance criterion: measure representative before/after runs for expensive changes, separately covering motion, settling/idle, multiple bodies and material endpoints. Keep stable topology/buffers and reusable scratch; avoid per-frame resource recreation and unnecessary allocation/upload. Invalidate every cache for relevant state changes. Do not silently lower quality/iterations or enable unsafe fast-math/machine-specific instruction flags for a timing gain. Correctness/portability take priority; document remaining bottlenecks honestly.

CMake 3.25+, C++20 and patched raylib 5.5; Windows/Linux Vulkan assets are pinned to official archives/files. Linux x64/arm64 restore via bootstrap_angle.cmake; Windows via bootstrap_angle.ps1; Mac needs no ANGLE. Native GL is WGL/GLX/NSGL. Physics/headless/no-Vulkan builds need no optional graphics runtime. Presets provide VS/MinGW/native/Ninja/universal Mac; native foreign builds require platform development headers/frameworks. RAII/main-thread context ownership is unchanged. Windows/POSIX Vulkan startup probes are isolated owned helpers. Hardware compatibility and code/binary signing are separate from source/cross-compile evidence; compatibility.md describes exact boundaries.
