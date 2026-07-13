# 跨语言调用的心智模型

这篇文档帮助你先看懂一件事：为什么 Rust、Python 或未来其他语言需要经过一层稳定接口，才能可靠调用 YunLink 的 C++ core。

读完这篇后，再去看 [跨语言接口开发路线](c-abi-development.md)、[Rust 绑定开发路线](rust-bindings-development.md) 和 [Python 绑定开发路线](python-bindings-development.md)，会轻松很多。

## 先从一个普通库文件开始

一个已经编译好的库文件里面有机器码。机器能执行它，但人很难直接读懂它。

如果这个库导出了一个函数名，例如 `yunlink_runtime_start`，别的程序可能能找到这个入口。但是只有函数名还不够。调用方还需要知道：

- 这个函数有几个参数。
- 每个参数是什么类型。
- 参数里的指针能不能是 null。
- 返回值代表成功还是失败。
- 如果函数返回了对象，谁负责释放对象。

所以，跨语言调用不只需要库文件。它还需要一份人类和编译器都能理解的接口描述。

## 接口描述负责补上缺失的信息

在 C 和 C++ 项目里，接口描述通常写在头文件里。YunLink 的 C 入口头文件是：

```text
include/yunlink/c/yunlink_c.h
```

这个头文件会继续包含更细的接口头文件：

```text
include/yunlink/c/abi/base.h
include/yunlink/c/abi/enums.h
include/yunlink/c/abi/types.h
include/yunlink/c/abi/functions.h
```

这里第一次正式命名这类接口：ABI 是 application binary interface 的缩写，意思是“二进制层面的调用约定”。它描述函数名、参数类型、结构体内存布局、错误码和对象释放规则。

## 为什么选择 C 风格接口

C++ 的能力很丰富，例如 class、template、reference、exception、`std::string` 和 `std::vector`。这些能力在 C++ 内部很好用，但对其他语言来说不够稳定。

C 风格接口更朴素。它主要使用整数、浮点数、指针、固定大小结构体和固定大小字符串 buffer。它不舒服，但很多语言都能理解。

YunLink 因此采用这条路径：

```text
C++ core
  -> C 风格稳定接口
    -> Rust / Python / future language bindings
```

这层 C 风格稳定接口就是 YunLink 的 C ABI。

## 只拿到库文件为什么不够

如果你只把 `libyunlink_ffi` 丢给一个人，而不给头文件、文档或绑定代码，他最多能通过工具看到一部分导出符号。

他可能看到：

```text
yunlink_runtime_start
yunlink_command_publish_goto
yunlink_runtime_poll_event
```

但他仍然不知道：

- `yunlink_runtime_start` 的第二个参数是不是配置指针。
- `yunlink_command_publish_goto` 的 payload 内存布局是什么。
- `yunlink_runtime_poll_event` 的输出 union 应该按哪个 tag 读取。
- 返回的整数 `0` 是成功，还是某种错误码。

所以库文件提供“入口”，头文件和文档提供“如何正确进入”。

## Rust 为什么要重新声明函数

Rust 编译器不会自动读懂一个 C 动态库里所有函数的参数类型。Rust 需要在代码里写出一份对应声明。

在 YunLink 里，这份声明位于：

```text
bindings/rust/yunlink-sys/src/functions.rs
```

示意代码如下：

```rust
unsafe extern "C" {
    pub fn yunlink_runtime_start(
        runtime: *mut yunlink_runtime_t,
        cfg: *const yunlink_runtime_config_t,
    ) -> yunlink_result_t;
}
```

这段代码告诉 Rust：

- 这个函数使用 C 调用约定。
- 函数名是 `yunlink_runtime_start`。
- 第一个参数是 runtime 指针。
- 第二个参数是配置结构体指针。
- 返回值是 YunLink 的结果码。

这里的 `unsafe` 不等于“这个函数一定有问题”。它的意思是 Rust 编译器不能独自证明这次调用一定安全。调用方必须保证指针有效、结构体布局正确、生命周期正确。

## Rust 结构体为什么要写成 C 布局

Rust 默认可以按自己的规则安排结构体字段。C 也有自己的结构体布局规则。

当一个 Rust struct 要传给 C ABI 时，双方必须看到同一种内存布局。Rust 侧用 `#[repr(C)]` 表达这个要求。

示意代码如下：

```rust
#[repr(C)]
pub struct yunlink_goto_command_t {
    pub x_m: f32,
    pub y_m: f32,
    pub z_m: f32,
    pub yaw_rad: f32,
}
```

`repr` 是 representation 的缩写，意思是“表示方式”。`#[repr(C)]` 的意思是让 Rust 按 C 的规则表示这个 struct。

## 为什么还要有 safe SDK

原始 C ABI 适合跨语言，但不适合业务开发。它会暴露很多底层细节：

- raw pointer
- null pointer
- 固定 buffer
- 整数错误码
- tagged union
- 手动创建和销毁 runtime

Rust 业务代码更希望看到：

- `Result<T, YunlinkError>`
- Rust enum
- Rust struct
- `Drop` 自动清理
- `String` 和 `&str`
- async 方法和 event stream

所以 YunLink 的 Rust 绑定分成两层：

```text
yunlink-sys
  原始绑定层，只忠实描述 C ABI

yunlink
  safe SDK，把原始绑定包装成 Rust 习惯的 API
```

业务代码应该用 `yunlink`。只有 SDK 内部、测试或教学展示才应该直接碰 `yunlink-sys`。

## Python 也遵循同一个原则

Python 不会直接绑定 C++ facade。它也先经过 C ABI。

YunLink 的 Python 绑定分成两层：

```text
_yunlink_native
  native extension，靠近 C ABI

yunlink
  高层 Python API，提供 dataclass、异常、同步和异步接口
```

这和 Rust 的分层思想相同。底层负责准确连接，上层负责好用和安全。

## 当前仓库的完整链路

现在可以把整条链路连起来：

```text
Rust Monitor / Python app / future language app
  -> safe language SDK
    -> raw binding or native extension
      -> C ABI: libyunlink_ffi
        -> C++ core/runtime/protocol
```

这里有一个重要边界：UI 和业务代码不应该绕过 safe language SDK 去直接调用 C ABI。直接调用会把指针、内存布局、错误码和生命周期问题扩散到业务层。

## 接下来怎么读

如果你要改 YunLink 的真实行为，读 [YunLink 本体开发路线](core-development.md)。

如果你要让一个 C++ 能力被其他语言调用，读 [跨语言接口开发路线](c-abi-development.md)。

如果你要扩展 Rust SDK，读 [Rust 绑定开发路线](rust-bindings-development.md)。

如果你要扩展 Python SDK，读 [Python 绑定开发路线](python-bindings-development.md)。

如果你要一次性贯通 core、C ABI、Rust、Python 和工具，读 [从新增能力到多语言包装的完整流程](feature-to-binding-playbook.md)。
