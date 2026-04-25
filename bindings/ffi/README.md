# bindings/ffi

本目录用于存放 `yunlink` 共享 C ABI 的版本说明、命名约定与跨语言集成文档。

当前稳定入口头文件仍位于 `include/yunlink/c/yunlink_c.h`，Rust / Python 封装都应仅依赖这层 ABI，而不是直接绑定 C++ facade。
