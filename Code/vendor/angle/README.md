# Pinned ANGLE runtime

Version 1.6 additionally includes official Electron 41.0.0 Linux x64/arm64 graphics assets under `linux-x64/` and `linux-arm64/`. These are unmodified libEGL.so/libGLESv2.so/libvulkan.so.1/full LICENSES.chromium.html only. Archive hashes match official SHASUMS256.txt: Linux x64 `A28D5AB638FA065853C80D5F27EA9D7EC7F8621D9242200F747798C5ED3193D4`; Linux arm64 `D825D1F482493A66C537E313B5F126C61C3DFB785232D33716C74AFDD18833DE`. Sources: https://github.com/electron/electron/releases/download/v41.0.0/electron-v41.0.0-linux-x64.zip and https://github.com/electron/electron/releases/download/v41.0.0/electron-v41.0.0-linux-arm64.zip. bootstrap_angle.cmake restores only these fixed entries after full-archive verification. Individual pinned file hashes are in cmake/AngleLinux.cmake. CMake verifies/stages/installs all four files; notices are retained adjacent to the Linux runtime, not embedded there. Linux binaries have not been executed here. macOS has no Vulkan/ANGLE assets in this project: it currently uses native OpenGL; see Docs/compatibility.md. The following Windows provenance remains unchanged.

Only these graphics assets are used from the official Electron v41.0.0 Windows x64 release (Chromium 146 / ANGLE 2.1.27037, git 1d3190bf5633). Electron/Node/browser code is not used by the app.

Source archive: https://github.com/electron/electron/releases/download/v41.0.0/electron-v41.0.0-win32-x64.zip

Archive SHA256 (matches official SHASUMS256.txt): `2E69A07219B05B625B3A2631A3E025F2F7B9DDB7419C49F1A5623D04C0D74D91`.

| Asset | SHA256 |
|---|---|
| libEGL.dll | AE32B2441B4AB85CD100FDEAA9DCFD99B1DA15E8FD41C8DE78512C69CB96F596 |
| libGLESv2.dll | B4F6D2158D1BD27DED6C65884EFE26005C45E6D15373FF1230A5DF2B6D2DFE00 |
| vulkan-1.dll | 2D9509B6CAF660F6057BBEADADB8474E740772AB861C8489BADFAD18E76A4056 |
| LICENSES.chromium.html | C1BC6CFDD6C5844720E5E6332698A6131F5402E77ABF1FDB9646740D38065A65 |

Full upstream Chromium/ANGLE/Vulkan-loader notices are retained and embedded in the EXE (--licenses). Assets are unmodified. bootstrap_angle.ps1 restores them from the hash-checked official archive. No network is required during normal builds with assets present.

ANGLE maps this project's GLES3-compatible calls/shaders to hardware Vulkan. GLFW explicitly requests the Vulkan platform; activation checks the driver, client API and native VkDevice/VkPhysicalDevice handles. It is not a handwritten native Vulkan renderer, software fallback, or OpenGL-rendered frame copied to a Vulkan presenter.
