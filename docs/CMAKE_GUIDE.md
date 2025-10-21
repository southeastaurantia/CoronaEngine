# CoronaEngine CMake Guide

This guide summarizes how the CoronaEngine repository is structured around CMake and how to extend or build it effectively.

## Quick Start
- Configure with a preset: `cmake --preset ninja-msvc` (Windows) or an appropriate preset from `CMakePresets.json`.
- Build with a preset: `cmake --build --preset msvc-debug` (or `msvc-release`, etc.).
- The multi-config Ninja generators keep all configurations under `build/`; Visual Studio presets generate solution files in `build/` as well.
- Run the runtime after a debug build: `build/*/Debug/*.exe`. Assets, Python DLLs, and other runtime files are copied automatically post-build.

## File Layout Overview
- `CMakeLists.txt`: orchestrates configuration. It extends `CMAKE_MODULE_PATH` with `misc/cmake`, includes the custom modules described below, and adds subdirectories in the order `src` -> `engine` (guarded by `BUILD_CORONA_RUNTIME`) -> `examples` (guarded by `CORONA_BUILD_EXAMPLES`).
- `CMakePresets.json`: defines cross-platform configure and build presets. The repository targets Ninja Multi-Config by default but offers Visual Studio 2022 and OS-specific presets.
- `src/CMakeLists.txt`: collects module libraries, creates the static library target `CoronaEngine`, and wires up runtime dependency collection and (optionally) editor resources.
- `engine/CMakeLists.txt`: builds the `Corona_runtime` executable, installs Helicon runtime DLLs (if available), copies Corona runtime dependencies, mirrors editor resources when enabled, and syncs the `assets/` directory.
- `examples/CMakeLists.txt`: declares helper functions and per-directory feature flags, wiring each example through `corona_add_example()` and optional resource copy.

## Custom CMake Modules (misc/cmake)
- `corona_options.cmake`: centralizes project-wide options. Booleans such as `BUILD_CORONA_RUNTIME`, `BUILD_CORONA_EDITOR`, `CORONA_BUILD_EXAMPLES`, and `CORONA_BUILD_VISION` live here. Defaults favor building the runtime and examples when the repository is top-level.
- `corona_python.cmake`: pins the embedded Python toolchain located under `third_party/Python-3.13.7`, configures dependency checking via `misc/pytools/check_pip_modules.py`, and exposes the `check_python_deps` custom target. Configuration fails fast if required packages are missing and auto-install is disabled.
- `corona_third_party.cmake`: enumerates FetchContent declarations for Corona-owned modules (CabbageHardware, CabbageConcurrent, CoronaLogger, CoronaResource, Vision) and upstream libraries (assimp, stb, EnTT, glfw). Features such as Vision or examples gate optional downloads.
- `corona_compile_config.cmake`: enforces C11/C++20, the MSVC runtime library policy, EnTT-related compile definitions, Python path macros, and UTF-8 charset flags.
- `corona_collect_module.cmake`: provides `corona_collect_module()` for gathering headers and sources from a module's `include/` and `src/` subdirectories and exports lists like `CORONA_<MODULE>_PUBLIC_HEADERS`. It also defines `corona_target_use_public_includes()` to attach the shared `include/` directory to targets.
- `corona_runtime_deps.cmake`: exposes `corona_configure_runtime_deps()` to record Python DLL and PDB artifacts onto `CoronaEngine` and `corona_install_runtime_deps()` to copy them next to executables via a Python helper or `copy_if_different` fallback.
- `corona_editor.cmake`: mirrors the runtime dependency pattern for CabbageEditor resources. When `BUILD_CORONA_EDITOR` is ON, it stores backend/frontend directories on `CoronaEngine` and injects a post-build copy step (with optional Node-based frontend build) into runtime or example targets.

