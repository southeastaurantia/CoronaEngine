# CoronaEngine Copilot Guide

## Architecture & Core Components
- **Engine class** (`src/engine.h`): Main orchestrator wrapping `Kernel::KernelContext::instance()` singleton. Manages system registration, lifecycle (initialize→run→shutdown), and exposes kernel services via `kernel()` accessor (logger, event_bus, system_manager).
- **Systems**: Located in `src/systems/src/<module>/`, each inherits `Kernel::SystemBase` with priority-based initialization order: DisplaySystem(100) > RenderingSystem(90) > AnimationSystem(80) > AudioSystem(70). Systems define priority via `get_priority()` override and run on dedicated threads at configurable FPS via `set_target_fps()`.
- **System registration**: Happens in `Engine::register_systems()` using `sys_mgr->register_system(std::make_shared<Systems::XxxSystem>())`. The system manager sorts by priority and handles lifecycle.
- **Event definitions**: Strongly-typed event structs live in `src/include/corona/events/<system>_events.h`. Access kernel's event bus via `kernel_.event_bus()` or `engine.event_bus()`.
- **Module structure**: Public headers under `src/include/corona/`, system headers under `src/systems/include/corona/systems/`, implementations under `src/systems/src/<module>/`. Follow existing patterns when adding new subsystems.
- **Planned architecture**: Aspirational patterns mention `Engine::instance()` singleton, `SafeCommandQueue` hubs, `DataCacheHub`, `EventBusHub`, and `CoronaEngineAPI` with entt::registry—refer to these in forward-looking code but verify current `KernelContext` usage first.

## Runtime Data Flow (Planned)
- CoronaEngineAPI::Scene/Actor will mutate the static registry and publish strongly typed events via Engine::events<T>().publish(topic, payload).
- RenderingSystem will subscribe to scene/actor topics, snapshot per-scene state, and drive RasterizerPipeline/ComputePipeline; mirror queue draining and subscription teardown patterns.
- AnimationSystem will iterate SafeDataCache entries for AnimationState and Model IDs captured via watch_state/watch_model.
- TransformUpdated events treat zero vectors as "no change"; when intentionally resetting transforms send explicit epsilon values or add dedicated flags.
- Asynchronous resource callbacks must re-dispatch onto the owning system queue (engine.get_queue(name()).enqueue(...)) before touching local state.

## Concurrency & Resources (Planned)
- System constructors will add their command queue with Engine::add_queue(name(), std::make_unique<SafeCommandQueue>()); tick() implementations cap queue work (e.g. RenderingSystem spins 128 jobs per frame).
- SafeCommandQueue::enqueue supports functors, member pointers, and shared_ptr overloads; keep captured state lightweight to avoid blocking the MPMC queue.
- SafeDataCache<T> (include/corona/threading/SafeDataCache.h) stores shared_ptr keyed by uint64_t; modify() locks per-id mutexes and safe_loop_foreach() retries ids that fail try_lock to avoid starving hot data.
- CoronaResource's ResourceManager is exposed via Engine::resources(); wrap loads with ResourceId::from(...) and use load_once_async(...) to hand results back via system queues.
- EventBusT<T> issues a per-subscription queue; persist the Subscription with topic/id/queue and unsubscribe in onStop() to prevent leaking consumers.

## CMake Build System
- **Presets**: Use `cmake --preset ninja-msvc` (Windows) from `CMakePresets.json`. Supports multi-config Ninja, Visual Studio 2022, and Clang variants.
- **Build workflow**: 
  - Configure: `cmake --preset ninja-msvc`
  - Build: `cmake --build --preset msvc-debug` (or msvc-release/relwithdebinfo/minsizerel)
  - Output: `build/` directory with config-specific subdirectories
- **Key options** (`misc/cmake/corona_options.cmake`): Toggle with `-D` flags:
  - `BUILD_CORONA_RUNTIME=ON`: Build main executable
  - `BUILD_CORONA_EDITOR=OFF`: Editor resource staging
  - `BUILD_CORONA_EXAMPLES=ON`: Example programs (default when top-level)
  - `CORONA_BUILD_VISION=OFF`: Vision module features
  - `CORONA_CHECK_PY_DEPS=ON`: Validate Python dependencies at configure time
  - `CORONA_AUTO_INSTALL_PY_DEPS=ON`: Auto-install missing Python packages
  - `BUILD_SHARED_LIBS=OFF`: Static linking (default)
- **Custom CMake modules** (`misc/cmake/`):
  - `corona_collect_module()`: Auto-discovers headers/sources from module subdirectories
  - `corona_install_runtime_deps()`: Copies Python DLLs/PDBs to target output dir
  - `corona_configure_corona_editor()`: Collects editor backend/frontend assets
  - `corona_add_example()`: Standardizes example target creation with asset/dependency copying
  - Extend these helpers instead of adding ad-hoc copy logic

## Code Style & Formatting
- **Naming conventions** (enforced by clang-tidy):
  - Types (classes/structs/enums): `CamelCase`
  - Functions: `snake_case()`
  - Variables: `snake_case`
  - Members: `snake_case_` (trailing underscore)
  - Constants/enum values: `kCamelCase`
- **Formatting**: Based on Google style (`.clang-format`): unlimited `ColumnLimit`, 4-space indents, no tabs. Run `./code-format.ps1` to format staged C++ files; add `-Check` for validation only. Add `-All` to format entire codebase.
- **Namespaces**: Top-level `Corona`, systems under `Corona::Systems`, events under `Corona::Events`. Always close with comments: `}  // namespace Corona::Systems`
- **Comments**: Prefer English for consistency. Focus on "why" over "what". Chinese acceptable for internal impl details but English for public APIs.

