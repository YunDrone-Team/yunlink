# Yunlink 缺口补全 Todo Checklist

本文档用于回答一个具体问题：哪些测试现在做不了，是因为 `yunlink` core 还没实现；哪些不是 core 问题，而是 Sunray/PX4/SITL/HIL 等业务环境还没有接入；哪些只是测试基础设施和发布门禁还没落地。

本文件是实施型补齐清单，不是当前覆盖率报告。覆盖率总览见 [docs/bindings/test-world-map.md](docs/bindings/test-world-map.md)，当前实现状态见 [docs/protocol/implementation-status.md](docs/protocol/implementation-status.md)。

## 目标与口径

- 目标：把“不能测”的原因拆清楚，并给出彻底补齐所需的实现项、验证项和证据来源。
- 口径：只要没有真实业务闭环证据，就不能把脚手架、fake publisher/subscriber 或 dry-run 记为业务覆盖。
- 范围：本文件只记录补齐工作，不实施 core、bridge、SITL、HIL、netem 或 CI 改造。
- 状态：所有条目默认 `[ ]`，表示尚未完成最终业务验收；某些已有脚手架会在 `Why blocked` 中说明，但仍不视为业务闭环。

## 三类缺失总览

| Category | 含义 | 本阶段处理 |
| --- | --- | --- |
| `core-blocked` | 协议、runtime、C ABI、SDK 或执行器 contract 尚未实现完整，导致对应测试不能成立。 | 必须进入 core/API 实现计划。 |
| `business-env-blocked` | core 已有部分能力，但真实 Sunray/PX4/SITL/HIL/ROS graph 未接入，导致只能测 adapter 或 fake loop。 | 必须进入业务集成计划。 |
| `infra-record-only` | 测试脚手架、弱网、soak、报告、门禁未落地；本阶段只记录问题，不要求立即补全。 | `record-only for now`。 |
| `cross-cutting` | 横跨协议、文档、发布、兼容性或观测性，需要随 P0/P1/P2 分批补齐。 | 纳入路线图和一致性检查。 |

## A. Core 未实现导致不能测

- [x] runtime 收包路径强制 TTL freshness，过期 command 返回 `Expired` 或稳定拒绝。
  - Category: `core-blocked`
  - Why blocked: 目前 TTL 已在线包和 codec 层存在，但 runtime/transport 收包路径不会自动传入 `now_ms` 并拒绝过期消息，因此不能测试真实接收端 TTL 执行语义。
  - Required implementation: 在 runtime envelope dispatch 前统一做 TTL freshness 判定；对 command 产生 `CommandPhase::kExpired` 或稳定 `ErrorCode::kTimeout`/拒绝事件；确保 state/event 的 TTL 策略与 command 区分。
  - Validation: 新增 runtime 测试覆盖过期 command 不进入 command subscriber、不触发 auto-result、不进入 external handler；bridge 侧 TTL 失败仍保持 `ttl-expired`。
  - Evidence source: `docs/protocol/implementation-status.md` 的 `TTL 执行 Partial`；`docs/protocol/integration-guide.md` 的 TTL 限制说明；`src/runtime/runtime_core.cpp`。

- [x] session 双向协商、active/lost/invalid/closed 状态收敛。
  - Category: `core-blocked`
  - Why blocked: 当前 `SessionClient::open_active_session` 单向发送 hello/auth/capabilities/ready，接收方本地推进状态，没有双向 ack、拒绝原因、断链 lost 收敛。
  - Required implementation: 增加 session server response 或 status 语义；定义 authentication rejected、capability rejected、protocol mismatch、peer lost、closed 的状态推进规则。
  - Validation: 新增 session 状态机测试覆盖 accepted、auth failed、capability mismatch、peer disconnect、runtime stop、reopen 后旧 session 不复活。
  - Evidence source: `src/runtime/runtime_session.cpp`；`docs/protocol/implementation-status.md` 的 `会话状态推进 Partial`。

- [x] capability negotiation 从记录 flags 升级为 runtime 裁决。
  - Category: `core-blocked`
  - Why blocked: 当前 capability flags 会被记录，但不会阻止不兼容 peer 建立 session 或发送 unsupported command，因此 capability mismatch 不能作为业务级拒绝路径测试。
  - Required implementation: 定义 required/optional capability 集合、peer capability intersection、unsupported feature rejection、capability rejected status/detail。
  - Validation: 新增 session/capability tests，覆盖缺少 external executor、bulk、planning、swarm/group、security capability 时稳定拒绝或降级。
  - Evidence source: `docs/protocol/implementation-status.md` 的 `能力协商 Partial`；`src/runtime/runtime_session.cpp`；`include/yunlink/core/semantic_message_types.hpp`。

