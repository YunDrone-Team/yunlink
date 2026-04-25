# Yunlink Rename Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将仓库内 `yunlink` / `yunlink` 全量重命名为 `yunlink`，并完成中文 README 与协议文档入口更新。

**Architecture:** 先统一目录、命名空间与构建目标，再更新文档入口和品牌文案，最后通过 CMake 配置与测试回归验证。整个过程不改协议行为，只改标识与接口名字。

**Tech Stack:** CMake, C++17, ripgrep, perl, ctest

---

### Task 1: 扫描与路径重命名

**Files:**
- Modify: `CMakeLists.txt`, `cmake/yunlinkConfig.cmake.in`
- Move: `include/yunlink/**` -> `include/yunlink/**`
- Move: `docs/protocol/yunlink-protocol-spec.md` -> `docs/protocol/yunlink-protocol-spec.md`

- [ ] 批量替换仓库内命名并修正构建入口。
- [ ] 重命名目录与协议主规范文件。

### Task 2: 文档与 README 更新

**Files:**
- Modify: `README.md`, `docs/README.md`, `docs/protocol/README.md`, `docs/protocol/*.md`

- [ ] 将 README 改为中文品牌叙述，加入现代 badge。
- [ ] 更新协议文档中的 `yunlink` 命名与链接。
- [ ] 增加 Rust / Python / JavaScript 接口规划说明。

### Task 3: 验证

**Files:**
- Test: `tests/*.cpp`

- [ ] 重新配置 CMake。
- [ ] 编译测试目标并运行核心测试。
