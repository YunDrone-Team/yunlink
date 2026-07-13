# 从新增能力到多语言包装的完整流程

这篇文档是一份操作手册。它回答一个常见问题：如果我要新增一个能力，怎样从 C++ core 一路贯通到 C ABI、Rust SDK、Python SDK 和工具界面？

## 先判断这个能力属于哪一层

新增能力前，先问：

- 它只是 C++ runtime 内部行为吗？
- 它需要被 Rust / Python 使用吗？
- 它需要出现在 Advanced Monitor 或外部集成里吗？
- 它是一次性命令，还是持续状态，还是事件？

如果只影响 C++ 内部，就不一定需要 C ABI。
如果任何语言 SDK 或外部工具要用，就必须经过 C ABI。

## 推荐顺序

不要从 UI 开始。推荐顺序是：

```text
1. C++ core 行为
2. C++ 测试
3. C ABI 类型和函数
4. C ABI contract / loader / roundtrip 测试
5. Rust raw layer: yunlink-sys
6. Rust safe layer: yunlink
7. Python native extension
8. Python high-level API
9. 工具或 UI
10. 文档和检查清单
```

这个顺序的好处是每一层都只包装前一层已经存在的事实。

## 示例：新增一个命令

假设要新增 `hover` 命令。

### 1. C++ core

先在 core 或 runtime 的命令模型里表达 `hover`。

可能涉及：

```text
include/yunlink/core/semantic/command_types.hpp
include/yunlink/runtime/command.hpp
src/runtime/command/
```

同时添加 C++ runtime 测试。

### 2. C ABI

新增 payload：

```c
typedef struct yunlink_hover_command {
    float duration_s;
    float yaw_rad;
} yunlink_hover_command_t;
```

新增函数：

```c
YUNLINK_C_API yunlink_result_t
yunlink_command_publish_hover(yunlink_runtime_t* runtime,
                              const yunlink_peer_t* peer,
                              const yunlink_session_t* session,
                              const yunlink_target_selector_t* target,
                              const yunlink_hover_command_t* payload,
                              yunlink_command_handle_t* out_handle);
```

然后在 `src/c/abi/` 实现 adapter。

### 3. Rust raw layer

在 `yunlink-sys` 中添加：

- `yunlink_hover_command_t`
- `yunlink_command_publish_hover`
- 如果有新 command kind，添加常量

### 4. Rust safe layer

在 `yunlink` 中添加：

- `HoverCommand`
- `Runtime::publish_hover`
- command result 的 command kind 映射
- 单测或 runtime roundtrip 测试

### 5. Python

在 Python 中添加：

- native extension 方法
- `HoverCommand` dataclass
- `Runtime.publish_hover`
- `Runtime.publish_hover_async`
- Python 测试

### 6. UI / 工具

如果 Advanced Monitor 需要展示：

- 添加输入控件
- 添加 command history 文案
- 添加 ABI 映射页说明
- 确认 UI 只调用 safe SDK

## 示例：新增一个状态 snapshot

状态 snapshot 和命令不同。状态通常是 runtime 产生事件，语言层订阅。

推荐路径：

1. 在 C++ core 中定义状态语义。
2. 在 runtime 中发布状态。
3. 在 C ABI 中添加 state payload 或 event payload。
4. 在 `yunlink_runtime_poll_event` 中通过 tagged union 暴露。
5. 在 Rust `events.rs` 中解析成 Rust enum。
6. 在 Python `events.py` 中解析成 dataclass。
7. 在 UI 的 State page 中展示。

状态类改动尤其要注意：

- event type 是否够用
- union 字段是否读对
- 字段单位是否清楚
- 旧 SDK 遇到未知 event type 是否安全忽略

## 示例：新增一个查询类接口

查询类接口通常使用 `out_*` 参数。

推荐形式：

```c
YUNLINK_C_API yunlink_result_t
yunlink_session_describe(yunlink_runtime_t* runtime,
                         const yunlink_session_t* session,
                         yunlink_session_info_t* out_info);
```

查询输出 struct 通常应该带 `struct_size`。调用方先初始化 struct，C ABI 再填字段。

Rust safe 层通常返回：

```rust
Result<Option<SessionInfo>>
```

Python safe 层通常返回：

```python
SessionInfo | None
```

## 每一层的提交前自查

| 层 | 自查问题 |
| --- | --- |
| C++ core | 行为是否有测试？是否破坏旧语义？ |
| C ABI | 是否只追加新函数？struct 是否需要 `struct_size`？ |
| Rust sys | C header 和 `#[repr(C)]` 是否一致？ |
| Rust safe | 是否没有向业务层泄漏 raw pointer？ |
| Python native | C ABI 错误是否转成 Python 异常？ |
| Python high-level | sync / async 语义是否一致？ |
| UI | 是否只调用 safe SDK？ |
| 文档 | 是否说明新增能力、测试命令和限制？ |

## 最小验证矩阵

根据改动范围选择验证：

```bash
# C++ / C ABI
cmake --build build/ninja-debug
ctest --test-dir build/ninja-debug -R "test_c_ffi_(v1|contract|loader)" --output-on-failure

# Rust
cargo test -p yunlink --manifest-path bindings/rust/Cargo.toml
cargo build --manifest-path tools/yunlink_rust_advanced_monitor/Cargo.toml

# Python
python -m unittest discover -s bindings/python/tests
```

如果改动跨越 core、ABI 和两个语言 SDK，应该跑完整相关矩阵，而不是只跑最后一层。

## 完成标准

一个跨语言能力完成时，应该能回答：

- C++ core 行为在哪里？
- C ABI 函数和 struct 在哪里？
- Rust raw 和 safe 层在哪里？
- Python native 和 high-level 层在哪里？
- 哪些测试证明它能运行？
- 哪篇文档告诉下一个人如何使用或维护它？
