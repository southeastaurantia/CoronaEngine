# CoronaEngine Copilot Guide
## Architecture & Modules
- Engine singleton in include/corona/core/Engine.h orchestrates system registration, SafeCommandQueue hubs, DataCacheHub, and EventBusHub; prefer Engine::instance() as the single access point.
- Runtime entrypoints in engine/main.cpp and engine/RuntimeLoop.cpp register the default Animation/Rendering/Audio/Display systems and gate them based on entt tags; follow examples/minimal_runtime_loop for custom loops.
- Systems reside under src/systems/<module>; each derives from src/systems/interface/public/ThreadedSystem.h to run on a dedicated worker thread at a configurable FPS, while their public headers live under include/corona/systems.
- External clients talk through include/corona/api/CoronaEngineAPI.h which wraps a static entt::registry, exposes Actor/Scene handles, and mirrors state into engine events.
- Shared engine-wide utilities live in include/corona/threading (SafeCommandQueue/EventBusT/SafeDataCache) and include/corona/utils/compiler_features.h (cross-platform macros).
## Runtime Data Flow
- CoronaEngineAPI::Scene/Actor mutate the static registry and immediately publish strongly typed events via Engine::events<T>().publish(topic, payload).
- RenderingSystem (src/systems/rendering/src/RenderingSystem.cpp) subscribes to scene/actor topics, snapshots per-scene state, and drives RasterizerPipeline/ComputePipeline; mirror its queue draining and subscription teardown in new render features.
- AnimationSystem (src/systems/animation/src/AnimationSystem.cpp) iterates SafeDataCache entries for AnimationState and Model IDs captured via watch_state/watch_model; keep those id sets in sync with cache mutations.
- TransformUpdated events treat zero vectors as “no change”; when intentionally resetting transforms send explicit epsilon values or add dedicated flags.
- Asynchronous resource callbacks must re-dispatch onto the owning system queue (engine.get_queue(name()).enqueue(...)) before touching local state.
## Concurrency & Resources
- System constructors add their command queue with Engine::add_queue(name(), std::make_unique<SafeCommandQueue>()); tick() implementations cap queue work (e.g. RenderingSystem spins 128 jobs per frame).
- SafeCommandQueue::enqueue supports functors, member pointers, and shared_ptr overloads; keep captured state lightweight to avoid blocking the MPMC queue.
- SafeDataCache<T> (include/corona/threading/SafeDataCache.h) stores shared_ptr keyed by uint64_t; modify() locks per-id mutexes and safe_loop_foreach() retries ids that fail try_lock to avoid starving hot data.
- CoronaResource’s ResourceManager is exposed via Engine::resources(); wrap loads with ResourceId::from(...) and use load_once_async(...) to hand results back via system queues.
- EventBusT<T> issues a per-subscription queue; persist the Subscription with topic/id/queue and unsubscribe in onStop() just like RenderingSystem to prevent leaking consumers.
## Build & Configuration
- Configure with cmake --preset ninja-msvc (or other presets in CMakePresets.json); output lives in build/.
- Build via cmake --build --preset msvc-debug (Debug/Release/RelWithDebInfo/MinSizeRel) and cmake --install --preset <build-preset> when packaging assets.
- Global toggles live in misc/cmake/corona_options.cmake (BUILD_CORONA_RUNTIME, BUILD_CORONA_EDITOR, CORONA_BUILD_EXAMPLES); flip them with -D flags when configuring.
- Custom CMake helpers under misc/cmake/ manage FetchContent, runtime dependency staging, and editor resource packaging—extend them instead of sprinkling ad-hoc logic.
- examples/minimal_runtime_loop builds when CORONA_BUILD_EXAMPLES=ON and provides the quickest regression smoke test for system/event changes.
## Python & Tooling
- include/corona/script/PythonAPI.h embeds CPython with PyConfig and hot-reload tracking; setLeakSafeReload(true) or the env CORONA_PY_LEAKSAFE=1 to skip DECREF during dev reloads.
- PythonHotfix and EngineScripts (same directory) watch asset timestamps and mirror modified scripts into release bundles—ensure new script entry points update those lists.
- PythonBridge::set_sender (include/corona/script/PythonBridge.h) registers a main-thread dispatcher; AnimationSystem::send_collision_event shows queueing events through Engine::get_queue("MainThread") for Python consumption.
- Logging flows through corona_logger; configure LogConfig in main before Engine::init and use CE_LOG_* macros for consistent output across subsystems.
