# Yunlink Payload Reference

## 1. Payload Index

下表是当前仓库已注册的全部 payload，数据以 [`message_traits.hpp`](../../include/yunlink/core/semantic/message_traits.hpp) 为准。

| Family | Type | Schema | Payload Type | Summary |
| --- | ---: | ---: | --- | --- |
| `Session` | 1 | 1 | `SessionHello` | 会话 hello 与能力初始声明 |
| `Session` | 2 | 1 | `SessionAuthenticate` | 会话认证请求 |
| `Session` | 3 | 1 | `SessionCapabilities` | 会话能力补充声明 |
| `Session` | 4 | 1 | `SessionReady` | 会话 ready 确认 |
| `Authority` | 1 | 1 | `AuthorityRequest` | 控制权申请、续约、释放、抢占 |
| `Authority` | 2 | 1 | `AuthorityStatus` | 控制权状态回报 |
| `Command` | 1 | 1 | `TakeoffCommand` | 起飞命令 |
| `Command` | 2 | 1 | `LandCommand` | 降落命令 |
| `Command` | 3 | 1 | `ReturnCommand` | 返航命令 |
| `Command` | 4 | 1 | `GotoCommand` | 位置目标命令 |
| `Command` | 5 | 1 | `VelocitySetpointCommand` | 速度设定命令 |
| `Command` | 6 | 1 | `TrajectoryChunkCommand` | 轨迹分块命令 |
| `Command` | 7 | 1 | `FormationTaskCommand` | 编队任务命令 |
| `CommandResult` | 1 | 1 | `CommandResult` | 命令执行阶段与结果 |
| `SystemService` | 1 | 1 | `FeatureListRequest` | 查询功能列表请求 |
| `SystemService` | 2 | 1 | `FeatureListResponse` | 查询功能列表响应 |
| `SystemService` | 3 | 1 | `FeatureGetRequest` | 查询单个功能详情请求 |
| `SystemService` | 4 | 1 | `FeatureGetResponse` | 查询单个功能详情响应 |
| `SystemService` | 5 | 1 | `FeatureStartRequest` | 启动功能请求 |
| `SystemService` | 6 | 1 | `FeatureStartResponse` | 启动功能响应 |
| `SystemService` | 7 | 1 | `FeatureStopRequest` | 停止功能请求 |
| `SystemService` | 8 | 1 | `FeatureStopResponse` | 停止功能响应 |
| `StateSnapshot` | 1 | 1 | `VehicleCoreState` | 基础飞行状态 |
| `StateSnapshot` | 2 | 1 | `Px4StateSnapshot` | PX4 综合状态快照 |
| `StateSnapshot` | 3 | 1 | `OdomStatusSnapshot` | 里程计源与状态摘要 |
| `StateSnapshot` | 4 | 1 | `UavControlFsmStateSnapshot` | 控制 FSM 状态摘要 |
| `StateSnapshot` | 5 | 1 | `UavControllerStateSnapshot` | 控制器内部状态摘要 |
| `StateSnapshot` | 6 | 1 | `GimbalParamsSnapshot` | 云台/视频流参数 |
| `StateSnapshot` | 7 | 1 | `LocalOdomSnapshot` | 局部里程计快照 |
| `StateSnapshot` | 8 | 1 | `UavControlCmdSnapshot` | 控制命令快照 |
| `StateSnapshot` | 9 | 1 | `UavControlStateSnapshot` | 控制总状态快照 |
| `StateSnapshot` | 10 | 1 | `OdomStateSnapshot` | 里程计系统总状态快照 |
| `StateSnapshot` | 11 | 1 | `SunrayRuntimeDiagnosticSnapshot` | Sunray runtime 诊断快照 |
| `StateSnapshot` | 12 | 1 | `CommandExecutionStatusSnapshot` | 命令执行状态快照 |
| `StateEvent` | 1 | 1 | `VehicleEvent` | 稀疏飞行/车辆事件 |
| `BulkChannelDescriptor` | 1 | 1 | `BulkChannelDescriptor` | 大流旁路通道描述 |

## 2. Reusable Composite Types

很多 payload 会复用下面这些基础复合类型。为了避免重复，具体 payload 字段表里直接引用这些名称。

### 2.1 `HeaderSnapshot`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `frame_id` | `string` | - | 坐标系名称 |
| `stamp_ns` | `uint64_t` | `ns` | 时间戳 |

### 2.2 `Vector2f`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `x` | `float` | context-dependent | X 分量 |
| `y` | `float` | context-dependent | Y 分量 |

### 2.3 `Vector3f`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `x` | `float` | context-dependent | X 分量 |
| `y` | `float` | context-dependent | Y 分量 |
| `z` | `float` | context-dependent | Z 分量 |

