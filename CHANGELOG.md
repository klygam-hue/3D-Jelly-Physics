# Changelog

## 1.9.3

- Fix high-Softness floor dragging becoming stuck while the grab point and line continue moving. Replace whole-body inversion backtracking with a bounded local fallback around threatened tetrahedra.
- Preserve admissible movement and elastic recovery of unaffected nodes, including roundoff at the minimum-volume boundary. Cache adjacency and fallback scratch storage; allocate nothing per solver step.
- Add independent-node inversion and floor reversal/hold/release regressions at 10, 15 and 43 FPS, bringing the suite to 45 groups. Check the requested cursor position and whole-body displacement, not just the displayed handle.
- Rebuild desktop, Android and iPad packages as version 1.9.3/mobile build 193. Show the version in the desktop window title.
- Retain immutable 1.9.2 and earlier releases as historical downloads.

## 1.9.2

- Smooth rapid mouse/touch reversals with finite handle acceleration and moving-hand XPBD damping; preserve elastic stretching, volume and throw momentum.
- Add high-Softness corner-reversal, floor-lift and throw-recovery regressions, bringing the headless suite to 43 groups.
- Match mobile Balanced/Detailed solver iterations to desktop (eight/ten).
- Rename the project to **3D Jelly Physics**, including window/launcher labels, build targets, packages, workflow artifacts and documentation.
- Add a native Android test build for ARM64/x86_64 and Android 8.0+, with adaptive touch controls and the shared 120 Hz physics/renderer.
- Add the native iPadOS arm64 port, unsigned IPA builder and device/simulator CI. Device installation requires Apple signing/provisioning; the validation record states the actual build boundary.
- Preserve update identity and existing project settings/cache paths; use version-independent mobile settings with a legacy fallback.

## 1.9.1

- Fix dragging during 10–15 FPS drops: process up to twelve fixed physics ticks per frame instead of eight, preserving 120 Hz simulation at 10 FPS. Keep fractional tick time instead of resetting interpolation after catch-up, and report time discarded during longer stalls. The version number remains 1.9.1.
- Add low-FPS timing, stall/reset, and floor-to-air corner-grab regressions for Balanced/Detailed meshes at 0, 50 and 100% Softness. The timing regression fails with the previous eight-tick limit.
- Fix excessive local stretching and impact flattening at high Softness with continuous finite-strain elastic hardening. Also resist local volume loss when a soft shape is pressed against the floor/walls. Small-strain compliance and the 0..100 slider mapping are preserved.
- Evaluate the nonlinear XPBD constraint gradient in both the denominator and position correction; remove the obsolete linear-denominator cache.
- Add shape/recovery regressions at 75, 80, 85, 90, 95 and 100 for Balanced/Detailed meshes, and extend random-shape wall-drag checks to 80 and 90.

## 1.9.0

- True rigid 0% Softness, expanded continuous soft range and mass-weighted rigid fitting.
- Weighted surface dragging instead of pulling one isolated point.
- Fixed-step handle speed independent of input/render frequency; inversion backtracking.
- Follow the current monitor's configured refresh rate; distinguish render and physics Hz.
- Preserve the render limiter when recreating OpenGL/Vulkan contexts.
- Extend numerical, interaction and graphics regression coverage; add source licensing.

## 1.8.0

- Independent rectangular ground dimensions, patterns, visual roughness and decorations.
- Transactional whole-body relocation and rejection of unsafe ground shrinking.

## 1.7.0

- Additive random physical shapes/colors, seeded checks and a 16-body budget.

## 1.6.0

- Linux ANGLE/Vulkan runtime layout and macOS native OpenGL/universal build profile.

## 1.5.0

- Hardware Vulkan through ANGLE, native OpenGL selection and safe context recreation.

## 1.4.0

- Animated glass-style interface, collapsible controls and accessibility modes.

## 1.3.0

- Physics/render caching, bounded scratch buffers, sleep and measured optimization.

## 1.2.0

- Adjustable transparency, refraction, gloss and tint.
