# yunlink

[![Linux CI](https://img.shields.io/github/actions/workflow/status/YunDrone-Team/yunlink/ci-linux.yml?branch=main&style=for-the-badge&label=Linux%20CI&logo=linux)](https://github.com/YunDrone-Team/yunlink/actions/workflows/ci-linux.yml)
[![macOS CI](https://img.shields.io/github/actions/workflow/status/YunDrone-Team/yunlink/ci-macos.yml?branch=main&style=for-the-badge&label=macOS%20CI&logo=apple)](https://github.com/YunDrone-Team/yunlink/actions/workflows/ci-macos.yml)
[![Windows CI](https://img.shields.io/github/actions/workflow/status/YunDrone-Team/yunlink/ci-windows.yml?branch=main&style=for-the-badge&label=Windows%20CI&logo=windows)](https://github.com/YunDrone-Team/yunlink/actions/workflows/ci-windows.yml)
![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?style=for-the-badge&logo=c%2B%2B)
![Build](https://img.shields.io/badge/CMake%20%2B%20Ninja-Ready-064F8C?style=for-the-badge&logo=cmake)
![Bindings](https://img.shields.io/badge/Bindings-Rust%20%7C%20Python%20%7C%20JavaScript-F28C28?style=for-the-badge)

`yunlink` 是面向无人机、无人车、地面站与集群系统的统一通信协议与基础通信库。这个仓库聚焦同一套 `yunlink` 协议核心，提供 `SecureEnvelope` 线包、语义消息模型、统一 `Runtime`、类型化 C++ SDK，以及可供 Rust、Python、JavaScript 复用的最小 C ABI。

## 命名统一

仓库内与对外暴露的核心标识统一使用 `yunlink`：

- GitHub 仓库：`YunDrone-Team/yunlink`
- CMake 项目与主库 target：`yunlink`
- C++ 命名空间：`yunlink`
- 头文件根目录：`include/yunlink/`
- C ABI 前缀：`yunlink_*`
- 协议主规范：`docs/protocol/yunlink-protocol-spec.md`

## 核心能力

- `SecureEnvelope` 编解码与流式拆包
  由 `ProtocolCodec` 和 `EnvelopeStreamParser` 负责定长头、payload、magic 重同步与帧级校验。
- 语义消息与 typed payload
  覆盖 `Session`、`Authority`、`Command`、`CommandResult`、`StateSnapshot`、`StateEvent`、`BulkChannelDescriptor` 等核心消息族。
- 统一 runtime
  通过 `Runtime`、`SessionClient`、`SessionServer`、`CommandPublisher`、`StateSubscriber`、`EventSubscriber` 组织会话、控制权、命令与状态流。
- 传输与事件分发
  提供 `UdpTransport`、`TcpClientPool`、`TcpServer` 与 `EventBus`，支撑本地联调与集成验证。
- 最小 C ABI
  位于 `include/yunlink/c/yunlink_c.h`，作为多语言绑定的稳定基础层。

## 仓库结构

- `include/yunlink/`
  对外头文件入口，包含 core、runtime、transport 与 C ABI。
- `src/`
  协议编解码、runtime 语义实现、TCP/UDP 传输实现。
- `tests/`
  协议、传输、runtime、快照上行与兼容路径的回归测试。
- `examples/`
  本地 smoke、UDP 发现、TCP 命令客户端、遥测接收与桥接样例。
- `docs/`
  协议规范、实现状态、接入说明、场景 walkthrough 与图示资源。
- `tools/`
  快速构建、质量检查、图示渲染等研发脚本。

## 快速开始

```bash
git clone https://github.com/YunDrone-Team/yunlink.git
cd yunlink
git submodule update --init --recursive
cmake --preset ninja-debug
python3 tools/build_fast.py --preset ninja-debug
ctest --test-dir build/ninja-debug --output-on-failure
```

推荐工作流使用 `CMake 3.25+` 与 `Ninja`。如果采用传统 `cmake -S . -B ...` 配置方式，顶层 `CMakeLists.txt` 当前仍兼容 `CMake 3.16+`。

## 常用命令

```bash
cmake --preset ninja-debug
python3 tools/build_fast.py --preset ninja-debug
python3 tools/build_fast.py --preset ninja-debug --target lint
ctest --test-dir build/ninja-debug -R smoke_examples_local --output-on-failure
python3 examples/smoke_local/run_smoke_local.py --bin-dir build/ninja-debug
./tools/render_protocol_diagrams.sh
doxygen docs/Doxyfile
```

## 文档入口

- 总导航：`docs/README.md`
- 协议导航：`docs/protocol/README.md`
- 协议主规范：`docs/protocol/yunlink-protocol-spec.md`
- 实现状态：`docs/protocol/implementation-status.md`
- 接入指南：`docs/protocol/integration-guide.md`
- 场景 walkthrough：`docs/protocol/scenario-walkthroughs.md`
- 迁移说明：`docs/protocol/migration-notes.md`
- 本地 API 文档：`build/doxygen/html/index.html`

## 多语言接口方向

`yunlink` 的演进方向保持“单协议核心，多语言薄封装”：

1. 稳定 `yunlink` 的 C++ 核心与 C ABI。
2. 让 Rust、Python、JavaScript 通过 FFI 或绑定层共享同一套协议语义。
3. 在需要的语言侧逐步补齐高层 typed SDK 与更贴近业务的开发体验。

## 当前边界

- 当前最小会话路径以 `Hello -> Authenticate -> Capabilities -> Ready` 为主，尚未完成完整双向协商、重连恢复与会话续接。
- `Authority` 当前仍是单全局租约，不按目标域分片，也不会自动对外发送 `AuthorityStatus`。
- `TargetScope::kGroup` 已能在线包与 payload 中表达，但 runtime 还没有接入真实组成员关系。
- `BulkChannelDescriptor` 已具备类型与编解码，runtime 侧尚未接入 bulk 消费者。

覆盖矩阵与限制细节见 `docs/protocol/implementation-status.md`。

## 质量护栏

- `.clang-format`
- `.clang-tidy`
- `tools/check_core_maxline.sh`
- GitHub Actions 构建与测试流程

## 仓库地址

[https://github.com/YunDrone-Team/yunlink](https://github.com/YunDrone-Team/yunlink)
