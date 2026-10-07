# 3D Jely — validation record

Date: 2026-10-07. Root: `C:\Users\User\Code Projekts\3D_Physiks`. User-specified source directory: Code.

## Version 1.9 — current softness, pacing and publication

Windows GCC 16.2 Release: **35 headless regression groups**, zero failures, 26.89 s (26.90 total). Debug: the same 35 groups passed in 316.91 s (317.07 total), concurrently with some graphics/cross-compilation work; this is test duration, not a performance comparison. Six new groups cover continuous/invertible softness and invalid inputs, arbitrary/180-degree proper rigid rotation and reflection rejection, normalized surface attachment and whole-body follow/recovery, Balanced/Detailed spring drop and live stiffness changes, input-batching independence, and all four rest-shape families under extreme floor/wall dragging.

The old narrow compliance range/isolated-node attachment could appear stiff or deform locally. The new exact 0% endpoint uses a mass-weighted rigid fit and rigid velocity field; maximum soft compliance is 0.2 rather than the old slider's roughly 0.000398. 50% retains the prior default exactly. In the reference Balanced high drop, maximum softness reached a minimum height/rest-height of about 0.578 and recovered; rigid strain stayed below 1e-10. Free-space gradual dragging produced roughly 27% peak edge strain at maximum softness and moved the whole center more than 2 m, then recovered to below 2% strain. These are numerical tests of this solver, not calibration against a physical gelatin sample.

A sharp wall pull at maximum softness initially exposed inverted tetrahedra. Fixed-substep physical handle pacing (12 m/s) removed input-frequency-dependent energy injection, and bounded substep volume backtracking addresses constraints/contact corrections that undo local barriers. All covered extreme tests now stay finite, within arena/velocity/volume limits and above the tested 0.05 tetrahedron threshold; the final guard targets 0.1 rest volume. It dissipates rejected motion rather than claiming collision-complete or arbitrary-force mathematical fidelity. No per-iteration heap allocation/topology reduction was added.

Release native OpenGL and hardware ANGLE/Vulkan each passed: 350-frame **9-assertion actual Softness slider/patch/drag/drop** checks; 480-frame **21-assertion existing UI** checks; 300-frame **7-assertion spawn/picking** checks; 350-frame **17-assertion Ground** checks; 300-frame scene/camera/picking/resize; 600-frame six-body maximum-softness rectangular-arena and rigid-body stress. API switching with four bodies/custom ground/max-softness passed 8 exact state checks starting GL and 7 starting Vulkan (the initial Vulkan selection is already active, so is a no-op), zero fallback/graphics/simulation errors. Two early runner assertions incorrectly expected 4 then 8 in both starts; app reports were already successful and the harness expectation was corrected, not the application state checks weakened.

Debug and standalone source-free Release both passed all 9 new Softness assertions on GL and verified native Vulkan devices. The final binary was rebuilt again after embedding the original MIT notice and rechecked outside the source tree. Fallback to GL on simulated Vulkan failure returned 0 with a reported fallback; strict failure and invalid negative softness returned 1 without a smoke-mode dialog. No formal memory allocator/leak tool or Vulkan validation layer run is claimed.

Runtime monitor Hz/render cap were **280**, independently confirmed by Windows Win32_VideoController.CurrentRefreshRate=280; telemetry and capture show 280 FPS in the checked scene. Physics remains 120 Hz and smoke uses deterministic stepping. Current-monitor polling/recreation resets the cap every 0.5 s, fixing a found same-monitor context-recreation limiter reset. Benchmark reports a zero cap. Unknown monitor rate uses uncapped software pacing with VSync; manual physical monitor moves, refresh changes, fractional Hz/VRR and other desktop configurations remain untested.

All 14 application/physics C++ units cross-compiled for Linux x64/arm64 and macOS x64/arm64; Linux headless ELFs linked, not executed on this Windows host. Full remote build status will be recorded separately below. NVIDIA/Intel hardware, foreign desktop GPU runs and macOS Vulkan remain unverified/not implemented respectively; no universal compatibility or bug-free claim. Documentation, vendored notices, private-spec exclusions and staged original-source secret patterns were reviewed. Git attributes explicitly preserve pinned runtime/notices byte-for-byte.

