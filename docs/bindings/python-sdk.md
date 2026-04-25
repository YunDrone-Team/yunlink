# Python SDK 指南

Python SDK 位于 `bindings/python/`，采用：

- `scikit-build-core`
- `nanobind`
- 纯 Python dataclass / queue / asyncio bridge

## 分层

- `_yunlink_native`
  低层 native extension，只桥接稳定 C ABI。
- `yunlink`
  高层 Python API，提供 dataclass 模型、同步接口、后台 polling 线程和 `asyncio` bridge。

## 公开模型

- `RuntimeConfig`
- `Runtime`
- `PeerConnection`
- `Session`
- `TargetSelector`
- `GotoCommand`
- `VehicleCoreState`
- `AuthorityLease`
- `CommandResultEvent`
- `VehicleCoreStateEvent`

## 使用流程

同步 API：

1. `Runtime.start(...)`
2. `connect(...)`
3. `open_session(...)`
4. `request_authority(...)`
5. `publish_goto(...)`
6. `subscribe()` 获取同步事件队列

异步 API：

- `subscribe_async()` 获取 `asyncio.Queue`
- `*_async()` 系列接口通过 `asyncio.to_thread()` 适配同步核心
- `renew_authority()` / `renew_authority_async()` 用于显式续租，不存在自动续租线程

## 异常模型

- native extension 抛出的 FFI 错误会统一映射为 `yunlink` 自己的异常层级。
- 基类是 `YunlinkError`。
- 当前已公开的稳定子类包括 `InvalidArgumentError`、`ConnectError`、`InvalidStateError`、`NotFoundError`。

## 线程与数据模型

- `Runtime.start(...)` 会启动后台 polling 线程。
- `close()` 会先停止 polling，再停止 native runtime，随后释放端口与线程资源。
- 同步 `queue.Queue` 与异步 `asyncio.Queue` 共用同一套 dataclass 事件模型。
- 异步桥接通过 `loop.call_soon_threadsafe(...)` 把 polling 线程中的事件投递到 `asyncio` 队列。
- 如果 polling 线程遇到 native 层异常，会向订阅者发出 `ErrorEvent(code=-1, ...)`，并可通过 `last_poll_error()` 读取最近一次错误。

## 本地验证

editable 安装：

```bash
python3 -m venv .venv
. .venv/bin/activate
python -m pip install -e bindings/python
python -m unittest discover -s bindings/python/tests
```

wheel smoke：

```bash
tools/bindings/build_python_wheel.sh
```
