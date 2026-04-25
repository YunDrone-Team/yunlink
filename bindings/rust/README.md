# Rust Bindings Workspace

本 workspace 拆分为两层：

- `yunlink-sys`
  手写的 FFI declarations + CMake build/link 探测。
- `yunlink`
  Tokio-first 的高层 Rust API。
