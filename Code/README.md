# 3D Jely Physiks

C++20 / XPBD soft-body laboratory, version 1.9. Windows/Linux/macOS platform paths; validation limits are explicit. The spelling **Jely Physiks** is intentional.

Source project: this `Code` directory. The repository root also contains the MIT license, release overview and dependency notices.
Project documentation: `..\Docs\plan.md` and `..\Docs\structure.md`.

## Launch

Windows currently has the built and tested EXE. Linux x64/arm64 has native OpenGL and a packaged ANGLE/Vulkan implementation; macOS Intel/Apple Silicon has a native OpenGL `.app`/universal build profile. These are not claims that Linux/macOS GPU runs or every AMD/NVIDIA/Intel driver have been tested. See `../Docs/compatibility.md` for exact commands, dependencies, runtime layouts, CI and outstanding hardware checks. Vulkan is unavailable on macOS in this ANGLE renderer; Metal is not mislabeled as Vulkan. No ready-tested Linux/Mac app binary is supplied here.

Open `Release\3D_Jely.exe`. This executable embeds shaders, fonts, optional ANGLE/Vulkan runtime and upstream notices; raylib/C++ runtime are static. No source, browser installation or separately downloaded DLL package is needed. Windows 10/11 x64 and a graphics driver are required. OpenGL needs 3.3; Vulkan mode additionally requires compatible hardware/driver. The bundled loader does not replace the GPU driver. Runtime DLLs are verified and extracted automatically to a private cache (see Graphics APIs below).

## Controls

### Softness and monitor frequency

Softness is a continuous 0..100% material control. **0% is a truly rigid shape** with mass-weighted quaternion fitting and rigid velocity projection; it moves/rotates without elastic deformation. Positive values use XPBD stretch/volume constraints, not vertex animation. The former 50% default compliance is retained; the expanded upper range (roughly 80..100%) produces much softer, noticeably compressing and stretching jelly. At maximum, a reference high drop compresses the default Balanced body's height by approximately 42%, then springs back. This is a bounded numerical check, not measured material calibration. Changing stiffness or grabbing wakes sleep.

Grabs distribute a normalized XPBD attachment over a bounded nearby surface patch, transmitting force into the lattice instead of pulling a lone vertex. The cursor goal is bounded to the arena; the physical handle advances at up to 12 m/s in fixed substeps, independent of mouse/render event rate. Local volume barriers plus bounded substep backtracking protect extreme pulls from tetrahedral inversion. Backtracking is dissipative and may slow an impossible/crushed manipulation; it is not self-collision or continuous surface collision. Stone contact remains the particle-based contact model with a final shape-preserving floor/wall correction.

Rendering/VSync follows the OS-reported refresh rate of the monitor containing the window, queried every 0.5 seconds and again after graphics context recreation. This machine reports **280 Hz**. Telemetry distinguishes **Monitor Hz**, measured **FPS** and **Physics 120 Hz**. Keeping the solver fixed avoids changing softness when moving to a different monitor. An unavailable/invalid refresh rate removes the software FPS cap while preserving VSync; benchmark mode remains uncapped. Integer configured refresh is not a real-time VRR measurement or a guarantee that a loaded GPU can deliver that many frames. Moving between physical monitors/change of refresh configuration remains a manual hardware check.

CLI: `--softness 0..100`; `--softness-automate --smoke 350` drives the actual slider's 0/50/100 endpoints, patch/whole-body drag, soft drop and telemetry checks. Reports include `softness_checks`, `softness_drag_peak`, `monitor_hz`, `render_fps_limit`, `physics_hz` and compliance.

### Ground customization

Click **Ground** in the top bar; it opens its settings even when the Controls menu is closed. Width and Depth are independent **6..32 m**, default **16 x 16 m**, matching the rectangular physical arena and its spawn/drag bounds. Plain/Grid/Tiles choose a plain surface, procedural anti-aliased grid or checker tiles. Pattern scale (0.25..4 m), strength, HSV color, visual roughness, outline, ring toggle and ring radius (0.5..10 m) are adjustable. A larger ring may be clipped by a small platform. Roughness changes highlight sharpness/intensity, not physical friction; the Physics friction slider remains separate.