### 2.4 `Quaternionf`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `x` | `float` | - | 四元数 X |
| `y` | `float` | - | 四元数 Y |
| `z` | `float` | - | 四元数 Z |
| `w` | `float` | - | 四元数 W |

### 2.5 `GeoPointSnapshot`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `latitude_deg` | `double` | `deg` | 纬度 |
| `longitude_deg` | `double` | `deg` | 经度 |
| `altitude_m` | `double` | `m` | 高度 |

### 2.6 `PoseSnapshot`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `position_m` | `Vector3f` | `m` | 位置 |
| `orientation` | `Quaternionf` | - | 姿态 |

### 2.7 `TwistSnapshot`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `linear_mps` | `Vector3f` | `m/s` | 线速度 |
| `angular_radps` | `Vector3f` | `rad/s` | 角速度 |

### 2.8 `TransformSnapshot`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `header` | `HeaderSnapshot` | - | 父坐标系与时间戳 |
| `child_frame_id` | `string` | - | 子坐标系名称 |
| `translation_m` | `Vector3f` | `m` | 平移 |
| `rotation` | `Quaternionf` | - | 旋转 |

### 2.9 `OdometrySnapshot`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `header` | `HeaderSnapshot` | - | 父坐标系与时间戳 |
| `child_frame_id` | `string` | - | 子坐标系名称 |
| `pose` | `PoseSnapshot` | - | 位姿 |
| `pose_covariance` | `double[36]` | context-dependent | 位姿协方差，按固定 36 项编码 |
| `twist` | `TwistSnapshot` | - | 速度 |
| `twist_covariance` | `double[36]` | context-dependent | 速度协方差，按固定 36 项编码 |

### 2.10 `PositionTargetSnapshot`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `header` | `HeaderSnapshot` | - | 坐标系与时间戳 |
| `coordinate_frame` | `uint8_t` | - | 坐标系编码 |
| `type_mask` | `uint16_t` | - | 屏蔽位 |
| `position_m` | `Vector3f` | `m` | 位置目标 |
| `velocity_mps` | `Vector3f` | `m/s` | 速度目标 |
| `acceleration_or_force` | `Vector3f` | `m/s^2` or force | 加速度或力 |
| `yaw_rad` | `float` | `rad` | 偏航角目标 |
| `yaw_rate_radps` | `float` | `rad/s` | 偏航角速度目标 |

### 2.11 `AttitudeTargetSnapshot`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `header` | `HeaderSnapshot` | - | 坐标系与时间戳 |
| `type_mask` | `uint8_t` | - | 实际编码为 1 字节 |
| `orientation` | `Quaternionf` | - | 目标姿态 |
| `body_rate_radps` | `Vector3f` | `rad/s` | 机体系角速度 |
| `thrust` | `float` | context-dependent | 推力 |

### 2.12 `SunrayTopicDiagnosticSnapshot`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `key` | `string` | - | 内部诊断 key |
| `topic` | `string` | - | 主题名 |
| `configured` | `bool` | - | 是否已配置 |
| `has_message` | `bool` | - | 是否已收到消息 |
| `publisher_count` | `uint32_t` | - | 发布者数量 |
| `message_count` | `uint64_t` | - | 累计消息数 |
| `hz` | `float` | `Hz` | 当前频率 |
| `age_ms` | `uint32_t` | `ms` | 最新消息年龄 |
| `stale` | `bool` | - | 是否陈旧 |
| `status` | `string` | - | 状态文本 |
| `detail` | `string` | - | 细节文本 |

## 3. Session Payloads

源码：

- [`include/yunlink/core/semantic/session_authority_types.hpp`](../../include/yunlink/core/semantic/session_authority_types.hpp)
- [`src/core/semantic_messages_session_authority.cpp`](../../src/core/semantic_messages_session_authority.cpp)

### 3.1 `SessionHello`

Summary: 会话 hello 与初始能力声明。

Identifiers:

- `message_family = Session`
- `message_type = 1`
- `schema_version = 1`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `node_name` | `string` | - | 节点名称 |
| `capability_flags` | `uint32_t` | bitmask | 能力位集合 |

### 3.2 `SessionAuthenticate`

Summary: 会话认证请求。

Identifiers:

- `message_family = Session`
- `message_type = 2`
- `schema_version = 1`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `shared_secret` | `string` | - | 共享密钥文本 |

### 3.3 `SessionCapabilities`

Summary: 会话能力补充声明。

Identifiers:

- `message_family = Session`
- `message_type = 3`
- `schema_version = 1`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `capability_flags` | `uint32_t` | bitmask | 能力位集合 |

### 3.4 `SessionReady`

Summary: 会话 ready 确认。

Identifiers:

- `message_family = Session`
- `message_type = 4`
- `schema_version = 1`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `accepted_protocol_major` | `uint8_t` | - | 被接受的协议 major 版本 |

