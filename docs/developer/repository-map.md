# 仓库地图与开发环境

这篇文档帮助你先找到代码位置。你不需要马上理解每个模块，只要先知道不同问题应该从哪里进入。

## 仓库顶层

| 路径 | 作用 |
| --- | --- |
| `include/yunlink/` | 对外 C++ 头文件、协议语义类型、C ABI 头文件 |
| `src/` | C++ 实现，包括 core、runtime、C ABI adapter |
| `bindings/rust/` | Rust workspace，包括 raw FFI crate 和 safe SDK |
| `bindings/python/` | Python SDK 与 native extension |
| `tests/` | C++ core、runtime、security、transport、bindings 测试 |
| `docs/protocol/` | 协议规范和实现状态 |
| `docs/bindings/` | bindings 总览、SDK 指南和测试矩阵 |
| `docs/developer/` | 面向开发者的循序渐进开发路线 |
| `tools/` | 构建、测试、监控工具和脚手架 |

## 你应该先能完成的本地动作

第一步不是修改代码，而是确认本地工具链能跑通。

### C++ / C ABI 构建

```bash
cmake -S . -B build/ninja-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/ninja-debug
```

如果你只关心 C ABI 相关测试，可以先跑：

```bash
ctest --test-dir build/ninja-debug -R "test_c_ffi_(v1|contract|loader)" --output-on-failure
```

### Rust SDK

Rust workspace 不在仓库根目录，而在 `bindings/rust/Cargo.toml`。

```bash
cargo test -p yunlink --manifest-path bindings/rust/Cargo.toml
```

Rust Advanced Monitor 是单独的 app crate：

```bash
cargo build --manifest-path tools/yunlink_rust_advanced_monitor/Cargo.toml
```

### Python SDK

Python 绑定同时依赖 C++ native extension 和 Python 包装层。先看：

```bash
docs/bindings/python-sdk.md
```

再根据当前平台选择 wheel 或本地 extension 构建路线。

## 推荐的 VS Code 设置

这个仓库根目录没有顶层 `Cargo.toml`，所以 rust-analyzer 不能只靠根目录自动发现 Rust 项目。需要指定 Rust 项目路径：

```json
{
  "rust-analyzer.linkedProjects": [
    "bindings/rust/Cargo.toml",
    "tools/yunlink_rust_advanced_monitor/Cargo.toml"
  ],
  "rust-analyzer.cargo.features": "all",
  "rust-analyzer.check.command": "clippy",
  "clangd.arguments": [
    "--compile-commands-dir=${workspaceFolder}/build/ninja-debug",
    "--background-index",
    "--clang-tidy",
    "--completion-style=detailed",
    "--header-insertion=iwyu"
  ],
  "C_Cpp.intelliSenseEngine": "disabled",
  "cmake.buildDirectory": "${workspaceFolder}/build/ninja-debug"
}
```

推荐扩展：

- `rust-lang.rust-analyzer`
- `llvm-vs-code-extensions.vscode-clangd`
- `ms-vscode.cmake-tools`
- `vadimcn.vscode-lldb`
- `tamasfe.even-better-toml`

## 先建立三个边界

### C++ core 是真实逻辑层

协议编解码、runtime、session、authority、命令和状态流都在 C++ core 里。语言 SDK 不应该重新实现这些协议规则。

### C ABI 是跨语言稳定边界

C ABI 暴露 `libyunlink_ffi`。它提供 C 风格函数、C 风格结构体、错误码和事件轮询接口。语言绑定只依赖它，不直接绑定 C++ facade。

### 语言 SDK 是面向使用者的舒适层

Rust 的 `yunlink` 和 Python 的 `yunlink` 会隐藏指针、固定字符串 buffer、`struct_size`、错误码转换和事件解析。业务代码应优先使用 SDK。

## 第一次改代码时的建议

如果你很久没看代码，先做一个只读练习：

1. 打开 `include/yunlink/c/abi/functions.h`
2. 找到 `yunlink_command_publish_goto`
3. 打开 `src/c/abi/commands/command.cpp`
4. 找到同名实现
5. 打开 `bindings/rust/yunlink-sys/src/functions.rs`
6. 找到 Rust raw extern 声明
7. 打开 `bindings/rust/yunlink/src/runtime/commands.rs`
8. 找到 safe Rust 方法 `publish_goto`

读完这条线，你就能看见一次能力如何从 C++ core 穿过 C ABI 到 Rust SDK。