- [x] disconnect / reconnect 后 peer、session、authority 的显式失效语义。
  - Category: `core-blocked`
  - Why blocked: 当前已有显式恢复测试，但 runtime 对旧 peer/session/authority 的失效语义仍偏应用层约定，不能可靠测试“断链后不得偷偷恢复 authority”。
  - Required implementation: 在 TCP disconnect、runtime stop、peer reconnect 时标记旧 peer/session stale；authority lease 与 peer generation 绑定。
  - Validation: 新增测试覆盖旧 peer handle 返回稳定错误、旧 session 不可继续发成功 command、reconnect 后必须 reopen/reacquire。
  - Evidence source: `tests/runtime/test_remote_reconnect.cpp`；`tools/testing/dual_host/office_wifi_lab.yaml` recovery suite。

- [x] Authority 按 target/entity/group 分片，不再只有单全局租约。
  - Category: `core-blocked`
  - Why blocked: 当前 `Runtime::current_authority` 只有一个全局 `AuthorityLease`，无法测试同一 runtime 同时持有不同 UAV/UGV/group target 的控制权。
  - Required implementation: 将 authority storage 改为按 normalized `TargetSelector` key 管理；claim/renew/release/preempt 都必须按 target 域执行。
  - Validation: 新增 `1 GCS -> 2 UAV` core 级测试，验证 UAV-1 和 UAV-2 authority 互不覆盖；新增 release 只释放对应 target。
  - Evidence source: `src/runtime/runtime_authority.cpp`；`docs/protocol/implementation-status.md` 的 `控制权租约 Partial`。

- [x] 主动 `AuthorityStatus` 回执与 rejected/preempted/released/expired 原因。
  - Category: `core-blocked`
  - Why blocked: `AuthorityStatus` payload 已定义，但 runtime 不主动发送，ground 端不能可靠区分 claim 被拒、preempt 成功、lease 过期、release 完成。
  - Required implementation: 在 authority request/renew/release/preempt 处理后向请求方发送 typed `AuthorityStatus`；包含 session、target、state、reason_code、detail。
  - Validation: 新增 subscribe/receive authority status 测试；双机 competition suite 改为验证 status，而不是只从 command 是否成功侧推断。
  - Evidence source: `include/yunlink/core/semantic_message_types.hpp`；`src/runtime/runtime_authority.cpp`；`docs/protocol/yunlink-protocol-spec.md`。

- [x] command 失败分支完整化：no-session、no-authority、wrong-target、expired、executor-timeout、session-lost、authority-lost。
  - Category: `core-blocked`
  - Why blocked: auto-result 目前只覆盖 `Received -> Accepted -> InProgress -> Succeeded`；失败分支大多依赖 bridge 自行推导或静默 return。
  - Required implementation: 明确 runtime 层和 external executor 层的失败责任边界；runtime 拒绝类失败必须形成可观测事件或 `CommandResult.Failed/Expired`。
  - Validation: 新增 runtime 测试覆盖 no-active-session、wrong-target、no-authority、expired；bridge 测试覆盖 executor-timeout、session-lost、authority-lost。
  - Evidence source: `src/runtime/runtime_command.cpp`；`sunray_v2/communication/sunray_yunlink_bridge/src/command_tracker.cpp`。

- [x] runtime 外部 executor contract 固化，bridge 和未来 UGV/swarm executor 都走同一结果语义。
  - Category: `core-blocked`
  - Why blocked: `CommandHandlingMode::kExternalHandler` 与 typed inbound command subscribe 已存在，但 executor 生命周期、result ownership、失败 detail/result_code 还没有正式 contract。
  - Required implementation: 文档化并固化 external executor API：收到 command、校验、回 `Received/Accepted/InProgress/Succeeded/Failed`、超时、取消、session/authority 失效。
  - Validation: 新增 conformance tests，bridge、fake UGV executor、fake swarm executor 都能复用同一套 result contract。
  - Evidence source: `tests/runtime/test_external_command_handling.cpp`；`docs/protocol/integration-guide.md`；`ros-sunray-bridge-overview.md`。

- [x] Group target 真实成员关系与 group_id 精确匹配。
  - Category: `core-blocked`
  - Why blocked: 当前 `TargetSelector::matches()` 对 `kGroup` 只按 `target_type` 粗匹配，不检查 `group_id` 或成员关系，无法测试真正 swarm/group 语义。
  - Required implementation: 增加 group membership registry 或 resolver；`kGroup` 必须匹配 group_id 和成员身份；payload 内 group_id 必须与 envelope target 对齐。
  - Validation: 新增 group membership 测试，覆盖错误 group_id 不投递、正确 group 成员投递、非成员不投递。
  - Evidence source: `include/yunlink/core/types.hpp`；`docs/protocol/implementation-status.md` 的 `Group target 成员语义 Partial`。

- [x] Broadcast target 策略层，明确哪些 message family 允许广播。
  - Category: `core-blocked`
  - Why blocked: broadcast 线包可表达，但当前缺少策略层，无法测试 command 是否允许 broadcast、state/event 是否允许 broadcast、错误广播是否拒绝。
  - Required implementation: 定义 message family 与 target scope allowlist；command 默认禁止 broadcast 或必须显式配置。
  - Validation: 新增 target policy 测试覆盖 command broadcast 拒绝、discovery/state event broadcast 允许、错误 target 返回稳定错误。
  - Evidence source: `include/yunlink/core/types.hpp`；`docs/protocol/implementation-status.md` 的 `Broadcast target Partial`。

