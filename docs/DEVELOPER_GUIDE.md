# CoronaEngine Developer Guide

This guide captures key workflows, conventions, and tips for extending or contributing to CoronaEngine.

## Core Concepts
- **Engine Singleton**: Access the engine through `Engine::instance()` (see `include/corona/core/Engine.h`). It coordinates system registration, command queues, the data cache hub, and event buses. Avoid creating additional engine instances.
- **Systems**: Each system derives from `include/corona/interfaces/ThreadedSystem.h` and runs on its own worker thread. Systems live under `src/systems/<module>`; their public headers are under `include/corona/systems`. Use `Engine::add_queue(name(), std::make_unique<SafeCommandQueue>())` to register per-system queues.
- **Data Flow**: Scene and actor operations go through `CoronaEngineAPI` (in `include/corona/api/CoronaEngineAPI.h`). This manipulates a static `entt::registry` and publishes events using `Engine::events<T>().publish(topic, payload)`. Rendering and animation systems listen on these topics to stay in sync.
- **Thread Safety**: When a resource callback operates off-thread, dispatch any engine mutations back onto that system’s queue (e.g., `engine.get_queue(name()).enqueue(...)`) to avoid touching local state from the wrong thread.

## Working With SafeCommandQueue
- Prefer capturing lightweight context when enqueueing lambdas to avoid blocking the MPMC queue.
- `SafeCommandQueue::enqueue` supports functors, member pointers, and `shared_ptr` overloads—pick the cheapest option for your workload.
- Systems typically limit the number of jobs drained per tick (RenderingSystem uses 128). Keep similar caps to avoid starving other tasks.

## SafeDataCache Usage
- Defined in `include/corona/threading/SafeDataCache.h`, storing `shared_ptr` keyed by `uint64_t`.
- `modify()` locks per-id mutexes; use it to mutate data safely.
- `safe_loop_foreach()` retries IDs when `try_lock` fails. Ensure your loops tolerate relocking to avoid starving hot data.

## Events and Transform Rules
- Transform updates treat zero vectors as “no change”. When resetting transforms intentionally, send epsilon values or append explicit flags to communicate zeroing intent.
- Always unsubscribe event handlers in `onStop()` using their stored subscription handles to prevent leaks.

## Resource Management
- Use `Engine::resources()` to access the `CoronaResource` manager.
- Wrap loads with `ResourceId::from(...)` and prefer `load_once_async(...)` to get callbacks scheduled onto system queues.
- When adding new resource types, follow the `CoronaResource` patterns so runtime dependency staging continues to work.

## Python Integration
- Python embedding lives under `include/corona/script`. Prefer `PythonBridge::set_sender` to register main-thread dispatchers; see `AnimationSystem::send_collision_event` for queueing events into the main thread for Python consumption.
- The embedded interpreter uses configuration from `misc/cmake/corona_python.cmake`. When adjusting Python scripts or hotfix pipelines, update `PythonHotfix` and `EngineScripts` lists to include new entry points.

## Logging
- Use the `corona_logger` facility with `CE_LOG_*` macros. Configure logging early in `main` before calling `Engine::init` to ensure consistent output.

## Build and Tooling Highlights
- Configure builds using presets defined in `CMakePresets.json` (e.g., `cmake --preset ninja-msvc`).
- `code-format.ps1` formats staged C++ changes via clang-format. Run `./code-format.ps1 -Check` in CI or pre-commit workflows.
- Enable editor resource staging with `-DBUILD_CORONA_EDITOR=ON`; this configures extra copy steps via `corona_editor.cmake`.
- The minimal example `examples/minimal_runtime_loop` is a quick smoke-test for system/event changes when `CORONA_BUILD_EXAMPLES=ON`.

## Contribution Workflow
1. **Branching**: Create feature branches from `main`. Keep changes scoped to a focused chunk of functionality.
2. **Formatting**: Run `./code-format.ps1` before committing to keep style consistent.
3. **Testing**: Use the minimal runtime loop example or custom smoke tests before opening a PR. Include new unit or integration tests when adding features.
4. **Documentation**: Update `docs/CMAKE_GUIDE.md`, this developer guide, or inline comments when workflows change. Keep comments concise and meaningful.
5. **Pull Requests**: Provide a summary, list behavioral changes, and describe tests executed. Tag subsystem owners when touching critical modules (rendering, threading, resource management).

## Debugging Tips
- Use Visual Studio or RenderDoc to debug rendering systems. The rendering system snapshots state derived from scene subscriptions—verify event publications when troubleshooting.
- For threading issues, enable logging around `SafeCommandQueue` usage to detect dropped jobs or slow drains.
- To isolate Python integration problems, run the embedded interpreter with `CORONA_PY_LEAKSAFE=1` to disable DECREF during hot reload and reduce noise.

## Adding New Systems
1. Define a new system under `src/systems/<module>` inheriting from `ThreadedSystem`.
2. Register the system in the runtime loop (`engine/RuntimeLoop.cpp`) and ensure it has a dedicated queue via `Engine::add_queue`.
3. Mirror the subscription and teardown patterns in existing systems (like `RenderingSystem`) so events and queues are managed cleanly.
4. Update `include/corona/systems` with the public interface header and expose any necessary API through `CoronaEngineAPI`.
5. Add tests or examples that exercise the new system, possibly through the minimal runtime loop example.

## Style Notes
- Stick to English for comments and log strings for consistency.
- Keep comments focused on rationale; avoid restating the obvious.
- Prefer modern C++20 features available in the codebase (structured bindings, smart pointers, `std::optional`) where they improve clarity.

Stay aligned with the architecture described in `include/corona/core/Engine.h` and the specialized modules under `src/`. When in doubt, check similar systems or consult the CoronaEngine maintainers for best practices.
