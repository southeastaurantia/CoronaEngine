# CoronaEngine C++ Dependency Map

This document summarizes the relationships between the C++ targets, subsystems, and external libraries defined by the CoronaEngine CMake build and reinforced by the engine headers/implementations.

## Engine Aggregation
- **CoronaEngine** (`src/CMakeLists.txt`) is the static umbrella library exported to consumers. It links the core engine layers (`CoronaCore`, `CoronaCoreServices`, `CoronaCoreKernel`), every runtime system, utility headers, the threading primitives, and logging/resource helpers. External dependencies propagated through this target include `EnTT::EnTT`, `ktm`, `CoronaResource::Resource`, `Corona::Logger`, `cabbage::concurrent`, and (conditionally) `CabbageHardware`.
- **EngineFacade** (`include/corona/core/Engine.h`) wraps access to the static engine singleton. It wires:
  - `EngineKernel` for `ISystem` registration and lookup.
  - `SafeCommandQueue` hubs per system to support cross-thread dispatch.
  - `SafeDataCache` and `EventBusT` hubs exposed through `cache<T>()` and `events<T>()`.
  - `SystemRegistry` for metadata about registered systems.
  - Resource, logging, and command scheduler services routed through `CoronaCoreServices`.

The kernel expects systems derived from `ThreadedSystem` to be registered, each backed by a named command queue (`Engine::add_queue(name(), std::make_unique<SafeCommandQueue>())`).

## Internal Targets and Dependencies
```
CoronaEngine (STATIC)
├─ CoronaCore (STATIC)
│  ├─ corona::core::kernel  → corona::interfaces
│  ├─ corona::core::services → {corona::interfaces, corona::thread, CoronaResource::Resource, Corona::Logger}
│  ├─ corona::interfaces    (INTERFACE headers)
│  ├─ corona::utils         (INTERFACE headers + compiler_features)
│  ├─ External: ktm (math), EnTT::EnTT (entity component registry)
│  └─ Private deps: corona::thread, CoronaResource::Resource, Corona::Logger
├─ CoronaSystemAnimation (STATIC)
│  ├─ Public deps: corona::system::interface, corona::core, CoronaResource::Resource
│  └─ Private deps: corona::script::python (Python-driven animation hooks)
├─ CoronaSystemAudio (STATIC)    → corona::system::interface, corona::core
├─ CoronaSystemDisplay (STATIC)  → corona::system::interface, corona::core
├─ CoronaSystemRendering (STATIC)
│  ├─ Public deps: corona::system::interface, corona::core, CoronaResource::Resource, CabbageHardware
│  └─ Exposes rendering events (`SceneEvents.h`) to downstream consumers
├─ CoronaScriptPython (STATIC)
│  ├─ Public deps: corona::core, Corona::Logger, python313 import libs, Python include/library directories
│  └─ Exposes public headers for `EngineScripts`, `PythonAPI`, `PythonBridge`, and `PythonHotfix`
├─ CoronaSystemInterface (INTERFACE)
│  └─ Depends on corona::interfaces and Corona::Logger; provides shared system-layer logging hooks
├─ CoronaThread (INTERFACE)
│  ├─ Sources for `SafeCommandQueue`, `SafeDataCache`, `EventBus`
│  └─ Links against Corona::Logger and cabbage::concurrent for logging and lock-free primitives
├─ CoronaUtils (INTERFACE)       → publishes `include/corona/utils` headers
├─ CoronaInterfaces (INTERFACE)  → publishes `ISystem`, `ThreadedSystem`, `ServiceLocator`, etc.
└─ CoronaSystem hubs (INTERFACE/STATIC) aggregated through `src/systems/CMakeLists.txt`
```

### Runtime Executable
- **Corona_runtime** (`engine/CMakeLists.txt`) links `CoronaEngine` and `EnTT::EnTT`, then attaches runtime copy steps:
  - `corona_install_runtime_deps` copies Python DLL/PDB files collected from `CoronaEngine`.
  - `helicon_install_runtime_deps` (if provided) stages Helicon/Vulkan pieces.
  - `corona_install_corona_editor` mirrors editor frontend/backend assets when the editor build flag is enabled.
  - Assets are copied with `cmake -E copy_directory` into the target output folder.

