# Sunray Protocol 当前实现状态

本文档描述当前 repo 的实现覆盖、已知限制与对接入者的实际含义。协议应然能力以 [sunray-unified-protocol-spec.md](sunray-unified-protocol-spec.md) 为准；本文档回答的是“当前仓库已经做到哪里”。

## 1. 覆盖矩阵

| 能力项 | 状态 | 说明 |
| --- | --- | --- |
| `SecureEnvelope` 编解码 | `Implemented` | 已支持头部、变长头、payload、checksum |
| `EnvelopeStreamParser` | `Implemented` | 已支持流式拆包与 magic 重同步 |
| 语义 payload 编码 | `Implemented` | 当前为手写二进制编码，不是 codegen schema |
| 会话状态推进 | `Partial` | 单向突发式握手，缺少双向协商、重连恢复 |
| 认证 | `Partial` | 当前仅共享字符串比较 |
| 能力协商 | `Partial` | 记录了 capability flags，未做能力裁决 |
| 控制权租约 | `Partial` | 单全局租约，无目标域分片，无 `AuthorityStatus` 回执 |
| 命令发布 API | `Implemented` | 已覆盖最小命令集合 |
| 命令执行 | `Partial` | 仅 runtime 自动结果流，无真实执行器 |
| 命令结果流 | `Partial` | 成功路径固定，失败路径未完备 |
| 状态快照订阅 | `Implemented` | `VehicleCoreState` 可订阅 |
| 状态事件订阅 | `Implemented` | `VehicleEvent` 可订阅 |
| Group target 线包表达 | `Implemented` | codec 与类型支持 |
| Group target 成员语义 | `Partial` | 当前 `matches()` 未接组成员关系 |
| Broadcast target | `Partial` | 线包可表达，缺少细粒度策略 |
| Bulk descriptor 运行时 | `SpecOnly` | 类型存在，运行时未消费 |
| QoS 到 transport 策略 | `Partial` | 字段已存在，调度策略未成型 |
| C ABI 语义接口 | `Partial` | 当前主要提供原始事件轮询，不是完整语义 SDK |

## 2. 已知限制

- TTL 尚未在所有收包路径中强制执行
- `AuthorityLease` 当前不是按目标域多实例管理
- `AuthorityStatus` 虽已定义 payload，但运行时未主动发送
- 当前状态类型集合较小，只覆盖 `VehicleCoreState` 和 `VehicleEvent`
- `BulkChannelDescriptor` 尚无 runtime 消费者
- 当前 payload 编解码是手写小端二进制实现，尚未切到代码生成 schema 工作流

## 3. 对开发者的含义

- 可以把主规范当作统一协议蓝图，但接入前必须对照本页覆盖矩阵
- 如果你需要群组成员执行语义、完整失败路径或 bulk 运行时，请把这些能力视为待补齐项，而不是已可直接依赖的现成功能
- 当规范与实现不一致时，应优先明确你是在做“规范对齐”还是“按当前实现接入”

## 4. 推荐联读

- 主规范：
  [sunray-unified-protocol-spec.md](sunray-unified-protocol-spec.md)
- 接入指南：
  [integration-guide.md](integration-guide.md)
- 场景 walkthrough：
  [scenario-walkthroughs.md](scenario-walkthroughs.md)