- [x] `TrajectoryChunkCommand` runtime 语义与 result 聚合策略。
  - Category: `core-blocked`
  - Why blocked: 类型和 codec 已存在，但 runtime 没有轨迹分块生命周期、chunk 顺序、final_chunk、重传/超时和结果聚合策略。
  - Required implementation: 已增加按 `session + peer + target` 的 runtime trajectory accumulator；`chunk_index` 必须从 0 递增，非 final chunk 缓存并回 `Accepted(detail=trajectory-chunk-buffered)`，final chunk 到达后合并 points 并一次性交给 executor；缺块、重复块、chunk timeout 稳定返回 `Failed`。
  - Validation: `test_trajectory_chunk_runtime` 覆盖缺首块、重复块、final_chunk 合并、executor result、chunk timeout；回归 `test_external_command_handling`、`test_runtime_ttl_freshness`、`test_authority_edges`、`test_routing_and_source_validation` 通过。bridge v1 继续在 Sunray raw-control 层返回 unsupported。
  - Evidence source: `include/yunlink/core/semantic_message_types.hpp`；`src/runtime/runtime_command.cpp`；`ros-sunray-bridge-overview.md`。

- [x] `FormationTaskCommand` 群组执行与成员级回执策略。
  - Category: `core-blocked`
  - Why blocked: 类型和 codec 已存在，但没有 group executor、成员级结果聚合、partial success、成员状态回流。
  - Required implementation: 已在 runtime 固化 formation group contract：`FormationTaskCommand` 必须使用 `TargetScope::kGroup`，payload `group_id` 必须与 envelope target 对齐；外部 group executor 使用原始 route/envelope 和同一 `correlation_id` 回群组级与成员级 `CommandResult`，成员级状态由 executor/业务层负责回流。
  - Validation: `test_formation_task_runtime` 覆盖 entity target 拒绝、group_id mismatch 拒绝、有效 group task 交给 fake swarm executor，并验证 group-level / member-level / aggregate result 都保留同一 correlation；回归 `test_trajectory_chunk_runtime`、`test_authority_target_partition`、`test_target_selector`、`test_routing_and_source_validation` 通过。
  - Evidence source: `docs/protocol/scenario-walkthroughs.md` swarm 场景；`include/yunlink/core/semantic_message_types.hpp`。

- [x] `BulkChannelDescriptor` runtime consumer 与旁路通道生命周期。
  - Category: `core-blocked`
  - Why blocked: `BulkChannelDescriptor` 可编解码，但 runtime 对 `MessageFamily::kBulkChannelDescriptor` 直接 return，没有 consumer、授权、生命周期或失败隔离。
  - Required implementation: 已增加 `BulkChannelDescriptor.channel_id/state/detail`、typed publish/subscribe、runtime active descriptor registry；`Ready` descriptor 注册，`Failed/Closed` descriptor 释放，主控制链路与 bulk 生命周期隔离。
  - Validation: `test_bulk_channel_runtime` 覆盖 descriptor delivery、ready 注册、failed 释放、channel failed 后 command plane 仍能 `Succeeded`；回归 `test_protocol`、`test_transport_loop`、`test_udp_source_isolation`、`test_runtime_control_paths`、`test_external_command_handling` 通过。
  - Evidence source: `src/runtime/runtime_core.cpp`；`src/core/semantic_messages_state_bulk.cpp`；`docs/protocol/implementation-status.md` 的 `SpecOnly`。

- [x] QoS 字段映射到 transport 行为：reliable ordered、latest、best effort、bulk。
  - Category: `core-blocked`
  - Why blocked: `QosClass` 字段存在，但没有真正驱动队列覆盖、丢弃策略、重试、latest coalescing 或 bulk 旁路。
  - Required implementation: 已补 runtime QoS policy：command 收包强制 `ReliableOrdered`，`ReliableLatest` state snapshot 按 source/message/target watermark 丢弃迟到旧快照，state event 默认 `BestEffort`，bulk descriptor 强制 `ReliableOrdered` 且 bulk 内容仍不进入主控制链路。完整异步发送队列/重试调度器仍作为后续发布级优化，不再阻塞当前业务语义测试。
  - Validation: `test_qos_runtime` 覆盖 best-effort command 拒绝、state latest stale drop、bulk descriptor qos reject、event best-effort 默认 QoS；回归 `test_bulk_channel_runtime`、`test_trajectory_chunk_runtime`、`test_formation_task_runtime`、`test_runtime_control_paths`、`test_external_command_handling` 通过。
  - Evidence source: `include/yunlink/core/types.hpp`；`docs/protocol/yunlink-protocol-spec.md` 的 QoS 章节。

