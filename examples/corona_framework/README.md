# Corona Framework Example

This directory hosts a full, buildable sample that showcases the standalone Corona Framework APIs: dependency injection, the messaging hub, and the runtime coordinator running systems on dedicated worker threads.

## Layout

- `main.cpp` – A console executable that wires two threaded systems (`beta.system` and `alpha.system`) via the framework runtime and messaging hub.
- `sample_manifest.json` – The manifest consumed by the example; it outlines system dependencies, factories, and tick intervals.

## Building

1. Configure the project with examples enabled (the default):
   ```powershell
   cmake --preset ninja-msvc
   ```
2. Build the example target:
   ```powershell
   cmake --build --preset msvc-debug --target corona_framework_example
   ```

The build step copies `sample_manifest.json` next to the generated executable so the runtime coordinator can discover it at launch.

## Running

Execute the sample from the build tree (path may vary per preset):

```powershell
./build/msvc-debug/examples/corona_framework/corona_framework_example.exe
```

Expected output (trimmed):

```
[beta.system] ready on thread 11660
[alpha.system] subscribed to demo.counter on thread 16972
[beta.system] published 1 on thread 11660
[alpha.system] observed 1 on thread 16972
...
[alpha.system] threshold met; shutting down
[beta.system] reached target
Example completed; total events published: 5
```

The log demonstrates how `beta.system` publishes events through the hub while `alpha.system` listens on its own worker thread, enforcing dependency order from the manifest and highlighting the runtime's multi-threaded execution.
