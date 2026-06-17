# tools

This directory contains developer-facing tools for building, checking, testing,
documenting, and exercising `yunlink`.

Most commands are expected to be run from the repository root unless noted
otherwise.

## Build Helpers

- `build_fast.py`
  Builds a CMake preset with a consistent parallelism policy of
  `max(1, cpu_count - 1)`.

  ```bash
  python3 tools/build_fast.py --preset ninja-debug
  python3 tools/build_fast.py --preset ninja-debug --target lint
  ```

- `build_advanced_monitor.sh`
  Builds the main `yunlink` static library and the Qt advanced monitor with one
  command. Add `--run` to launch the monitor after a successful build.

  ```bash
  tools/build_advanced_monitor.sh
  tools/build_advanced_monitor.sh --run
  tools/build_advanced_monitor.sh --run -- --remote-ip=127.0.0.1 --remote-tcp-port=9696
  ```

- `build_rust_advanced_monitor.sh`
  Builds the Rust/egui advanced monitor prototype through Cargo. Add `--run` to
  launch it after a successful build.

  ```bash
  tools/build_rust_advanced_monitor.sh
  tools/build_rust_advanced_monitor.sh --run -- --remote-ip=127.0.0.1 --remote-tcp-port=9696
  ```

## Code Quality

- `run_clang_format.sh`
  Formats C++ source files in place according to `.clang-format`.

- `check_clang_format.sh`
  Checks C++ formatting without modifying files. Use this in CI or before a
  review when you want a non-mutating guard.

- `run_clang_tidy.sh`
  Runs clang-tidy over the configured build directory. It expects CMake
  configuration to have generated `compile_commands.json`.

  ```bash
  tools/run_clang_tidy.sh --build-dir build/ninja-debug
  ```

- `check_core_maxline.sh`
  Enforces the source size and fanout limits described by `.codex/maxline.json`.

- `check_codestyle.sh`
  Aggregates the main code quality checks:
  formatting check, clang-tidy, and maxline.

  ```bash
  tools/check_codestyle.sh --build-dir build/ninja-debug
  ```

## Documentation

- `render_protocol_diagrams.sh`
  Renders PlantUML protocol diagrams from `docs/diagrams/plantuml/src/` into
  `docs/diagrams/plantuml/svg/`.

  It uses `PLANTUML_BIN` when set, otherwise it looks for `plantuml` on `PATH`
  or at `/opt/homebrew/bin/plantuml`.

  ```bash
  tools/render_protocol_diagrams.sh
  ```

## Bindings

The `tools/bindings/` directory contains helpers for validating and packaging
the C ABI, Rust bindings, Python bindings, and cross-language interoperability.

- `tools/bindings/run_all.sh`
  End-to-end bindings validation entry point. It builds the FFI target, runs C
  ABI tests, builds Rust bindings, installs the Python package in a local venv,
  runs Python tests, runs the interop matrix, and builds the Python wheel.

- `tools/bindings/build_rust_bindings.sh`
  Builds and tests the Rust binding crates.

- `tools/bindings/build_python_wheel.sh`
  Builds the Python wheel package for distribution testing.

- `tools/bindings/run_interop_matrix.py`
  Runs the configured interop scenarios under `tools/bindings/scenarios/` and
  `tools/bindings/interop/`.

## Testing Infrastructure

The `tools/testing/` directory contains the staged test infrastructure for
dual-host, weak-network, performance, and report workflows. See
`tools/testing/README.md` for the detailed suite model and output layout.

Important subdirectories:

- `tools/testing/dual_host/`
  Dual-host deployment, bootstrap, suite execution, and log collection scripts.

- `tools/testing/netem/`
  Linux `tc netem` profile application and cleanup helpers for weak-network
  experiments.

- `tools/testing/perf/`
  Performance suite and metric collection helpers.

- `tools/testing/report/`
  Summary rendering for generated test artifacts.

## Advanced Monitor

The `tools/yunlink_advanced_monitor/` directory contains a Qt Widgets ground
station monitor built on top of `libyunlink`.

It is used to inspect runtime, session, link, authority, command, state, and
system-service behavior through a UI. See
`tools/yunlink_advanced_monitor/README.md` for build instructions, command-line
arguments, and current UI semantics.

The `tools/yunlink_rust_advanced_monitor/` directory contains a Rust/egui
teaching prototype for the same C ABI path. It is intentionally separate from
the Qt tool and should grow only through the safe Rust SDK plus `yunlink-sys`.

## Typical Workflows

Build and run the default test suite:

```bash
cmake --preset ninja-debug
python3 tools/build_fast.py --preset ninja-debug
ctest --test-dir build/ninja-debug --output-on-failure
```

Run code quality checks:

```bash
python3 tools/build_fast.py --preset ninja-debug --target lint
```

Validate bindings:

```bash
tools/bindings/run_all.sh
```

Render protocol diagrams:

```bash
tools/render_protocol_diagrams.sh
```

Build and run the advanced monitor:

```bash
tools/build_advanced_monitor.sh --run
```

Build and run the Rust advanced monitor prototype:

```bash
tools/build_rust_advanced_monitor.sh --run
```
