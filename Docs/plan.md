# 1.9.3 PhysikX integration plan

- [x] Pin PhysikX native core at 4c14b7592cfe6e29981e37d06c71ae6691baae4b.
- [x] Link a renderer-independent Jelly extension to native PhysikX particle prediction.
- [x] Retain 1.9.3 tetrahedral volume/inversion/contact/grab behavior and current version/build 193.
- [x] Wire static engine targets into desktop, Android and iPad builds; retain license notices.
- [x] Add native execution, reconfiguration/ownership and C ABI regression checks.
- [ ] Pass the full 48-group suite, desktop builds/graphics and mobile runtime checks.
- [ ] Publish source and five platform packages as 1.9.3 PhysikX (tag v1.9.3-physikx).

The existing immutable v1.9.3 release remains available. The PhysikX variant keeps
application version 1.9.3 / mobile build 193; its tag and filenames identify the engine.
