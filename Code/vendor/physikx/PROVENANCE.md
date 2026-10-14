# PhysikX integration provenance

Native engine: **PhysikX 0.2.0-beta.1**, repository `klygam-hue/PhysikX`,
pinned commit `4c14b7592cfe6e29981e37d06c71ae6691baae4b`.
`Code/src/physikx.cpp` and the C++ wrapper are unchanged upstream copies.
The C header adds `PX_STATIC` for static linkage; ABI v1 layouts and symbols are unchanged.
No download, access token or engine DLL is needed when compiling this snapshot.

`Jelly/` is an integration extension, derived from 3D Jelly Physics 1.9.3,
commit `5153a59a092ab41e4dcb61c1c8ace388241b97cd`. It contains the existing
MIT-licensed tetrahedral XPBD material, rigid fitting, volume/inversion guard,
shape sampling and math. Prediction is executed by native PhysikX worlds:
one native body per mesh node; gravity, damping and speed limiting run in PhysikX.
Collision masks are zero in the predictor because deformable contact projection
and weighted cursor grabbing are coupled to the tetrahedral solver in the host world.
This does not claim upstream primitive distance links implement a volume-preserving gel.
Each soft body uniquely owns its predictor, rebuilt only when timestep/damping changes.
No scene rendering, windowing or mobile SDK is used by the engine targets.

The native core is governed by `LICENSE` (PhysikX proprietary evaluation terms).
The transplanted Jelly module remains governed by `Jelly/LICENSE` (MIT).
The copyright holder explicitly requested integration and publication in this project.
This does not relicense PhysikX as MIT or revoke permissions on older MIT copies.
Recipients must comply with both components' applicable licenses.
