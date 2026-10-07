# 3D Jely — actual project structure

Root: `C:\Users\User\Code Projekts\3D_Physiks`. Source: Code. Documentation: Docs. This supersedes the older path in the supplied prompt.

```text
3D_Physiks/
├── .github/workflows/desktop.yml   # repository-root desktop/arm64 CI; run steps use Code
├── .gitattributes                 # LF source; pinned runtime/notices never transformed
├── .gitignore                     # build/cache/tools/private specification exclusions
├── README.md                      # release overview, controls, platform and physics scope
├── LICENSE                        # MIT, original project only
├── THIRD_PARTY_NOTICES.md          # dependency licenses/provenance
├── CHANGELOG.md
├── Docs/
│   ├── idea.md                      # retained original (empty)
│   ├── Prompt.txt                   # retained original specification
│   ├── plan.md                      # implementation state and roadmap
│   ├── structure.md                 # this file
│   ├── validation.md                # build/runtime evidence and scope limits
│   ├── preview.png                  # current app capture, not a mockup
│   ├── release-1.9.md               # release assets, launch instructions and verification scope
│   ├── performance.md               # measured optimization and benchmark limits
│   ├── interface.md                 # Liquid Glass UI behavior and verification
│   ├── graphics.md                  # OpenGL/Vulkan architecture and validation
│   └── compatibility.md             # platform/GPU matrix, build/package/validation boundaries
└── Code/
    ├── .gitignore
    ├── CMakeLists.txt
    ├── CMakePresets.json
    ├── README.md                     # includes Ground controls/CLI and rectangular-arena behavior
    ├── LICENSE                       # original MIT notice installed alongside dependency licenses
    ├── bootstrap_tools.ps1
    ├── bootstrap_angle.ps1           # pinned/hash-checked graphics runtime restore
    ├── bootstrap_angle.cmake         # portable Linux x64/arm64 asset restore
    ├── build_release.ps1
    ├── build_release.bat
    ├── assets/
    │   ├── JetBrainsMono-Regular.ttf
    │   ├── OFL.txt
    │   ├── InterVariable.ttf          # current embedded UI font (OFL)
    │   └── Inter-OFL.txt
    ├── cmake/
    │   ├── EmbeddedFont.hpp.in
    │   ├── GraphicsRuntime.rc.in      # Windows-only embedded DLLs/notices
    │   ├── AngleLinux.cmake           # Linux individual hashes, runtime staging/install
    │   └── Info.plist.in              # high-resolution macOS .app metadata
    ├── include/jely/
    │   ├── app/Application.hpp
    │   ├── app/FrameProfiler.hpp
    │   ├── input/CameraController.hpp
    │   ├── math/Vec3.hpp
    │   ├── math/RigidFit.hpp          # graphics-free proper quaternion fit for rigid endpoint
    │   ├── physics/SoftBody.hpp
    │   ├── physics/PhysicsWorld.hpp
    │   ├── render/Renderer.hpp
    │   ├── render/Backend.hpp
    │   ├── render/Compatibility.hpp   # graphics-independent platform/capability/vendor policy
    │   ├── render/GraphicsSession.hpp
    │   ├── render/Shaders.hpp
    │   ├── render/Surface.hpp
    │   ├── render/SceneTargets.hpp
    │   ├── scene/Scene.hpp
    │   ├── scene/Appearance.hpp
    │   ├── ui/Panel.hpp
    │   ├── ui/Motion.hpp
    │   └── ui/Glass.hpp
    ├── src/
    │   ├── main.cpp
    │   ├── app/Application.cpp
    │   ├── input/CameraController.cpp
    │   ├── physics/SoftBody.cpp
    │   ├── physics/PhysicsWorld.cpp
    │   ├── render/Renderer.cpp
    │   ├── render/Backend.cpp
    │   ├── render/GraphicsSession.cpp
    │   ├── render/Surface.cpp
    │   ├── render/SceneTargets.cpp
    │   ├── scene/Scene.cpp
    │   ├── scene/Appearance.cpp
    │   ├── ui/Panel.cpp
    │   └── ui/Glass.cpp
    ├── tests/
    │   └── PhysicsTests.cpp
    ├── vendor/
    │   ├── raylib-5.5/               # pinned source + guarded dual-context compatibility patches
    │   └── angle/                    # Windows DLLs plus linux-x64/linux-arm64 SO/notices
    ├── tools/
    │   └── w64devkit/                # portable compiler/CMake and original notices
    ├── build/
    │   ├── release/                  # initial validation build, generated
    │   ├── local-release/            # build script's canonical Release tree
    │   │   ├── generated/EmbeddedFont.hpp
    │   │   ├── Release/3D_Jely.exe
    │   │   └── jely_tests.exe
    │   └── local-debug/              # separate Debug tree
    └── Release/
        └── 3D_Jely.exe               # installed, standalone user executable
```

