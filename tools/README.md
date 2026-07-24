# Developer Tools

Run commands from the repository root.

## Build And Quality

- `build_fast.py`: builds a configured CMake preset with bounded parallelism.
- `run_clang_format.sh`: formats current C++ v2 sources and tests.
- `check_clang_format.sh`: checks formatting without writing files.
- `run_clang_tidy.sh`: checks translation units from a compile database.
- `check_core_maxline.sh`: enforces source length and directory fanout limits.
- `check_codestyle.sh`: runs the configured quality checks.

## Bindings

- `bindings/run_all.sh`: builds ABI 2, tests the Rust workspace, tests Python,
  and verifies the Python wheel.
- `bindings/build_rust_bindings.sh`: tests all Rust workspace crates.
- `bindings/build_python_wheel.sh`: builds, installs, and tests a wheel in an
  isolated environment.

Wire v1 scenarios, fixed vehicle examples, and the old advanced monitors are
not part of YunLink 2.0. Product integration tests belong in the product
adapter and consumer repositories.
