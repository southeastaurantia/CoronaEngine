# Runtime Dependencies

## Engine Requirements
CoronaEngine is a native C++20 engine built as a static library plus a runtime executable. To run the produced binaries you need:
- A 64-bit OS (Windows, Linux, or macOS) that matches the build preset used during compilation.
- A C++ runtime compatible with the selected toolchain (MSVC, libstdc++, or libc++).
- Access to the assets shipped in `assets/`; the runtime looks for these relative to the executable or the install prefix.

## Third-Party Libraries
The build pulls and statically links the following components via CMake `FetchContent` (see `misc/cmake/corona_third_party.cmake`):
- `assimp` for model loading.
- `stb` headers for image utilities.
- `entt` for the entity-component-system core.
- `CabbageHardware`, `CabbageConcurrent`, and `CoronaResource` for engine subsystems maintained alongside CoronaEngine.
- `glfw` when `CORONA_BUILD_EXAMPLES` is enabled for windowing/input in examples.

These libraries are fetched at configure time into `build/_deps/` and built as part of the standard presets. No manual installation is required unless you want to override them with local copies.

## Platform Notes
- **Windows**: Visual C++ redistributables matching the compiler version must be installed on machines running MSVC-built binaries.
- **Linux**: Ensure the target has the same libc/glibc baseline as the build machine. Package managers usually supply compatible versions.
- **macOS**: Ship the application bundle with the generated `CoronaEngine` dylibs if you reconfigure to build shared targets.

## Optional Tools
- GPU debugging/profiling tools (RenderDoc, Nsight) can assist when developing renderer pipelines.
- The engine logs to file and console via `Corona::Logger`; ensure the target environment allows file writes for log output.