Generated compiler/CMake intermediate files inside build and internal raylib/toolchain trees are deliberately abbreviated. They are not project-owned source modules. No external runtime assets or shader directory is required: assets and shader strings are compiled into the EXE. The original assets/shader source remain editable.

## Modules and ownership

Version 1.9: PhysicsSettings maps an exact rigid 0% endpoint and continuous exponential positive compliance (50% retains 0.000025; 100%=0.2). RigidFit.hpp provides a graphics-independent Horn quaternion fit using a fixed-size Jacobi eigenproblem; SoftBody owns immutable lumped masses/rest COM, projects the complete rest shape and a rigid linear/angular velocity field. PhysicsWorld applies final shape-preserving floor/wall translation after rigid particle contact iterations. Positive softness retains XPBD edges/local volumes; SoftBody::guardInversion backtracks an inadmissible substep with existing previous positions and bounded fixed scratch. Grab owns at most 32 normalized surface weights, cursor goal, physical target/offset and multipliers; physical target speed advances inside fixed substeps, not input events. AppOptions/CLI provides softness initialization and nine actual-slider/drag/drop smoke assertions. Application queries current monitor Hz/name at 0.5 s intervals and after context recreation, setting the render cap independently of fixed physics; Panel distinguishes monitor Hz/FPS/physics Hz. Benchmark bypasses cap/VSync. No new runtime dependency, physics thread or per-iteration heap allocation is introduced.

The root Git repository includes Code/Docs, original MIT notices, third-party provenance and desktop workflow; local original Prompt.txt/idea.md, compiler, build/install output and generated caches are excluded. Binary/runtime notices are marked -text to preserve SHA256-pinned bytes across checkout. Unix CI packages are tar.gz archives so executable permissions survive artifact download; Windows embeds the original MIT plus dependency notices. CI compilation/software graphics are not hardware claims.

Version 1.8 Ground: Appearance owns width/depth, pattern type/scale/strength, roughness, outline, ring radius and shared platform HSV; resetGround is independent. Scene uses halfExtent as X half-width and halfDepth as Z half-width. PhysicsWorld::resizeGround validates 6..32 m dimensions, precomputes whole-body shifts, rejects unfit/new-overlap configurations before mutation, then translates position/previous/framePrevious consistently without changing rest/mass/velocities/steps/settings. Successful size changes release the grab and wake sleep. Spawn/contact/drag bounds and debug cage share the rectangular scene data. Application reconciles requested appearance size before physics/spawn, reverting rejected dimensions with a UI message. Renderer retains a unit floor mesh and scales its model matrix, sets ground uniforms, anti-aliases procedural grid/tiles and invalidates/rescales shadow projection with footprint changes. Panel's always-visible Ground button selects a fifth subpage (Appearance remains highlighted); original four tab hit positions are retained. AppOptions provides validated initialization/350-frame UI regression controls. No new source module, dependency, image asset or dynamic floor topology is introduced.

Version 1.7 additive spawning: BodyShape/ShapeKind describes bounded rounded-box, ellipsoid, capsule-like and pillow rest geometry; SoftBody constructs actual rest nodes, edges, tetrahedral volumes/masses from the descriptor and retains immutable optional per-body RGB. PhysicsWorld owns a separate mt19937_64 spawn stream (fresh normal seed; deterministic test override), a 16-body budget and structural revision. Spawn builds a valid candidate transactionally, computes existing bounds once, selects a clear column, appends without modifying old particles/settings/grab/steps and wakes sleep. Reset clears extras without reseeding. Renderer uses structural revision for cache invalidation and grows/shrinks its existing mesh-owner vector, allocating only new body meshes; world-owned random colors survive graphics-context recreation. Panel has a separately hit-tested top-center Spawn button visible even with the menu collapsed, counter/limit feedback and animation; Application handles that request/B before physics and exposes bounded seeded CLI/UI test paths. This adds no new dependency or source module. All four rest families retain the existing simply connected lattice/surface interpolation/contact approximations.

