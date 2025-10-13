# CMake Guide

## Presets Overview
CoronaEngine ships with `CMakePresets.json` to streamline configuration and builds.

- `ninja-msvc`: Windows builds with MSVC and the Ninja Multi-Config generator.
- `ninja-clang`: Windows builds with LLVM clang/clang++.
- `vs2022`: Windows builds using the Visual Studio 2022 generator.
- `ninja-linux-gcc` and `ninja-linux-clang`: Linux builds targeting GCC or Clang.
- `ninja-macos`: macOS builds via Ninja Multi-Config.

Each preset inherits from the hidden `base` preset, which sets:
- `binaryDir` to `${sourceDir}/build`.
- `installDir` to `${sourceDir}/install`.
- `CMAKE_EXPORT_COMPILE_COMMANDS` to `ON` for tooling integration.

## Configuring
Run one of the configure presets (switch host-specific names) from the project root:

```powershell
cmake --preset ninja-msvc
```

or, for Linux Clang:

```bash
cmake --preset ninja-linux-clang
```

This stage downloads third-party dependencies with `FetchContent` and generates the Ninja files under `build/`.

## Building
Use a build preset that matches the previously selected configure preset:

```powershell
cmake --build --preset msvc-debug
```

Available configurations per toolchain: `Debug`, `Release`, `RelWithDebInfo`, and `MinSizeRel`.

## Adding Targets
- Core engine code lives in `src/` and is compiled into the `CoronaEngine` static library via `src/CMakeLists.txt`.
- The runtime executable in `engine/` links against `CoronaEngine` and owns the `main` entry point.
- Examples reside under `examples/`; add a new folder with its own `CMakeLists.txt` that calls `corona_add_example()`.
- Custom build logic is defined in `misc/cmake/`; prefer extending existing helper functions before creating new ones.

## Tips
- Regenerate the build files whenever `CMakeLists.txt` or dependencies change.
- Delete the `build/` folder or use a different binary directory if you need a clean configure.
- `cmake --install --preset <build-preset>` copies runtime assets and binaries into the preset’s `installDir`.