- [x] protocol/schema version mismatch 在 runtime 层稳定拒绝。
  - Category: `core-blocked`
  - Why blocked: codec 和部分测试有 protocol mismatch 概念，但 runtime 层需要统一拒绝、错误事件和 session 收敛，才能做端到端兼容测试。
  - Required implementation: 已在 `handle_envelope` 入口校验 `protocol_major/header_version/schema_version`；不匹配时发布稳定 error event，command 会返回 `CommandResult.Failed(detail=runtime-protocol-version-mismatch|runtime-schema-version-mismatch)`，业务 handler 不会被调用。
  - Validation: `test_runtime_version_rejection` 覆盖 schema mismatch command 拒绝、protocol/header mismatch state 拒绝；回归 `test_session_security`、`test_runtime_ttl_freshness`、`test_qos_runtime`、`test_protocol`、`test_parser_resilience` 通过。
  - Evidence source: `docs/protocol/implementation-status.md`；`tests/security/test_session_security.cpp`。

- [x] payload 边界、固定容量截断、未知 enum、malformed semantic payload 的稳定错误策略。
  - Category: `core-blocked`
  - Why blocked: parser malformed frame 已覆盖，但 typed semantic payload 的字段边界、未知 enum 和固定字符串截断策略还没有统一到 runtime 错误语义。
  - Required implementation: 已定义 semantic payload policy：字符串编码固定截断到 1024 字节；未知 `CommandKind/CommandPhase/VehicleEventKind/BulkStreamType/BulkChannelState` decode 失败；malformed command 不再产生 auto success，而是 `CommandResult.Failed(detail=semantic-payload-decode-failed)`；malformed state/event/bulk 发布稳定 decode error event。
  - Validation: `test_semantic_payload_policy` 覆盖长字符串截断、未知 enum reject、malformed command failed result、malformed state error event；回归 `test_protocol`、`test_parser`、`test_parser_resilience`、`test_qos_runtime`、`test_runtime_version_rejection`、`test_external_command_handling`、`test_runtime_control_paths` 通过。
  - Evidence source: `src/core/semantic_messages_command.cpp`；`src/core/semantic_messages_session_authority.cpp`；`src/core/semantic_messages_state_bulk.cpp`。

- [x] C ABI typed session/command/state API，而不只靠原始事件轮询。
  - Category: `core-blocked`
  - Why blocked: C ABI 当前主要提供 runtime create/start、connect、session、authority、基础 command publish、vehicle state publish 和 raw event poll；缺 typed subscribe/helper 层。
  - Required implementation: 已新增 `yunlink_session_info_t` + `yunlink_session_describe`、`yunlink_runtime_poll_command_result`、`yunlink_runtime_poll_vehicle_core_state`；保留 raw `yunlink_runtime_poll_event` 兼容旧调用，thin wrapper 不再必须解析 union 才能拿 session/command-result/vehicle-core-state。
  - Validation: `test_c_ffi_v1` 改用 typed session describe 和 typed poll helper 验证完整 loop；`test_c_ffi_contract` 与 `test_c_ffi_loader` 通过。Rust/Python 完整 typed SDK 扩展仍可在 bindings 层继续补，不再阻塞 C ABI 基础语义面。
  - Evidence source: `include/yunlink/c/yunlink_c.h`；`src/c/yunlink_c.cpp`；`docs/protocol/implementation-status.md` 的 `C ABI 语义接口 Partial`。

- [x] C ABI struct layout / `struct_size` / symbol export / tri-platform ABI contract。
  - Category: `core-blocked`
  - Why blocked: C ABI 已有 `struct_size` 字段，但 layout、alignment、symbol export、Windows/macOS/Linux 加载矩阵还不完整，无法支撑发布级 ABI 兼容测试。
  - Required implementation: 已在 `test_c_ffi_contract` 固定关键 public struct 的字段容量、`struct_size` 首字段 offset 和 command result 字段顺序；`test_c_ffi_loader.py` 增加导出符号 allowlist；`tools/bindings/run_all.sh` 和 GitHub Actions macOS/Windows bindings job 纳入 `test_c_ffi_(v1|contract|loader)`。
  - Validation: 本地 `cmake --build build/ci-ninja --target yunlink_ffi test_c_ffi_v1 test_c_ffi_contract && ctest --test-dir build/ci-ninja -R "test_c_ffi_(v1|contract|loader)" --output-on-failure` 通过；三平台 contract 由 CI matrix 执行。
  - Evidence source: `tests/bindings/test_c_ffi_contract.cpp`；`.github/workflows/ci.yml`；`include/yunlink/c/yunlink_c.h`。

- [x] 安全能力从 shared_secret 字符串比较升级到可测试的认证标签、重放保护或 key epoch 策略。
  - Category: `core-blocked`
  - Why blocked: 当前认证只是 shared_secret 字符串比较，`SecurityContext.auth_tag/key_epoch` 尚未形成真实安全语义，无法测试重放、篡改、key rotation。
  - Required implementation: 已增加 security-tag v1：当 runtime 声明 `kCapabilitySecurityTags` 时，出站 envelope 写入 `security.key_epoch` 和基于 shared secret/envelope/payload 的 auth tag；入站强制校验 key epoch、auth tag，并用 `(key_epoch, source, message_id)` 做 replay window。
  - Validation: `test_security_tags` 覆盖正常签名命令、replay reject、tampered payload/tag reject、old key epoch reject；回归 `test_session_security`、`test_routing_and_source_validation`、`test_runtime_version_rejection`、`test_semantic_payload_policy`、`test_external_command_handling` 通过。
  - Evidence source: `include/yunlink/core/types.hpp` `SecurityContext`；`docs/protocol/yunlink-protocol-spec.md` 安全上下文；`docs/protocol/implementation-status.md` 的 `认证 Partial`。