**Reset ground** resets only ground appearance/size, preserving background, clock, jelly material, body colors, camera and physical settings. Reset scene/R preserves the chosen ground dimensions. The original Appearance color/grid/ring controls edit the same shared values. Ground settings are session-only like other appearance settings.

The floor remains the simulation's bounded rectangular arena, not a ledge that bodies can fall off. On shrinking it, an out-of-bounds body is translated as a whole, preserving rest geometry, mass, velocities and interpolation; changing size ends an active grab and wakes sleep. If a body cannot fit, or relative translation could create/worsen contact, resizing is rejected atomically with a visible message and the prior dimensions remain. Conservative bounding boxes may reject some feasible arrangements; spread/reset bodies first. It does not squeeze the jelly, remove bodies or reset the simulation. The visible plane, wall/contact bounds, spawn placement, drag clamp and debug cage use the same width/depth. Shadows adapt their projection; floor/target GPU meshes are not rebuilt while dragging sliders.

CLI initialization supports `--ground-width 24 --ground-depth 10 --ground-style tiles --ground-scale 0.8 --ground-strength 0.7 --ground-roughness 0.25 --ground-ring-radius 3 --no-outline`, plus existing `--no-ring`/theme settings. `--ui-tab 4` opens Ground directly; `--ground-automate --smoke 350` runs 17 real UI-path assertions. These commands do not save preferences or imply foreign-platform GPU validation.

**Spawn jelly** is always visible at the top center, even with the Controls menu closed; **B** does the same. Each press adds a new physical body without replacing/resetting existing bodies, settings, camera, pause or an active grab. New bodies have randomly generated rounded-box, ellipsoid, capsule-like or pillow rest geometry, varying proportions/size/yaw, and their own random bright HSV-derived color. Edge/tetrahedral rest constraints and nodal masses are constructed from that actual geometry, not a visual scaling trick. Spawned colors remain independent of the Jelly tab's base color/presets; global transparency/refraction/gloss/tint still affect every body.

The visible counter includes the existing preset bodies. Maximum is **16 simultaneous jellies**, with a clear limit message and no replacement; Reset/R or selecting a preset clears the additions. This bounds physics/transparency resource growth, not a promise of constant FPS at maximum Detailed quality. Spawn wakes sleeping simulation; while paused it adds the body but does not advance physics. Spawn selects a clear column above existing particle bounds, with contact-radius clearance, avoiding explosive initial overlap. Existing GPU mesh owners/buffers are kept when adding bodies; only the new mesh is created. All four families are simply connected lattice shapes, not arbitrary concave/topology-changing meshes. Existing particle-contact/optical approximations still apply.

Repeatable verification options: `--spawn 0..14` adds initial bodies, `--spawn-seed N` seeds this spawner only, and `--spawn-automate --smoke 300` drives the actual button three times (paused, then with menu closed), checks existing poses/colors/picking, then resumes falling. Normal launches use a fresh random seed; scene resets do not restart the random sequence. Reports include `spawned`, `spawn_checks`, per-body shape/color and `spawn_message`.

| Control | Action |
|---|---|
| Left mouse on jelly | Grab, stretch, drag; release to throw |
| Right mouse | Orbit camera |
| Middle mouse | Pan camera |
| Mouse wheel | Zoom |
| W/A/S/D | Move camera target |
| Space | Pause/resume |
| N while paused | One fixed physics step |
| R | Reset current scene |
| J | Launch impulse |
| Spawn jelly / B | Add a random physical jelly with its own color (up to 16 bodies) |
| C | Reset camera |
| F1 | Physics debug overlay |
| Menu button (top left) / Tab | Animate collapse/reopen; preserve all current settings |
| F11 | Toggle borderless fullscreen |
| Esc | Exit |

The Physics tab provides Drop/High drop/Duet presets, live softness/gravity/damping/friction and Balanced/Detailed meshes. The block and sphere have been removed from every playable preset, including their colliders. Quality changes reset topology. All physical settings persist across scene reset. Simulation exceptions stop physics and display a reset prompt.

