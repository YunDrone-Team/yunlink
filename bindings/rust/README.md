# Rust Bindings Workspace

本 workspace 拆分为两层：

- `yunlink-sys`
  手写的 FFI declarations + CMake build/link 探测。
- `yunlink`
  Tokio-first 的高层 Rust API。

面向使用者的说明见 [Rust SDK 指南](../../docs/bindings/rust-sdk.md)。

面向维护者的路线见 [Rust 绑定开发路线](../../docs/developer/rust-bindings-development.md)。如果你还不熟悉 C ABI 和 `yunlink-sys` 的关系，建议先读 [开发者文档入口](../../docs/developer/README.md)。