| File / type | Responsibility |
|---|---|
| Vec3 | Independent double-precision vector arithmetic and signed tetrahedral volume |
| PhysicsSettings | Bounds checking and tunable solver/contact parameters |
| Node | Rest/current/previous/frame-previous positions, velocity, inverse mass, contact normal |
| DistanceConstraint / VolumeConstraint | Topology, rest values, XPBD multipliers and inversion barrier multiplier |
| SoftBody | Rounded lattice, consistent tetrahedra, lumped masses, integration, constraint projection, velocity reconstruction, stats and interpolation |
| Scene | Empty playable presets with wall bounds; optional analytic sphere/box collections retained for collision test fixtures |
| Appearance / HsvColor / LightingState | Independent environment/material HSV; time-of-day and automatic cycle; transmission/refraction/gloss/tint; separate material and environment reset; validation |
| PhysicsWorld | Fixed-step orchestration, environment/interbody contacts, sweep-and-prune scratch, compliant grab and reset |
| Surface | Welded high-resolution render topology, trilinear node binding, non-shrinking visual smoothing, normals |
| RenderMesh | RAII owner of raylib dynamic CPU/GPU mesh; updates existing vertex/normal buffers and supports ray picking |
| SceneTargets | RAII owner of two color/depth composition buffers and per-body exit-depth buffer; transactional resize and framebuffer color/depth blit |
| Renderer | RAII shader/material/mesh/shadow-map resources, packed shadow and exit-depth passes, thickness-based refraction/transmission, far-to-near composition, floor/background and debug overlay |
| GraphicsRuntime / GraphicsApi | Native OpenGL versus hardware ANGLE/Vulkan selection, checked embedded runtime cache/loading, GLES compatibility gateway, driver/device verification, isolated startup probe and persisted API preference |
| GraphicsSession | Owns context/window, renderer and panel; destroys GPU resources before context; recreates graphics without replacing world/camera/settings; restores the prior API on failure |
| CameraInput / CameraController | Input sampling separated from orbit/pan/zoom/movement math; same apply method used by smoke checks |
| UiState / Panel | Embedded font, Physics/Appearance/Jelly tabs, independent environment/material controls, time presets/auto cycle, telemetry and error feedback |
| AnimatedValue / MenuLayout (ui/Motion.hpp) | Graphics-independent exact critically damped animation and rounded menu hit geometry; headless tests cover timing, reversal and hidden input release |
| Glass (ui/Glass.hpp, ui/Glass.cpp) | RAII blur/material shaders, quarter-resolution separable GPU backdrop blur, optional matching-size MSAA resolve, rounded refractive/rim-lit glass panels, scene revision cache and transactional resize |
| AppOptions / Application | Window lifetime, bounded fixed-step accumulator, render interpolation, picking/drag plane, shortcuts, appearance launch arguments, smoke automation and report |
| FrameProfiler | Optional uncapped benchmark stage samples, 60-frame warm-up, means/p95; reports CPU submission and frame wall time, not isolated GPU execution |
| main.cpp | Argument validation, startup error boundary and native Windows error dialog |
| PhysicsTests.cpp | Graphics-independent numerical and geometry regression suite |

All simulation and rendering operations happen on the main thread. Physics bodies and constraints are value-owned vectors; Renderer exclusively owns RenderMesh through unique_ptr. Borrowed shader/texture references in materials are freed exactly once by Renderer. UI/font and GPU resources die before the window/context. Simulation state is not global; raylib and its project compatibility gateway own main-thread-only context state.

Version 1.5: GraphicsSession owns the window/context, Renderer and Panel; Application separately owns PhysicsWorld/Camera/UiState. API changes destroy old GPU owners before closing that context, then reconstruct graphics from the same world. Four tabs include Graphics. GraphicsRuntime uses a main-thread-only C gateway for raylib/GLFW context hints/procedure compatibility; no world/render threads share mutable context state. Native OpenGL remains WGL/GL3.3; Vulkan is ANGLE/EGL/GLES3.1 mapped to native hardware Vulkan. Activation checks real device handles. A bounded hidden child probe isolates startup driver crashes; failures restore the previous working backend. Ordinary UI selection persists, smoke does not modify preference.