## B. 业务环境未接入导致不能测

- [ ] 真实 Sunray ROS graph integration，不只 fake publisher/subscriber。
  - Category: `business-env-blocked`
  - Why blocked: 现有 ROS bridge 测试证明 mapping/tracker/loopback，但没有启动 Sunray controller/FSM/localization 的真实 ROS graph。
  - Required implementation: 准备 Sunray ROS launch 测试环境，启动真实 topic producers/consumers，并让 bridge 使用默认 topic contract。
  - Validation: `rosnode list`/`rostopic info` 验证 bridge 订阅/发布真实 Sunray topic；发送 Yunlink command 后真实 `/sunray/uav_control_cmd` 被 Sunray 节点消费。
  - Current execution note: 2026-04-25 在当前本机确认 `roscore/roslaunch/rostopic/catkin_make` 不存在，Docker 可用但没有 ROS1 image，Docker context 只有本机 `default/desktop-linux`，没有可用 SSH ROS1 host；因此不能启动真实 Sunray ROS graph，不能打勾。
  - Evidence source: `docs/bindings/ros-sunray-bridge-overview.md`；`sunray_v2/communication/sunray_yunlink_bridge/test/test_bridge_loopback.cpp`。

- [ ] bridge 默认 topic contract 与 `/uav_name`、`/uav_id` 在 Sunray 启动链路中验证。
  - Category: `business-env-blocked`
  - Why blocked: launch smoke 只证明参数存在时 bridge 能启动，未证明 Sunray 正式启动链路会正确设置 `/uav_name`、`/uav_id` 和默认 topic。
  - Required implementation: 在 Sunray launch 里加入 bridge 或 companion launch，明确参数来源和 namespace。
  - Validation: 在 ROS1 Docker/Ubuntu host 上启动 Sunray + bridge，确认 namespace 为 `/uav1` 等真实值，topic 无需 override 即可连通。
  - Evidence source: `sunray_yunlink_bridge/src/bridge_node.cpp`；`docs/bindings/ros1-docker-ubuntu26-guide.md`。

- [ ] PX4 SITL + MAVROS + Sunray + bridge + Rust ground 的 `Takeoff/Goto/Land/Return` 闭环。
  - Category: `business-env-blocked`
  - Why blocked: 当前没有 PX4 SITL/MAVROS/Sunray 控制闭环；不能证明 Yunlink command 真正驱动飞控状态变化。
  - Required implementation: 建立可复现 SITL compose/launch，包含 PX4、MAVROS、Sunray、bridge、Rust ground client。
  - Validation: 自动化执行 Takeoff、Goto、Land、Return，验证 CommandResult、FSM transition、PX4 local pose/landed state。
  - Evidence source: `docs/bindings/test-world-map.md`；`ros-sunray-bridge-overview.md`。

- [ ] 真实 `CommandResult` 由 Sunray FSM 收敛推导，而不是纯 loopback。
  - Category: `business-env-blocked`
  - Why blocked: loopback 测试手动喂 FSM 状态，不能证明真实 Sunray FSM 的时序、异常和收敛模式。
  - Required implementation: 使用真实 `UAVControlFSMState` topic 驱动 `CommandTracker`，覆盖 Takeoff、Land、Return、Goto、VelocitySetpoint。
  - Validation: 记录 `Received -> Accepted -> InProgress -> Succeeded/Failed` 的真实时间线和 correlation_id。
  - Evidence source: `sunray_yunlink_bridge/src/command_tracker.cpp`；`test_command_tracker.cpp`。

- [ ] raw-control v1 deferred 参数语义：起飞高度、最大速度、返航等待。
  - Category: `business-env-blocked`
  - Why blocked: bridge overview 中 `relative_height_m`、`max_velocity_mps`、`loiter_before_return_s` 明确标记为 `deferred`，当前只映射 Sunray command enum，不能证明这些 Yunlink intent 参数被真实执行器尊重。
  - Required implementation: 明确 Sunray raw control 是否能承接这些参数；若不能，设计 planning bridge 或 Sunray 配置层映射策略，并定义不支持时的稳定 result detail。
  - Validation: 在 SITL 或真实 Sunray graph 中发送带高度、速度、返航等待参数的命令，验证参数生效或稳定返回 unsupported/deferred detail。
  - Evidence source: `docs/bindings/ros-sunray-bridge-overview.md` 的 command mapping 表；`sunray_yunlink_bridge/src/ros_adapter.cpp`。

