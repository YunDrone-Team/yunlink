# yunlink_rust_advanced_monitor

Rust/egui teaching prototype for the Advanced Monitor.

The important shape is:

```text
egui UI -> runtime_client -> safe yunlink crate -> yunlink-sys -> C ABI -> C++ core
```

The UI intentionally includes an `ABI` page that shows the mapping between safe
Rust calls, raw `yunlink-sys` symbols, exported C ABI functions, and the C
structs crossing the language boundary.

## Build

```bash
cargo build --manifest-path tools/yunlink_rust_advanced_monitor/Cargo.toml
```

## Run

```bash
cargo run --manifest-path tools/yunlink_rust_advanced_monitor/Cargo.toml -- \
  --remote-ip=127.0.0.1 \
  --remote-tcp-port=14130 \
  --udp-bind-port=14131 \
  --udp-target-port=14131 \
  --tcp-listen-port=14231
```

For a local demo, start an air peer first:

```bash
cargo run -p yunlink --example air_roundtrip --manifest-path bindings/rust/Cargo.toml -- \
  14030 14030 14130
```

Then start this monitor with `--remote-tcp-port=14130`.

## Current Scope

- Runtime start, peer connect, session open
- Authority request/release
- Takeoff, Land, Return, Goto, Velocity command publishing
- Command result and VehicleCoreState event display
- ABI explanation page

System service, discovery, and rich Sunray snapshots are represented in the UI
as planned pages. They require additional C ABI surface before they can be wired
without bypassing the translation layer.
