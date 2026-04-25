# Yunlink Rename Design

> Goal: 将仓库从 `yunlink`/`yunlink` 彻底重命名为 `yunlink`，作为云纵科技核心机器人通讯协议与基础通信库，不保留旧命名兼容层。

## Scope

- 仓库与对外品牌文案统一为 `yunlink`。
- C++ namespace、头文件路径、C ABI 前缀、CMake project/target/config 名统一改为 `yunlink`。
- 协议文档与 README 全中文化，并补充 Rust / Python / JavaScript 接口规划。
- 不改动协议线包版本语义与当前运行时行为，只做命名和文档重塑。

## Decisions

- 采用断代式重命名，不保留 `yunlink` 转发头或别名。
- 主规范文件重命名为 `docs/protocol/yunlink-protocol-spec.md`。
- 顶层库 target 与命名空间均使用 `yunlink`，以便后续多语言绑定围绕同一核心命名展开。

## Verification

- 重新配置并构建测试。
- 运行核心测试，确认重命名未破坏编译链接。