The project patches three pinned raylib files under JELY_DUAL_GRAPHICS: rcore_desktop_glfw.c selects EGL/ANGLE Vulkan, rcore.c clears stale input on recreation, and rlgl.h uses the compatibility loader, clears dead context state, avoids invalid GLES enables and uses sized D32F depth. The frontend adapts #version 330 to ESSL300 with high precision, clear-depth and texture-swizzle entry points; shader bodies/quality remain unchanged. General unused desktop-only raylib APIs are not claimed GLES-compatible. All framebuffer creation is checked for completeness.

Windows RC embeds libEGL.dll/libGLESv2.dll/vulkan-1.dll and full notices. CMake validates pinned hashes and tracks resource dependencies; MinGW RC uses an explicit preprocessor/temp-file command to handle spaced paths. GraphicsRuntime verifies cache bytes, loads absolute paths, owns module handles and restores its process-only ANGLE feature overrides. LocalAppData stores generated runtime/preference; executable-side 3D_JelyData is a portable fallback. No Vulkan SDK/browser is needed to launch. CPU-only tests require neither runtime nor GPU. Screenshots flush/capture before swap, because EGL/Vulkan may discard the presented back buffer. Known optional AMD maintenance-presentation failure is bypassed without reducing MSAA/quality; no system changes.

Version 1.4 UI flow: Application samples Panel::Input and calls update before camera/picking, then Renderer draws the scene and exposes a borrowed finished color texture plus revision. Glass prepares one shared GPU backdrop before any UI is drawn; all glass components read this separate texture, never the active framebuffer. Transparent scenes reuse the existing composited texture. Opaque/invisible scenes resolve the main framebuffer at matching render dimensions before downsampling. Blur refreshes only on scene revision/target resize; pointer highlights and UI animations do not invalidate it. Panel draws animated content with matching translated input coordinates and prevents clicks through the launcher, visible/moving menu, telemetry/help and active slider drags. Animated collapse keeps a launcher visible; tabs/material/physics settings persist. Reduce-motion and solid-UI modes are session settings. Windows client-area animation preference is read at startup. Debug overlays are excluded from the transparent scene's shared backdrop.

Version 1.3 cache ownership: SoftBody owns precomputed surface indices and per-substep edge inverse denominators. PhysicsWorld owns reusable broadphase/sleep snapshots and conservative global sleep state, invalidating on physical input/settings/environment/node edits. Surface owns sparse trilinear bindings and flat CSR smoothing adjacency, rebuilding binding on physics resolution changes. RenderMesh owns the exact pose cache and skips unchanged float-buffer uploads. Renderer owns cached uniform locations, caster/light shadow validity and retained transparent scene layers; camera/appearance/geometry/ordering/target generation changes invalidate reuse. Opaque/invisible presentation needs no SceneTargets allocation on initial launch. Existing transparent targets are retained if switching temporarily to an endpoint. Panel owns the ASCII glyph lookup, borrows/restores the previous shapes texture, and refreshes full volume stats at 10 Hz. No cache changes the solver topology/quality or physical double precision. FrameProfiler allocates samples only for explicit benchmark runs.

Appearance is graphics-independent and shared by UI and Renderer. LightingState derives a consistent key light and background from the 24-hour clock. HsvColor converts the independently controlled palettes to RGB. The shadow framebuffer packs depth into RGBA; blending is disabled during that pass because alpha stores depth bits. The lit pass unpacks depth for PCF and applies sRGB-to-linear conversion, filmic tone mapping and display contrast. All playable presets are obstacle-free; collision fixtures are explicitly created only by headless tests. PhysicsWorld has mutable scene access for those fixtures.

