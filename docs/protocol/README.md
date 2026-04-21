# 协议文档导航

本目录把协议相关文档拆分为“一个主规范 + 多个配套说明文档”。主规范负责定义协议要求，配套文档负责接入、场景、实现状态和迁移说明。

## 阅读顺序

首次接入建议按以下顺序阅读：

1. [sunray-unified-protocol-spec.md](sunray-unified-protocol-spec.md)
2. [integration-guide.md](integration-guide.md)
3. [scenario-walkthroughs.md](scenario-walkthroughs.md)
4. [implementation-status.md](implementation-status.md)

如果你正在做存量系统迁移，再补读：

5. [migration-notes.md](migration-notes.md)

## 文档分工

- [sunray-unified-protocol-spec.md](sunray-unified-protocol-spec.md)
  唯一协议主规范。定义 `SecureEnvelope`、消息族、状态机、字段、编号域与协议约束。
- [integration-guide.md](integration-guide.md)
  面向接入者。给出构建、依赖、SDK 入口、最小联调路径与排障观察点。
- [scenario-walkthroughs.md](scenario-walkthroughs.md)
  面向开发与联调。给出单 UAV、单 UGV、Swarm 三类完整场景 walkthrough。
- [implementation-status.md](implementation-status.md)
  面向使用者与维护者。说明当前 repo 的覆盖矩阵、限制与落地含义。
- [migration-notes.md](migration-notes.md)
  面向迁移者。说明旧通信认知迁移到当前统一协议模型时最容易踩的坑。

## 边界

- 主规范不承担长篇教程、联调步骤或实现报告。
- 配套说明文档不重复定义协议字段和编号域，而是引用主规范。
- `docs/doxygen_output/` 仅视为 API Reference 生成产物，不属于手写协议文档主入口。
