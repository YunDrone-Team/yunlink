# YunLink 本体开发路线

这篇文档面向要修改 YunLink C++ core 的开发者。这里的“本体”指协议、runtime、session、authority、命令和状态流这些真实行为，而不是语言包装层。

## 先理解问题

语言 SDK 只是把能力暴露给 Rust、Python 或其他语言。真实的协议规则和运行时行为仍在 C++ core 里。

如果你发现一个功能在所有语言里都应该变化，通常不要先改 Rust 或 Python。你应该先判断这个变化是不是 C++ core 的行为变化。

## 本体开发的主要区域

| 区域 | 路径 | 什么时候改 |
| --- | --- | --- |
| 协议语义类型 | `include/yunlink/core/semantic/` | 新增消息语义、命令类型、状态类型 |
| 协议编解码 | `src/core/`, `include/yunlink/core/` | 修改线包、校验、解析行为 |
| runtime 行为 | `src/runtime/`, `include/yunlink/runtime/` | 修改连接、session、authority、命令发布、状态分发 |
| C ABI adapter | `src/c/abi/`, `include/yunlink/c/abi/` | 把已有 core 能力暴露给语言绑定 |
| 测试 | `tests/` | 证明行为、边界和回归 |

## 一个能力从 core 到语言层的路径

以 Goto 命令为例，开发路径大致是：

```text
CommandPublisher::publish_goto
  -> C ABI: yunlink_command_publish_goto
    -> Rust raw: yunlink_sys::yunlink_command_publish_goto
      -> Rust safe: Runtime::publish_goto
        -> UI 或业务代码
```

这个图说明一个重要原则：如果 C++ core 没有能力，语言绑定不应该自己伪造能力。语言绑定只应该翻译和包装 core 已有能力。

## 修改 core 时的步骤

### 1. 先写清楚行为变化

在改代码前，先回答三个问题：

- 这个变化影响协议字段，还是只影响 runtime 内部策略？
- 这个变化是否需要跨进程、跨语言暴露？
- 旧调用方遇到新字段或新状态时应该怎样退化？

如果答案涉及跨语言暴露，就必须继续看 [跨语言接口开发路线](c-abi-development.md)。

### 2. 修改 C++ 公开类型或 runtime 实现

常见路径：

- `include/yunlink/core/semantic/*`
- `include/yunlink/runtime/*`
- `src/runtime/*`
- `src/core/*`

如果只是修复内部行为，尽量不要同时扩大 C ABI。先把 core 行为稳定下来，再决定是否暴露。

### 3. 添加或更新 C++ 测试

测试应该放在最接近行为的位置：

- runtime lifecycle：`tests/runtime/`
- transport 行为：`tests/transport/`
- security / routing：`tests/security/`
- FFI contract：`tests/bindings/`

最小验证命令：

```bash
cmake --build build/ninja-debug
ctest --test-dir build/ninja-debug --output-on-failure
```

如果完整测试太慢，先跑相关测试，再在提交前跑完整回归。

## 什么时候需要同步 C ABI

出现下面任一情况，就需要同步 C ABI：

- Rust / Python / 未来语言 SDK 需要访问这个能力
- Advanced Monitor 或其他工具需要跨语言调用
- 这个能力属于外部集成场景，而不是纯内部实现细节

同步 C ABI 不是简单加一个函数名。你还需要考虑：

- 参数类型能不能被 C 稳定表达
- 字符串和 buffer 谁拥有
- struct 是否需要 `struct_size`
- 错误码是否已有表达
- 事件是否能通过 polling queue 表达
- 旧调用方遇到新字段是否安全

下一步读 [跨语言接口开发路线](c-abi-development.md)。

## 不建议做的事

- 不要让 Rust 或 Python 直接绑定 C++ facade。
- 不要在语言 SDK 里重新实现协议核心规则。
- 不要为了一个语言的便利破坏 C ABI 的稳定结构。
- 不要把 callback ABI 作为第一选择。当前项目约定是 `polling queue + language-side adapter`。
- 不要在一次改动里同时重构 core、扩 ABI、改 SDK、改 UI，除非有清晰测试闭环。

## core 改动完成的判断

一个 core 改动至少需要满足：

- 行为有 C++ 测试证明。
- 如果影响协议文档，`docs/protocol/` 已更新。
- 如果需要跨语言访问，C ABI 和语言绑定路线已同步。
- 如果改变错误码或事件语义，Rust / Python 的错误和事件映射已检查。
