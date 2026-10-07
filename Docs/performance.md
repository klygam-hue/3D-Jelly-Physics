# 3D Jely 1.3 — measured optimization

## Version 1.9 — softness stability cost and monitor pacing

24 alternating before/after runs, Windows/Radeon, 600 frames with 60 warm-up, unchanged Duet/default quality/full UI; active explicitly disables sleep. Monitor cap/VSync are bypassed in benchmark mode. Median of three mean frame-wall measurements:

| API / workload | 1.8 ms | 1.9 ms | Change |
|---|---:|---:|---:|
| OpenGL settling | 1.85254 | 1.87419 | +0.02165 ms / +1.17% |
| OpenGL active | 2.99836 | 3.01648 | +0.01812 ms / +0.60% |
| Vulkan settling | 2.39474 | 2.38349 | -0.01125 ms / -0.47% |
| Vulkan active | 3.68718 | 3.71316 | +0.02598 ms / +0.70% |

Run means: GL settling old 1.85254/1.82443/1.86015, new 1.87920/1.86793/1.87419; GL active old 2.99836/2.99830/3.04611, new 3.01648/3.06551/3.01358. Vulkan settling old 2.40320/2.39140/2.39474, new 2.40811/2.38349/2.38060; Vulkan active old 3.69909/3.65520/3.68718, new 3.72021/3.67064/3.71316. Variations are small; this is not a claimed broad speedup. CPU submission/frame wall are not isolated GPU timing or cross-vendor FPS promises.

The new volume admissibility check adds a bounded linear scan per active soft substep; backtracking occurs only when needed. Rigid fitting uses fixed stack matrices and existing nodal storage; rigid mode skips edge denominator preparation. Patch search/sort/allocation happens once on grab, not per solver iteration, and stores at most 32 weights. Handle motion advances inside fixed physics, so moving from 120 to 280 Hz rendering cannot increase its allowed physical speed. Monitor/name query is at 2 Hz and after context recreation, outside the solver. Stable mesh/framebuffer, blur/shadow/scene and sleep caches remain; no precision, solver iterations, optical resolution or physics topology was lowered. Higher softness changes actual dynamics/sleep latency, so the default comparison is not a maximum-softness throughput guarantee.

The final original-MIT embedding is a static metadata-only rebuild after these measurements; it adds no per-frame work. Historical milestones follow.

Version 1.8 Ground uses one retained unit plane/model scaling, shader-only anti-aliased patterns and existing framebuffers. The full control smoke logs show only floor/body mesh uploads (2 total), one composition-target generation and no floor reallocation while changing dimensions. Resize builds bounded CPU body-bound/shift scratch only when dimensions actually differ; paused/unchanged appearance continues using scene/shadow/blur caches. The fixed 2048 shadow map adapts projection rather than allocating a new texture; larger world coverage naturally lowers texel density. No simulation precision/iterations or material surface topology are reduced.

Paired three-alternating-run Windows/Radeon benchmark, versions 1.7/1.8, 600 frames/60 warm-up, Duet/default quality and full UI: GL means before 1.80155/1.81824/1.80072 ms versus after 1.85467/1.82885/1.82517 ms; medians 1.80155 -> 1.82885 ms (+0.02730 ms, 1.5%). Vulkan means before 2.35873/2.35816/2.32313 versus after 2.39574/2.39638/2.37074 ms; medians 2.35816 -> 2.39574 ms (+0.03758 ms, 1.6%). This is a small added feature cost, not a speedup; native GL remains faster on this host. CPU/frame wall timings include presentation/submission, not isolated GPU execution, interactive FPS guarantees or other vendors/platforms. New default visible 16 x 16 ground matches unchanged default physical bounds, instead of 1.7's oversized 20 x 20 visual floor; lighting/projection/material parameters also differ, so this is an end-to-end version comparison, not shader-only attribution.

