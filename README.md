# sunray_communication_lib

[![CI](https://img.shields.io/github/actions/workflow/status/YunDrone-Team/sunray_communication_lib/ci.yml?branch=main&style=for-the-badge&label=CI)](https://github.com/YunDrone-Team/sunray_communication_lib/actions/workflows/ci.yml)
![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?style=for-the-badge&logo=c%2B%2B)
![CMake Presets](https://img.shields.io/badge/CMake%20Presets-3.25%2B-064F8C?style=for-the-badge&logo=cmake)
![Ninja](https://img.shields.io/badge/Build-Ninja-000000?style=for-the-badge&logo=ninja)
![Targets](https://img.shields.io/badge/Targets-UAV%20%7C%20UGV%20%7C%20Swarm-0A7E8C?style=for-the-badge)
![SDK](https://img.shields.io/badge/API-C%2B%2B%20%2B%20C-2E8B57?style=for-the-badge)

`sunray_communication_lib` 是 Sunray 的统一通信库，面向地面站、无人机机载计算机、无人车车载计算机与集群控制场景。项目提供稳定的 `SecureEnvelope` 线包、语义化消息层、统一 runtime、类型化 C++ SDK 与最小 C ABI，用于构建控制、状态、事件与会话类通信链路。

## 文档入口

- 总导航：
  [docs/README.md](docs/README.md)
- 协议导航页：
  [docs/protocol/README.md](docs/protocol/README.md)
- 协议主规范：
  [docs/protocol/sunray-unified-protocol-spec.md](docs/protocol/sunray-unified-protocol-spec.md)
- 当前实现状态：
  [docs/protocol/implementation-status.md](docs/protocol/implementation-status.md)
- 接入指南：
  [docs/protocol/integration-guide.md](docs/protocol/integration-guide.md)
- 场景 walkthrough：
  [docs/protocol/scenario-walkthroughs.md](docs/protocol/scenario-walkthroughs.md)
- 迁移说明：
  [docs/protocol/migration-notes.md](docs/protocol/migration-notes.md)
- API Reference（本地生成）：
  `build/doxygen/html/index.html`

## 快速开始

```bash
git clone https://github.com/YunDrone-Team/sunray_communication_lib.git
cd sunray_communication_lib
git submodule update --init --recursive
cmake --preset ninja-debug
python3 tools/build_fast.py --preset ninja-debug
ctest --test-dir build/ninja-debug --output-on-failure
```

`cmake --preset ...` 这一推荐工作流依赖 `CMake 3.25+`。如果你只走传统 `cmake -S . -B ...` 配置路径，则顶层 `CMakeLists.txt` 的最低要求是 `3.16`。构建并行度由 `tools/build_fast.py` 自动计算，策略固定为 `max(1, 逻辑核心数 - 1)`。

## 当前仓库提供的能力

- `ProtocolCodec` 与 `EnvelopeStreamParser`
  负责 `SecureEnvelope` 编解码、流式拆包和 magic 重同步。
- 语义消息模型与 typed payload 编解码
  覆盖 `Session`、`Authority`、`Command`、`CommandResult`、`StateSnapshot`、`StateEvent`、`BulkChannelDescriptor` 七大消息族。
- 传输与事件分发
  提供 `EventBus`、`UdpTransport`、`TcpClientPool`、`TcpServer`。
- 统一 C++ runtime 入口
  通过 `Runtime`、`SessionClient`、`SessionServer`、`CommandPublisher`、`StateSubscriber`、`EventSubscriber` 组织会话、控制权、命令与状态流。
- 最小 C ABI
  位于 `include/sunraycom/c/sunraycom_c.h`，当前以原始事件轮询为主。
- 示例与回归测试
  `examples/` 提供最小联调样例，`tests/` 覆盖协议、传输、runtime 和 UAV 上行快照路径。

## 构建、示例与文档生成

常用命令：

```bash
cmake --preset ninja-debug
python3 tools/build_fast.py --preset ninja-debug
python3 tools/build_fast.py --preset ninja-debug --target lint
ctest --test-dir build/ninja-debug -R smoke_examples_local --output-on-failure
python3 examples/smoke_local/run_smoke_local.py --bin-dir build/ninja-debug
./tools/render_protocol_diagrams.sh
doxygen docs/Doxyfile
```

当前质量护栏包括：

- `.clang-format`：统一 C++ 排版
- `.clang-tidy`：静态检查
- `tools/check_core_maxline.sh`：核心代码文件行数约束
- GitHub Actions：自动验证构建、lint 与测试链路

## 示例与依赖

- 示例程序位于 `examples/`
- `standalone Asio 1.30.2` 以 Git submodule 固定在 `thirdparty/asio`
- 头文件目录为 `thirdparty/asio/asio/include`
- Doxygen 本地输出目录为 `build/doxygen/html/`

## 当前边界

- 当前最小会话路径是发起方单向发送 `Hello -> Authenticate -> Capabilities -> Ready` 四条消息，接收方本地推进到 `Active`；并没有完整双向协商或重连恢复。
- `Authority` 当前是单全局租约，不按目标域分片，也不会自动对外发送 `AuthorityStatus`。
- `TargetScope::kGroup` 可以在线包和 payload 中表达，但当前 runtime 的目标匹配还没有接入真实组成员关系。
- `BulkChannelDescriptor` 已有类型与编解码，但 runtime 还没有 bulk 消费者。

这些边界和更细的覆盖矩阵请直接阅读 [docs/protocol/implementation-status.md](docs/protocol/implementation-status.md)。

## 仓库

- GitHub:
  [YunDrone-Team/sunray_communication_lib](https://github.com/YunDrone-Team/sunray_communication_lib)
