# Rust SDK 指南

Rust SDK 位于 `bindings/rust/`，采用 workspace split：

- `yunlink-sys`
  手写 FFI 声明、CMake 探测和 `yunlink_ffi` 共享库链接。
- `yunlink`
  Tokio-first 的高层 API。

## 公开模型

- `RuntimeConfig`
- `Runtime`
- `PeerConnection`
- `Session`
- `TargetSelector`
- `GotoCommand`
- `VehicleCoreState`
- `AuthorityLease`
- `Event`

## 使用流程

1. `Runtime::start(config)`
2. `connect(ip, port)`
3. `open_session(peer, node_name)`
4. `request_authority(...)`
5. `publish_goto(...)`
6. `subscribe()` 接收 `CommandResult` / `VehicleCoreState`
7. `release_authority(...)`

## 异步策略

- API 以 Tokio 环境为主。
- 事件订阅通过 `tokio::sync::broadcast` 暴露。
- FFI polling 在线程中进行，再桥接到 Tokio channel。
- 没有内建自动恢复；断链后由调用方显式执行 reconnect / reopen / reacquire。
- authority 续租通过 `renew_authority(...)` 显式触发，不存在自动续租后台任务。

## 错误模型

- FFI 错误统一映射为 `YunlinkError::Ffi(FfiErrorCode)`。
- `FfiErrorCode` 覆盖 `InvalidArgument`、`ConnectError`、`RuntimeStopped`、`NotFound` 等稳定结果码。
- C 字符串参数中的 `NUL` 会映射为 `YunlinkError::Nul`。

## 背压与取消

- 事件订阅底层是 `tokio::sync::broadcast`，当前容量固定为 `64`。
- 慢消费者会收到 `RecvError::Lagged(_)`，这就是 Rust 侧 v1 的背压信号。
- 取消某个等待事件的 Tokio task 不会关闭 runtime，也不会影响其他订阅者继续消费。

## 链接加载

- `yunlink-sys` 通过仓库内 CMake 构建出的 `yunlink_ffi` 共享库完成链接。
- 本地验证命令会覆盖 macOS/Linux 的构建与测试路径。
- GitHub Actions 里的 `bindings-linux`、`bindings-macos`、`bindings-windows` job 负责三平台持续验证。

## 本地验证

```bash
cargo test -p yunlink --manifest-path bindings/rust/Cargo.toml
cargo build --examples -p yunlink --manifest-path bindings/rust/Cargo.toml
```
