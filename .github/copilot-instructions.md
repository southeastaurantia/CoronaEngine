# CoronaEngine Copilot Guide

## Architecture & Core Components
- **Engine class** (`src/include/corona/engine.h`): Main orchestrator wrapping `Kernel::KernelContext::instance()` singleton. Manages system registration, lifecycle (initialize→run→shutdown), and exposes kernel services via `kernel()` accessor (logger, event_bus, system_manager).
- **Systems**: Located in `src/systems/src/<module>/`, each inherits from `CoronaFramework`'s `Kernel::SystemBase`. They are initialized based on priority.
  - **Priority Order**: Display(100) > Optics(90) > Geometry(85) > Animation(80) > Mechanics(75) > Acoustics(70).
  - Systems define priority via a `get_priority()` override and run on dedicated threads.
- **System registration**: Happens in `Engine::register_systems()` using `sys_mgr->register_system(std::make_shared<Systems::XxxSystem>())`. The system manager sorts by priority and handles the lifecycle.
- **Event definitions**: Strongly-typed event structs live in `src/include/corona/events/<system>_events.h`. Access the kernel's event bus via `kernel_.event_bus()` or `engine.event_bus()`.
- **Module structure**: Public headers are under `src/include/corona/`, system headers under `src/systems/include/corona/systems/`, and implementations under `src/systems/src/<module>/`. Follow existing patterns when adding new subsystems.
- **Planned architecture**: Aspirational patterns mention an `Engine::instance()` singleton, `SafeCommandQueue` hubs, `DataCacheHub`, `EventBusHub`, and a `CoronaEngineAPI` with `entt::registry`. Refer to these in forward-looking code but verify current `KernelContext` usage first.

## CMake Build System
- **Presets**: Use `cmake --preset ninja-msvc` (Windows) from `CMakePresets.json`. This supports multi-config Ninja, Visual Studio 2022, and Clang variants.
- **Build workflow**: 
  - Configure: `cmake --preset ninja-msvc`
  - Build: `cmake --build --preset msvc-debug` (or `msvc-release`/`relwithdebinfo`/`minsizerel`)
  - Output: `build/` directory with config-specific subdirectories.
- **Key options** (`misc/cmake/corona_options.cmake`): Toggle with `-D` flags:
  - `BUILD_CORONA_RUNTIME=ON`: Build the main executable.
  - `BUILD_CORONA_EXAMPLES=ON`: Build example programs.
  - `CORONA_CHECK_PY_DEPS=ON`: Validate Python dependencies at configure time.
  - `CORONA_AUTO_INSTALL_PY_DEPS=ON`: Auto-install missing Python packages.
- **Custom CMake modules** (`misc/cmake/`):
  - `corona_collect_module()`: Auto-discovers headers/sources from module subdirectories.
  - `corona_runtime_deps()`: Copies runtime dependencies (e.g., Python DLLs) to the target output directory.
  - Extend these helpers instead of adding ad-hoc copy logic.

## System Development Patterns
- **Lifecycle hooks**: Override `initialize(ISystemContext* ctx)`, `update()`, and `shutdown()` from `Kernel::SystemBase`. Set the system name via `get_name()` and priority via `get_priority()`.
- **Main loop**: `Engine::run()` starts all system threads via `sys_mgr->start_all()`, then ticks at 120 FPS. Systems execute their `update()` method on their dedicated threads. Call `request_exit()` to signal a graceful shutdown.
- **Event publishing**: Systems access `kernel_.event_bus()` to publish/subscribe to events. Define event structs in the appropriate `*_events.h` files.
- **Logging**: Access the logger via `kernel_.logger()`. Use methods: `info()`, `warning()`, `error()`. The logger may be null during early initialization, so check before use.

## Python Integration
- **Embedded Python**: Uses a bundled Python toolchain. Configuration is in `misc/cmake/corona_python.cmake`.
- **Dependencies**: Listed in `misc/pytools/requirements.txt`. Dependency checking and installation can be toggled via CMake options.

## Dependencies & Third-Party Libraries
- **Managed by FetchContent** (`misc/cmake/corona_third_party.cmake`):
  - **Core**: EnTT (ECS), ktm (math), nlohmann_json, spdlog.
  - **Windowing & Rendering**: glfw, volk, VulkanMemoryAllocator, glslang, SPIRV-Cross.
  - **Asset Loading**: assimp, stb.
- **CoronaFramework integration**: The engine depends on `CoronaFramework`'s `Kernel::KernelContext` for core services (system management, logging, event bus).

## Key Files Reference
- **Engine core**: `src/include/corona/engine.h`, `src/engine.cpp` - Main engine class and system orchestration.
- **System base**: `Kernel::SystemBase` interface (from CoronaFramework dependency).
- **System implementations**: `src/systems/src/{acoustics,animation,display,geometry,mechanics,optics}/*.cpp`.
- **System headers**: `src/systems/include/corona/systems/*.h`.
- **CMake entry**: `CMakeLists.txt` → includes modules from `misc/cmake/`.
- **Build presets**: `CMakePresets.json` - Definitions for configure/build presets.

## Common Workflows
1. **Add a new system**: Create the system class under `src/systems/src/<name>/`, inherit from `Kernel::SystemBase`, set its priority via `get_priority()`, and register it in `Engine::register_systems()`.
2. **Format code**: Run `./code-format.ps1` (PowerShell) to format staged C++ changes before committing.
3. **Rebuild after CMake changes**: Re-run `cmake --preset ninja-msvc` then `cmake --build --preset msvc-debug`.
