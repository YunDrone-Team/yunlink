# Local Verification

## Version metadata correction

The current CMake and Python package versions are 2.0.3. Release verification
checks wheel filenames, embedded metadata, CMake version and the release tag.
The historical `v2.0.2` tag at `a8f1ebc` retains its original
source, which declared 2.0.1; the tag is not moved or republished. This is a
metadata correction, not a protocol or ABI change: `SOVERSION` remains 2 and
the Rust crate versions remain unchanged.

Run Core with Profiles disabled and enabled:

```bash
cmake --preset ninja-debug
cmake --build --preset ninja-debug
ctest --test-dir build/ninja-debug --output-on-failure

cmake --preset ci-ninja-profiles
cmake --build --preset ci-ninja-profiles
ctest --test-dir build/ci-ninja-profiles --output-on-failure
```

Run language bindings and architecture guards:

```bash
cargo test --workspace --manifest-path bindings/rust/Cargo.toml
tools/bindings/run_all.sh
python3 tests/test_protocol_boundaries_v2.py
python3 tests/test_python_wheels.py
```

The Profile-enabled C++ build requires a system Protobuf installation. Rust
uses its vendored compiler. Platform or simulation validation is a separate
integration gate and must not be inferred from these local checks.
