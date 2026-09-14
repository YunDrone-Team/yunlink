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

GitHub Actions builds native wheels for CPython 3.10 through 3.13 on these
platforms:

- macOS arm64
- macOS x86_64
- Linux x86_64 (manylinux)
- Linux arm64/aarch64 (manylinux)
- Windows x86_64

Every branch and pull request build stores the wheels as workflow artifacts.
Pushing a `v*.*.*` tag verifies the same matrix before attaching all wheels to
the GitHub Release. The matrix verifier rejects missing Python versions, wrong
architectures, and non-manylinux Linux wheels.

Profile packages are separate from the generic runtime facade so applications
can choose which schemas they compile or import.
