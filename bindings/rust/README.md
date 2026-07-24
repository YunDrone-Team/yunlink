# Rust Workspace

- `yunlink-sys`: ABI 2 declarations and native build/link integration.
- `yunlink`: owned safe facade with generic family events.
- `yunlink-profiles`: generated Mobility and Sunray Protobuf messages.

The safe facade copies every callback-lifetime view and payload. Test all three
crates with:

```bash
cargo test --workspace --manifest-path bindings/rust/Cargo.toml
```