## 4. Authority Payloads

源码：

- [`include/yunlink/core/semantic/session_authority_types.hpp`](../../include/yunlink/core/semantic/session_authority_types.hpp)
- [`src/core/semantic_messages_session_authority.cpp`](../../src/core/semantic_messages_session_authority.cpp)

### 4.1 `AuthorityRequest`

Summary: 控制权申请、抢占、续约或释放请求。

Identifiers:

- `message_family = Authority`
- `message_type = 1`
- `schema_version = 1`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `action` | `AuthorityAction` as `uint8_t` | - | 请求动作 |
| `source` | `ControlSource` as `uint8_t` | - | 控制来源 |
| `lease_ttl_ms` | `uint32_t` | `ms` | 租约时长 |
| `allow_preempt` | `bool` | - | 是否允许抢占 |

### 4.2 `AuthorityStatus`

Summary: 控制权状态与原因回报。

Identifiers:

- `message_family = Authority`
- `message_type = 2`
- `schema_version = 1`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `state` | `AuthorityState` as `uint8_t` | - | 当前控制权状态 |
| `session_id` | `uint64_t` | - | 当前关联会话 ID |
| `lease_ttl_ms` | `uint32_t` | `ms` | 当前租约时长 |
| `reason_code` | `uint16_t` | - | 结果原因码 |
| `detail` | `string` | - | 结果说明 |

## 5. Command Payloads

源码：

- [`include/yunlink/core/semantic/command_types.hpp`](../../include/yunlink/core/semantic/command_types.hpp)
- [`src/core/semantic_messages_command.cpp`](../../src/core/semantic_messages_command.cpp)

命令类 payload 通常和下面这些 envelope 字段一起理解：

- `session_id`
- `message_id`
- `correlation_id`
- `target`
- `ttl_ms`

### 5.1 `TakeoffCommand`

Summary: 起飞命令。

Identifiers:

- `message_family = Command`
- `message_type = 1`
- `schema_version = 1`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `relative_height_m` | `float` | `m` | 相对起飞高度 |
| `max_velocity_mps` | `float` | `m/s` | 最大起飞速度 |

### 5.2 `LandCommand`

Summary: 降落命令。

Identifiers:

- `message_family = Command`
- `message_type = 2`
- `schema_version = 1`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `max_velocity_mps` | `float` | `m/s` | 最大降落速度 |

### 5.3 `ReturnCommand`

Summary: 返航命令。

Identifiers:

- `message_family = Command`
- `message_type = 3`
- `schema_version = 1`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `loiter_before_return_s` | `float` | `s` | 返航前盘旋时间 |

### 5.4 `GotoCommand`

Summary: 位置目标命令。

Identifiers:

- `message_family = Command`
- `message_type = 4`
- `schema_version = 1`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `x_m` | `float` | `m` | 目标 X |
| `y_m` | `float` | `m` | 目标 Y |
| `z_m` | `float` | `m` | 目标 Z |
| `yaw_rad` | `float` | `rad` | 目标偏航角 |

### 5.5 `VelocitySetpointCommand`

Summary: 速度设定命令。

Identifiers:

- `message_family = Command`
- `message_type = 5`
- `schema_version = 1`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `vx_mps` | `float` | `m/s` | X 方向速度 |
| `vy_mps` | `float` | `m/s` | Y 方向速度 |
| `vz_mps` | `float` | `m/s` | Z 方向速度 |
| `yaw_rate_radps` | `float` | `rad/s` | 偏航角速度 |
| `body_frame` | `bool` | - | 是否使用机体系 |

### 5.6 `TrajectoryChunkCommand`

Summary: 轨迹分块命令。

Identifiers:

- `message_family = Command`
- `message_type = 6`
- `schema_version = 1`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `chunk_index` | `uint32_t` | - | 分块序号 |
| `final_chunk` | `bool` | - | 是否最后一块 |
| `points` | `vector<TrajectoryPoint>` | - | 轨迹点数组 |

`TrajectoryPoint` 编码顺序：

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `x_m` | `float` | `m` | 点位 X |
| `y_m` | `float` | `m` | 点位 Y |
| `z_m` | `float` | `m` | 点位 Z |
| `vx_mps` | `float` | `m/s` | 期望速度 X |
| `vy_mps` | `float` | `m/s` | 期望速度 Y |
| `vz_mps` | `float` | `m/s` | 期望速度 Z |
| `yaw_rad` | `float` | `rad` | 期望偏航角 |
| `dt_ms` | `uint32_t` | `ms` | 到下一个点的时间间隔 |

### 5.7 `FormationTaskCommand`

Summary: 编队任务命令。

Identifiers:

- `message_family = Command`
- `message_type = 7`
- `schema_version = 1`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `group_id` | `uint32_t` | - | 目标组 ID |
| `formation_shape` | `uint8_t` | - | 编队形态编码 |
| `spacing_m` | `float` | `m` | 编队间距 |
| `label` | `string` | - | 编队任务标签 |

## 6. Command Result Payload

源码：

- [`include/yunlink/core/semantic/command_types.hpp`](../../include/yunlink/core/semantic/command_types.hpp)
- [`src/core/semantic_messages_command.cpp`](../../src/core/semantic_messages_command.cpp)

### 6.1 `CommandResult`

Summary: 命令执行阶段、结果码和文本说明。

Identifiers:

- `message_family = CommandResult`
- `message_type = 1`
- `schema_version = 1`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `command_kind` | `CommandKind` as `uint16_t` | - | 对应的命令类型 |
| `phase` | `CommandPhase` as `uint8_t` | - | 当前阶段 |
| `result_code` | `uint16_t` | - | 结果码 |
| `progress_percent` | `uint8_t` | `%` | 当前进度 |
| `detail` | `string` | - | 文本说明 |

Decode constraints:

- `command_kind` 必须在当前实现支持范围内
- `phase` 必须在当前实现支持范围内

## 7. System Service Payloads

源码：

- [`include/yunlink/core/semantic/system_service_types.hpp`](../../include/yunlink/core/semantic/system_service_types.hpp)
- [`src/core/semantic_messages_system_service.cpp`](../../src/core/semantic_messages_system_service.cpp)

这组 payload 可以视为一组语义化服务请求/响应。

### 7.1 `FeatureListRequest`

Summary: 查询功能列表请求。

Identifiers:

- `message_family = SystemService`
- `message_type = 1`
- `schema_version = 1`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `reserved` | `uint8_t` | - | 保留字段 |

### 7.2 `FeatureListResponse`

Summary: 查询功能列表响应。

Identifiers:

- `message_family = SystemService`
- `message_type = 2`
- `schema_version = 1`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `success` | `bool` | - | 请求是否成功 |
| `message` | `string` | - | 响应文本 |
| `feature_names` | `vector<string>` | - | 功能名称列表 |

### 7.3 `FeatureGetRequest`

Summary: 查询单个功能详情请求。

Identifiers:

- `message_family = SystemService`
- `message_type = 3`
- `schema_version = 1`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `feature_name` | `string` | - | 功能名称 |

### 7.4 `FeatureGetResponse`

Summary: 查询单个功能详情响应。

Identifiers:

- `message_family = SystemService`
- `message_type = 4`
- `schema_version = 1`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `success` | `bool` | - | 请求是否成功 |
| `message` | `string` | - | 响应文本 |
| `name` | `string` | - | 功能名 |
| `group` | `string` | - | 功能分组 |
| `running` | `bool` | - | 是否正在运行 |
| `description` | `string` | - | 功能描述 |
| `auto_start` | `bool` | - | 是否自动启动 |
| `depends_on` | `vector<string>` | - | 依赖功能列表 |
| `stop_timeout_sec` | `float` | `s` | 停止超时 |
| `start_preview_units` | `vector<string>` | - | 启动预览 unit 列表 |
| `start_preview_commands` | `vector<string>` | - | 启动预览命令列表 |

### 7.5 `FeatureStartRequest`

Summary: 启动功能请求。

Identifiers:

- `message_family = SystemService`
- `message_type = 5`
- `schema_version = 1`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `feature_name` | `string` | - | 功能名称 |
| `override_args` | `vector<string>` | - | 额外参数 |
| `restart_if_running` | `bool` | - | 若已运行是否重启 |
| `start_with_terminal` | `bool` | - | 是否带终端启动 |

### 7.6 `FeatureStartResponse`

Summary: 启动功能响应。

Identifiers:

- `message_family = SystemService`
- `message_type = 6`
- `schema_version = 1`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `success` | `bool` | - | 请求是否成功 |
| `message` | `string` | - | 响应文本 |
| `feature_name` | `string` | - | 功能名称 |

### 7.7 `FeatureStopRequest`

Summary: 停止功能请求。

Identifiers:

- `message_family = SystemService`
- `message_type = 7`
- `schema_version = 1`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `feature_name` | `string` | - | 功能名称 |
| `force` | `bool` | - | 是否强制停止 |

### 7.8 `FeatureStopResponse`

Summary: 停止功能响应。

Identifiers:

- `message_family = SystemService`
- `message_type = 8`
- `schema_version = 1`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `success` | `bool` | - | 请求是否成功 |
| `message` | `string` | - | 响应文本 |
| `feature_name` | `string` | - | 功能名称 |

## 8. State Snapshot Payloads

源码：

