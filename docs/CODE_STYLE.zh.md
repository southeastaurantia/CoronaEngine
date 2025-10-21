# CoronaEngine 代码风格指南

本文基于仓库中的 clang-format 与 clang-tidy 配置文件，总结工程使用的格式与命名规范。

## 格式化（clang-format）
- 基础风格：Google。
- 行宽：不限（`ColumnLimit: 0`），仅在确实提升可读性时换行。
- 缩进：四个空格，不使用制表符。
- 推荐通过 `./code-format.ps1` 执行格式化；使用 `-Check` 参数可以只校验不修改文件。

## 命名约定（clang-tidy）
- 类、结构体、接口、枚举类型：`CamelCase`。
- 自由函数：`snake_case`。
- 变量：`snake_case`。
- 成员变量：`snake_case_`（结尾带下划线）。
- 枚举常量、常量：`kCamelCase`。
- 私有成员仍需 `_` 结尾，检查器会忽略类似 `main` 的入口函数。

## 静态检查（clang-tidy）
- 启用了 `google-*` 规则集，同时排除 `google-build-using-namespace` 与 `google-readability-todo`。
- 可在 IDE 集成或 CMake 构建链路中执行 clang-tidy，建议在提交复杂改动前修正警告。

请在提交前运行格式化，并解决 clang-tidy 报告的问题，以减少代码审查中的格式噪声。