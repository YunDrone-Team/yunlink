# 文档总览

本目录只保留仍在维护的手写文档、协议图源和文档生成配置。生成产物与阶段性整理清单不再作为仓库文档主入口保留。

## 推荐阅读

建议按下面顺序进入：

1. [protocol/README.md](protocol/README.md)
2. [protocol/sunray-unified-protocol-spec.md](protocol/sunray-unified-protocol-spec.md)
3. [protocol/implementation-status.md](protocol/implementation-status.md)
4. [protocol/integration-guide.md](protocol/integration-guide.md)
5. [protocol/scenario-walkthroughs.md](protocol/scenario-walkthroughs.md)
6. [protocol/migration-notes.md](protocol/migration-notes.md)

## 目录说明

- `protocol/`
  协议主规范、接入指南、场景 walkthrough、实现状态和迁移说明。
- `diagrams/plantuml/src/`
  协议图的 PlantUML 源文件。
- `diagrams/plantuml/svg/`
  当前导出的 SVG 图。
- `Doxyfile`
  本地生成 API Reference 的配置文件。

## 生成约定

- 协议图可通过 `./tools/render_protocol_diagrams.sh` 重新导出。
- API Reference 可通过 `doxygen docs/Doxyfile` 在本地生成。
- Doxygen HTML 输出目录为 `build/doxygen/html/`，不作为手写文档提交。
- `build/` 下的内容都视为生成产物，而不是 `docs/` 的正式组成部分。