- [`include/yunlink/core/semantic/state_types.hpp`](../../include/yunlink/core/semantic/state_types.hpp)
- [`include/yunlink/core/semantic/state_types_sunray.hpp`](../../include/yunlink/core/semantic/state_types_sunray.hpp)
- [`src/core/state/semantic_messages_state_bulk_core.cpp`](../../src/core/state/semantic_messages_state_bulk_core.cpp)
- [`src/core/state/semantic_messages_state_bulk_extended.cpp`](../../src/core/state/semantic_messages_state_bulk_extended.cpp)
- [`src/core/state/semantic_messages_state_snapshots_core.cpp`](../../src/core/state/semantic_messages_state_snapshots_core.cpp)
- [`src/core/state/semantic_messages_state_snapshots_extended.cpp`](../../src/core/state/semantic_messages_state_snapshots_extended.cpp)

### 8.1 `VehicleCoreState`

Summary: 基础飞行状态。

Identifiers:

- `message_family = StateSnapshot`
- `message_type = 1`
- `schema_version = 1`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `armed` | `bool` | - | 是否解锁 |
| `nav_mode` | `uint8_t` | - | 导航模式编码 |
| `x_m` | `float` | `m` | 位置 X |
| `y_m` | `float` | `m` | 位置 Y |
| `z_m` | `float` | `m` | 位置 Z |
| `vx_mps` | `float` | `m/s` | 速度 X |
| `vy_mps` | `float` | `m/s` | 速度 Y |
| `vz_mps` | `float` | `m/s` | 速度 Z |
| `battery_percent` | `float` | `%` | 电量百分比 |

### 8.2 `Px4StateSnapshot`

Summary: PX4 综合状态快照。

Identifiers:

- `message_family = StateSnapshot`
- `message_type = 2`
- `schema_version = 1`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `header` | `HeaderSnapshot` | - | 坐标系与时间戳 |
| `connected` | `bool` | - | 是否已连接 FCU |
| `rc_available` | `bool` | - | 是否有 RC |
| `armed` | `bool` | - | 是否解锁 |
| `flight_mode` | `uint8_t` | - | 飞行模式编码 |
| `system_status` | `uint8_t` | - | 系统状态编码 |
| `landed_state` | `uint8_t` | - | 起降状态编码 |
| `battery_voltage_v` | `float` | `V` | 电压 |
| `battery_current_a` | `float` | `A` | 电流 |
| `battery_percentage` | `float` | `%` | 电量百分比 |
| `fcu_load` | `uint16_t` | - | FCU 负载 |
| `external_pose` | `PoseSnapshot` | - | 外部位姿 |
| `external_velocity` | `TwistSnapshot` | - | 外部速度 |
| `local_pose` | `PoseSnapshot` | - | 本地位姿 |
| `local_velocity` | `TwistSnapshot` | - | 本地速度 |
| `setpoint_coordinate_frame` | `uint8_t` | - | 设定值坐标系 |
| `setpoint_local_type_mask` | `uint16_t` | - | 本地设定值 mask |
| `pos_setpoint_m` | `Vector3f` | `m` | 位置设定值 |
| `vel_setpoint_mps` | `Vector3f` | `m/s` | 速度设定值 |
| `acc_setpoint_mps2` | `Vector3f` | `m/s^2` | 加速度设定值 |
| `yaw_setpoint_rad` | `float` | `rad` | 偏航设定值 |
| `yaw_rate_setpoint_radps` | `float` | `rad/s` | 偏航角速度设定值 |
| `setpoint_att_type_mask` | `uint16_t` | - | 姿态设定值 mask |
| `orientation_setpoint` | `Quaternionf` | - | 姿态设定值 |
| `body_rate_setpoint_radps` | `Vector3f` | `rad/s` | 机体系角速度设定值 |
| `thrust_setpoint` | `float` | context-dependent | 推力设定值 |
| `satellites` | `uint8_t` | - | 卫星数量 |
| `gps_status` | `int8_t` | - | GPS 状态编码 |
| `gps_service` | `uint8_t` | - | GPS service 编码 |
| `latitude_deg` | `double` | `deg` | 纬度 |
| `longitude_deg` | `double` | `deg` | 经度 |
| `altitude_m` | `double` | `m` | 高度 |
| `latitude_raw_deg` | `double` | `deg` | 原始纬度 |
| `longitude_raw_deg` | `double` | `deg` | 原始经度 |
| `altitude_amsl_m` | `double` | `m` | AMSL 高度 |

### 8.3 `OdomStatusSnapshot`

Summary: 里程计源与状态摘要。

Identifiers:

- `message_family = StateSnapshot`
- `message_type = 3`
- `schema_version = 1`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `external_source_name` | `string` | - | 外部里程计源名称 |
| `external_source_id` | `uint8_t` | - | 外部里程计源 ID |
| `localization_mode_name` | `string` | - | 定位模式名称 |
| `localization_mode` | `uint8_t` | - | 定位模式编码 |
| `has_odometry` | `bool` | - | 是否有里程计数据 |
| `has_relocalization` | `bool` | - | 是否有重定位数据 |
| `odom_timeout` | `bool` | - | 里程计是否超时 |
| `relocalization_data_valid` | `bool` | - | 重定位数据是否有效 |
| `last_odometry_age_ms` | `uint32_t` | `ms` | 最新里程计年龄 |
| `global_frame_id` | `string` | - | 全局坐标系名 |
| `local_frame_id` | `string` | - | 局部坐标系名 |
| `base_frame_id` | `string` | - | 机体坐标系名 |

### 8.4 `UavControlFsmStateSnapshot`

Summary: 控制 FSM 状态摘要。

Identifiers:

- `message_family = StateSnapshot`
- `message_type = 4`
- `schema_version = 1`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `takeoff_relative_height_m` | `double` | `m` | 起飞相对高度 |
| `takeoff_max_velocity_mps` | `double` | `m/s` | 起飞最大速度 |
| `land_type` | `uint8_t` | - | 降落类型编码 |
| `land_max_velocity_mps` | `double` | `m/s` | 降落最大速度 |
| `home_point_m` | `Vector3f` | `m` | Home 点 |
| `control_command` | `uint8_t` | - | 当前控制命令编码 |
| `yunlink_fsm_state` | `uint8_t` | - | Yunlink FSM 状态编码 |

### 8.5 `UavControllerStateSnapshot`

Summary: 控制器内部状态摘要。

Identifiers:

- `message_family = StateSnapshot`
- `message_type = 5`
- `schema_version = 1`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `reference_frame` | `uint8_t` | - | 参考系编码 |
| `controller_type` | `uint8_t` | - | 控制器类型编码 |
| `desired_position_m` | `Vector3f` | `m` | 目标位置 |
| `desired_velocity_mps` | `Vector3f` | `m/s` | 目标速度 |
| `current_position_m` | `Vector3f` | `m` | 当前位置 |
| `current_velocity_mps` | `Vector3f` | `m/s` | 当前速度 |
| `position_error_m` | `Vector3f` | `m` | 位置误差 |
| `velocity_error_mps` | `Vector3f` | `m/s` | 速度误差 |
| `desired_yaw_rad` | `double` | `rad` | 目标偏航角 |
| `current_yaw_rad` | `double` | `rad` | 当前偏航角 |
| `yaw_error_rad` | `double` | `rad` | 偏航误差 |
| `thrust_from_px4` | `double` | context-dependent | PX4 推力 |
| `thrust_from_controller` | `double` | context-dependent | 控制器推力 |

### 8.6 `GimbalParamsSnapshot`

Summary: 云台/视频流参数。

Identifiers:

- `message_family = StateSnapshot`
- `message_type = 6`
- `schema_version = 1`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `stream_type` | `uint8_t` | - | 流类型编码 |
| `encoding_type` | `uint8_t` | - | 编码类型编码 |
| `resolution_width` | `uint16_t` | `px` | 宽度 |
| `resolution_height` | `uint16_t` | `px` | 高度 |
| `bitrate_kbps` | `uint16_t` | `kbps` | 码率 |
| `frame_rate` | `float` | `Hz` | 帧率 |

### 8.7 `LocalOdomSnapshot`

Summary: 局部里程计快照。

Identifiers:

- `message_family = StateSnapshot`
- `message_type = 7`
- `schema_version = 1`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `header` | `HeaderSnapshot` | - | 父坐标系与时间戳 |
| `child_frame_id` | `string` | - | 子坐标系名 |
| `pose` | `PoseSnapshot` | - | 位姿 |
| `pose_covariance` | `double[36]` | context-dependent | 位姿协方差 |
| `twist` | `TwistSnapshot` | - | 速度 |
| `twist_covariance` | `double[36]` | context-dependent | 速度协方差 |

### 8.8 `UavControlCmdSnapshot`

Summary: 控制命令快照。

Identifiers:

- `message_family = StateSnapshot`
- `message_type = 8`
- `schema_version = 1`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `header` | `HeaderSnapshot` | - | 坐标系与时间戳 |
| `cmd_source` | `uint8_t` | - | 命令来源编码 |
| `control_cmd` | `uint8_t` | - | 控制命令编码 |
| `desired_pos_m` | `Vector3f` | `m` | 期望位置 |
| `desired_vel_mps` | `Vector3f` | `m/s` | 期望速度 |
| `desired_acc_mps2` | `Vector3f` | `m/s^2` | 期望加速度 |
| `desired_jerk` | `Vector3f` | context-dependent | 期望 jerk |
| `desired_body_xy_pos_m` | `Vector2f` | `m` | 机体系 XY 位置 |
| `desired_body_xy_vel_mps` | `Vector2f` | `m/s` | 机体系 XY 速度 |
| `fixed_height_m` | `float` | `m` | 固定高度 |
| `desired_wgs84_pos` | `GeoPointSnapshot` | - | WGS84 目标点 |
| `yaw_mode` | `uint8_t` | - | 偏航模式编码 |
| `desired_yaw_rad` | `float` | `rad` | 期望偏航角 |
| `desired_yaw_rate_radps` | `float` | `rad/s` | 期望偏航角速度 |

