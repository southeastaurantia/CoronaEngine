# 代码风格

## 格式化
- 仓库采用 `.clang-format`，基于 Google 风格，缩进宽度 4，禁用 Tab。
- 建议在编辑器中开启保存即格式化；手动格式化示例：

```bash
clang-format -i path/to/file.cpp
```

- 未设置最大列宽（`ColumnLimit: 0`），请根据可读性自行换行。

## 命名
`.clang-tidy` 启用了 Google 可读性命名规则并做了本地定制：
- 类、结构体、接口、枚举采用 `CamelCase`。
- 自由函数使用 `snake_case`。
- 变量与成员变量使用 `snake_case`，私有成员以 `_` 结尾。
- 常量（枚举值、全局或成员）采用 `kCamelCase` 前缀风格。
- `main` 类函数被排除在命名检查之外。

## 静态检查
- clang-tidy 运行 `google-*` 检查，排除了 `google-build-using-namespace` 与 `google-readability-todo`。
- 可在 IDE 中集成或手动执行：

```bash
clang-tidy path/to/file.cpp --config-file=.clang-tidy
```

- 处理报告的告警，如需忽略请在代码中注明原因。

## 最佳实践
- 头文件保持自包含，只包含实际需要的头。
- 优先使用引擎现有子系统，避免重复造轮子。
- 对复杂逻辑撰写简洁注释，避免描述显而易见的操作。
- 更改行为时同步更新文档与示例。
