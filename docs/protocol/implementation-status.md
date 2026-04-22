# Sunray Protocol 当前实现状态

本文档描述当前 repo 的实现覆盖、已知限制与对接入者的实际含义。协议应然能力以 [sunray-unified-protocol-spec.md](sunray-unified-protocol-spec.md) 为准；本文档回答的是“当前仓库已经做到哪里”。

## 1. 覆盖矩阵

| 能力项 | 状态 | 说明 |
| --- | --- | --- |
| `SecureEnvelope` 编解码 | `Implemented` | 已支持头部、变长头、payload、checksum |
| `EnvelopeStreamParser` | `Implemented` | 已支持流式拆包与 magic 重同步 |
| `EventBus` | `Implemented` | 已支持 `Envelope` / `Error` / `Link` 三类事件订阅与发布 |
| UDP / TCP transport | `Implemented` | 已支持 `UdpTransport`、`TcpClientPool`、`TcpServer` |
| 语义 payload 编码 | `Implemented` | 当前为手写二进制编码，不是 codegen schema |
| 会话状态推进 | `Partial` | 发起方单向发送四条会话消息，接收方本地推进状态；缺少双向协商、重连恢复与断链状态收敛 |
| 认证 | `Partial` | 当前仅做 `shared_secret` 字符串比较 |
| 能力协商 | `Partial` | capability flags 会记录，但未做能力裁决 |
| 控制权租约 | `Partial` | 单全局租约，无目标域分片，无 `AuthorityStatus` 主动回执 |
| 命令发布 API | `Implemented` | 已覆盖最小命令集合 |
| 命令执行 | `Partial` | 当前只有 runtime 自动结果流，无真实执行器接入 |
| 命令结果流 | `Partial` | 已有 `Received -> Accepted -> InProgress -> Succeeded` 默认路径，失败分支未完备 |
| 状态快照订阅 | `Implemented` | 已支持 `VehicleCoreState`、`Px4StateSnapshot`、`OdomStatusSnapshot`、`UavControlFsmStateSnapshot`、`UavControllerStateSnapshot`、`GimbalParamsSnapshot` |
| 状态事件订阅 | `Implemented` | `VehicleEvent` 可订阅 |
| Group target 线包表达 | `Implemented` | codec 与类型支持 |
| Group target 成员语义 | `Partial` | 当前 `TargetSelector::matches()` 对 `kGroup` 只按 `target_type` 粗匹配，不检查 `group_id` 或成员关系 |
| Broadcast target | `Partial` | 线包可表达，当前也是按 `target_type` 粗匹配，缺少细粒度策略层 |
| `BulkChannelDescriptor` 编解码 | `Implemented` | 类型、traits 与 payload codec 已存在 |
| Bulk descriptor 运行时 | `SpecOnly` | runtime 尚无 bulk consumer 或旁路通道管理 |
| TTL 执行 | `Partial` | `ProtocolCodec::decode(..., now_ms)` 能判过期，但 transport/runtime 收包路径当前不会自动传入 `now_ms` |
| QoS 到 transport 策略 | `Partial` | 字段已存在，但没有形成真正的调度/覆盖策略 |
| C++ runtime facade | `Implemented` | `Runtime` 与辅助类已可直接联调单机最小闭环 |
| C ABI 语义接口 | `Partial` | 当前主要提供原始事件轮询，不是完整 typed SDK |

## 2. 已验证路径

当前仓库已经有回归测试或示例覆盖的路径包括：

- `test_protocol`
  线包 roundtrip、checksum、TTL codec 判定、`AuthorityStatus.session_id` 64 位 roundtrip。
- `test_parser`
  流式拆包与广播/命令混合路径。
- `test_transport_loop`
  TCP 传输下的 `VehicleCoreState` 发布与订阅。
- `test_udp_source_isolation`
  UDP 分源解析与状态事件发布。
- `test_compat_roundtrip`
  单 UAV 最小控制闭环。
- `test_runtime_control_paths`
  会话建立、控制权申请/续租/释放/抢占，以及默认命令结果流。
- `test_uav_state_snapshots`
  UAV 专用快照类型的发布与订阅。
- `smoke_examples_local`
  `example_tcp_command_client`、`example_telemetry_receiver`、`example_discovery_udp`、`example_udp_tcp_bridge` 组合联调。

## 3. 已知限制

- TTL 只在直接调用 `ProtocolCodec::decode(..., now_ms)` 时会判过期；runtime 收包路径当前不会自动拒绝过期消息
- `AuthorityLease` 当前不是按目标域多实例管理
- `AuthorityStatus` 虽已定义 payload，但运行时未主动发送
- 控制权与命令执行判断当前主要绑定 `session_id`，不是完整的“按目标域精确租约”模型
- 当前 UAV 侧关键上行面已覆盖，但 UGV 与更细粒度业务状态仍需继续补齐
- `TargetScope::kGroup` 仍缺真实组成员关系与群组执行器，不能把它当成完整 swarm runtime
- `BulkChannelDescriptor` 尚无 runtime 消费者
- 会话没有双向协商回执、断链恢复和重试策略
- C ABI 没有 `SessionClient` / `CommandPublisher` 这类 typed 语义辅助层
- 当前 payload 编解码是手写小端二进制实现，尚未切到代码生成 schema 工作流

## 4. 对开发者的含义

- 可以把主规范当作统一协议蓝图，但接入前必须对照本页覆盖矩阵
- 如果你需要群组成员执行语义、完整失败路径或 bulk 运行时，请把这些能力视为待补齐项，而不是已可直接依赖的现成功能
- 如果你依赖 TTL 拒绝、Authority 显式回执或群组精确路由，需要自己补实现，或者明确把这些能力列入规范对齐工作项
- 当规范与实现不一致时，应优先明确你是在做“规范对齐”还是“按当前实现接入”

## 5. 推荐联读

- 主规范：
  [sunray-unified-protocol-spec.md](sunray-unified-protocol-spec.md)
- 接入指南：
  [integration-guide.md](integration-guide.md)
- 场景 walkthrough：
  [scenario-walkthroughs.md](scenario-walkthroughs.md)