Version 1.7 spawning bounds normal UI growth at 16 bodies. Current-body AABBs are computed once per spawn rather than rescanned for all placement trials, and only the new physical GPU mesh is allocated: the three-spawn smoke log has 5 uploads total (floor + original + three additions), not 11 (floor + successively recreated 1/2/3/4 bodies). Structural revision invalidates shadow/scene caches on append/reset. Geometry/color RNG is never evaluated per frame. Sleep/steady-state caching remains, without precision/quality reduction. More moving bodies still cost more solver/narrowphase/transparency work, and maximum-body Detailed FPS is not guaranteed. Current multi-body scope/results are in validation.md; previous 1.6 API timings below are historical, not a measured 1.7 speedup.

Version 1.6 portable services run at initialization/backend recreation or preference saves, not in the solver/render hot path. Six alternating current 600-frame settling-Duet runs (60 warm-up frames) gave median mean frame wall 1.80596 ms native GL and 2.30645 ms Vulkan/ANGLE, compared with 1.5's 1.79887/2.33540 ms on this same Windows/Radeon host: normal run-to-run variation, no material hot-path regression or new speedup claim. GPU isolation/cross-vendor or foreign-platform benchmarking is not performed. Context/capability policy does not reduce iterations, precision, material fidelity, meshes, blur resolution or quality. No unsafe fast-math/CPU-specific optimization was introduced. Full active/paused historical API comparison remains in graphics.md.

Version 1.5 API comparison is in graphics.md. On the tested setup, Vulkan/ANGLE is functional but slower than native OpenGL; no Vulkan speedup is claimed. Existing quality/CPU optimizations remain in both.

Version 1.4 adds the Liquid Glass-inspired UI. Its paired timing/caching evidence is in interface.md: added median frame cost 0.192 ms for settling Duet, 0.204 ms for continuous no-sleep Duet and 0.127 ms paused on this machine. Static blur updates once per 600-frame run. The following optimization table describes historical 1.3 versus 1.2, not the latest interface's absolute cost or a guaranteed FPS.

Date: 2026-10-07. Windows x64, GCC 16.2 / w64devkit 2.10.0, static Release, raylib 5.5, Radeon RX 7600 XT / OpenGL 3.3, 1440 x 900. These are local measurements, not a universal performance guarantee.

## Method

An instrumented pre-optimization 1.2 executable and final 1.3 Release alternated for three runs per case. Each run uses 600 rendered frames, two fixed 120 Hz physics ticks per render frame, with the first 60 frames excluded. Reported values are the median of the three run means (540 measured frames per run). Same Balanced resolution, three substeps, eight iterations, camera and default material unless stated. Normal frame limiting/VSync is disabled only by `--benchmark`. There was no concurrent compiler build during these paired runs.

The ten-second drop-to-rest sequence includes sleep in 1.3. The active case disables sleep explicitly; 1.2 has no sleeping implementation. Active means continuous solver work, including the physically settled portion, not a guarantee of perpetual visible motion. The appearance clock stays fixed (no Auto day). Real-time interactive rendering has a different physics/render-frame ratio.

Frame wall time measures the complete application frame including presentation/driver waits. Physics and mesh stages are CPU timings; draw submission is not isolated GPU execution. No GPU timestamp query, formal allocator profiler or cross-machine run was performed. Do not interpret reciprocal frame means as guaranteed gameplay FPS; normal mode remains capped at 120 FPS.

## Results

| Sequence | Before frame mean, ms | After frame mean, ms | Lower frame cost |
|---|---:|---:|---:|
| Single jelly, 65% transparent, settling + sleep | 1.51265 | 0.656039 | 56.6% |
| Duet, 65% transparent, settling + sleep | 2.99272 | 1.56012 | 47.9% |
| Duet, opaque, settling + sleep | 3.02360 | 1.63511 | 45.9% |
| Duet, 65% transparent, continuous no-sleep solver | 2.97851 | 2.74183 | 7.9% |

The large settling gains primarily avoid unnecessary idle work; they must not be presented as a twofold acceleration of active XPBD solving. In the continuous-load case, surface update falls from 0.321520 to 0.191270 ms (40.5% less); physics from 2.423130 to 2.354170 ms (2.8% less); CPU draw submission from 0.199391 to 0.165119 ms (17.2% less). Medians are calculated separately per stage and need not sum to median frame time. Driver jitter remains visible in individual runs/p95.

Raw frame means, ms (three runs in execution order):

