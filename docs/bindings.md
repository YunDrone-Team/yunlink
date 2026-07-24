# Language Bindings

The C ABI version is 2 and exposes generic runtime operations. It uses
callback-lifetime string, `TypeRef`, target, and byte-slice views. A callback
must not retain any view after returning.

Rust and Python copy all callback fields and payload bytes immediately. Their
public events are grouped by generic family (`Stream`, `Action`, `Rpc`, and so
on); they do not expose fixed product unions.

Rust:

```bash
cargo test --workspace --manifest-path bindings/rust/Cargo.toml
```

Python source tests require the built `yunlink_ffi` library. The full helper
creates an isolated environment, tests the runtime, and verifies the wheel:

```bash
tools/bindings/run_all.sh
```

Profile packages are separate from the generic runtime facade so applications
can choose which schemas they compile or import.
