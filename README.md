# SunrayComLib

SunrayComLib 是从 Sunray Station 中拆分出的跨平台通信库，提供协议兼容的 TCP/UDP 传输能力、旧协议适配层、C++ API 与 C ABI。

本仓库对齐 `sunray-codestyle`（v1.1，2026-02-03）中的可执行约束：命名风格、`clang-format`、Ruff、Doxygen 文件头与接口注释。

## 主要能力

- 基于统一运行时的 UDP/TCP 收发
- 协议编解码与流式解析
- 兼容旧版 `legacy_sunray` 数据模型与编解码
- 提供 C++ 与 C 两套调用接口

## 构建与测试

```bash
cmake -S . -B build -DSUNRAYCOM_BUILD_EXAMPLES=ON -DSUNRAYCOM_BUILD_TESTS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## 代码风格与格式化

C++（`.clang-format`）：

```bash
clang-format -style=file -i $(rg --files include src examples tests | rg '\.(hpp|h|cpp)$')
```

Python（`pyproject.toml` + Ruff）：

```bash
ruff format .
ruff check .
```

## 本机冒烟测试（多进程）

手动执行：

```bash
python3 examples/smoke_local/run_smoke_local.py --bin-dir build
```

仅运行冒烟用例：

```bash
ctest --test-dir build -R smoke_examples_local --output-on-failure
```

该冒烟测试会在单机上编排多个示例进程，覆盖两条链路：

1. TCP 直连：`tcp_command_client -> telemetry_receiver`
2. UDP 转 TCP：`discovery_udp -> udp_tcp_bridge -> telemetry_receiver`

## 文档生成（Doxygen）

```bash
doxygen docs/Doxyfile
```

生成入口：`docs/doxygen_output/html/index.html`

## 目录结构

- `include/`：公开头文件
- `src/`：核心实现
- `compat/legacy_sunray/`：旧协议模型与编解码兼容层
- `examples/`：示例程序
- `tests/`：单元、集成与回归测试
- `docs/`：协议说明与迁移文档
