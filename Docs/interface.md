# 3D Jely 1.4 — Liquid Glass-inspired interface

Version 1.5 retains this interface and adds a fourth Graphics tab with real OpenGL/Vulkan (ANGLE) selection, driver status and rollback. Hardware/context/caching evidence is in graphics.md. The timing table below is historical 1.4 UI evidence, not latest API timings.

Original Windows/OpenGL UI inspired by Apple's Liquid Glass material language. It is not Apple's native implementation and makes no claim of pixel-identical platform behavior. No Apple fonts, artwork or private framework are distributed. Physics/material calibration is unchanged by this UI update.

## User controls

- Top-left menu button collapses the settings panel with a reversible slide/fade. The Controls launcher always remains visible and reopens the same panel. Tab invokes the same visibility state. Scene, material, physics values and selected tab survive collapse.
- Physics / Appearance / Jelly remain available. The selected pill glides; newly selected content fades in. Buttons animate hover/press/selection; sliders retain immediate, precise value control.
- Motion on/off below the open panel disables/enables transitions. Windows client-area animation preference is respected at startup. Reduced motion uses instant state changes, including close/reopen.
- Glass on/off switches to an opaque UI for readability or reduced GPU work. These modes are session settings, independent of jelly transparency/environment controls.
- The UI captures pointer input over its actual visible shape, launcher, telemetry/help, error box and active slider drag. Closing does not leave an invisible rectangle blocking interaction with the jelly.
- Launch switches: `--menu-hidden`, `--reduced-motion`, `--solid-ui`, `--ui-tab 0/1/2`. Defaults: menu expanded, Jelly tab, glass enabled; motion enabled unless disabled by the OS preference.

## Material and rendering

Glass owns two quarter-resolution render targets and two embedded shaders. A downsample plus horizontal/vertical five-fetch Gaussian blur produces one shared backdrop. Each rounded panel uses signed-distance coverage, screen-space edge refraction, desaturated/tinted backdrop, soft shadows, bright rim and restrained pointer highlight. Text is drawn afterward and is never blurred.

Transparent scenes lend the already-composited scene texture; neither a screenshot nor a CPU framebuffer readback is used for glass. At opaque/invisible endpoints, a full-render-size color target resolves the main framebuffer (including MSAA) before downsampling. Sampling never reads the active draw attachment. Target replacement is transactional and old resources are released after a successful allocation. All GPU/font resources die before the window/context.

Renderer exposes a scene revision. Unchanged geometry/camera/appearance/targets reuse the blur; hover, fading, layout and pointer highlights do not rerun it. Resize and source-path changes invalidate it. Opaque UI avoids preparing the blur; already-allocated targets are retained until resize/destruction rather than repeatedly released/recreated on toggles. Blurring the transparent scene excludes debug overlays. UI optics are an artistic screen-space material, not ray-traced physical glass.

Motion uses a closed-form critically damped response. Tests compare a single elapsed interval against repeated 120 Hz updates and cover mid-transition reversal, zero dt and reduced-motion snapping. Menu geometry and input transforms follow the same animated offset. Dynamic hover state uses fixed-capacity arrays, not per-frame allocations. Unicode/multiline font drawing retains the fallback path.

Inter is embedded from the official rsms/inter repository under SIL OFL 1.1, together with license text (`3D_Jely.exe --licenses`). Source retains the former JetBrains Mono and its license. Font hash: `4989B125924991B90D05B2D16E0E388C48F7D5BB8B30539BBF9C755278D0CCAF`. No network access or runtime font directory is needed to build/run with the checked-in assets/toolchain.

Design reference: [Apple Materials guidance](https://developer.apple.com/design/human-interface-guidelines/materials). Font source: [official Inter repository](https://github.com/rsms/inter), [license](https://github.com/rsms/inter/blob/master/LICENSE.txt).

## Performance evidence

Same Windows/GCC/RX 7600 XT setup, 1440 x 900; 600 frames per run, 60 excluded warm-up frames, three alternating Release 1.3/1.4 runs. Values are medians of run means. Normal VSync/120 FPS limit is disabled only for benchmark. No compilation or other app instance ran concurrently with these timing pairs.

| Workload | 1.3 frame mean, ms | 1.4 glass UI, ms | Added cost, ms |
|---|---:|---:|---:|
| Transparent Duet, settling/sleep | 1.597010 | 1.788660 | 0.191650 |
| Transparent Duet, no-sleep solver | 2.739890 | 2.943850 | 0.203960 |
| Paused single body | 0.331670 | 0.458241 | 0.126571 |

The glass UI has a real rendering cost; this is not described as another speedup. In each paused run blur_updates=1 for 600 frames. Duet with sleep updates blur 349 times; continuous no-sleep updates 600 times. Existing physics/surface/shadow caching is preserved. The earlier optimization comparison in performance.md is historical 1.3, not a timing guarantee for this new UI.

Frame wall time includes submission/presentation/driver waits, not isolated GPU execution. Render-frame smoke advances two physics ticks; ordinary interactive rendering has a different ratio. These local results do not guarantee FPS on other devices. Solid UI is available for lower-cost presentation. No formal GPU timestamp, allocator/leak or cross-platform profile was run.

## Verification

Twenty-one headless groups passed in Release/Debug, including the unchanged nineteen physics/geometry/appearance groups and two UI animation/geometry groups. Final Release suite: 8.87 s (8.88 total); Debug: 87.20 s (87.22 total). Final UI-only softness-preservation guard, stable responsive control IDs and smoke GL diagnostics were rebuilt afterward in Debug.

`--smoke 480 --ui-automate` feeds deterministic pointer positions/press/down state through the same Panel update/button/slider path used by physical input. Twenty-one assertions cover all tabs, transparency 0/65/100, sliding close/reopen, mid-animation reversal, collapsed launcher input capture and former-panel input release, Detailed/Balanced resets, reduced-motion instant close/open, glass/opaque toggles and two resizes. Opening Physics must preserve the exact stored compliance, rather than round-trip it through log/pow without slider input.

Separate captures were inspected for expanded/collapsed menus, all tabs, day/night/sunset, solid UI and opaque/invisible bodies. At deterministic noon/120-frame state, the unaffected 3D crop (x=350..1179, y=260..814) matches Release 1.3 pixel-for-pixel. Synthetic camera/picking/grab actions remain separate from UI pointer-path testing. Manual hardware mouse/keyboard, sustained interactive accessibility and multi-monitor/high-DPI use remain unverified. Minimum logical window remains 1160 x 900; smaller/mobile layouts are not implemented.
