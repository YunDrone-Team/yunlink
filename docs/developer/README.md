# YunLink 开发者文档入口

这组文档面向两类读者：

- 想修改 YunLink 本体的人，也就是会接触 C++ core、runtime、协议语义和测试的人。
- 想给 YunLink 做语言包装的人，也就是会接触 C ABI、Rust SDK、Python SDK 或未来其他语言 SDK 的人。

读这组文档不要求你先懂 C ABI。你只需要知道：一个程序可以把自己的能力做成库，另一个语言可以通过一层稳定接口调用这个库。后面会逐步把这层稳定接口命名为 C ABI。

## 推荐学习顺序

第一次看这个仓库时，建议按下面顺序读：

1. [仓库地图与开发环境](repository-map.md)
2. [跨语言调用的心智模型](cross-language-mental-model.md)
3. [YunLink 本体开发路线](core-development.md)
4. [跨语言接口开发路线](c-abi-development.md)
5. [Rust 绑定开发路线](rust-bindings-development.md)
6. [Python 绑定开发路线](python-bindings-development.md)
7. [从新增能力到多语言包装的完整流程](feature-to-binding-playbook.md)
8. [验证、发布与回归检查清单](verification-release-checklist.md)
9. [文档维护与排布规则](documentation-governance.md)

如果你只想快速知道“我该改哪里”，可以先看下面这张表。

| 你要做的事 | 先读 | 主要改动位置 |
| --- | --- | --- |
| 重新理解库文件、C ABI、raw binding 和 safe SDK 的关系 | [跨语言调用的心智模型](cross-language-mental-model.md) | `include/yunlink/c/`, `bindings/rust/`, `bindings/python/` |
| 修改协议语义或 runtime 行为 | [YunLink 本体开发路线](core-development.md) | `include/yunlink/core/`, `src/runtime/`, `src/core/`, `tests/` |
| 新增一个 C ABI 函数 | [跨语言接口开发路线](c-abi-development.md) | `include/yunlink/c/abi/`, `src/c/abi/`, `tests/bindings/` |
| 给 Rust SDK 增加 safe API | [Rust 绑定开发路线](rust-bindings-development.md) | `bindings/rust/yunlink-sys/`, `bindings/rust/yunlink/` |
| 给 Python SDK 增加方法 | [Python 绑定开发路线](python-bindings-development.md) | `bindings/python/src/ffi_module.cpp`, `bindings/python/src/yunlink/` |
| 一次性把 C++ 能力贯通到语言 SDK | [完整流程](feature-to-binding-playbook.md) | C++ core + C ABI + 语言绑定 + tests |
| 判断新内容应该写进哪篇文档 | [文档维护与排布规则](documentation-governance.md) | `docs/protocol/`, `docs/bindings/`, `docs/developer/`, `tools/` |

## 最小心智模型

YunLink 的核心逻辑在 C++ 里。其他语言不直接绑定 C++ facade，而是通过一层稳定的 C 风格接口调用核心逻辑。这层接口的职责是让不同语言看到同一套函数、结构体和错误码。

这层接口就是 C ABI。ABI 是 application binary interface 的缩写，意思是“二进制层面的调用约定”。它关心函数名、参数类型、结构体布局、内存所有权和错误码。

当前仓库中的分层是：

```text
应用 / 工具
  -> 语言 SDK，例如 Rust 的 yunlink 或 Python 的 yunlink
    -> 原始绑定层，例如 Rust 的 yunlink-sys 或 Python native extension
      -> C ABI: libyunlink_ffi
        -> C++ core/runtime/protocol
```

这意味着业务代码应该尽量调用语言 SDK，而不是直接调用 C ABI。C ABI 是稳定边界，不是最舒服的业务 API。

## 文档维护原则

- 中文是主体语言，必要的代码名和行业术语保留英文。
- 每篇文档先讲问题，再讲机制，最后讲代码位置。
- 新增能力时，必须同步更新对应路线文档或检查清单。
- 新增文档时，必须保证它能从 `docs/README.md`、本入口或相关目录 README 到达。
- 不在 developer 文档里重复协议字段定义。协议字段仍以 `docs/protocol/` 为准。
- 不把生成产物写进文档目录。构建输出仍放在 `build/`、`output/` 或 Cargo target 目录。