- [ ] 状态上行 freshness：Px4State 20 Hz、controller 20 Hz、FSM/Odom heartbeat。
  - Category: `business-env-blocked`
  - Why blocked: mapping 测试验证字段转换，但没有真实频率、heartbeat、节流和丢包条件下的 freshness 证据。
  - Required implementation: 在真实或 SITL ROS graph 中采集 bridge 上行 timestamp、source topic rate、Yunlink receive rate。
  - Validation: 验证 Px4StateSnapshot 和 controller snapshot 约 20 Hz；FSM 变更即发且 2 Hz heartbeat；Odom 变更即发且 1 Hz heartbeat。
  - Evidence source: `bridge_node.cpp` `publish_uplinks`；`ros-sunray-bridge-overview.md` 状态上行规则。

- [ ] `VehicleEvent` 正式对外面与真实事件来源。
  - Category: `business-env-blocked`
  - Why blocked: overview 明确一期不正式发布 `VehicleEvent`，当前只有 snapshots，没有定义 Sunray/PX4 哪些状态变化会上升为事件。
  - Required implementation: 定义 event taxonomy、来源 topic、去重/节流、severity、correlation_id/session_id 关联和与 snapshot 的边界。
  - Validation: 在真实或 SITL ROS graph 中触发 arm/disarm、mode change、failsafe、localization degraded 等事件，验证 ground 端收到稳定 `VehicleEvent`。
  - Evidence source: `docs/bindings/ros-sunray-bridge-overview.md` deferred 表；`docs/protocol/yunlink-protocol-spec.md` event 语义。

- [ ] Velocity body-frame 高度合成在真实 PX4 state 下验证。
  - Category: `business-env-blocked`
  - Why blocked: 单元测试用 fake `Px4State` 验证 fixed_height 合成，未证明真实 PX4 local pose 与 Sunray body velocity command 兼容。
  - Required implementation: 在 SITL 或真实 Sunray graph 中发送 body-frame velocity，并检查 fixed_height 使用最新 PX4 高度。
  - Validation: 验证 `MOVE_VELOCITY_BODY` command 的 `desired_body_xy_vel` 和 `fixed_height` 与实时 PX4 state 一致。
  - Evidence source: `sunray_yunlink_bridge/src/ros_adapter.cpp`；`test_command_mappings.cpp`。

- [ ] ROS publish 失败、topic 未订阅、Sunray FSM 不收敛、执行超时的失败场景。
  - Category: `business-env-blocked`
  - Why blocked: 当前 bridge 的 `ros-publish-failed` 很难在真实 ROS publisher 语义下触发，FSM 不收敛和执行超时还缺业务环境。
  - Required implementation: 准备故障注入 launch：不启动 Sunray consumer、冻结 FSM、错误 namespace、关闭 localization/PX4 state。
  - Validation: command 返回 `ros-publish-failed`、`execution-timeout`、`missing-px4-state-for-body-velocity-height-synthesis` 等稳定 detail。
  - Evidence source: `bridge_node.cpp`；`ros_adapter.cpp`；`test_command_mappings.cpp`。

- [ ] HIL / 真机安全门控，包括 kill switch、地理围栏、最大速度/高度、人工接管。
  - Category: `business-env-blocked`
  - Why blocked: 真机测试不能只看协议通；必须有安全门控，否则不能把远程控制闭环算作可交付业务覆盖。
  - Required implementation: 定义 HIL checklist、operator procedure、kill switch、geofence、velocity/altitude caps、manual takeover。
  - Validation: HIL 前置安全检查通过；每个 command 有人工中止路径；异常触发后 CommandResult 和 Sunray/PX4 状态可追踪。
  - Evidence source: `docs/bindings/test-world-map.md` HIL 缺口；Sunray/PX4 业务要求。

- [ ] UGV 或非 UAV endpoint 的业务闭环，避免协议只被 UAV 场景验证。
  - Category: `business-env-blocked`
  - Why blocked: 协议定义了 `AgentType::kUgv` 等类型，但当前主要围绕 UAV/Sunray，不能证明协议对非 UAV 控制成立。
  - Required implementation: 准备 fake UGV executor 或真实 UGV adapter，验证 velocity/trajectory/state/event。
  - Validation: UGV endpoint 完成 connect、session、authority、velocity command、state event、failure result。
  - Evidence source: `include/yunlink/core/types.hpp` AgentType；`docs/protocol/scenario-walkthroughs.md` UGV 场景。

- [ ] planning bridge 二期：`planning_cmd/state`、TrajectoryChunk、FormationTask。
  - Category: `business-env-blocked`
  - Why blocked: bridge v1 明确不接 planning，也明确对 `TrajectoryChunkCommand` 和 `FormationTaskCommand` 返回 unsupported。
  - Required implementation: 设计 Sunray planning bridge，接入 `UAVPlanningCMD/UAVPlanningState`，明确 trajectory 和 formation 的 Sunray 侧 contract。
  - Validation: planning command 能被 Sunray planner 消费；planning state 上行；TrajectoryChunk/FormationTask 不再只返回 `unsupported-by-sunray-raw-control-v1`。
  - Evidence source: `docs/bindings/ros-sunray-bridge-overview.md` unsupported/deferred 表；`sunray_yunlink_bridge/src/ros_adapter.cpp`。

