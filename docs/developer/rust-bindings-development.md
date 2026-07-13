# Rust 绑定开发路线

这篇文档讲如何开发 YunLink 的 Rust 绑定。Rust 绑定分成两层：`yunlink-sys` 和 `yunlink`。

## 先看一个现象

C ABI 函数使用指针、固定 buffer、整数错误码和 C 布局结构体。这些形状适合跨语言，但不适合直接写业务代码。

Rust 需要先有一层能“碰到 C ABI”的 raw binding，再有一层把 raw binding 包成安全、清晰、符合 Rust 习惯的 API。

## 两个 crate 的分工

| crate | 路径 | 职责 |
| --- | --- | --- |
| `yunlink-sys` | `bindings/rust/yunlink-sys/` | raw FFI 声明、C struct、常量、构建和链接 `libyunlink_ffi` |
| `yunlink` | `bindings/rust/yunlink/` | safe SDK、错误映射、字符串转换、事件解析、runtime 生命周期管理 |

业务代码和 UI 应该依赖 `yunlink`，不要直接依赖 `yunlink-sys`。

## `yunlink-sys` 是什么

`yunlink-sys` 是原始绑定层。它故意保持接近 C：

- C 风格名字，例如 `yunlink_runtime_start`
- C 风格整数常量，例如 `YUNLINK_RESULT_OK`
- C 布局 struct，例如 `#[repr(C)] yunlink_runtime_config_t`
- raw pointer，例如 `*mut yunlink_runtime_t`
- `unsafe extern "C"` 函数声明

它的目标不是舒服，而是准确。

重要文件：

- `bindings/rust/yunlink-sys/build.rs`
- `bindings/rust/yunlink-sys/src/constants.rs`
- `bindings/rust/yunlink-sys/src/types.rs`
- `bindings/rust/yunlink-sys/src/events.rs`
- `bindings/rust/yunlink-sys/src/functions.rs`

## `yunlink` 是什么

`yunlink` 是 safe SDK。它把 raw ABI 变成 Rust 调用方更自然的 API。

它负责：

- 创建和销毁 runtime handle。
- 初始化 `struct_size`。
- 把 Rust `String` / `&str` 写入固定 C buffer。
- 把 C buffer 转回 Rust `String`。
- 把 raw result code 转成 `YunlinkError`。
- 把 C tagged union 事件解析成 Rust enum。
- 用 `Drop` 清理 runtime。

重要文件：

- `bindings/rust/yunlink/src/lib.rs`
- `bindings/rust/yunlink/src/runtime.rs`
- `bindings/rust/yunlink/src/runtime/commands.rs`
- `bindings/rust/yunlink/src/types.rs`
- `bindings/rust/yunlink/src/types/commands.rs`
- `bindings/rust/yunlink/src/events.rs`
- `bindings/rust/yunlink/src/error.rs`
- `bindings/rust/yunlink/src/ffi_util.rs`

## 新增 Rust SDK 能力的流程

### 1. 确认 C ABI 已经存在

先在 `include/yunlink/c/abi/functions.h` 找到对应函数。再确认 `src/c/abi/` 有实现和测试。

如果 C ABI 不存在，先读 [跨语言接口开发路线](c-abi-development.md)。

### 2. 更新 `yunlink-sys`

如果是函数，更新：

```text
bindings/rust/yunlink-sys/src/functions.rs
```

如果是 struct，更新：

```text
bindings/rust/yunlink-sys/src/types.rs
bindings/rust/yunlink-sys/src/events.rs
```

如果是 enum 或常量，更新：

```text
bindings/rust/yunlink-sys/src/constants.rs
```

要求：

- `#[repr(C)]` 必须和 C 头文件布局一致。
- 字段顺序必须一致。
- 整数宽度必须一致。
- `Default` 应该填好 `struct_size` 和零值 buffer。

### 3. 更新 safe 类型

在 `bindings/rust/yunlink/src/types.rs` 或 `types/commands.rs` 增加 Rust 友好的类型。

例如 C ABI 里可能是：

```rust
pub body_frame: u8
```

safe Rust 类型应该是：

```rust
pub body_frame: bool
```

safe 层负责转换。

### 4. 更新 Runtime 方法

在 `runtime.rs` 或 `runtime/commands.rs` 增加方法。方法内部可以使用 `unsafe` 调 raw ABI，但公开 API 应该尽量 safe。

原则：

- `unsafe` 范围尽量小。
- 所有 FFI result code 都走 `ensure(...)`。
- 所有 C 字符串都走 `CString` 或 `ffi_util`。
- raw handle 不暴露给业务代码。

### 5. 更新事件解析

如果新增事件类型，需要更新：

```text
bindings/rust/yunlink/src/events.rs
```

C ABI 事件是 tagged union。safe 层必须先看 tag，再读对应 union 字段。

### 6. 添加测试

最小测试命令：

```bash
cargo test -p yunlink --manifest-path bindings/rust/Cargo.toml
```

根据改动类型补测试：

- 错误映射：`bindings/rust/yunlink/src/lib.rs` 内的单元测试或独立测试。
- runtime 行为：`bindings/rust/yunlink/tests/`。
- roundtrip：已有 runtime loop 测试。

## Rust Advanced Monitor 的位置

Rust Advanced Monitor 位于：

```text
tools/yunlink_rust_advanced_monitor/
```

它的业务层应该只调用 safe `yunlink` crate。唯一可以引用 `yunlink-sys` 的地方是教学展示模块，例如 ABI 映射页。

当前分层：

```text
ui
  -> runtime_client
    -> yunlink
      -> yunlink-sys
        -> C ABI
```

如果 UI 里出现 raw pointer、`unsafe extern` 或 `yunlink_sys::yunlink_*` 直接业务调用，通常说明边界被破坏了。

## Rust 绑定完成的判断

一个 Rust 绑定改动完成时，应该满足：

- `yunlink-sys` 与 C header 同步。
- safe `yunlink` 暴露了业务可用 API。
- `unsafe` 没有泄漏到 UI 或普通调用方。
- 错误码和事件解析有测试覆盖。
- `cargo test -p yunlink --manifest-path bindings/rust/Cargo.toml` 通过。
- 如果影响 Rust monitor，`cargo build --manifest-path tools/yunlink_rust_advanced_monitor/Cargo.toml` 通过。
