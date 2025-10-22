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

The example initialises the logging service with both console and file sinks. Log lines will appear in the terminal and in `corona_framework_example.log` under the current working directory. Adjust this behaviour by editing the `logging::logging_config` passed to `register_logging_services` in `main.cpp`.

Expected output (trimmed):

```
2025-08-05T08:07:06.123 [INFO] [tid=11660] [beta.system] ready
2025-08-05T08:07:06.124 [INFO] [tid=16972] [alpha.system] subscribed to demo.counter
2025-08-05T08:07:06.174 [INFO] [tid=11660] [beta.system] published 1
2025-08-05T08:07:06.174 [INFO] [tid=16972] [alpha.system] observed 1
...
2025-08-05T08:07:07.022 [INFO] [tid=16972] [alpha.system] threshold met; shutting down
2025-08-05T08:07:07.023 [INFO] [tid=11660] [beta.system] reached target
2025-08-05T08:07:07.124 [INFO] [tid=11660] [example] completed; total events published: 5
```

The log demonstrates how `beta.system` publishes events through the hub while `alpha.system` listens on its own worker thread. Each log line includes the worker thread identifier, highlighting the runtime's multi-threaded execution and the shared logging service.
