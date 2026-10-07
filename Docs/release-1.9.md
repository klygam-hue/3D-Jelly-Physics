# 3D Jely Physiks 1.9.0

## Download

- **3D_Jely.exe**: locally tested standalone Windows 10/11 x64 Release. Shaders, fonts,
  original MIT and optional ANGLE/Vulkan graphics runtime/notices are embedded.
- **3D_Jely-linux-x64-1.9.tar.gz**: Linux x64 CI package, including adjacent pinned
  Vulkan runtime and notices. Native build/physics and software-Mesa OpenGL checks;
  hardware Vulkan/AMD/NVIDIA/Intel validation is still outstanding.
- **3D_Jely-macos-universal-1.9.tar.gz**: Intel + Apple Silicon `.app`, macOS 11+,
  native OpenGL. CI build/physics verified, desktop GPU run unverified; unsigned and
  not notarized. **No macOS Vulkan implementation.**
- **3D-Jely-Physiks-source-1.9.zip**: complete tracked source, assets, pinned dependency
  runtime/notices, build files and documentation; no compiler/build/cache/private prompt.
- **SHA256SUMS.txt**: integrity hashes for the published files, not code-signing certificates.

Unix tar archives preserve executable permissions. Extract into a new directory:

```sh
mkdir jely
tar -xzf 3D_Jely-linux-x64-1.9.tar.gz -C jely
./jely/Release/3D_Jely --backend opengl
# macOS archive instead: open jely/3D_Jely.app
```

Linux needs an X11/XWayland desktop and graphics/system libraries listed in
compatibility.md. These packages are build-verified, not universal hardware guarantees.
macOS security may require explicit user approval for unsigned software; no global
Gatekeeper/security setting is changed. Windows Vulkan needs an installed compatible
GPU driver; the bundled loader does not supply one. The app uses no network at launch.

## Highlights

- Exact rigid **Softness 0%**; expanded soft range with whole-body stretching,
  elastic impact compression and recovery, not predefined animation.
- Distributed surface grabbing, fixed-step handle speed and bounded volume safeguards.
- Actual monitor-configured render pacing (280 Hz on the tested host), distinct from
  the fixed 120 Hz physics solver and measured frame rate.
- Transparent material customization, random shape/color spawning, a 24-hour scene,
  independent rectangular ground customization and animated collapsible glass controls.
- Native OpenGL / verified hardware Vulkan through ANGLE on supported platforms.

Controls: left mouse grab/throw, right orbit, middle pan, wheel zoom; B spawn,
Space pause, N paused step, R reset, J impulse, Tab controls, F1 debug, F11 fullscreen.
Use roughly 80..100% Softness for visibly soft jelly; 50% retains the previous default.

## Verification and limits

35 numerical regression groups passed Windows Release/Debug; both native GL and
hardware Vulkan passed slider/drag/drop, existing UI, spawn, ground, scene/resize,
multi-body endpoint and state-preserving API-switch checks. The Windows EXE also
runs outside the source tree. Paired default workloads showed a median frame-time
change within about 1.2% compared with 1.8, without reducing quality; this is not an
FPS guarantee at the 16-body Detailed limit. Full evidence is in validation.md and
performance.md, with CI/software graphics separated from real desktop hardware runs.

Physics/contact/optical approximations are explicit: no exact measured-material
calibration, self-collision or continuous surface collision; optical transport is
screen-space. Monitor refresh is the configured integer rate, not live VRR telemetry.
No exhaustive bug-free or every-driver claim is made.

## License

Original code/documentation: MIT. Third-party licenses and notices remain intact and
separate, summarized in THIRD_PARTY_NOTICES.md. `--licenses` prints embedded notices.

Windows EXE SHA256:
`80B35985351EF00D4B85B9654B3C7A5CF42193E52784DB1152FBF2463D4D6BC4`
