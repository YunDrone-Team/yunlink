# Python 绑定开发路线

这篇文档讲如何开发 YunLink 的 Python 绑定。Python 绑定也分成两层：native extension 和高层 Python API。

## 先看一个现象

Python 调用 C++ core 时，不能直接拿 C++ 对象当普通 Python 对象用。它需要一层 native extension，把 C ABI 能力桥接到 Python。

YunLink 当前使用 `nanobind` 编写 native extension，再用纯 Python 提供更舒服的 dataclass、同步队列和 asyncio 接口。

## Python 绑定分层

| 层 | 路径 | 职责 |
| --- | --- | --- |
| native extension | `bindings/python/src/ffi_module.cpp` | 连接 C ABI，管理 native runtime 指针，抛出 Python 异常 |
| Python model | `bindings/python/src/yunlink/types.py` | dataclass 类型和参数模型 |
| Python runtime | `bindings/python/src/yunlink/runtime.py` | 同步 API、后台 polling、asyncio bridge |
| Python errors | `bindings/python/src/yunlink/errors.py` | 错误层级 |
| Python events | `bindings/python/src/yunlink/events.py` | 事件模型 |
| tests | `bindings/python/tests/` | 合约和 runtime loop 测试 |

## native extension 的职责

native extension 是靠近 C ABI 的层。它应该做：

- 调用 `yunlink_*` C ABI 函数。
- 把 `yunlink_result_t` 转成 Python 异常。
- 管理 runtime 生命周期。
- 把 C ABI 输出结构体转换成 Python 可用值。

它不应该重新实现协议规则。协议规则仍然属于 C++ core。

## 高层 Python API 的职责

高层 Python API 面向使用者。它应该做：

- 提供 `Runtime.start(...)`。
- 提供同步方法，例如 `connect(...)`、`publish_goto(...)`。
- 提供异步方法，例如 `publish_goto_async(...)`。
- 用 dataclass 表达输入和事件。
- 用后台 polling 线程把 native event 分发给同步和异步订阅者。

## 新增 Python SDK 能力的流程

### 1. 确认 C ABI 已存在

先看：

```text
include/yunlink/c/abi/functions.h
include/yunlink/c/abi/types.h
```

如果 C ABI 不存在，先读 [跨语言接口开发路线](c-abi-development.md)。

### 2. 更新 native extension

在 `bindings/python/src/ffi_module.cpp` 里添加桥接方法。

要求：

- 不让 C++ exception 穿过边界。
- C ABI 错误统一转换成 Python 异常。
- 输出 handle 或事件要转换成 Python 可理解的数据。
- native runtime 仍然由 `RuntimeCore` 管理。

### 3. 更新 Python dataclass

如果新增命令、状态或事件，更新：

```text
bindings/python/src/yunlink/types.py
bindings/python/src/yunlink/events.py
```

dataclass 字段名应该优先表达业务含义，单位应该写在字段名或文档里，例如 `_m`、`_mps`、`_ms`。

### 4. 更新高层 Runtime

在 `bindings/python/src/yunlink/runtime.py` 里增加同步方法。如果需要异步接口，可以用 `asyncio.to_thread()` 包装同步核心。

当前设计不做自动恢复：

- reconnect 显式调用
- reopen session 显式调用
- authority renew 显式调用

不要在 Python 层偷偷增加自动续租或自动重连，除非核心设计和文档同步更新。

### 5. 更新导出入口

如果新增公开类型或函数，检查：

```text
bindings/python/src/yunlink/__init__.py
```

确保用户可以从 `yunlink` 包里导入。

### 6. 添加测试

常用验证：

```bash
python3 -m venv .venv
. .venv/bin/activate
python -m pip install -e bindings/python
python -m unittest discover -s bindings/python/tests
```

如果涉及 wheel：

```bash
tools/bindings/build_python_wheel.sh
```

## Python 绑定完成的判断

一个 Python 绑定改动完成时，应该满足：

- native extension 调用的是 C ABI，不是直接绑定 C++ facade。
- 高层 Python API 暴露了用户可用的方法。
- 同步和异步接口语义一致。
- 错误被转换成 `YunlinkError` 层级。
- 事件模型与 Rust SDK 的同类事件保持语义一致。
- `python -m unittest discover -s bindings/python/tests` 通过。
- 如果发布 wheel，wheel smoke 通过。
