# 运行时依赖

## 引擎需求
CoronaEngine 使用 C++20 构建为静态库与运行时可执行程序。运行生成的二进制需要：
- 与构建配置匹配的 64 位操作系统（Windows、Linux 或 macOS）。
- 与所用工具链对应的 C++ 运行时（MSVC、libstdc++、libc++）。
- 可访问 `assets/` 下的资源，运行时会根据可执行文件位置或安装前缀查找资源。

## 第三方库
通过 `FetchContent` 静态链接以下组件（详见 `misc/cmake/corona_third_party.cmake`）：
- `assimp`：模型加载。
- `stb`：图像与实用头文件。
- `entt`：实体组件系统。
- `CabbageHardware`、`CabbageConcurrent`、`CoronaResource`：引擎配套子系统。
- 当启用 `CORONA_BUILD_EXAMPLES` 时，示例会额外拉取 `glfw` 以提供窗口与输入支持。

上述依赖会在配置阶段下载至 `build/_deps/` 并随构建一起编译，无需手动安装。如需覆盖版本，可在本地提供替代路径。

## 平台说明
- **Windows**：运行 MSVC 构建的二进制需安装匹配版本的 Visual C++ 运行库。
- **Linux**：目标机器需具备与构建机兼容的 libc/glibc 版本，可通过包管理器安装。
- **macOS**：若改为构建动态库，需将 `CoronaEngine` 库文件与应用 Bundle 一同分发。

## 可选工具
- 推荐使用 RenderDoc、Nsight 等 GPU 调试工具辅助渲染管线开发。
- 引擎通过 `Corona::Logger` 输出日志，确保目标环境允许写入日志文件。