## System Development Patterns
- **Lifecycle hooks**: Override `initialize(ISystemContext* ctx)`, `update()`, `shutdown()` from `Kernel::SystemBase`. Set name via `get_name()` and priority via `get_priority()`.
- **Main loop**: `Engine::run()` starts all system threads via `sys_mgr->start_all()`, then ticks at 60 FPS (`sleep_for(16ms)`). Systems execute `update()` on their dedicated threads. Call `request_exit()` to signal graceful shutdown.
- **Event publishing**: Systems access `kernel_.event_bus()` to publish/subscribe. Define event structs in appropriate `*_events.h` files following existing patterns.
- **Logging**: Access via `kernel_.logger()` which returns CoronaLogger pointer. Use methods: `info()`, `warning()`, `error()`. Logger may be null during early init—check before use.
- **Thread safety**: Each system runs on its own thread. Engine's main loop uses atomic flags (`exit_requested_`, `running_`, `initialized_`) for state management. Frame timing tracked with `std::chrono::high_resolution_clock`.

## Python Integration
- **Embedded Python**: Uses bundled `third_party/Python-3.13.7` toolchain. Configuration in `misc/cmake/corona_python.cmake` with dependency validation via `misc/pytools/check_pip_modules.py`.
- **Dependencies**: Listed in `misc/pytools/requirements.txt`. Toggle checking/auto-install via `CORONA_CHECK_PY_DEPS` and `CORONA_AUTO_INSTALL_PY_DEPS` options.
- **Script location**: Python API headers planned for `src/script/` (currently scaffolding). Future patterns: `PythonAPI.h`, `PythonBridge.h`, `PythonHotfix`, `EngineScripts`.
- **Leak-safe reload**: Set `CORONA_PY_LEAKSAFE=1` environment variable to skip DECREF during hot-reload debugging (planned feature).
- **Dependency check target**: Run `cmake --build --preset <preset> --target check_python_deps` for detailed Python dependency diagnostics.

## Dependencies & Third-Party Libraries
- **FetchContent managed** (`misc/cmake/corona_third_party.cmake`):
  - **Core**: EnTT (ECS), ktm (math), nlohmann_json, spdlog/CoronaLogger
  - **Rendering**: Vulkan ecosystem (volk, VulkanMemoryAllocator, glslang, SPIRV-Cross), stb (image loading)
  - **Windowing**: glfw
  - **Models**: assimp
  - **Corona modules**: CoronaLogger, CoronaResource, CabbageHardware, CabbageConcurrent
- **CoronaFramework integration**: The engine depends on CoronaFramework's `Kernel::KernelContext` for core services (system management, logging, event bus).

## Testing & Examples
- **Example structure**: Located in `examples/engine/` with `main.cpp` demonstrating basic runtime loop: create Engine, initialize, register signal handlers, run, shutdown.
- **Signal handling**: Examples use `std::signal()` to catch `SIGINT`/`SIGTERM` and call `engine.request_exit()` for graceful shutdown.
- **Running examples**: After build, executables are in `build/<config>/<preset>/` with assets and runtime dependencies auto-copied post-build.
- **Smoke testing**: `examples/engine/` serves as regression test—keep it buildable when making engine changes.

## Key Files Reference
- **Engine core**: `src/engine.{h,cpp}` - main engine class and system orchestration
- **System base**: CoronaFramework's `Kernel::SystemBase` interface (external dependency)
- **System implementations**: `src/systems/src/{animation,audio,display,rendering}/*.cpp`
- **System headers**: `src/systems/include/corona/systems/*.h`
- **Events**: `src/include/corona/events/*.h` - event struct definitions
- **CMake entry**: `CMakeLists.txt` → includes modules from `misc/cmake/`
- **Build presets**: `CMakePresets.json` - configure/build preset definitions
- **Documentation**: `docs/{DEVELOPER_GUIDE,CMAKE_GUIDE,CODE_STYLE,CPP_DEPENDENCY}.md`

## Common Workflows
1. **Add new system**: Create under `src/systems/src/<name>/`, inherit `Kernel::SystemBase`, set priority via `get_priority()`, register in `Engine::register_systems()`.
2. **Format code**: Run `./code-format.ps1` (PowerShell) to format all staged C++ changes before commit. Use `-Check` for CI validation.
3. **Rebuild after CMake changes**: Re-run `cmake --preset ninja-msvc` then `cmake --build --preset msvc-debug`. Clear `build/_deps/` if FetchContent needs refresh.
4. **Debug system initialization**: Check Engine's logger output for system registration sequence and initialization success/failure messages.
5. **Add build option**: Define in `misc/cmake/corona_options.cmake` with `option()`, add status message, document in `docs/CMAKE_GUIDE.md`.

## Migration Notes
- **Transitional architecture**: Code references both current `KernelContext`-based patterns and planned `Engine::instance()` singleton patterns with `SafeCommandQueue`, `EventBusT`, `SafeDataCache`. When implementing new features, verify which pattern is currently active.
- **Missing components**: References to `CoronaEngineAPI`, `Actor/Scene` handles, `entt::registry` wrapping, `RasterizerPipeline`, and specific async patterns represent future architecture—stub them when needed or coordinate with maintainers.
