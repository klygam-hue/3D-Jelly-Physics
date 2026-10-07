# Third-party notices

The root MIT license covers original project code/documentation only.

| Component | Provenance and license location |
|---|---|
| raylib 5.5 / bundled GLFW and headers | `Code/vendor/raylib-5.5/LICENSE`; original source/header notices retained |
| ANGLE, EGL and Vulkan loader binaries | Official Electron 41.0.0 graphics assets, not the Electron application; full notices in `Code/vendor/angle/LICENSES.chromium.html` and corresponding Linux directories |
| Inter font | `Code/assets/Inter-OFL.txt`, SIL Open Font License 1.1; embedded with the font |
| Retained JetBrains Mono font | `Code/assets/OFL.txt`, SIL Open Font License 1.1 |
| Optional w64devkit build tools | Downloaded separately by `Code/bootstrap_tools.ps1`; upstream tool notices remain in that local toolchain; not shipped as runtime application code |

Five raylib/GLFW source files contain guarded project compatibility changes for
context recreation and the GLES/ANGLE path; the source and original notices remain
available. ANGLE runtime binaries are unmodified and SHA256-pinned; exact provenance
and hashes are in `Code/vendor/angle/README.md` and build/bootstrap files.

Windows embeds the complete graphics notices; Linux packages them next to its runtime.
`3D_Jely.exe --licenses` prints the embedded original MIT and dependency notices. Mathematical references, not copied
source code: [XPBD](https://matthias-research.github.io/pages/publications/XPBD.pdf) and
[Horn's quaternion fit](https://people.csail.mit.edu/bkph/papers/Absolute_Orientation.pdf).
