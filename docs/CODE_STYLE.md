# CoronaEngine Code Style

This document summarizes the formatting and naming rules enforced by our clang-format and clang-tidy configurations.

## Formatting (clang-format)
- Base style: Google.
- Line length: unlimited (`ColumnLimit: 0`). Break lines only when it improves readability.
- Indentation: four spaces; tabs are never used.
- Apply formatting with `./code-format.ps1` (add `-Check` to validate without modifying files).

## Naming (clang-tidy)
- Class, struct, interface, and enum types use `CamelCase`.
- Free functions use `snake_case`.
- Variables use `snake_case`.
- Data members use `snake_case_` (trailing underscore).
- Enum constants and constants use `kCamelCase`.
- Private members should still end with `_`, and the checker ignores `main`-like entry points.

## Linting (clang-tidy)
- Enabled check set: `google-*` with `google-build-using-namespace` and `google-readability-todo` disabled.
- Run clang-tidy through your preferred IDE integration or via CMake tools when reviewing non-trivial changes.

Keep commit diffs format-clean by running the formatter after edits and addressing clang-tidy findings before opening pull requests.