Important:

- 这个 payload 本身没有单独的 “yunlink metadata” 字段
- 与会话、路由、回复匹配相关的信息仍然在外层 `SecureEnvelope`

### 8.9 `UavControlStateSnapshot`

Summary: 控制总状态快照。

Identifiers:

- `message_family = StateSnapshot`
- `message_type = 9`
- `schema_version = 1`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `header` | `HeaderSnapshot` | - | 坐标系与时间戳 |
| `agent_name` | `string` | - | agent 名称 |
| `agent_id` | `uint8_t` | - | agent ID |
| `controller_types` | `uint8_t` | - | 控制器类型集合编码 |
| `takeoff_relative_height_m` | `double` | `m` | 起飞相对高度 |
| `takeoff_max_velocity_mps` | `double` | `m/s` | 起飞最大速度 |
| `land_type` | `uint8_t` | - | 降落类型编码 |
| `land_max_velocity_mps` | `double` | `m/s` | 降落最大速度 |
| `home_point_m` | `Vector3f` | `m` | Home 点 |
| `control_state` | `uint8_t` | - | 控制状态编码 |
| `last_cmd` | `UavControlCmdSnapshot` | - | 最近命令快照 |
| `self_odom` | `OdometrySnapshot` | - | 自身里程计 |
| `odometry_lost` | `bool` | - | 里程计是否丢失 |
| `odometry_valid` | `bool` | - | 里程计是否有效 |
| `controller_output_type` | `uint8_t` | - | 控制输出类型编码 |
| `position_target` | `PositionTargetSnapshot` | - | 位置目标 |
| `attitude_target` | `AttitudeTargetSnapshot` | - | 姿态目标 |

### 8.10 `OdomStateSnapshot`

Summary: 里程计系统总状态快照。

Identifiers:

- `message_family = StateSnapshot`
- `message_type = 10`
- `schema_version = 1`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `header` | `HeaderSnapshot` | - | 坐标系与时间戳 |
| `external_source` | `uint8_t` | - | 外部源编码 |
| `subtopic_name_external_odom` | `string` | - | 外部里程计订阅 topic |
| `odometry_valid` | `bool` | - | 里程计是否有效 |
| `odometry_update_hz` | `float` | `Hz` | 更新频率 |
| `subtopic_name_external_relocalization` | `string` | - | 重定位订阅 topic |
| `pubtopic_name_local_odom` | `string` | - | local odom 发布 topic |
| `pubtopic_name_global_odom` | `string` | - | global odom 发布 topic |
| `local_odom` | `OdometrySnapshot` | - | 本地里程计 |
| `global_odom` | `OdometrySnapshot` | - | 全局里程计 |
| `world_frame_name` | `string` | - | world frame 名称 |
| `global_frame_name` | `string` | - | global frame 名称 |
| `local_frame_name` | `string` | - | local frame 名称 |
| `base_frame_name` | `string` | - | base frame 名称 |
| `world_to_global_tf` | `TransformSnapshot` | - | world 到 global 变换 |
| `global_to_local_tf` | `TransformSnapshot` | - | global 到 local 变换 |
| `local_to_base_tf` | `TransformSnapshot` | - | local 到 base 变换 |

### 8.11 `SunrayRuntimeDiagnosticSnapshot`

Summary: Sunray runtime 诊断快照。

Identifiers:

- `message_family = StateSnapshot`
- `message_type = 11`
- `schema_version = 1`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `header` | `HeaderSnapshot` | - | 坐标系与时间戳 |
| `agent_key` | `string` | - | 诊断 agent key |
| `stale_timeout_ms` | `uint32_t` | `ms` | 过期阈值 |
| `external_odom` | `SunrayTopicDiagnosticSnapshot` | - | external odom 诊断 |
| `odom_state` | `SunrayTopicDiagnosticSnapshot` | - | odom_state 诊断 |
| `local_odom` | `SunrayTopicDiagnosticSnapshot` | - | local_odom 诊断 |
| `global_odom` | `SunrayTopicDiagnosticSnapshot` | - | global_odom 诊断 |
| `uav_control_cmd` | `SunrayTopicDiagnosticSnapshot` | - | uav_control_cmd 诊断 |
| `uav_control_state` | `SunrayTopicDiagnosticSnapshot` | - | uav_control_state 诊断 |
| `px4_state` | `SunrayTopicDiagnosticSnapshot` | - | px4_state 诊断 |
| `worst_level` | `string` | - | 最差等级文本 |
| `summary` | `string` | - | 汇总文本 |