The Jelly tab opens by default. Transparency ranges from 0% opaque to 100% invisible, with a 65% default. It changes the actual visibility of the floor and other bodies through the jelly; shadows fade with it. Invisible bodies remain simulated and pickable. Refraction controls the distortion of the visible scene, Gloss controls highlights, and Tint strength controls thickness-dependent color filtering. Hue/Saturation/Brightness offer arbitrary material colors, with Mint/Berry/Honey presets. Duet uses a complementary hue for the second body. Reset material restores only the jelly settings.

The Appearance tab provides a full 24-hour clock with Night/Dawn/Day/Sunset buttons. It changes sun/moon direction, key-light color/intensity, ambient illumination, background brightness and the projected shadows. Auto day advances one full cycle in 120 seconds while the simulation is unpaused. Contrast is adjustable from 0.8x to 2.0x, with a stronger 1.25x default and filmic tone mapping.

Background and platform each have independent Hue, Saturation and Brightness sliders, with live color swatches. Ocean/Sand/Slate buttons apply matching palettes. Grid and Ring buttons independently toggle the platform decorations. Reset scene look restores environment defaults while preserving the jelly material and physics. Appearance changes survive physics scene resets; they are session settings and are not saved between launches.

## Graphics APIs

Open the **Graphics** tab and click **OpenGL** or **Vulkan**. The active API and actual driver are shown there and in telemetry. Switching recreates the graphics context/resources in the same process, preserving simulation nodes/velocities, physics/material values, pause state and camera. The first Vulkan activation may briefly pause while checking the driver/compiling pipelines. Leave borderless fullscreen with F11 before switching; this restriction avoids unsafe window-mode recreation.

OpenGL is the native WGL/GLX/NSGL desktop OpenGL path. **Vulkan (ANGLE)** is real hardware Vulkan rendering through ANGLE's GLES-compatible frontend on Windows/Linux: scene, shadow, exit-depth, refraction, blur and UI draws execute on Vulkan, not an OpenGL frame copied to a Vulkan presenter. It is not a handwritten Vulkan renderer. Activation verifies context version, driver/client API and native VkDevice/VkPhysicalDevice handles; software SwiftShader/llvmpipe/lavapipe/softpipe is rejected.

The selected API is saved when changed through normal UI use. If Vulkan initialization fails, the previous working API (OpenGL at startup) is restored with a visible explanation. An isolated hidden child-process probe prevents a driver startup crash/hang from taking down the main app. A known AMD optional maintenance-presentation crash is avoided by using standard swapchain synchronization, keeping MSAA/visual quality. Only this process's ANGLE feature overrides are changed/restored; system drivers, registry and other apps are untouched.

The three pinned runtime DLLs and full notices are embedded. A Vulkan launch verifies embedded/cached SHA256, extracts into `%LOCALAPPDATA%\3D_Jely\runtime` and uses absolute safe library loading. If the profile is not writable, an executable-side `3D_JelyData` cache is used; if neither location is writable, OpenGL remains available. Graphics preference is in `graphics.txt` under the same application data root. Cache files are generated runtime data, not required distribution sidecars. No network is used at launch.

```text
3D_Jely.exe --backend opengl
3D_Jely.exe --backend vulkan
3D_Jely.exe --backend vulkan --strict-backend --smoke 300 --automate --report C:\temp\vulkan.txt
3D_Jely.exe --backend opengl --strict-backend --smoke 1500 --switch-cycles 12 --report C:\temp\switches.txt
```

`--backend` overrides saved preference for that launch. Smoke defaults to OpenGL unless explicit; smoke does not save user preference. `--strict-backend` fails instead of falling back, useful for verification. `--switch-cycles N` drives the actual Graphics buttons and requires at least 120*N+20 smoke frames (1..20 cycles); reports verify preserved state. `--simulate-vulkan-failure` tests the fallback path. `graphics_api`, `graphics_driver`, `vulkan_device_confirmed`, `graphics_errors`, `api_switches` and `api_state_checks` provide evidence. Internal driver-probe flags are not user settings.