## C. 测试基础设施与门禁缺失

本节全部为 `record-only for now`。这些问题会影响可信度和发布节奏，但本阶段不要求补全，也不应把脚手架存在误记为业务覆盖。

- [ ] netem profile 真实矩阵：delay、jitter、loss、reorder、duplication、短断网。
  - Category: `infra-record-only`
  - Why blocked: `tools/testing/netem/profiles` 和 apply/clear 脚本已存在，但主要是 dry-run/脚手架，没有把 profile 纳入真实双机验收。
  - Required implementation: record-only for now；未来将 netem profile 绑定到 dual-host suite，记录 profile、case、latency、failure reason。
  - Validation: 未来每个 profile 至少跑 baseline/recovery/competition/routing 一轮，并生成 summary。
  - Evidence source: `docs/bindings/network-profiles.md`；`tools/testing/netem/`。

- [ ] 30min / 2h / overnight soak。
  - Category: `infra-record-only`
  - Why blocked: tests/soak 目录与 perf 脚手架存在，但没有长期运行、周期性 command/state/authority renew 的证据。
  - Required implementation: record-only for now；未来实现 soak runner，支持固定时长、固定 topology、周期性操作。
  - Validation: 30min、2h、overnight 产物分别包含 pass/fail、资源曲线、延迟分布、恢复事件。
  - Evidence source: `tests/soak/README.md`；`tools/testing/perf/run_perf_suite.py`。

- [ ] CPU、RSS、thread、fd/socket、queue depth、P50/P95/P99 latency 采集。
  - Category: `infra-record-only`
  - Why blocked: 当前 report/perf 可以聚合部分产物，但 runtime、bridge 和 dual-host case 没有统一指标采集 contract。
  - Required implementation: record-only for now；未来给 runner 增加 metrics collector 和 case timeline。
  - Validation: summary JSON/Markdown 包含资源指标和 latency percentile，失败时仍输出部分指标。
  - Evidence source: `tools/testing/report/render_summary.py`；`tools/testing/perf/run_perf_suite.py`。

- [ ] PR / nightly / release 三层 gate。
  - Category: `infra-record-only`
  - Why blocked: CI 目前覆盖 lint、build、CTest、bindings matrix，但双机、netem、soak、ROS bridge/SITL 未进入分层门禁。
  - Required implementation: record-only for now；未来定义 PR=fast, nightly=dual-host/netem/30min, release=HIL/2h/overnight。
  - Validation: GitHub Actions 或外部 lab runner 产出分层报告，release gate 缺项会阻断发布。
  - Evidence source: `.github/workflows/ci.yml`；`docs/bindings/testing-todo-checklist.md`。

- [ ] 失败自动采集：host logs、runtime stdout/stderr、case summary、network profile、commit hash。
  - Category: `infra-record-only`
  - Why blocked: dual-host runner 有 summary 产物，但失败诊断信息还不足以完全复盘弱网/多 host 问题。
  - Required implementation: record-only for now；未来统一日志目录、host metadata、network profile、commit/build id、stdout/stderr。
  - Validation: 故意制造失败后，summary 能定位失败 host、命令、退出码、关键日志和 profile。
  - Evidence source: `tools/testing/dual_host/run_suite.py`；`tools/testing/dual_host/collect_logs.sh`。

- [ ] 双机有线 LAN、Wi-Fi + netem、host clock drift、网卡切换。
  - Category: `infra-record-only`
  - Why blocked: Office Wi-Fi 已实测，但没有有线基线、弱网组合、时钟漂移或网卡切换实验。
  - Required implementation: record-only for now；未来准备 lab topology profiles 和 host preflight。
  - Validation: 每种网络 topology 都有 baseline 与 recovery summary。
  - Evidence source: `tools/testing/dual_host/office_wifi_lab.yaml`；`docs/bindings/dual-host-lab-guide.md`。

- [ ] `2 GCS -> 2 UAV`、`1 GCS -> N UAV`、容量/并发压力入口。
  - Category: `infra-record-only`
  - Why blocked: 现有双机实测覆盖 `2 GCS -> 1 UAV` 与 `1 GCS -> 2 UAV`，但容量/并发矩阵未建。
  - Required implementation: record-only for now；未来扩展 helper scripts 和 runner config，支持 N endpoints、端口池、结果隔离。
  - Validation: 多 GCS 多 UAV 并发 command/state 不串线，authority 按 target 分片后可稳定运行。
  - Evidence source: `tools/bindings/python_ground_dual_uav.py`；`tools/bindings/python_ground_competition.py`。

## D. 本轮补充发现的漏项

- [ ] fuzz：codec/parser/semantic payload fuzz。
  - Category: `cross-cutting`
  - Why blocked: 当前 parser resilience 是手写 case，不能覆盖大量随机 wire/payload 组合。
  - Required implementation: 增加 libFuzzer/AFL 或 property-based fuzz harness，分别覆盖 envelope parser、semantic decode、C ABI input。
  - Validation: CI/nightly 至少跑 smoke fuzz corpus；崩溃输入进入 regression corpus。
  - Evidence source: `tests/core/test_parser_resilience.cpp`；`src/core/` codec files。