| Case | Before | After |
|---|---|---|
| Single | 1.51193, 1.51265, 1.51417 | 0.652959, 0.656039, 0.668431 |
| Duet | 2.97691, 2.99272, 3.00343 | 1.55666, 1.58571, 1.56012 |
| Opaque duet | 2.97453, 3.05884, 3.02360 | 1.63511, 1.61365, 1.64627 |
| Continuous active | 2.97851, 3.02011, 2.97205 | 2.69854, 2.88632, 2.74183 |

Paused 600-frame check: mean mesh update 0.000724 ms, frame wall 0.336013 ms, p95 frame wall 1.197 ms; physics_steps=0. This is a cache-hit diagnostic, not a matched old/new performance comparison.

Reproduce from an existing output directory:

```text
3D_Jely.exe --smoke 600 --benchmark --scene 0 --report C:\temp\single.txt
3D_Jely.exe --smoke 600 --benchmark --scene 2 --report C:\temp\duet.txt
3D_Jely.exe --smoke 600 --benchmark --scene 2 --transparency 0 --report C:\temp\opaque.txt
3D_Jely.exe --smoke 600 --benchmark --scene 2 --no-sleep --report C:\temp\active.txt
3D_Jely.exe --smoke 600 --benchmark --paused --report C:\temp\paused.txt
```

## Changes and safety

- Physics: one damping exponential per substep, cached edge denominator per substep, inactive barrier early-out, cached surface-node indices, conservative body AABB rejection, squared-distance contact rejection and one contact sqrt.
- Sleep: all bodies must be settled for 180 ticks below 1 mm/s; upward support required under gravity; finite, volume and minimum-tetrahedron guards. Input/reset/settings/direct scene or node edits invalidate sleep. Unsupported low-gravity movement is tested. Sleep is global, not a full per-body contact-island system.
- Surface: immutable sparse trilinear bindings, physical-node interpolation once, flat adjacency and cached inverse degree. Same welded topology, four Taubin passes and area-weighted normals.
- Graphics: cached shader locations and exact pose/float-buffer comparisons; unchanged GPU buffers not uploaded. Shadows invalidate on caster/light changes. Transparent finished-layer reuse checks mesh, camera, appearance, target generation and body ordering. Opaque/invisible paths skip optical composition; opaque presentation now uses the main framebuffer MSAA hint. Midrange transparent composition remains single-sample.
- UI: constant-time ASCII glyph lookup, shared font/shape atlas, volume telemetry at 10 Hz. Unicode/multiline text falls back to raylib. Previous shape texture is restored before unloading the owned font.
- Benchmark samples allocate only when explicitly requested. Persistent scratch/buffers are reused. No unsafe fast-math, architecture-specific compiler target, lower resolution/iteration count or reduced physical precision was introduced.

## Correctness evidence and remaining work

Nineteen headless groups pass in Release and Debug. The 20-second drop/rest still runs with sleep disabled and retains volume 0.999966 / max speed 3.42855e-6 m/s. New checks cover approaching contacts despite AABB rejection, cached binding rebuild/affine deformation, supported sleep/wake and unsupported low gravity.

At the same deterministic 120-frame state/noon lighting, the one-body and duet 3D crops (x=335..1439, y=250..814) match the old render pixel-for-pixel. Fixed UI controls differ at only 22 pixels by more than two intensity levels, with maximum channel difference five, consistent with tiny atlas/batching antialias differences; layout/text remain visually unchanged. Dynamic telemetry is excluded. Paused 120/600-frame captures verify stable retained-layer presentation. Automated Release/Debug checks exercise opacity 0/65/100, lighting/material changes, grabbing, pause/step/resume, resets, debug and window resize. They do not replace manual physical mouse use or long-duration profiling.

Remaining expensive work during deformation is XPBD tetrahedral solving and smoothing/normals; Auto day/camera motion correctly prevents relevant cache reuse. Future GPU deformation, per-body sleeping or threading needs separate correctness and performance validation. Existing optical/contact approximation limits remain in plan.md. Optimization is an ongoing acceptance criterion, not a claim that every possible workload or hardware is maximally optimized.
