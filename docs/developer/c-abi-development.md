# 跨语言接口开发路线

这篇文档讲如何开发 YunLink 的 C ABI。C ABI 是 C 风格的二进制调用约定，它让 Rust、Python 或其他语言可以调用 C++ core。

## 先看一个现象

Rust 不能直接理解一个 C++ 对象的内部布局。Python 也不能直接知道 C++ 模板、引用、异常和析构规则。

如果每个语言都直接绑定 C++ facade，维护成本会很高，出错方式也会很多。因此 YunLink 选择先暴露一层 C 风格接口。所有语言都只依赖这层接口。

这层接口就是 C ABI。

## C ABI 在仓库里的位置

| 内容 | 路径 |
| --- | --- |
| 基础宏和导出符号 | `include/yunlink/c/abi/base.h` |
| 错误码、枚举值 | `include/yunlink/c/abi/enums.h` |
| C layout 结构体 | `include/yunlink/c/abi/types.h` |
| C ABI 函数声明 | `include/yunlink/c/abi/functions.h` |
| 聚合入口头文件 | `include/yunlink/c/yunlink_c.h` |
| C ABI 实现 | `src/c/abi/` |
| C ABI 合约测试 | `tests/bindings/test_c_ffi_contract.cpp` |
| C ABI v1 行为测试 | `tests/bindings/test_c_ffi_v1.cpp` |
| 动态库加载测试 | `tests/ffi/test_c_ffi_loader.py` |

## C ABI 设计原则

### 1. 只暴露 C 能稳定表达的形状

C ABI 函数应该使用这些类型：

- 整数类型，例如 `uint8_t`、`uint32_t`、`uint64_t`
- 浮点类型，例如 `float`
- 指针，例如 `yunlink_runtime_t*`
- 固定大小的 struct
- 固定大小的 char buffer

不要在 C ABI 里暴露：

- C++ class
- C++ reference
- `std::string`
- `std::vector`
- exception
- 模板类型

### 2. 跨边界对象用 opaque handle

opaque handle 是“不透明句柄”。调用方只拿到一个指针，但不知道里面是什么。

例如 runtime：

```c
typedef struct yunlink_runtime yunlink_runtime_t;
```

Rust 侧会把它看成：

```rust
pub type yunlink_runtime_t = core::ffi::c_void;
```

调用方不能读这个对象，只能把它传回 C ABI 函数。

### 3. 可扩展 struct 要带 `struct_size`

`struct_size` 是调用方看到的结构体大小。C++ 实现可以用它判断调用方使用的是新版本还是旧版本。

适合带 `struct_size` 的 struct：

- 配置 struct
- 查询输出 struct
- 可能继续追加字段的 selector 或 option struct

不一定需要带 `struct_size` 的 struct：

- 永远固定的小 payload
- 协议已经固定的事件 payload

### 4. 字符串优先用固定 buffer

当前 ABI 倾向使用固定大小的 `char buffer`。这样可以避免跨语言分配和释放内存。

示例：

```c
char node_name[128];
char detail[256];
```

语言 SDK 负责把它转换成自己的字符串类型。

### 5. 普通事件使用 polling；短生命周期复杂 view 可使用受限 callback

callback ABI 会引入线程归属、异常传播、生命周期和语言运行时问题。当前项目采用：

```text
C++ runtime event queue
  -> C ABI poll function
    -> language-side adapter
```

Rust 侧会在后台线程里 poll，再转换成 `broadcast` event stream。

配置资源响应是一个受限例外：schema、snapshot 和字段错误包含多层变长数组，C ABI 使用 callback-lifetime readonly view，避免跨语言分配器。所有 view 只在 callback 返回前有效；Rust/Python 必须在 callback 内立即深拷贝，callback 不能抛异常或保存裸指针。

## 新增一个 C ABI 能力的流程

### 1. 确认 C++ core 已有能力

先确认能力在 C++ core 中存在。不要先从 ABI 开始设计。

例如命令发布能力通常先在：

```text
include/yunlink/runtime/command.hpp
src/runtime/command/
```

### 2. 设计 C ABI 类型

如果要传入复杂参数，先在 `include/yunlink/c/abi/types.h` 添加 C struct。

要求：

- 使用固定宽度整数。
- 明确单位，例如 `_m`、`_mps`、`_ms`。
- 如果未来可能扩展，添加 `struct_size`。
- 字符串使用固定 buffer，写清楚大小。

### 3. 设计 C ABI 函数

在 `include/yunlink/c/abi/functions.h` 添加函数声明。

规则：

- 旧函数签名不改。
- 新函数只追加。
- 返回 `yunlink_result_t`，除非确实是无失败路径。
- 输出值通过 `out_*` 指针返回。
- 指针参数必须允许实现检查 null。

### 4. 实现 adapter

在 `src/c/abi/` 下实现同名函数。adapter 的职责是把 C 类型转换成 C++ core 类型，然后调用已有 runtime/core。

adapter 不应该承载复杂业务规则。复杂规则应该在 C++ core。

### 5. 更新 C ABI 测试

至少考虑三类测试：

```bash
ctest --test-dir build/ninja-debug -R "test_c_ffi_(v1|contract|loader)" --output-on-failure
```

- contract 测试：结构体大小、常量、函数基本存在性。
- v1 行为测试：真实调用闭环。
- loader 测试：动态库能被加载，符号能被找到。

### 6. 同步语言绑定

C ABI 新能力合入前，至少要确认目标语言绑定是否同步：

- Rust raw：`bindings/rust/yunlink-sys`
- Rust safe：`bindings/rust/yunlink`
- Python native extension：`bindings/python/src/ffi_module.cpp`
- Python high-level API：`bindings/python/src/yunlink/`

## 常见错误

### 忘记 Rust raw 层

C header 加了函数，但 `bindings/rust/yunlink-sys/src/functions.rs` 没加同名 extern，Rust 就无法调用。

### 忘记 safe 层

raw extern 加了，但 `bindings/rust/yunlink` 没包 safe API，业务代码仍然不能舒服地使用。

### 改了旧函数签名

这会破坏 ABI 兼容。v1 原则是旧函数不改签名，新能力追加新函数。

### 用动态分配字符串跨语言返回

这会立刻引入谁释放、何时释放、用哪个 allocator 释放的问题。除非设计完整的 `free` API，否则优先固定 buffer。

### 在 C ABI 里引入 C++ 异常

C ABI 函数不应该让 C++ exception 穿越边界。错误应该转换成 `yunlink_result_t`。

## C ABI 改动完成的判断

一个 C ABI 改动完成时，应该满足：

- C header 声明和 C++ 实现一致。
- 新 struct 的布局在测试里有覆盖。
- 新函数能被 loader 或 contract 测试发现。
- 至少一个 roundtrip 或行为测试证明它能调用。
- 目标语言绑定已同步或文档明确说明尚未暴露。
- `docs/bindings/overview.md` 或相关 SDK 文档已更新。
