# bindings/ffi

本目录用于存放 `yunlink` 共享 C ABI 的版本说明、命名约定与跨语言集成文档。

当前稳定入口头文件仍位于 `include/yunlink/c/yunlink_c.h`，Rust / Python 封装都应仅依赖这层 ABI，而不是直接绑定 C++ facade。

如果你要新增或维护 ABI，请从 [跨语言接口开发路线](../../docs/developer/c-abi-development.md) 开始读。它会说明 opaque handle、`struct_size`、固定字符串 buffer、事件 polling 和 ABI 测试应如何配合。

如果你要把一个 C++ core 能力一路暴露到 Rust、Python 或工具界面，请读 [从新增能力到多语言包装的完整流程](../../docs/developer/feature-to-binding-playbook.md)。
