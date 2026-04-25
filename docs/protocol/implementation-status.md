# Yunlink 协议当前实现状态

本文档描述当前 repo 的实现覆盖、已知限制与对接入者的实际含义。协议应然能力以 [yunlink-protocol-spec.md](yunlink-protocol-spec.md) 为准；本文档回答的是“当前仓库已经做到哪里”。

## 1. 覆盖矩阵

| 能力项 | 状态 | 说明 |
| --- | --- | --- |
| `SecureEnvelope` 编解码 | `Implemented` | 已支持头部、变长头、payload、checksum |
| runtime protocol/schema mismatch 拒绝 | `Implemented` | runtime 收包入口已拒绝 `protocol_major/header_version/schema_version` 不匹配；command 返回稳定 `CommandResult.Failed`，非 command 发布 error event |
| `EnvelopeStreamParser` | `Implemented` | 已支持流式拆包与 magic 重同步 |
| `EventBus` | `Implemented` | 已支持 `Envelope` / `Error` / `Link` 三类事件订阅与发布 |
| UDP / TCP transport | `Implemented` | 已支持 `UdpTransport`、`TcpClientPool`、`TcpServer` |
| 语义 payload 编码 | `Implemented` | 当前为手写二进制编码，不是 codegen schema |
| 语义 payload 稳定错误策略 | `Implemented` | 字符串编码固定截断到 1024 字节；未知 enum decode 失败；malformed command 返回稳定 `CommandResult.Failed`，malformed state/event/bulk 发布 decode error |
| 会话状态推进 | `Implemented` | 已支持 ready ack、active/lost/invalid/closed 收敛，断链后旧 session 不会静默复活 |
| 认证 | `Partial` | 默认仍兼容 `shared_secret` 会话认证；声明 `kCapabilitySecurityTags` 后 runtime 会生成/验证 auth tag、key epoch，并维护 replay window |
| 能力协商 | `Implemented` | capability flags 已作为 runtime required bitset 裁决，不满足能力的 session 进入 invalid |
| 控制权租约 | `Implemented` | authority 已按 normalized target 分片，并主动发送 `AuthorityStatus` grant/reject/preempt/release/expire 回执 |
| 命令发布 API | `Implemented` | 已覆盖最小命令集合 |
| 命令执行 | `Partial` | 当前只有 runtime 自动结果流，无真实执行器接入 |
| 命令结果流 | `Implemented` | runtime gate 失败分支、TTL expired、显式 executor result helper 和 external-handler 关闭 auto-result 已覆盖；真实执行器结果仍属于业务集成层 |
| `TrajectoryChunkCommand` runtime 聚合 | `Implemented` | runtime 已按 session/peer/target 缓存分块，校验递增顺序、重复块、缺块和 chunk timeout；final chunk 到达后合并后交给 executor |
| `FormationTaskCommand` runtime 合同 | `Implemented` | runtime 已要求 formation 命令使用 `kGroup` target，且 payload `group_id` 必须与 envelope target 对齐；外部 group executor 可用同一 correlation 回群组/成员级结果 |
| 状态快照订阅 | `Implemented` | 已支持 `VehicleCoreState`、`Px4StateSnapshot`、`OdomStatusSnapshot`、`UavControlFsmStateSnapshot`、`UavControllerStateSnapshot`、`GimbalParamsSnapshot` |
| 状态事件订阅 | `Implemented` | `VehicleEvent` 可订阅 |
| Group target 线包表达 | `Implemented` | codec 与类型支持 |
| Group target 成员语义 | `Implemented` | `TargetSelector::matches()` 已按 endpoint `group_ids` 精确匹配 group_id |
| Broadcast target | `Implemented` | command broadcast 被 runtime policy 拒绝，state/event broadcast 允许按目标类型分发 |
| `BulkChannelDescriptor` 编解码 | `Implemented` | 类型、traits 与 payload codec 已存在，包含 `channel_id/state/detail` 生命周期字段 |
| Bulk descriptor 运行时 | `Implemented` | runtime 已提供 typed publish/subscribe、active descriptor registry，并在 `Failed/Closed` 时释放 registry；bulk 失败不阻塞主 command plane |
| TTL 执行 | `Implemented` | runtime 收包入口已执行 TTL freshness；过期 command 返回 `CommandResult.Expired(detail=runtime-ttl-expired)` |
| QoS 到 runtime/transport 策略 | `Partial` | command 已强制 `ReliableOrdered`，state `ReliableLatest` 已丢弃迟到旧快照，event 默认 `BestEffort`，bulk descriptor 强制 `ReliableOrdered`；尚未实现完整异步发送队列调度器 |
| C++ runtime facade | `Implemented` | `Runtime` 与辅助类已可直接联调单机最小闭环 |
| C ABI 语义接口 | `Partial` | 已提供 typed session describe、typed command-result poll、typed vehicle-core-state poll 与基础 typed command/state publish；PX4/Odom/FSM/Bulk 等完整 typed SDK 仍待扩展 |
| C ABI layout / symbol contract | `Implemented` | `test_c_ffi_contract` 固定关键 struct layout/字段容量，`test_c_ffi_loader` 校验共享库加载和导出符号 allowlist；CI 三平台 bindings job 纳入 FFI contract/loader |

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

- 当前 UAV 侧关键上行面已覆盖，但 UGV 与更细粒度业务状态仍需继续补齐
- `TargetScope::kGroup` 已具备 group_id 成员匹配，但真实 swarm coordinator / group executor 仍属于业务层
- QoS 已有 runtime policy，但还不是完整异步发送队列、重试和 backpressure 调度器
- security tag v1 可测试 auth tag、replay 和 key epoch，但不是正式密码学强度的 AEAD/HMAC 方案
- C ABI 已有核心 typed helper，但 PX4/Odom/FSM/Bulk 等完整 typed SDK 仍待 bindings 层继续扩展
- 当前 payload 编解码是手写小端二进制实现，尚未切到代码生成 schema 工作流

## 4. 对开发者的含义

- 可以把主规范当作统一协议蓝图，但接入前必须对照本页覆盖矩阵
- 如果你需要真实 Sunray/PX4/SITL/HIL、UGV adapter 或 swarm executor，请把它们视为业务环境集成项，而不是 core runtime 已经能单独证明的能力
- 如果你依赖发布级安全强度或完整 QoS 调度，需要在当前 v1 runtime policy 之上继续做安全/传输工程化
- 当规范与实现不一致时，应优先明确你是在做“规范对齐”还是“按当前实现接入”

## 5. 推荐联读

- 主规范：
  [yunlink-protocol-spec.md](yunlink-protocol-spec.md)
- 接入指南：
  [integration-guide.md](integration-guide.md)
- 场景 walkthrough：
  [scenario-walkthroughs.md](scenario-walkthroughs.md)
