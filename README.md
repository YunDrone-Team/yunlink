# sunray_communication_lib

[![CI](https://img.shields.io/github/actions/workflow/status/YunDrone-Team/sunray_communication_lib/ci.yml?branch=main&style=for-the-badge&label=CI)](https://github.com/YunDrone-Team/sunray_communication_lib/actions/workflows/ci.yml)
![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?style=for-the-badge&logo=c%2B%2B)
![CMake](https://img.shields.io/badge/CMake-3.25%2B-064F8C?style=for-the-badge&logo=cmake)
![Ninja](https://img.shields.io/badge/Build-Ninja-000000?style=for-the-badge&logo=ninja)
![Targets](https://img.shields.io/badge/Targets-UAV%20%7C%20UGV%20%7C%20Swarm-0A7E8C?style=for-the-badge)
![SDK](https://img.shields.io/badge/API-C%2B%2B%20%2B%20C-2E8B57?style=for-the-badge)

`sunray_communication_lib` 是 Sunray 的统一通信库，面向地面站、无人机机载计算机、无人车车载计算机与集群控制场景。项目提供稳定的 `SecureEnvelope` 线包、语义化消息层、统一 runtime、类型化 C++ SDK 与最小 C ABI，用于构建控制、状态、事件与会话类通信链路。

## 文档导航

- 协议导航页：
  [docs/protocol/README.md](docs/protocol/README.md)
- 协议主规范：
  [docs/protocol/sunray-unified-protocol-spec.md](docs/protocol/sunray-unified-protocol-spec.md)
- 接入指南：
  [docs/protocol/integration-guide.md](docs/protocol/integration-guide.md)
- 场景 walkthrough：
  [docs/protocol/scenario-walkthroughs.md](docs/protocol/scenario-walkthroughs.md)
- 实现状态：
  [docs/protocol/implementation-status.md](docs/protocol/implementation-status.md)
- 迁移说明：
  [docs/protocol/migration-notes.md](docs/protocol/migration-notes.md)
- API Reference（可选生成）：
  `docs/doxygen_output/html/index.html`

## 快速开始

```bash
git clone https://github.com/YunDrone-Team/sunray_communication_lib.git
cd sunray_communication_lib
git submodule update --init --recursive
cmake --preset ninja-debug
python3 tools/build_fast.py --preset ninja-debug
ctest --test-dir build/ninja-debug --output-on-failure
```

默认推荐使用 `Ninja` 作为构建后端。构建并行度由 [`tools/build_fast.py`](/Users/groove/Project/work/YunDrone/sunray_communication_lib/tools/build_fast.py) 自动计算，策略固定为 `max(1, 逻辑核心数 - 1)`。旧的 `cmake -S . -B build` 路径仍可用，但不再是推荐入口。

## 项目能力

- 面向单 UAV、单 UGV、group target、broadcast 四类目标域
- 提供 `Session`、`Authority`、`Command`、`CommandResult`、`StateSnapshot`、`StateEvent`、`BulkChannelDescriptor` 七大消息族
- 内建会话、能力协商、控制权租约与三段式命令结果流
- 支持 TCP / UDP 承载，主控制链路与 bulk 描述协同工作
- 默认采用 `CMake + Ninja` 工作流，并内建 `clang-format`、`clang-tidy`、`maxline` 质量护栏

## 构建与质量检查

常用命令：

```bash
cmake --preset ninja-debug
python3 tools/build_fast.py --preset ninja-debug
python3 tools/build_fast.py --preset ninja-debug --target lint
ctest --test-dir build/ninja-debug -R smoke_examples_local --output-on-failure
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

手动运行 smoke 示例：

```bash
python3 examples/smoke_local/run_smoke_local.py --bin-dir build/ninja-debug
```

## 仓库

- GitHub:
  [YunDrone-Team/sunray_communication_lib](https://github.com/YunDrone-Team/sunray_communication_lib)