### Examples
- `corona_add_example()` (in `examples/CMakeLists.txt`) standardizes example targets. Examples link `glfw`, `CoronaEngine`, and `cabbage::concurrent`, inheriting the same runtime dependency staging.

## External Libraries Introduced by CMake
- **CoronaResource::Resource**: resource loading and management (shared with runtime staging helpers).
- **Corona::Logger**: logging backend required across threading/system layers.
- **cabbage::concurrent**: lock-free concurrency primitives powering `SafeCommandQueue`.
- **CabbageHardware**: rendering back end (Vulkan/Helicon integration) used by `CoronaSystemRendering`.
- **EnTT::EnTT**: entity-component registry, surfaced through `CoronaCore` and consumed by systems and runtime examples.
- **ktm**: math utilities consumed by `CoronaCore` when available.
- **python313**: embedded interpreter for the scripting subsystem.
- **glfw**: window/input library linked by examples (only when `CORONA_BUILD_EXAMPLES` is enabled).
- **Vision**: optional vision module available behind the `CORONA_BUILD_VISION` option; when enabled, `corona_third_party.cmake` fetches and exposes the target for linking.

## Runtime Data & Control Flow
1. **System Registration**: `Engine::register_system<T>()` (see `Engine.h`) instantiates systems, ensures their queues exist, and calls `configure_system`. Each system derives from `ISystem`/`ThreadedSystem`, gaining access to the command queue API.
2. **Command Queues**: `SafeCommandQueue` provides multi-producer, single-consumer queues per system. Systems enqueue work for themselves or for other systems using `Engine::get_queue(name)`.
3. **Event Bus**: `Engine::events<T>()` returns the typed `EventBusT` for publishing strongly typed events. Rendering and animation subscribe to actor/scene topics propagated through `CoronaEngineAPI`.
4. **Data Cache**: `Engine::cache<T>()` yields the shared `SafeDataCache` for caching resources (e.g., animation state). The cache locks per ID and retries failed locks to prevent starvation.
5. **Resource Loads**: `Engine::resources()` gives access to the global `ResourceManager`. Systems use `ResourceId::from(...)` and `load_once_async(...)` to retrieve assets. Callbacks that touch system state must re-dispatch onto the owning system queue.
6. **Python Bridge**: The scripting module exposes `PythonBridge::set_sender` to route events onto the main thread (`Engine::get_queue("MainThread")`). `AnimationSystem::send_collision_event` demonstrates this pattern.
7. **Transforms & Events**: Transform updates treat zero vectors as "no change"; to reset transforms, send epsilon values or dedicated flags to avoid being ignored downstream.

## Configuration Options Influencing Dependencies
- `BUILD_CORONA_RUNTIME`, `BUILD_CORONA_EDITOR`, `CORONA_BUILD_EXAMPLES`, and `CORONA_BUILD_VISION` toggle whole modules and their dependent targets.
- `CORONA_PYTHON_USE_EMBEDDED_FALLBACK` forces use of the bundled Python distribution, ensuring `CoronaScriptPython` and runtime dependencies resolve consistently.
- `CORONA_CHECK_PY_DEPS` / `CORONA_AUTO_INSTALL_PY_DEPS` control whether Python requirements are validated during configure (handled by `corona_python.cmake`).

## Using This Map
- Use the target graph above when introducing new modules to ensure they link against the correct layer (interfaces → core → systems → runtime).
- When adding dependencies, prefer augmenting the relevant module CMake file rather than injecting libraries directly into `CoronaEngine`.
- If a new system requires external resources, update `corona_third_party.cmake` and mirror the staging approach in `corona_runtime_deps.cmake` / `corona_editor.cmake` as needed.

This documentation is derived from the current CMakeLists and representative headers (notably `include/corona/core/Engine.h`). Update it whenever new targets or cross-module dependencies are introduced.