See `..\Docs\graphics.md` for implementation, checks, limits and API benchmarks. ANGLE assets are checked at CMake configuration; if missing, `bootstrap_angle.ps1` restores the pinned, official, hash-verified assets. No browser/Electron framework is installed or used.

## Liquid Glass-inspired interface

Original Windows/OpenGL implementation inspired by Apple's material language, not an Apple framework or exact native replica. Floating rounded panels use a GPU-blurred scene backdrop, subtle edge refraction, highlights/shadows and an embedded Inter font. Pills respond to hover/press, selection glides between tabs, new content fades in, and the menu slides/fades closed and reopens from an always-visible Controls launcher. UI captures pointer input; the closed panel releases its former scene area.

Motion on/off and Glass on/off buttons below the open menu offer reduced motion and an opaque, easier-to-read UI. Windows client-area animation preference is respected at startup. Tab and the top-left menu button work in either mode; reduced motion switches instantly. Collapsing/changing tabs preserves settings. UI choices are session-only. See `..\Docs\interface.md` for implementation, verification and performance scope.

Additional launch switches: `--menu-hidden`, `--reduced-motion`, `--solid-ui`, `--ui-tab 0/1/2/3/4` (Physics/Appearance/Jelly/Graphics/Ground). `--licenses` prints embedded font and graphics dependency notices and exits. Reports include `ui_checks`, `menu_open`, `glass_generations` and `blur_updates`; a static paused scene needs one blur update, not one per frame.

## Optimization

Supported bodies sleep together after at least 1.5 seconds below 1 mm/s, with finite/volume safeguards. Impulses, grabs, resets and changed physical settings wake them immediately; direct node/scene edits are also detected. An unsupported body under gravity cannot sleep. Rendering reuses unchanged meshes, shadows and transparent scene layers; camera/lighting/material/resize changes invalidate the appropriate cache. Quality, double-precision physics, 120 Hz stepping, substeps and solver iteration counts are not reduced. See `..\Docs\performance.md` for measured before/after costs and their scope.

Performance checks (existing absolute output directory required):

```text
3D_Jely.exe --smoke 600 --benchmark --scene 2 --report C:\temp\jely-benchmark.txt
3D_Jely.exe --smoke 600 --benchmark --scene 2 --no-sleep --report C:\temp\jely-active.txt
3D_Jely.exe --smoke 600 --benchmark --paused --report C:\temp\jely-paused.txt
```

`--benchmark` requires at least 120 smoke frames, disables VSync/frame limiting only for that run, excludes the first 60 frames and reports stage means/p95. Physics timing is per render frame (two fixed ticks in smoke), not per tick. Draw timing measures CPU submission; frame wall time includes presentation/driver waits, not isolated GPU execution. These measurements are not guaranteed interactive FPS. `--no-sleep` keeps solving resting bodies for continuous-load regression; `--paused` starts paused and still permits ordinary controls.

## Rebuild

On this machine, a portable compiler/CMake is available in `tools\w64devkit`. Double-click `build_release.bat`, or run:

```powershell
.\build_release.ps1
.\build_release.ps1 -Configuration Debug
```

The script configures CMake, builds, runs the headless regression suite, then installs the optimised executable to `Release`. Debug stays in `build/local-debug/Debug` and preserves the installed Release. `-SkipTests` is an explicit developer option. The complete source remains in place.

If the portable tools are absent, run `bootstrap_tools.ps1` to download the pinned official w64devkit 2.10.0 archive with SHA256 verification. It stays inside tools; no global installation or PATH changes are made outside the script process. Alternatively install Visual Studio 2022 Desktop development with C++ / CMake, open this folder, select the vs2022 preset, and build the release or debug preset.

Manual Visual Studio workflow:

```powershell
cmake --preset vs2022
cmake --build --preset release
ctest --preset release
cmake --install build/vs2022 --config Release --prefix .
```

Portable physics-only builds require no graphics dependency:

