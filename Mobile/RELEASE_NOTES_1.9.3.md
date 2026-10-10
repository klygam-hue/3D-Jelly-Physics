# 3D Jelly Physics 1.9.3 — floor dragging and recovery

Fix jelly becoming stuck against the floor at high Softness while only the grab point and its line continue moving. A threatened tetrahedron previously caused the inversion safeguard to reject movement of the entire body. The safeguard now limits individual node motion near the threatened element, allowing the rest of the mesh to move and recover. The fast path for valid poses and the material/handle settings are retained.

The shared regression suite adds an independent-node inversion check and floor reversal/hold/release tests at 10, 15 and 43 FPS, including 95–100% Softness, corner and face grips, Balanced/Detailed meshes. These checks measure the raw requested cursor position, whole-body displacement, positive tetrahedra and recovery after release. No per-step scratch allocations are introduced. The desktop window title identifies version 1.9.3.

Packages: Windows x64, Linux x64, macOS universal, native Android test APK (ARM64/x86_64, Android 8.0+, ES 3.0), and unsigned native arm64 iPad IPA (iPadOS 16+). Mobile version/build: 1.9.3/193. The unsigned IPA requires your own Apple signing/provisioning before installation. Android uses a development certificate; a signature mismatch may require uninstalling an older test build and clears its settings.

Validation records: [Mobile/VALIDATION.md](https://github.com/klygam-hue/3D-Jelly-Physics/blob/main/Mobile/VALIDATION.md). Numerical tests and emulator/simulator checks do not replace testing on the reported old laptop, physical Android GPU drivers or M-series iPads. The physics remains a numerical approximation rather than a calibrated real-gel model.

Version 1.9.2 is an immutable published release and is retained. Download the 1.9.3 package for your platform, extract it to a new folder on desktop, and run the executable from that folder.
