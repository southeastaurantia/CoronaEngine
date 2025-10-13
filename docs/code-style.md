# Code Style

## Formatting
- The repository uses `.clang-format` with `BasedOnStyle: Google`, `IndentWidth: 4`, and tabs disabled.
- Set your editor to run clang-format on save in C++ sources to avoid drift; manual fallback:

```bash
clang-format -i path/to/file.cpp
```

- No line length limit is enforced (`ColumnLimit: 0`), but maintain readability by wrapping thoughtfully.

## Naming
`.clang-tidy` enforces Google readability naming rules with custom overrides:
- Classes, structs, interfaces, and enums use `CamelCase`.
- Free functions use `snake_case`.
- Variables and data members use `snake_case`; private members end with `_`.
- Constants (enum values, globals, members) use the `kCamelCase` prefix style.
- `main`-like functions are exempt from the naming check.

## Linting
- Clang-Tidy runs the `google-*` checks except for `google-build-using-namespace` and `google-readability-todo`.
- Integrate clang-tidy with your IDE or run manually:

```bash
clang-tidy path/to/file.cpp --config-file=.clang-tidy
```

- Address reported warnings or document intentional deviations with comments.

## Best Practices
- Keep headers self-contained; include what you use.
- Prefer engine subsystems over ad hoc utilities—extend existing modules when feasible.
- Document complex logic with focused comments; avoid obvious restatements.
- Update documentation and examples alongside behavioral changes.
