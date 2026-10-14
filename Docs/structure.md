# Project structure

- `Code/vendor/physikx/Code/`: pinned native PhysikX C ABI and C++ wrapper.
- `Code/vendor/physikx/Jelly/`: math and tetrahedral soft-body extension; calls native prediction.
- `Code/vendor/physikx/PROVENANCE.md`: source pin, integration boundary and licenses.
- `Code/include/jely/{math,physics}/`: compatibility aliases into PhysikX::Jelly.
- `Code/src/physics/PhysicsWorld.cpp`: scene, contact, grab and sleeping orchestration.
- `Code/src/{app,render,ui,input}/`: desktop graphics and interaction.
- `Code/tests/PhysicsTests.cpp`: shared physics plus native engine regression groups.
- `Mobile/`: native Android/iPad app, touch tests and platform builders.
- `.github/workflows/`: cross-platform validation and staging into the PhysikX draft.
- `Docs/plan.md`: integration completion checklist.

Native PhysikX and Jelly are statically linked, including into the mobile native library.
The native core retains its proprietary license; the existing host and Jelly extension retain MIT.