## Key Targets
- `CoronaEngine`: static library aggregating all engine modules. It links module targets declared under `src/*` and exports public headers using `corona_target_use_public_includes()`.
- `Corona_runtime`: main executable that spins up the default runtime loop and consumes the runtime dependency copy pipeline.
- `minimal_runtime_loop`: example showing how to reuse `engine/RuntimeLoop.cpp` from outside the runtime target and how assets/runtime deps are staged for examples.

## Configuration Options
| Option | Default | Description |
| --- | --- | --- |
| `BUILD_SHARED_LIBS` | `OFF` | Build libraries as shared objects instead of static. |
| `BUILD_CORONA_RUNTIME` | `ON` | Adds the `engine/` runtime executable target. |
| `BUILD_CORONA_EDITOR` | `OFF` | Enables editor resource collection and post-build staging. |
| `CORONA_BUILD_EXAMPLES` | `ON` when top-level | Adds all example subdirectories and their feature toggles. |
| `CORONA_BUILD_VISION` | `OFF` | Fetches and links Corona Vision modules. |
| `CORONA_CHECK_PY_DEPS` | `ON` | Validates Python packages during configure. |
| `CORONA_AUTO_INSTALL_PY_DEPS` | `ON` | Allows the checker to install missing Python requirements. |
| `CORONA_PYTHON_USE_EMBEDDED_FALLBACK` | `ON` | Forces use of the bundled Python toolchain. |

Tune any option with `cmake --preset <configure-preset> -D<OPTION>=<value>` or override interactively when running `cmake -S . -B build`.

## Adding a New Engine Module
1. Create a directory under `src/<module>` with optional `include/` and `src/` subfolders.
2. Add a `CMakeLists.txt` that calls `corona_collect_module(<Name> ${CMAKE_CURRENT_SOURCE_DIR})`.
3. Use the generated `CORONA_<NAME>_PUBLIC_HEADERS` and `CORONA_<NAME>_PRIVATE_SOURCES` to populate a new `add_library()` target.
4. Link dependencies and call `corona_target_use_public_includes(<target>)` if the headers live under the shared `include/` tree.
5. Append the target to the list in `src/CMakeLists.txt` so `CoronaEngine` links it.

## Working With Examples
- Each folder under `examples/` can define `BUILD_CORONA_EXAMPLE_<FOLDER>` to toggle builds. The root CMake automatically declares these options based on directory names.
- Examples should call `corona_add_example()` to inherit runtime dependency staging, editor resource mirroring, and optional asset copying. Additional sources (such as `engine/RuntimeLoop.cpp`) can be attached via `target_sources()`.

## Runtime and Editor Asset Staging
- Python runtime files: collected during configure via `corona_configure_runtime_deps()` and copied with a generated dependency list or `copy_if_different` fallback.
- Assets directory: both the runtime and any example that passes `COPY_ASSETS` replicate `assets/` into the target output directory.
- Editor resources: when the editor is enabled, `corona_install_corona_editor()` runs a wrapper script that calls `misc/pytools/editor_copy_and_build.py` and (on Windows) uses the bundled Node runtime from `editor/CabbageEditor/Env`.

## Troubleshooting Tips
- If configuration fails on Python checks, re-run `cmake --build --preset <preset> --target check_python_deps` to see detailed output from `check_pip_modules.py`.
- FetchContent pulls repositories at configure time; clear `build/_deps/` if you need to force updates.
- The current `cmake_minimum_required` is set to 4.0 to match the CMake preset header. Ensure you are running a recent CMake version (3.28+) even though the file advertises 4.0.
- Multi-config Ninja stores all configurations in one tree. Use `cmake --build --preset msvc-release --config Release` if your CMake version requires `--config`.

## Conventions Recap
- Prefer wrapping new helper logic into modules under `misc/cmake/` for reuse.
- Stage all runtime copies with the provided Python helpers to avoid redundant writes.
- Keep project options declared in `corona_options.cmake` and document new switches in this guide.