Transparent rendering first builds an opaque background/floor layer. Per body, a front-culling pass captures the back surface eye depth packed into RGBA, covering 128 world units. The lit front surface measures thickness against that capture, bends the scene sample, applies color absorption and blends transmission with lit reflection. SceneTargets owns two color/depth layers and one reusable exit-depth target. A color/depth blit copies the accumulated layer; the shader samples the source while writing a different destination, avoiding framebuffer feedback. Bodies are sorted far-to-near by center distance with stable index ties. Front/back culling and color blending are restored before presentation. At 100% transparency the body passes are skipped and no body shadow remains. Target replacement on resize allocates all new resources before releasing the old set; Renderer exposes a generation counter for smoke checks.

```text
Input + Panel -> Application -> PhysicsWorld -> SoftBody
                                 |              |
                                 Scene          node states
                                                |
CameraController -> Renderer <- RenderMesh <- Surface
                       |
                 native OpenGL / ANGLE hardware Vulkan passes
```

## Build configuration and dependencies

Version 1.6: JELY_ENABLE_VULKAN defaults on only for pinned Windows x64/Linux x64/arm64 targets; an unavailable macOS Vulkan request is rejected explicitly. OpenGL-only builds require no ANGLE assets. JELY_GRAPHICS_TESTS=ON registers separate display-dependent scene/UI tests. All native app targets compile the guarded rlgl/context/input gateway; bundled GLFW additionally has guarded explicit EGL/GLES library selection and ANGLE X11 Window-value handling (five patched source files total). Linux enables X11 and disables the native Wayland build dependency, using XWayland on those desktops.

Linux installs the executable and adjacent runtime/angle directory with four pinned assets, including full notices, linking dl/Threads for POSIX library/probe services. macOS configures a high-resolution-capable `.app`, default native GL, universal Intel/Apple Silicon preset and 11.0 deployment target. Per-user API preferences use Windows LocalAppData, Linux XDG configuration, or macOS Application Support; POSIX uses mkstemp/write/fsync/rename and isolated posix_spawn/waitpid probing. Compatibility.hpp has no GPU vendor allowlist. Prepared GitHub CI builds MSVC/Ubuntu/macOS universal plus arm64 headless; no remote run is claimed. Compatibility.md records exact dependencies, signing/driver restrictions and evidence. Historical Windows details below remain applicable to that target only.

CMake 3.25+, C++20. jely_physics has no graphics dependency; jely_tests links only it. JELY_BUILD_APP=OFF is headless; JELY_BUILD_TESTS=OFF omits tests. 3D_Jely links static raylib/physics and embeds the pinned ANGLE runtime on Windows. Windows RC is enabled; bcrypt/ole32/uuid provide integrity/known-folder support. Standard DLLs remain required. Presets provide VS2022 x64 and MinGW Debug/Release. Vulkan mode requires Windows 10/11 x64 and a compatible driver; no Vulkan SDK/browser is needed to launch.

Scene shaders are in include/jely/render/Shaders.hpp; glass/blur literals in src/ui/Glass.cpp. CMake embeds Inter/OFL through EmbeddedFont.hpp.in and the pinned ANGLE DLLs/notices through GraphicsRuntime.rc.in. --licenses prints both. Old JetBrains Mono/OFL is retained. Inter SHA256: 4989B125924991B90D05B2D16E0E388C48F7D5BB8B30539BBF9C755278D0CCAF. raylib license/third-party notices remain; five source files have the documented guarded context patches. ANGLE provenance/hashes are in vendor/angle/README.md. Builds need no network with compiler/assets present. Audio/clipboard image are disabled; duplicate raylib mesh-buffer definitions are still normalized without changing its config file.

MSVC uses static /MT or /MTd and mainCRTStartup; MinGW links static libgcc/libstdc++ in the GUI subsystem. Only standard Windows DLLs are statically imported; embedded graphics runtime is dynamically loaded if Vulkan is selected. A real compatible GPU driver is required. Source/build resources have no machine-specific absolute paths; generated caches/RC resolve assets from this checkout. Build script resolves tools relative to itself.

build_release.ps1 owns configure -> compile -> CTest -> Release install; Debug stays under build. bootstrap_tools.ps1 pins w64devkit 2.10.0 with checksum. bootstrap_angle.ps1 restores fixed graphics assets from official Electron 41.0.0 with pinned archive SHA256, without installing a browser/framework. tools/build/Release and generated executable-side 3D_JelyData are runtime/build outputs, not replacement source.
