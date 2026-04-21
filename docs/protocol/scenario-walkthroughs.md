# Sunray Protocol 场景 Walkthrough

本文档提供三个协议落地场景的完整 walkthrough。这里强调“开发者如何走通一个闭环”；字段与状态定义仍以 [sunray-unified-protocol-spec.md](sunray-unified-protocol-spec.md) 为准。

## 1. 场景 A：地面站控制单架 UAV

![单 UAV 场景](../diagrams/plantuml/svg/scenario_single_uav_control.svg)

### 场景目标

地面站与单架 UAV 建立会话，申请控制权，下发 `GotoCommand`，接收命令结果，并持续订阅 `VehicleCoreState` 与 `VehicleEvent`。

### 参与方

- 地面站 `Ground Station`
- UAV 机载协议端点 `Onboard Computer`
- 机载执行器 `Vehicle Executor`

### 成功路径

1. 建立可靠链路并选择 peer route
2. 发送 `SessionHello`、`SessionAuthenticate`、`SessionCapabilities`、`SessionReady`
3. 会话进入 `Active`
4. 地面站发出 `AuthorityRequest(action=Claim)`
5. UAV 端授予控制权
6. 地面站发送 `GotoCommand`
7. UAV 端回传 `Received`、`Accepted`、`InProgress`、`Succeeded`
8. UAV 端持续上报 `VehicleCoreState`，必要时上报 `VehicleEvent`

### 关键字段

| 关注项 | 说明 |
| --- | --- |
| `session_id` | 会话必须非零且在命令与结果中保持一致 |
| `correlation_id` | 命令根消息等于自身 `message_id`；结果指向命令 `message_id` |
| `target` | 应为 `TargetSelector::for_entity(kUav, <id>)` |
| `ttl_ms` | 运动命令应显式设置 |

### 失败分支

- 共享密钥不匹配：会话不进入 `Active`
- 未申请到控制权：`GotoCommand` 不应被执行
- 目标 UAV ID 不匹配：UAV 端应忽略该命令
- 命令过期：应进入 `Expired` 或被拒绝执行

### SDK 映射

- `SessionClient::open_active_session()`
- `Runtime::request_authority()`
- `CommandPublisher::publish_goto()`
- `EventSubscriber::subscribe_command_results()`
- `StateSubscriber::subscribe_vehicle_core()`

### 最小闭环检查表

- 是否拿到非零 `session_id`
- `SessionServer::has_active_session()` 是否为真
- 是否看到命令结果流且 `correlation_id` 指向原命令
- 是否收到 `VehicleCoreState`

## 2. 场景 B：地面站控制单台 UGV

![单 UGV 场景](../diagrams/plantuml/svg/scenario_single_ugv_control.svg)

### 场景目标

地面站对单台 UGV 建立控制链路，申请控制权，下发 `VelocitySetpointCommand` 或轨迹分块命令，并持续观察车辆状态与故障事件。

### 成功路径

1. 建立会话并进入 `Active`
2. 地面站申请 UGV 单体控制权
3. 地面站周期发送 `VelocitySetpointCommand`，必要时发送 `TrajectoryChunkCommand`
4. UGV 端回传命令结果流
5. UGV 端周期发送 `VehicleCoreState`，故障时发送 `VehicleEvent(kind=Fault)`

### 关键字段

| 关注项 | 说明 |
| --- | --- |
| `target` | 应为 UGV 单体目标 |
| `body_frame` | 标明速度指令坐标系 |
| `ttl_ms` | 连续控制命令必须短 TTL |
| `VehicleEvent.severity` | 故障等级 |

### 失败分支

- 控制权被其他端持有且不允许抢占
- 连续控制链路抖动导致命令超时
- 车辆状态仍在上报，但控制命令不再被接受，应优先排查控制权或会话状态

### SDK 映射

- `CommandPublisher::publish_velocity_setpoint()`
- `CommandPublisher::publish_trajectory_chunk()`
- `EventSubscriber::subscribe_vehicle_event()`

### 最小闭环检查表

- 是否持有该 UGV 的控制权租约
- `VelocitySetpointCommand` 是否具备短 TTL
- 是否收到 `Fault` 或其它关键事件
- 快照与事件是否未相互污染

## 3. 场景 C：地面站控制 UAV 集群

![Swarm 场景](../diagrams/plantuml/svg/scenario_swarm_control.svg)

### 场景目标

地面站对一个 UAV 群组目标发送 `FormationTaskCommand`，并获取群组级回执以及成员级状态/事件回流。

### 参与方

- 地面站
- 集群会话或协调端点
- 群组执行器
- 至少两个成员 UAV

### 成功路径

1. 地面站与集群端点建立 `Session`
2. 地面站对 `TargetScope::kGroup` 目标申请控制权
3. 地面站发送 `FormationTaskCommand`，`target.group_id` 与 payload `group_id` 一致
4. 集群端点返回群组级 `CommandResult`
5. 各成员 UAV 回传各自状态快照和关键事件

### 关键字段

| 关注项 | 说明 |
| --- | --- |
| `target.scope` | 必须为 `kGroup` |
| `target.group_id` | 群组目标标识 |
| `FormationTaskCommand.group_id` | 应与 envelope 一致 |
| `CommandResult.correlation_id` | 统一指向群组任务根消息 |

### 失败分支

- 群组不存在或成员为空
- 只有部分成员在线，群组任务只能部分接受
- 个别成员执行失败，群组级终态应能体现失败或部分成功策略

### SDK 映射

- `CommandPublisher::publish_formation_task()`
- `EventSubscriber::subscribe_command_results()`
- `StateSubscriber::subscribe_vehicle_core()`

### 最小闭环检查表

- 是否真正使用 `kGroup` 目标，而不是应用层拆多条单播
- `group_id` 是否在 envelope 与 payload 中一致
- 是否既看到群组级结果，又看到成员级状态回流

## 4. 当前 repo 的落地提示

- 单 UAV 最小闭环已经有较完整的测试覆盖
- 单 UGV 与 Swarm 目前更偏协议语义和消息层验证
- 真实执行器、群组聚合器与 bulk 运行时能力的当前覆盖情况，请参考 [implementation-status.md](implementation-status.md)