```text
cmake -S . -B build/headless -DJELY_BUILD_APP=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build/headless
ctest --test-dir build/headless --output-on-failure
```

## Validation and limits

`jely_tests` has twenty-nine groups: physics/geometry/appearance/sleep/UI, graphics policy, random spawning and independent/transactional rectangular-ground resize/style/reset. Tests need no graphics context. A substring selects a group, for example `jely_tests.exe "ground"`; no match fails. Hardware checks run separately. Project C++ cross-compilation and platform runs are different evidence; see compatibility.md/validation.md for the exact boundary.

UI input-path regression: `3D_Jely.exe --smoke 480 --ui-automate --screenshot C:\temp\ui.png --report C:\temp\ui.txt`. It feeds deterministic pointer positions/presses through the same Panel input/button/slider path as human input. It checks tabs, transparency endpoints, collapse/reopen/reversal, hidden input release, quality resets, motion/glass modes and resize. This is synthetic testing, not manual hardware-input validation. Non-benchmark smoke also checks OpenGL errors in runtime logs.

Graphics smoke test (use absolute output paths):

```text
3D_Jely.exe --smoke 300 --automate --screenshot C:\temp\jely.png --report C:\temp\jely.txt
```

The output directory must already exist. Smoke stepping uses deterministic fixed increments instead of wall time. Automation exercises actions and presets directly; it does not replace physical mouse/keyboard testing. Normal physics uses a bounded accumulator at 120 Hz, three substeps and eight solver iterations (ten for Detailed); render pacing follows the monitor. It discards excessive backlog to remain responsive under load; simulation can then run slower than real time. Telemetry displays the fixed-step solver duration, not total GPU/frame time.

Optional launch/preview arguments: `--time 0` through `--time 24`, `--theme 0` (Ocean), `--theme 1` (Sand), `--theme 2` (Slate), `--contrast 1.25`, `--no-grid`, `--no-ring`, `--transparency 65` (0..100), `--refraction 0.65`, `--gloss 0.8`, `--tint 0.6` (each 0..1), `--cycle-day`, `--scene 0/1/2` and `--view 0/1/2` (default, +X side, -X side). These initialize the same settings used by the UI. Automated smoke actions override some settings and resize the window twice to exercise framebuffer ownership. Invalid values are rejected before window creation.

Environment collision uses particles, including interior nodes. Interbody contact uses surface particles and sweep-and-prune. Self collision, continuous collision detection and arbitrary triangle obstacles are future work. Transmission/refraction is a screen-space approximation: a back-face eye-depth pass estimates thickness, and separate composition buffers expose the floor and farther bodies without reading the active framebuffer. It is not ray-traced optical transport. Whole bodies are sorted by center distance; deeply interleaved or non-convex surfaces can still show ordering artifacts. Exit-depth capture covers up to 128 world units. Shadows use uniform opacity attenuation rather than colored volumetric transport. Numerical safeguards bound velocity and dragging; volume barriers are not an absolute inversion guarantee. See plan.md for current verification evidence and limitations.

## Licenses and dependencies

Original project code/documentation: MIT (`LICENSE` here and at the repository root). The original MIT notice installs as `licenses/3D-Jely-Physiks-MIT.txt`; it does not replace the retained raylib license. Third-party components retain their own licenses, summarized at the root in THIRD_PARTY_NOTICES.md.

raylib 5.5: zlib license/third-party notices retained; guarded project compatibility patches support context recreation/GLES calls. Inter/OFL is embedded, earlier JetBrains Mono/OFL retained. ANGLE/EGL/Vulkan loader are pinned graphics-only assets from official Electron v41.0.0; full unmodified upstream notices in vendor/angle/LICENSES.chromium.html are embedded and printed by --licenses. No Electron/Node/browser framework runs in this app. Portable compiler/tools retain their notices and are not needed to launch. Five custom scene/glass shaders are embedded and adapted to ESSL only on Vulkan.

The named video in the supplied specification was not accessible during implementation; this project follows the written concept with an original implementation. Video-specific visual parity is unverified.