Paired 24-run 1.8/1.9 settling/active benchmark results are in performance.md. Windows standalone EXE: **35,047,790 bytes**, SHA256 **80B35985351EF00D4B85B9654B3C7A5CF42193E52784DB1152FBF2463D4D6BC4**. Static imports are Windows system DLLs only (bcrypt/GDI32/KERNEL32/msvcrt/ole32/SHELL32/USER32/WINMM); graphics runtime/shaders/fonts/original MIT and full graphics notices are embedded. Original source is MIT; third-party licenses remain separate. Publication status is pending verification at this point.

### Native CI and packaging (version 1.9)

[Initial source CI](https://github.com/klygam-hue/3D-Jely-Physiks/actions/runs/37691159136) and [permission-preserving package CI](https://github.com/klygam-hue/3D-Jely-Physiks/actions/runs/37691548361) both completed successfully in all four jobs: Windows 2022/MSVC full app/package and headless regression; Ubuntu 22.04 x64 full app/package, headless regression and five actual software-OpenGL scene/UI/spawn/Ground/Softness smokes; Ubuntu 24.04 ARM64 headless regression; macOS 14 universal x64+arm64 full SDK app/package and native headless regression. Source/workflow revision of the latter is `670451c866c577b862fcd3b25ff9602b77672725`; subsequent release-description changes do not alter the C++/physics/shaders.

Initial Linux graphics CTest: all five passed, 265.37 s total; individual scene/UI/spawn/Ground/Softness durations 54.80/74.43/47.03/29.62/59.49 s. Renderer was llvmpipe (LLVM 15.0.7), not a physical GPU. Downloaded reports confirm 21 original UI/9 Softness checks, zero graphics errors, `monitor_hz=0` and `render_fps_limit=0`, exercising the unavailable-refresh fallback. This is real software rendering evidence, not hardware Vulkan or universal throughput. Windows/MSVC and macOS jobs do not run desktop graphics checks; Linux ARM64 is headless only.

A fresh local Git checkout also configured/built the complete Windows app and passed all 35 groups in 26.84 s, proving source/asset checkout and pinned-byte integrity rather than relying on the old build cache. Unix CI tar archives retain executable permissions. Inspected macOS binary has fat Mach-O magic and two architectures; Linux archive contains an executable and all four adjacent pinned runtime/notices. Packages are unsigned/not notarized, with physical foreign GPU behavior still outstanding.

The editable project is published to the owner's [private repository](https://github.com/klygam-hue/3D-Jely-Physiks). Original private Prompt.txt/idea.md, compiler/build/install/cache data and credentials are excluded; source scanning found no credential pattern in original tracked sources. The original specification remains locally untouched. Release binaries/source archive/checksum publication verification is the remaining delivery step at this snapshot.

## Version 1.8 — historical ground customization

Ground opens its own subpage from an always-available top-bar button, retaining the original tab hit positions. Width/depth 6..32 m, default 16 x 16, shared HSV color, plain/grid/checker tiles, anti-aliased pattern scale/strength, visual roughness, outline and ring radius are real shader/render controls. resetGround is independent of background/clock/jelly/settings. Physics remains a bounded arena: floor footprint, X/Z walls, spawner, grab clamp and debug cage agree; bodies do not fall off an unsupported platform edge. Shrinking translates whole bodies without altering rest geometry/mass/velocity/interpolation, ends grabs/wakes sleep and atomically rejects unfit or potentially worsening contacts. Conservative AABBs may reject some geometrically feasible arrangements; spread/reset bodies rather than forcing compression.

Windows Release passed 29 groups in 12.67 s (12.68 total). Debug full suite passed in 138.87 s (138.91 total); final contact-rejection refinements passed all 3 focused Ground groups in Debug. New tests cover independent ranges/reset, exact non-deforming relocation/velocity preservation, rectangular spawn/contact bounds, rejected stretched-body resize, new/worsening contact rejection and atomic state preservation. The Debug EXE was temporarily locked by an owned concurrent smoke run during incremental relink; it was successfully relinked after the run ended, without terminating user processes.

Both native GL and hardware Vulkan Release/Debug: 350-frame Ground input automation passed all 17 assertions, exercising dimensions/endpoints, physical bounds, style, roughness, pattern, HSV, ring/outline, independent reset/paused poses and closed-menu navigation. Existing 480-frame/21-assertion UI, 600-frame/7-assertion spawn/picking and 600-frame six-body 6 x 32 checker-arena runs passed with no graphics/simulation error. Final refined Release again passed 17 Ground assertions on both APIs. Four API changes with 24 x 10 ground and four random/preset bodies preserved all tracked state, zero fallback/errors. Customized 24 x 10 checker capture with four bodies passed 600 frames and reached supported sleep; volume ratios 0.999897..0.999966, minimum tetrahedral ratio 0.995207.

The delivered standalone EXE additionally passed both APIs' 350-frame/17-assertion Ground checks outside the source directory. Paired 1.7/1.8 three-run median frame wall rose by 0.02730 ms GL / 0.03758 ms Vulkan in settling Duet (performance.md). These are explicit measured costs, not claims of a Vulkan speedup or universal FPS.

Ground automation logs show exactly two mesh uploads (unit plane + original body), one composition-target generation and no additional framebuffer generation during control changes. Width/depth use a model matrix, patterns use derivatives/math rather than generated textures, and appearance/shadow/backdrop cache invalidation is explicit. The fixed 2048 shadow map changes projection to cover the footprint; larger footprints naturally reduce shadow texels per world unit. No physics precision/iterations or material/surface quality were lowered. Paired performance evidence is recorded in performance.md.

All 14 C++ modules cross-compiled for Linux x64/arm64 and macOS Intel/Apple Silicon; Linux headless test ELFs linked, not executed. Full non-Windows SDK/graphics runs and NVIDIA/Intel hardware remain unverified; no macOS Vulkan implementation is added. Current Windows EXE: 35,019,342 bytes; SHA256 `ED30191E9D1123366DB3FBEAC2AD799AB339EFD332104D106BAC6FF4518AB7B9`. Earlier sections retain historical evidence, not the current binary.

## Version 1.7 — historical additive random spawn

Spawn jelly/B adds a real body without replacing existing bodies, pause/camera/settings/grab/step count. Four bounded rest-shape families (rounded box, ellipsoid, capsule-like, pillow) vary size, proportions, rounding and yaw; nodal masses/rest edges/positive tetrahedral rest volumes derive from their actual geometry. Each spawned body owns immutable random bright RGB generated from HSV; global optical controls still apply. A separate seeded-test/fresh-normal RNG is owned by PhysicsWorld. Reset clears additions, not the RNG sequence. Clear-column placement uses precomputed current body bounds and particle-radius margins. Limit 16 gives feedback, not silent removal. Header button/counter stays available with menu closed, and its full click rectangle blocks scene input.

Windows GCC 16.2 Release passed all **26 groups** in 12.75 s (12.76 total). Debug full suite passed in 139.13 s (139.14 total); subsequent final color-variation assertions passed all 3 focused spawn groups in Debug. Tests cover every family, 24 deterministic seeds at resolutions 5/7, positive volumes/masses and smooth normals, selected 4-second drop simulations, distinct colors, exact existing positions/velocities/settings/step/grab preservation, limit/no replacement, reset and wake/fall after sleep. This is bounded XPBD/contact validation, not exact real-material calibration.

Both GL and hardware Vulkan: strict 300-frame actual-button spawn automation passed 7 assertions (3 additions, paused old-body preservation, own color/finite state and 3 surface-picking checks); it includes menu collapse before the third click and resumes physics afterward. Release 600-frame/8-body tests passed, with volume ratios 0.999591..0.999967, minimum tetrahedral ratios at least 0.549155, finite bodies and no GL/API error. Existing 480-frame/21-assertion UI plus 300-frame scene/camera/pick/resize tests passed in both APIs. Debug spawn UI passed 7 assertions in both APIs. Seeded seven-body paused stress passed 4 API changes/4 exact state checks, zero fallback/errors; immutable shapes/colors remain world-owned across recreation.

Logs of the 3-addition spawn run show exactly 5 GPU mesh uploads: floor + 1 original body + 3 new bodies. No old mesh was recreated on append. Topology/buffers stay reusable; only on spawn are candidate geometry/constraints, bounds and extra capacity allocated. The 16-body limit bounds memory/collision/transparency growth but does not guarantee constant FPS, particularly at Detailed resolution. Standard sleep/cache/precision/iterations/optical quality remain unchanged. No per-frame random generation or scene reset is used to simulate spawning.

Project C++ units were rechecked on Linux x64/arm64 and macOS Intel/Apple Silicon with verified Zig/Clang; Linux headless binaries linked, not executed. Foreign SDK/graphics runs and NVIDIA/Intel hardware remain unverified. Source-free Windows Vulkan seven-body launch/capture passed 600 frames, no graphics error, finite bodies and eventual supported sleep. Screenshot shows the always-visible counter and independent colors/shapes. After final hit-rectangle refinement, the delivered source-free EXE again passed both APIs' 7-assertion spawning/picking tests. Current EXE: 35,000,217 bytes; SHA256 `26ECD8CBE441E49F0CDEDCB2EBE1553321D2E26EA93716EF36F87B8E1AED0B53`. Historical measurements below are not the current EXE.

## Version 1.6 — historical cross-platform update

Added platform/capability policy without AMD/NVIDIA/Intel filtering, GL/ES version checks and software-Vulkan rejection. Linux uses pinned x64/arm64 ANGLE assets, absolute EGL/GLES paths, real VkDevice queries, correct X11 Window values and isolated POSIX probing; settings use XDG/macOS paths and atomic writes. Mac gets native GL/universal .app configuration, not pretend Vulkan/Metal. See compatibility.md for commands and outstanding limitations.

Windows GCC 16.2: final Release passed 23 groups in 8.94 s (8.96 total). Debug passed 23 groups in 105.59 s (105.62 total, alongside cross-compilation; not a performance comparison). OpenGL-only build without embedded Vulkan assets passed 23 groups plus strict 300-frame scene/480-frame 21-assertion UI tests, 16.81 s total. Its unavailable Vulkan startup restored GL, exit 0/one reported fallback/no graphics errors; strict unavailable startup exited 1 with a nonmodal explanation.

Main Release, both APIs: 300-frame scene/camera/pick/resize and 480-frame/21-assertion UI checks passed; RX 7600 XT hardware Vulkan confirmed, no graphics errors. 780-frame switching passed 12 changes/12 exact state checks, zero fallback/errors. One first UI attempt exited early at frame 254 while another graphics test process was running, with no GL error; the serial repeat completed all checks. Desktop input/focus must not be shared by simultaneous graphics tests; the early attempt is not counted as success. Final source-free runs from a directory initially containing only EXE passed strict GL/Vulkan 300 frames, 5 camera/1 pick checks, with automatic cache extraction.

Debug also completed both APIs' 300-frame scene/480-frame 21-assertion UI checks and 12-change/12-preserved-state stress with zero errors/fallbacks. Both Linux runtime CMake hash/staging/install profiles were exercised in a Windows dummy-target fixture; deleting/moving one generated staged SO and rebuilding restored it despite no executable relink. This validates portable packaging logic only, not Linux binary execution/framework linkage.

Verified Zig 0.15.2/Clang cross-compiled all 14 project C++ units for Linux x64/arm64 and macOS Intel/Apple Silicon; Linux headless test ELFs also linked. Final check had no project warnings. A missing macOS signal.h declaration was fixed. Complete non-Windows raylib/GLFW/framework builds, foreign test execution and GPU runs were **not** performed; CI is prepared, not dispatched. NVIDIA/Intel hardware, Linux/macOS runtime, Khronos Vulkan validation and prolonged manual high-DPI/fullscreen/multi-GPU remain unverified.

Current EXE: 34,981,167 bytes; SHA256 `60F6EAD8A2B35776747DAAA2182EA49988C91BF4633A85F242D63DD9D382DB25`. Six alternating 600-frame/60-warmup duet means: GL 1.79827/1.80596/1.82702 ms; Vulkan 2.33829/2.30645/2.29982 ms. Medians: 1.80596 versus 2.30645 ms. These are CPU/frame wall measurements on this AMD/Windows host, not isolated GPU or cross-vendor claims. No material hot-path regression versus 1.5; solver/visual quality unchanged. OpenGL stays default.

## Version 1.5 — historical hardware Vulkan/OpenGL update

Graphics settings now select native OpenGL or hardware Vulkan (ANGLE), report actual driver/API and verify native Vulkan device/physical-device handles. All current scene/shadow/transmission/blur/UI passes execute in that backend; no OpenGL frame is merely presented through Vulkan. Shader bodies remain intact; compatibility adapts shader version/precision, clear-depth and component swizzle/sized depth formats. MSAA x4 requested in both, same solver/topology/quality. Unsupported activation restores the prior working API without replacing simulation/camera/settings. Normal UI choice persists; smoke leaves preference untouched.

An actual AMD driver crash at first presentation was debugged and avoided by disabling the optional maintenance path within this process, using standard synchronization. An isolated, hidden, two-frame strict driver probe additionally contains startup crashes/timeouts before main activation. No registry/driver/global environment change or implicit-layer disable. Screenshots capture before swap; all FBO creation checks completeness; old window input and dead context state are cleared. Fullscreen switching is explicitly rejected with an F11 instruction.

GCC 16.2 / Windows x64 builds: Release twenty-two groups passed in 8.85 s (8.86 total); Debug in 89.60 s (89.62 total). Separate JELY_BUILD_APP=OFF build passed the same twenty-two groups in 8.88 s (8.89 total), validating the graphics-independent boundary. Project code builds without emitted warnings. Driver: AMD RX 7600 XT / 26.8.1 (2.0.395); Vulkan 1.4.349 via pinned ANGLE 2.1.27037 / 1d3190bf5633.

Release/Debug, both APIs: 480-frame UI input automation passed all 21 assertions; 300-frame scene/action/resize checks passed five camera checks, one pick, two finite bodies/unit volume and no obstacles/error. graphics_errors=0. Strict Vulkan reports vulkan_device_confirmed=1. Opacity 0/65/100 two-body side-view captures differ from OpenGL in the 3D crop by at most one channel level, zero pixels differing >2. Both render paths were visually inspected, including glass and all controls.

Release 1500-frame stress: 24 actual button-driven API switches, 24 exact pose/velocity/settings/appearance/step/camera preservation checks, zero fallback/error; 150 FBO allocations and releases. Debug four-switch paused test also passed. These are bounded lifecycle checks, not formal leak profiling. Simulated unavailability correctly leaves active OpenGL with a reason; strict mode exits 1 without a blocking dialog. Source-free launches (directory initially containing only EXE) passed strict Vulkan/OpenGL 300-frame camera/scene/resize tests and generated the checked cache automatically. Embedded font/graphics notices output verified.

Pinned official graphics DLLs/notices are embedded; startup performs SHA256/absolute loading. Profile data or an executable-side fallback stores only generated cache/preference. No browser/SDK/network/manual assets required at launch, but real GPU driver support and a writable cache location remain needed for Vulkan. Current EXE 34,979,987 bytes; SHA256 `A2FE6E3E93574593B6DC2288A35B1760C99D3E80B1C00D018D0542B5D66CB154`. Static imports: bcrypt, GDI32, KERNEL32, msvcrt, ole32, SHELL32, USER32, WINMM. Embedded EGL/GLES/Vulkan libraries load dynamically; no extra user-distributed DLL package.

Three-pair benchmark medians (600 frames, 60 warm-up excluded): settling Duet OpenGL 1.79887 / Vulkan 2.33540 ms; no-sleep Duet 2.95547 / 3.61374; paused 0.479307 / 0.821648. Vulkan is slower here; OpenGL stays default. Caching/blur/CPU optimizations retained; no quality cuts. graphics.md records timing methodology, not isolated GPU execution or universal FPS.

Khronos Vulkan validation layers are not installed and were not run. Manual hardware input, prolonged sessions, multi-monitor/high-DPI, other GPU vendors and future device-loss scenarios remain unverified. Safe startup/fallback and covered zero-error tests do not mean every driver is bug-free. Original real-material calibration remains separate and pending.

## Version 1.4 — historical Liquid Glass-inspired UI

Original GPU-blurred/refraction/rim-lit rounded UI, animated pills/tab indicator/content, reversible menu slide/fade and persistent Controls launcher. Top-left button and Tab share the visibility state. Reduced-motion and opaque UI modes are independent of jelly appearance/physics; OS client-area animation preference is read at startup. Inter/OFL and both new shaders are embedded. UI input uses animated geometry and active-slider capture; a collapsed menu releases the formerly covered scene area.

Final Release: twenty-one headless groups, zero failures, 8.87 s (8.88 total). Debug: twenty-one groups, zero failures, 87.20 s (87.22 total). Final UI-only exact-softness preservation/stable responsive control IDs and smoke GL checks were rebuilt afterward in Debug. Numerical solver is unchanged; opening the Physics tab no longer writes a log/pow-rounded compliance without actual slider input.

Final 480-frame UI pointer-path runs in Release, Debug and a directory containing only the EXE each returned exit 0, empty error and ui_checks=21. Coverage: all tabs, 0/65/100 transparency, collapse/reopen, reversal, launcher capture, hidden-panel input release, Detailed/Balanced reset, exact softness preservation, reduced-motion instant toggle, opaque/glass modes and resize. Each recorded glass_generations=4, menu_open=1; FBO allocations/releases 19/19. Logs contain no shader/font warnings or OpenGL errors. This is bounded lifecycle evidence, not a formal leak audit.

Separate 300-frame physics/camera/action smoke in both configurations returned zero, camera checks 5/5, picking 1/1, two finite bodies, unit volume and no obstacles/error. Final paused 0/100 endpoint checks found no GL errors. Expanded/collapsed, all tabs, solid UI, night/day/sunset and endpoint captures were inspected. The unaffected noon/120-frame 3D crop matches 1.3 pixel-for-pixel. Twenty-one synthetic UI assertions use the same input/control path as humans, but manual hardware-input and prolonged use remain unperformed.

Quarter-resolution shared GPU blur: paused 600-frame runs update once; no per-frame CPU framebuffer readback. Three alternating pairs show added median frame cost 0.192 ms settling Duet, 0.204 ms continuous no-sleep Duet, 0.127 ms paused. interface.md records methodology, performance/GPU timing limits and accessibility controls; old optimization gains below describe 1.3, not new UI absolute costs.

Current Code/Release/3D_Jely.exe: 5,274,490 bytes; SHA256 `A9C74C406FD2788BE8170314A9C37B8B80467A7701A162798FE246C0E14AB04B`. Imports remain GDI32, KERNEL32, msvcrt, SHELL32, USER32, WINMM only. No external runtime assets/new library. Embedded font license output checked through --licenses. Source/font licenses retained. Apple's native framework/visual parity, multi-monitor/high-DPI and non-Windows/Visual Studio builds are not claimed verified. Real material calibration remains a separate pending physics task.

## Version 1.3 — historical optimization update

Release full suite: nineteen groups, zero failures, 8.90 seconds (8.92 total). Debug full suite: nineteen groups, zero failures, 89.91 seconds (89.94 total). Final graphics-only glyph/shape batching and body-order cache guard were rebuilt afterward; the unchanged numerical core remains covered by the full Debug run. Both final configurations passed 300-frame graphics/action/resize smoke runs. Project code built without emitted warnings.

New regression groups: separated broadphase bodies still collide when approaching; cached surface binding rebuild and affine deformation; sleep/wake under force, grab, settings and direct scene/body edits; unsupported low-gravity bodies never freeze. Continuous 20-second rest explicitly disables sleep and retains volume ratio 0.999966 / maximum velocity 3.42855e-6 m/s.

Final Release/Debug automated reports: frames=300, physics_steps=280 after scene reset, camera_checks=5, picking_checks=1, obstacles=0, empty error, two finite bodies, volume=1 / 1, render_target_generations=3 after two resizes. The source-free Release run repeated these checks successfully from a directory containing only 3D_Jely.exe. It requires no source, external font/shader asset or runtime library package.

Deterministic noon 120-frame one/two-body 3D crops match the pre-optimization executable pixel-for-pixel. UI layout/text were inspected; fixed controls have only 22 pixels differing by more than two levels (maximum five). Dynamic telemetry is excluded. Paused cache checks at 120/600 frames, opacity endpoint transitions, lighting/material changes, grabbing, debug/reset and resize cover invalidation/presentation. Manual physical mouse/keyboard and long-duration testing remain unperformed. See performance.md for the three-repeat paired benchmarks, active/no-sleep separation and CPU/GPU timing limitations.

Current artifact: Code/Release/3D_Jely.exe, 4,637,197 bytes; SHA256 `0758A0D645375DC64F2E6B6BEBA699E4AA56901278171723B47F3B0153B092FE`. PE imports rechecked: GDI32, KERNEL32, msvcrt, SHELL32, USER32, WINMM only. No new dependency. Opaque/invisible fast paths avoid composition targets; midrange optical composition remains single-sample. Source/licenses preserved. Each final Release/Debug/portable automated run logged ten FBO allocations and ten releases. This is lifecycle evidence, not formal leak profiling.

## Version 1.2 — historical transparency/material update

Adjustable transparency (0% opaque, 65% default, 100% invisible), thickness-driven screen refraction and tint, Gloss, HSV material colors and presets, independent scene/material resets, and a 120-second automatic day cycle. The light orbit is continuous at sunrise/sunset. Jelly controls open in their own tab. Invisible bodies remain simulated and pickable; their mesh and shadow passes are skipped.

Renderer now captures an opaque scene, measures per-body exit eye-depth with front-face culling, and composites sorted bodies through separate color/depth buffers. It samples a source buffer while writing a different target; color/depth blits preserve occlusion. Resource ownership and transactional resize are encapsulated by SceneTargets. No new dependency or external runtime resource.

Final Release: fifteen headless groups passed in 10.37 seconds. Full Debug suite passed in 131.01 seconds. After small independent-reset and horizon-orbit refinements, the affected material/appearance groups were run again in Debug and passed. Final Release ran the complete updated suite again. No-match test filters return failure. The physical core remains unchanged.

Graphics checks: 120-frame runs at 0%, 65%, 100%, night and sunset; two-body runs from default and opposing side views; gloss/refraction/tint disabled individually; all returned exit 0 and exported screenshots. The platform is visible through the default jelly, 100% removes the body/shadow, and near/far order reverses with the side views. Pixel comparison in the same body region against default 65% changed 42,217 pixels for opaque/invisible endpoints, 6,423 for refraction disabled, 5,197 for matte gloss and 32,660 for tint disabled (difference >2 intensity levels). Those comparisons use identical physical state and noon lighting, excluding UI telemetry.

Auto cycle check advanced 12.0 to 12.2 hours over the deterministic 120-frame preview. Material reset preserved environment values; scene reset preserved material values. Horizon direction continuity and day-cycle wrap are covered by headless assertions.

Final source-free 300-frame Release run: exit 0, five camera checks, one pick check, two finite bodies, volume 1.0 / 1.0, no obstacles/error, transparency 65, refraction 0.65, gloss 0.8. Two automatic resizes yielded target generation 3. Logs recorded ten FBO allocations and ten releases; this is lifecycle evidence, not a formal allocator/leak audit. Final Debug graphics/action/resize checks also completed without error. Manual mouse/keyboard use remains unverified.

Current artifact: Code/Release/3D_Jely.exe, 4,615,862 bytes; SHA256 `2562F08ED4E0A687C5930ADC5883CB9DF4A4C83B4FA1D87913B1D04282472E87`. Static runtimes and embedded font/three shaders retained. Only the same six Windows system DLLs are imported. Compositing textures are single-sample. Optical transport is approximate: no ray tracing, colored volumetric shadows or exact ordering of deeply interleaved surfaces. Settings remain session-only.

## Version 1.1 — historical appearance update

Sphere and block removed from all playable presets, including collision. High drop replaces Obstacle; analytical collision support is retained for explicit headless test fixtures. New Appearance tab controls time of day, contrast, independent HSV background/platform palettes, grid and ring. Sun/moon direction, temperature/intensity, ambient light and sky brightness respond to the clock. Packed shadow depth with disabled blending fixes precision; linear color conversion and filmic mapping increase contrast.

Release and Debug builds and all fourteen regression groups passed: 10.47 seconds total Release CTest time and 131.03 seconds Debug time. New checks validate empty presets/high drop, HSV sector transitions and independent colors, day/night light differences and invalid settings. The baseline results below are historical.

Three 120-frame graphics runs (12:00/Ocean, 00:00/Ocean, 17:36/Sand) returned exit 0, exported screenshots and reported zero obstacles and no simulation error. At the identical simulated instant, all three bodies had volume ratio 0.999972, minimum tetrahedron ratio 0.998046 and maximum speed 0.203002 m/s: changing appearance did not alter physical state. Screenshots were visually inspected after fixing HSV blue interpolation and disabling alpha blending in the packed-depth pass. Day and sunset casts have distinct directions/lengths; night retains a readable lit jelly on a dark platform.

A 120-frame Slate run with contrast 1.5 and both Grid/Ring disabled also returned exit 0. The 300-frame automated run exercised time presets/palettes, returned exit 0 and reported five camera checks, one surface ray-pick, no obstacles, two finite bodies and volume ratios 1.0 / 1.0. No physical mouse/keyboard manual test was run.

Historical 1.1 Release size: 4,597,121 bytes. SHA256: `D2EBCCF63D1FB0BD8651E52262DBE1499EBECD99A190FBE7CA6E66A9EF887D89`. A final source-free-directory 300-frame portable launch returned exit 0, five camera checks, one pick check, no obstacles, no error and two finite bodies with volume ratios 1.0 / 1.0. The working directory contained only 3D_Jely.exe before launch. A final 60-frame Debug graphics/action run returned exit 0 with the same camera/picking checks, a finite body and volume ratio 1.0. PE imports were checked again and remain limited to the same six system DLLs. No external dependency or runtime asset was added.

## Historical version 1.0 baseline

The following measurements and hash describe the original build, not the current 1.1 executable. They are retained as regression history.

## Verified environment

Windows x64; w64devkit 2.10.0, GCC 16.2.0; CMake; statically built raylib 5.5 with GLFW/OpenGL 3.3. Graphics runtime reports ATI Technologies / AMD Radeon RX 7600 XT, OpenGL 3.3 Core Profile, GLSL 4.60. Window/render size 1440 x 900; display 1920 x 1080.

## Build and numerical tests

The provided build script was run for Release and Debug. Both compile and link successfully. Final project-owned sources build with -Wall -Wextra -Wpedantic without emitted warnings. Third-party headers are system includes; audio is disabled and duplicate raylib buffer-size config definitions are normalized. Upstream CMake deprecation messages are disabled by the script. Vendored source is unmodified.

```powershell
.\build_release.ps1
.\build_release.ps1 -Configuration Debug
```

Release CTest: 1 registered suite, twelve internal groups, zero failures; 10.02 seconds total. Debug CTest: zero failures; 121.61 seconds total. Final graphics-only refinements were rebuilt after that run; numerical source did not change.

| Group | Result |
|---|---|
| Consistent tetra topology, positive rest volumes and total lumped mass | PASS |
| Rigid translation / zero gravity rest | PASS |
| Free fall displacement and undeformed volume | PASS |
| 20-second drop, floor clearance and stable resting speed | PASS |
| Strong impulse, walls and elastic recovery | PASS |
| Sphere and AABB obstacle penetration checks | PASS |
| Compliant stretch, drag, release and resting recovery | PASS |
| Two-body contact and volume | PASS |
| Soft and higher-resolution settings | PASS |
| Deterministic state replay | PASS |
| Welded manifold render topology, orientation, normals and interpolation | PASS |
| Invalid topology, settings and grab indices | PASS |

Rest test: volume ratio 0.999966; maximum node velocity 3.42855e-6 m/s. Balanced body: 125 nodes, 384 tetrahedra. Surface: 3,458 welded vertices, 6,912 triangles. Detailed body: 343 nodes, 1,296 tetrahedra.

## Graphics and interaction logic

Final Release smoke: 300 frames, exit code 0, screenshot export successful, empty error field. Scene resets mean physics_steps=280 refers only to the final Duet scene, not all preceding simulation steps. The run exercises launch, compliant grab/release, pause, single stepping, resume, all scene presets, all material palettes and debug toggles directly through application actions. Five camera checks exercise the same CameraController::apply used by real input: orbit, pan, zoom, target movement, reset. A projected center ray successfully intersects the uploaded smooth surface.

```text
frames=300
physics_steps=280
solver_ms=1.37153
bodies=2
screenshot=1
camera_checks=5
picking_checks=1
error=
finite=1 volume=0.999984 min_tet=0.998022 max_speed=0.964594
finite=1 volume=1 min_tet=1 max_speed=2.27495
```

Final Debug graphics/action smoke: 60 frames, exit code 0; 5 camera checks, 1 picking check; body finite and volume ratio 1.0. No startup or shader error. Screenshots were visually inspected for materials, floor, obstacles, lighting, shadows, control layout and telemetry. Runtime logs show resource destruction before window shutdown.

Camera/action automation is synthetic. Physical mouse events, keyboard shortcuts, slider hit-testing, actual throw gesture and sustained interactive use have not been manually exercised. Visual Studio and other platforms are supported by configuration but were not built here. No sanitizer or formal memory-leak profiler was run. FPS/solver measurements describe this run, not a guaranteed performance target.

## Portable executable

Release is built with static raylib and C++/GCC runtimes. Fonts and GLSL shaders are embedded. PE import inspection reports only GDI32.dll, KERNEL32.dll, msvcrt.dll, SHELL32.dll, USER32.dll, WINMM.dll. OpenGL still requires the Windows graphics driver.

A copy was launched with a source-free working directory. The executable does not load any external assets. Final release artifact: Code/Release/3D_Jely.exe, 4,585,760 bytes; SHA256 `96BB2A3DCFC248AF938601AC4348F0C6F48B1FE74BE973DADE635732E1A6C3B0`. It matches the canonical Code/build/local-release/Release build. No source was removed or replaced by the artifact.

## Resolved findings

- raylib TakeScreenshot ignores directory components; use explicit image export and check its return status.
- Render interpolation now always follows the accumulator while unpaused, including render frames with zero physics ticks.
- Debug output remains under build; the build script installs Release only.
- Softness is displayed as a useful percentage instead of logarithmic compliance.
- Third-party warning noise removed by targeted dependency configuration/system includes; project warnings remain enabled.

## Known scope limits

Particle-based contact, no self collision or CCD, no arbitrary triangle obstacle mesh. No true optical refraction/transparency, SSAO or inversion-proof FEM. The supplied video's actual content was unavailable; no claim of video-specific parity is made. plan.md records the future roadmap.