### 8.12 `CommandExecutionStatusSnapshot`

Summary: 命令执行状态快照。

Identifiers:

- `message_family = StateSnapshot`
- `message_type = 12`
- `schema_version = 1`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `header` | `HeaderSnapshot` | - | 坐标系与时间戳 |
| `agent_name` | `string` | - | agent 名称 |
| `agent_id` | `uint8_t` | - | agent ID |
| `session_id` | `uint64_t` | - | 关联会话 ID |
| `command_message_id` | `uint64_t` | - | 原命令消息 ID |
| `command_correlation_id` | `uint64_t` | - | 原命令关联 ID |
| `command_kind` | `CommandKind` as `uint16_t` | - | 命令类型 |
| `execution_state` | `uint8_t` | - | 执行状态编码 |
| `progress_percent` | `uint8_t` | `%` | 执行进度 |
| `active` | `bool` | - | 是否仍活跃 |
| `terminal` | `bool` | - | 是否终态 |
| `success` | `bool` | - | 是否成功 |
| `result_code` | `uint16_t` | - | 结果码 |
| `detail` | `string` | - | 细节说明 |
| `control_state` | `uint8_t` | - | 控制状态编码 |
| `px4_landed_state` | `uint8_t` | - | PX4 起降状态编码 |
| `ready_for_takeoff` | `bool` | - | 是否满足起飞条件 |
| `ready_for_land` | `bool` | - | 是否满足降落条件 |
| `busy_reason` | `string` | - | 忙碌原因 |

Decode constraints:

- `command_kind` 必须在当前实现支持范围内
- `execution_state` 必须在当前实现支持范围内

## 9. State Event Payloads

源码：

- [`include/yunlink/core/semantic/state_types.hpp`](../../include/yunlink/core/semantic/state_types.hpp)
- [`src/core/state/semantic_messages_state_bulk_extended.cpp`](../../src/core/state/semantic_messages_state_bulk_extended.cpp)

### 9.1 `VehicleEvent`

Summary: 稀疏车辆事件。

Identifiers:

- `message_family = StateEvent`
- `message_type = 1`
- `schema_version = 1`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `kind` | `VehicleEventKind` as `uint8_t` | - | 事件种类 |
| `severity` | `uint8_t` | - | 严重级别 |
| `detail` | `string` | - | 事件说明 |

Decode constraints:

- `kind` 必须在当前实现支持范围内

## 10. Bulk Descriptor Payloads

源码：

- [`include/yunlink/core/semantic/state_types.hpp`](../../include/yunlink/core/semantic/state_types.hpp)
- [`src/core/state/semantic_messages_state_bulk_extended.cpp`](../../src/core/state/semantic_messages_state_bulk_extended.cpp)

### 10.1 `BulkChannelDescriptor`

Summary: 大流旁路通道描述。它描述的是旁路通道，不是大流内容本体。

Identifiers:

- `message_family = BulkChannelDescriptor`
- `message_type = 1`
- `schema_version = 1`

| Field Name | Type | Units | Description |
| --- | --- | --- | --- |
| `channel_id` | `uint32_t` | - | 通道 ID |
| `stream_type` | `BulkStreamType` as `uint8_t` | - | 大流类型 |
| `state` | `BulkChannelState` as `uint8_t` | - | 通道状态 |
| `uri` | `string` | - | 旁路通道地址 |
| `mtu_bytes` | `uint32_t` | `bytes` | MTU |
| `reliable` | `bool` | - | 是否可靠传输 |
| `detail` | `string` | - | 说明文本 |

Decode constraints:

- `stream_type` 必须在当前实现支持范围内
- `state` 必须在当前实现支持范围内

## 11. Common Pitfalls

### 11.1 Payload Is Not the Whole Message

不要把 payload 和整条消息混为一谈。  
完整消息是 `SecureEnvelope + payload + checksum`。

### 11.2 Metadata Lives on the Envelope

很多“上下文信息”不在 payload 里，而在 envelope 上，例如：

- `session_id`
- `correlation_id`
- `source`
- `target`
- `ttl_ms`

这在 `UavControlCmdSnapshot` 这类 payload 上尤其容易被误解。

### 11.3 Binary Layout Changes Need Version Discipline

只要 payload 字段顺序、字段类型、嵌套展开顺序发生变化，就可能造成：

- 新旧二进制互相无法解码
- 运行时报 `semantic-payload-decode-failed`

如果修改 payload 结构，发送端和接收端必须一起升级并重启相关进程。
