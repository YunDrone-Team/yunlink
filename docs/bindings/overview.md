# Yunlink Bindings 架构总览

`yunlink` 的多语言绑定遵循统一原则：

- C++ core 继续承载协议编解码、runtime、会话、authority、命令与状态流。
- 稳定共享层是 `include/yunlink/c/yunlink_c.h` 暴露的 bindings-oriented C ABI v1。
- Rust / Python 只依赖这层 C ABI，不直接绑定 C++ facade。
- 事件模型统一为 `polling queue + language-side adapter`，不做跨语言 callback ABI。
- 第一阶段只覆盖地面站与机载计算机之间的最小闭环，不额外开放完整 codec SDK。
- authority 生命周期保持显式：`request -> renew(optional) -> release`，没有自动续租或隐藏恢复。

## 目录分工

- `bindings/ffi/`
  共享 ABI 的版本与命名说明。
- `bindings/rust/`
  Rust workspace，拆成 `yunlink-sys` 和高层 `yunlink`。
- `bindings/python/`
  Python package、native extension 和 wheel 构建入口。
- `bindings/js/`
  预留目录，当前不实现。
- `tests/bindings/`
  FFI 合约测试与 bindings 相关测试。
- `tools/bindings/`
  构建、wheel、interop matrix 脚本。

## FFI v1 约束

- opaque runtime handle
- typed peer / session / target / command handle
- typed command publish
- typed state / event / command-result poll
- explicit lifecycle and explicit recovery
- explicit authority renew without automatic lease management
- ABI version constant + shared-library naming + visibility macros

## 当前边界

- 没有自动重连
- 没有自动重建 session
- 没有自动续租 / 自动重新申请 authority
- 没有 JavaScript SDK
- 没有完整 codec API 对外暴露

这意味着语言层 API 会更透明，也要求上层业务明确感知连接、session 和 authority 的生命周期。
