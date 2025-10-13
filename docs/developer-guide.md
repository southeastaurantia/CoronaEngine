# CoronaEngine Developer Guide

## Getting Started
- Clone the repository and initialize submodules if present.
- Install a modern C++20 compiler toolchain; see [Runtime Dependencies](runtime-dependencies.md) for platform-specific notes.
- Generate a build directory using one of the CMake presets described in [CMake Guide](cmake.md).
- Build the engine library and the runtime executable via `cmake --build`.

## Repository Layout
- `src/`: Core engine code compiled into the `CoronaEngine` static library.
- `engine/`: Runtime executable that links `CoronaEngine` and hosts the main loop.
- `examples/`: Discoverable examples; create a new folder for new samples.
- `misc/cmake/`: Custom CMake modules and helper functions.
- `assets/`: Shared runtime assets bundled into examples and runtime as needed.

## Typical Workflow
1. Configure using a preset, e.g. `cmake --preset ninja-msvc`.
2. Build with `cmake --build --preset msvc-debug` or select a config preset in your IDE.
3. Add new engine features under `src/` and expose them through public headers when needed.
4. Prototype gameplay or systems inside a dedicated example under `examples/`.
5. Keep code style aligned with [Code Style](code-style.md) and run clang-tidy if your IDE does not do it automatically.

## Debugging Tips
- Enable verbose logging through `CE_LOG_*` macros for engine subsystems.
- Use the `Debug` configuration for assert and log-heavy builds; `RelWithDebInfo` is a good compromise when profiling.
- Remember that the engine is modular—modify systems in isolation and add coverage via examples.

## Contribution Checklist
- Format your code before committing; see [Code Style](code-style.md).
- Add or update documentation in `docs/` when introducing new subsystems or workflows.
- Provide a sample or update an existing example if you change observable behaviour.
- Ensure unit tests (if present) and sample builds pass across supported presets.
