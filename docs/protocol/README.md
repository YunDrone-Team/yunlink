# 协议文档导航

本目录把协议相关文档拆分为“协议主规范 + 当前实现状态 + 接入说明 + 场景 walkthrough + 迁移说明”。这样可以把“协议应然模型”和“当前仓库已经做到哪里”明确分开，避免规范、教程和实现报告互相污染。

## 如何阅读

- 想理解协议蓝图：
  先读 [sunray-unified-protocol-spec.md](sunray-unified-protocol-spec.md)
- 想按当前 repo 真实能力接入：
  先读 [implementation-status.md](implementation-status.md)，再读 [integration-guide.md](integration-guide.md)
- 想看端到端场景：
  读 [scenario-walkthroughs.md](scenario-walkthroughs.md)
- 想从历史认知迁移：
  读 [migration-notes.md](migration-notes.md)

首次接入推荐顺序：

1. [sunray-unified-protocol-spec.md](sunray-unified-protocol-spec.md)
2. [implementation-status.md](implementation-status.md)
3. [integration-guide.md](integration-guide.md)
4. [scenario-walkthroughs.md](scenario-walkthroughs.md)

如果你正在做存量系统迁移，再补读：

5. [migration-notes.md](migration-notes.md)

## 文档分工

- [sunray-unified-protocol-spec.md](sunray-unified-protocol-spec.md)
  唯一协议主规范。定义 `SecureEnvelope`、消息族、状态机、字段、编号域与协议约束。
- [implementation-status.md](implementation-status.md)
  当前仓库实现的覆盖矩阵、已验证路径、限制与接入含义。
- [integration-guide.md](integration-guide.md)
  面向接入者。给出构建前提、公开 API、最小联调路径与排障观察点。
- [scenario-walkthroughs.md](scenario-walkthroughs.md)
  面向开发与联调。给出单 UAV、单 UGV、Swarm 三类场景的协议路径，以及当前 repo 对这些场景的可落地点和限制。
- [migration-notes.md](migration-notes.md)
  面向迁移者。说明旧通信认知迁移到当前统一协议模型时最容易踩的坑。

## 边界

- 主规范定义协议约束，不承担长篇教程、联调步骤或实现覆盖报告。
- 接入指南和场景文档不重复定义协议字段与编号域，而是引用主规范。
- 当前实现边界以 [implementation-status.md](implementation-status.md) 为准，而不是由规范正文隐含推出。
- API Reference 通过 `doxygen docs/Doxyfile` 本地生成到 `build/doxygen/`，不属于手写协议文档主入口。
