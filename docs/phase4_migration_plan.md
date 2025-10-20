# Phase 4 Migration Plan

## Objectives
- Replace the existing `public/` and `private/` folder convention with the classic `include/` + `src/` layout.
- Relocate the public API tree (`include/corona/**`) to the repository root and minimise the surface area exposed to external clients.
- Simplify CMake targets by removing install rules and harmonising include directory declarations while keeping the build green.
- Allow temporary build breakage during the transition, but keep track of follow-up tasks.

## Module Inventory & Current Public Headers
| Module | Current Public Roots | Key Public Headers | External Consumers |
|--------|----------------------|--------------------|--------------------|
| `core` | `include/corona/core` (detail helpers under `include/corona/core/detail`) | `CoronaEngineAPI.h`, `Engine.h`, `detail/EngineKernel.h`, `SystemRegistry.h`, `detail/SystemHubs.h`, `components/*`, `events/*`, `testing/MockServices.h` | Runtime entry points, engine embedding, system registration, component/event definitions |
| `systems/animation` | `src/systems/animation/public` | `AnimationSystem.h` | Runtime loop and potential external plugins |
| `systems/audio` | `src/systems/audio/public` | `AudioSystem.h` | Runtime loop |
| `systems/display` | `src/systems/display/public` | `DisplaySystem.h` | Runtime loop |
| `systems/rendering` | `src/systems/rendering/public` | `RenderingSystem.h` | Runtime loop, custom render features |
| `systems/interface` | (empty) | — | Placeholder, can be merged into interfaces |
| `thread` | `include/corona/threading` | `SafeCommandQueue.h`, `SafeDataCache.h`, `EventBus.h` | Used by systems & engine |
| `utils` | `src/utils/public` | `compiler_features.h`, helper macros | Shared |
| `script` | `src/script/public` | `PythonAPI.h`, `PythonBridge.h` | Optional embedding |
| `engine` | mix of headers/sources, no dedicated public dir | `RuntimeLoop.h` | Executable entry |
| `src/include/corona/interfaces` | standalone | `Concurrency.h`, `ISystem.h`, `ServiceLocator.h`, `Services.h`, `SystemContext.h`, `ThreadedSystem.h` | Shared abstraction layer |

> _Note_: Several modules (e.g. `thread`, `utils`, `script`) still expose headers via `public/` subfolders; they follow the same pattern and will be addressed in later steps.

## Target Public Include Layout
Proposed unified tree under `include/` (relative to repository root):
```
include/
  corona/
    api/CoronaEngineAPI.h                # primary embedding API
    core/
      Engine.h                           # facade (minimal public wrappers)
      SystemRegistry.h                   # plugin interfaces
      components/ActorComponents.h
      components/SceneComponents.h
      events/ActorEvents.h
    interfaces/                          # existing abstraction layer (moved from src/include)
      Concurrency.h
      ISystem.h
      ServiceLocator.h
      Services.h
      SystemContext.h
      ThreadedSystem.h
    systems/                             # optional direct system access
      AnimationSystem.h
      AudioSystem.h
      DisplaySystem.h
      RenderingSystem.h
  threading/                           # relocated from src/thread/public (2025-10-20)
      SafeCommandQueue.h
      SafeDataCache.h
      EventBus.h
    utils/compiler_features.h
    testing/MockServices.h               # keep in-tree for now; may move to tests later
```
Additional notes:
- `EngineKernel.h`, `SystemHubs.h`, and other purely internal headers will migrate to `src/core/`; temporarily they reside under `include/corona/core/detail` until dependent systems stop including them directly.
- If we decide a header should **not** be part of the public API, we will relocate it under the relevant `src/` directory and adjust includes accordingly.
- The scripting headers can live under `include/corona/script/` if external projects rely on them; otherwise they can remain internal.

## High-Level Migration Steps
1. **Create root `include/` tree:** copy/move `src/include/corona` to `include/corona/interfaces` and update include paths.
2. **Core module cleanup:**
   - Move required public headers (`CoronaEngineAPI.h`, component/event definitions) into the new `include/corona` hierarchy.
  - Relocate internal-only headers (`EngineKernel.h`, `SystemHubs.h`, etc.) into `src/core/` and adjust includes (in progress: parked under `include/corona/core/detail` until downstream usage is untangled).
3. **Systems modules:** move each system's `public` headers into `include/corona/systems/` and flatten their source directories (merge `private` into `src`).
4. **Shared utilities (`thread`, `utils`, `script`):** migrate their public headers into `include/corona/threading`, `include/corona/utils`, and optionally `include/corona/script`.
5. **CMake harmonisation:** simplify each target's include directories to point to `include/` and its own `src/` tree; remove install-related logic while keeping dependency declarations intact.
6. **Prune dead/duplicate headers:** after the moves, audit for obsolete includes or files that can be eliminated.

## Immediate TODOs
- [x] Copy `src/include/corona/interfaces` to `include/corona/interfaces` and point existing targets to the new location.
- [x] Draft a mapping table for `src/core/public` headers to their new home (2025-10-20: Engine + components/events/testing moved into `include/corona/core/**`, detail helpers parked under `include/corona/core/detail`).
- [ ] Identify headers in `src/thread/public`, `src/utils/public`, and `src/script/public` that must stay public and assign future locations (2025-10-20: thread headers relocated to `include/corona/threading`; utils/script pending).
- [x] Update `docs/architecture_refactor_todo.md` with progress notes after each migration step (2025-10-20: include tree + CMake include paths adjusted).
- [ ] Run a CMake configure/build after each major batch of moves to ensure we keep track of breakages.

This document will be updated as we execute each step in Stage Four.