- [ ] observability contract：每个 result/detail/result_code 可追踪。
  - Category: `cross-cutting`
  - Why blocked: 当前 detail 字符串已经出现，但 result_code/detail taxonomy 未统一，跨 runtime、bridge、SDK、report 很难自动聚合。
  - Required implementation: 定义 result_code 枚举、detail 命名规则、correlation_id/message_id/session_id 追踪字段。
  - Validation: command failure report 能按 result_code 聚合；bridge 和 runtime 同一失败原因使用同一 taxonomy。
  - Evidence source: `CommandResult` 类型；`bridge_node.cpp` failure detail；`test-world-map.md`。

- [ ] backward compatibility：协议版本、ABI version、bindings wheel install matrix。
  - Category: `cross-cutting`
  - Why blocked: 当前有 ABI version 和 wheel smoke，但缺跨版本 wire compatibility、旧 wheel 加载、新 runtime 兼容测试。
  - Required implementation: 建立 compatibility fixtures，保留旧 wire sample、旧 ABI sample、wheel install matrix。
  - Validation: 新版本能读旧样本或稳定拒绝；ABI break 必须显式 bump；Python/Rust bindings 安装 smoke 覆盖 release artifact。
  - Evidence source: `YUNLINK_FFI_ABI_VERSION`；`.github/workflows/ci.yml`；`tools/bindings/build_python_wheel.sh`。

- [ ] docs drift check：spec、implementation-status、test-world-map、todochecklist 一致性。
  - Category: `cross-cutting`
  - Why blocked: 现在协议规范、实现状态、测试世界地图和本清单都在快速变化，容易出现“规范说有、实现没有、测试误标已完成”。
  - Required implementation: 增加 docs consistency checklist 或脚本，扫描关键能力词：TTL、AuthorityStatus、Group、Bulk、QoS、SITL、HIL。
  - Validation: PR 中修改协议能力时必须同步 implementation-status、test-world-map 或 todochecklist。
  - Evidence source: `docs/protocol/yunlink-protocol-spec.md`；`docs/protocol/implementation-status.md`；`docs/bindings/test-world-map.md`。

- [ ] release artifact verification：Linux/macOS/Windows 产物加载与 smoke。
  - Category: `cross-cutting`
  - Why blocked: CI 覆盖三平台构建与部分 bindings，但 release artifact 的打包、下载后加载、示例运行、符号验证还不完整。
  - Required implementation: 增加 release artifact job，验证 shared library、headers、Rust crate、Python wheel、example smoke。
  - Validation: 从 artifact 安装后运行 C loader、Rust example、Python runtime loop。
  - Evidence source: `.github/workflows/ci.yml`；`tests/ffi/test_c_ffi_loader.py`；`bindings/python/tests`。

## 推荐执行顺序

### P0：先解除 core 阻塞，让关键失败语义可测

- 实现 runtime TTL freshness 与过期 command 结果。
- 实现 session lost/invalid/closed 收敛。
- 实现 authority target 分片与 `AuthorityStatus` 主动回执。
- 固化 external executor result contract。
- 补齐 command 失败分支测试。
- 接入真实 Sunray ROS graph，验证 bridge 默认 topic contract。
- 建立最小 PX4 SITL + Sunray + bridge + Rust ground 闭环。

### P1：补业务闭环和弱网前置证据

- 完成真实 Sunray FSM 推导的 CommandResult。
- 验证状态上行 freshness 和 body-frame velocity 高度合成。
- 增加 ROS/Sunray 故障注入场景。
- 实现 Group target 精确匹配或明确延后 swarm。
- 跑最小 netem profile：delay、loss、短断网。
- 建立 30min soak 和基础 metrics 采集。

### P2：发布级能力和长期路线

- 实现 TrajectoryChunk、FormationTask、Bulk runtime、QoS 行为。
- 增强安全认证、重放保护和 key epoch。
- 完成 C ABI typed API、layout contract、tri-platform artifact verification。
- 接入 HIL / 真机安全门控。
- 建立 2h / overnight soak、nightly/release gate、docs drift check。

## 完成判定

- 所有 `core-blocked` 条目都有对应实现、自动化测试和 implementation-status 更新。
- 所有 `business-env-blocked` 条目至少有 SITL 或真实 ROS graph 验收证据；HIL 条目有安全门控记录。
- 所有 `infra-record-only` 条目仍可暂不实现，但必须有 owner、未来触发条件和验收口径。
- `docs/bindings/test-world-map.md` 的业务覆盖率可以从 `32/100` 更新到至少 `50/100`，且理由来自真实闭环证据。
- `docs/protocol/implementation-status.md` 中与本文件相关的 `Partial` / `SpecOnly` 项已减少或解释清楚。
- `todochecklist.md`、`test-world-map.md`、`implementation-status.md`、`ros-sunray-bridge-overview.md` 不再互相矛盾。
