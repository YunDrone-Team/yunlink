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
- `TakeoffCommand`
- `LandCommand`
- `ReturnCommand`
- `GotoCommand`
- `VehicleCoreState`
- `AuthorityLease`
- `Event`
- `ConfigValue` / `ConfigFieldSchema` / `ConfigSnapshot`
- `ConfigurationResponse` / `ConfigurationHandle`

## 使用流程

1. `Runtime::start(config)`
2. `connect(ip, port)`
3. `open_session(peer, node_name)`
4. `request_authority(...)`
5. `publish_goto(...)`
6. `subscribe()` 接收 `CommandResult` / `VehicleCoreState`
7. `release_authority(...)`

配置资源使用 `configuration_resource_list/describe/get/patch/apply(...)` 发请求，通过 `subscribe_configuration()` 接收拥有所有权的响应。C ABI callback view 不会暴露到 safe SDK。

## 动作命令

`TakeoffCommand`、`LandCommand` 和 `ReturnCommand` 是无字段类型，只表达动作意图：

```rust
runtime.publish_takeoff(&peer, &session, &target, &TakeoffCommand).await?;
runtime.publish_land(&peer, &session, &target, &LandCommand).await?;
runtime.publish_return(&peer, &session, &target, &ReturnCommand).await?;
```

高度、速度和返航前盘旋时间不属于这三个 payload。底层 C ABI 会写入一个值为 `0` 的保留字节，调用方不应自行赋予该字节业务语义。

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

## 维护路线

如果你要扩展 Rust SDK，请读 [Rust 绑定开发路线](../developer/rust-bindings-development.md)。如果这次改动需要先新增 C ABI 函数或 struct，请先读 [跨语言接口开发路线](../developer/c-abi-development.md)。

## 本地验证

```bash
cargo test -p yunlink --manifest-path bindings/rust/Cargo.toml
cargo build --examples -p yunlink --manifest-path bindings/rust/Cargo.toml
```
