# Sunray ROS1 YunLink Bridge

本文档描述 `sunray_v2/communication/yunlink_ros_bridge` 的当前实现。它是独立 ROS1 catkin 包，负责 YunLink 与 Sunray ROS topic/service 之间的转换，不调用 Sunray controller 或定位模块的内部 C++ API。

![System Context](../diagrams/plantuml/svg/ros_sunray_bridge_system_context.svg)

## 当前边界

bridge 当前负责：

- 从 `/sunray/system_info` 的 `agents_info[0]` 获取当前 Agent 身份和 topic 命名空间。
- 接收一个完整 `UavControlCommand` 并发布 `sunray_msgs/UAVControlCMD`。
- 按 YAML 订阅 Sunray ROS 状态并发布类型化 YunLink snapshot。
- 转发 feature、运行日志和类型化配置系统服务。
- 提供活跃 ROS topic 目录和按会话显式订阅的只读原始样本。
- 维护 YunLink TCP session、UDP 端点发现、QoS 节流和 ROS diagnostics。

bridge 当前不负责：

- 不修改或扩展 Sunray ROS 消息定义。
- 不推导飞行任务是否成功，也不实现 command tracker。
- 不把 `UAVCommandExecutionStatus` 自动关联到某条 YunLink command 的 `correlation_id`。
- 不订阅旧的 Takeoff/Land/Return/Goto/VelocitySetpoint 快捷命令。
- 不把 planning、图像或点云 topic 自动加入持续遥测；它们只能由活动会话显式订阅。
- 不读取或修改 `.sunray_env`，也不提供任意 ROS publish/service 或 shell。
- 不发布 `VehicleEvent`。

![Component View](../diagrams/plantuml/svg/ros_sunray_bridge_component_view.svg)

## 连接与发现

默认配置位于 `communication/yunlink_ros_bridge/config/yunlink_ros_bridge_base.yaml`：

- TCP 监听端口：`9696`
- UDP bind/target 端口：`9696` / `9898`
- UDP 发现广播端口：`9966`
- 发现模式：`query_only`，收到认证探测后抖动单播回复

`remote_ip` 为空或 `remote_tcp_port` 为 `0` 时，bridge 被动等待地面端连接。默认不周期广播；新版 Monitor 主动发送认证 query，bridge 在响应窗口内单播 reply。配置远端地址后，bridge 也可以主动拨号。session 失效后，1 秒重连定时器同时清理该会话创建的原始 topic 订阅。

状态 snapshot 发送前必须存在 active peer session。目标选择器当前固定为 Ground Station broadcast；这表示 snapshot 面向已建立会话的地面站类型目标，不表示 bridge 在无 session 时向网络裸广播业务状态。

## ROS 接口

Agent 作用域 topic 在 YAML 中保存为相对后缀，启动时基于
`/<agent_name><agent_id>` 展开；绝对覆盖路径保持原样：

| Direction | ROS topic | ROS type |
| --- | --- | --- |
| Subscribe | `/<agent>/sunray/localization/local_odom` | `nav_msgs/Odometry` |
| Subscribe | `/<agent>/sunray/localization/odom_state` | `sunray_msgs/OdomState` |
| Subscribe + Publish | `/<agent>/sunray/uav_control/control_cmd` | `sunray_msgs/UAVControlCMD` |
| Subscribe | `/<agent>/sunray/uav_control/control_state` | `sunray_msgs/UAVControlState` |
| Subscribe | `/<agent>/sunray/uav_control/command_execution_status` | `sunray_msgs/UAVCommandExecutionStatus` |
| Subscribe | `/<agent>/sunray/px4_state` | `sunray_msgs/Px4State` |
| Subscribe | `/sunray/system_info` | `sunray_msgs/SystemInfo` |
| Publish | `/yunlink_ros_bridge/monitor_diagnostics` | `diagnostic_msgs/DiagnosticArray` |

## 命令下行

唯一入口 `UavControlCommand` 一一映射 `control_cmd`、惯性系位置/速度/加速度、机体系
XY 位置/速度、固定高度、yaw 模式/值/速率和 controller type。Bridge 强制
`cmd_source=SUNRAY_STATION`，拒绝范围外枚举以及 NaN/Inf。

Runtime 使用 `CommandHandlingMode::kExternalHandler`，而不是默认 `kAutoResult`。命令先经过 session、authority、
target 和 TTL gate；Bridge 成功发布 ROS 消息后回复 `Accepted`，参数非法或本地控制 topic
没有 subscriber 时回复 `Failed`。`Accepted` 不证明无人机完成动作，只代表命令已进入 Sunray 控制输入，最终
飞行动作结果由 `UAVCommandExecutionStatus` 状态通道表达。

![Command Downlink Sequence](../diagrams/plantuml/svg/ros_sunray_bridge_command_downlink_sequence.svg)

## 状态上行

| ROS source | YunLink snapshot | 默认最高频率 |
| --- | --- | --- |
| `local_odom` | `LocalOdomSnapshot` | 10 Hz |
| `odom_state` | `OdomStateSnapshot` | 2 Hz |
| `control_cmd` | `UavControlCmdSnapshot` | 不节流 |
| `control_state` | `UavControlStateSnapshot` | 5 Hz |
| `command_execution_status` | `CommandExecutionStatusSnapshot` | 不节流 |
| `px4_state` | `Px4StateSnapshot` | 跟随上游，当前约 10 Hz |
| `/sunray/system_info` | `HostSystemSnapshot` | 跟随上游，当前 1 Hz |

节流发生在 ROS callback 中，只限制最高转发频率；bridge 不生成 heartbeat，也不会在 source topic 停止后重复发送旧 snapshot。

`Px4StateSnapshot` 直接读取当前嵌套 MAVROS 字段：

- `state`：连接、manual input、armed、flight mode、system status。
- `extended_state`：landed state。
- `sys_status`：电池与 FCU load。
- `local_odometry` / `external_odometry`：位姿与速度。
- `local_setpoint` / `attitude_setpoint`：位置、速度、加速度、姿态与推力设定值。
- `gps_raw`：fix type、卫星数、经纬度与高度。

单位转换：

- `voltage_battery`: mV -> V。
- `current_battery`: 10 mA -> A。
- `battery_remaining`: 整数百分比 -> `0..1`；负值当前写为 `0`。
- `gps_raw.lat/lon`: `1e-7 deg` -> deg。
- `gps_raw.alt`: mm -> m。
- `flight_mode`: 直接保留 `mavros_msgs/State.mode` 字符串。

![State Uplink Sequence](../diagrams/plantuml/svg/ros_sunray_bridge_state_uplink_sequence.svg)

## 系统服务

启用 `system_service.enable_system_services` 后，bridge 把以下 YunLink 请求转发到 `sunray_system_ns` 下的 ROS service：

- `FeatureListRequest`
- `FeatureGetRequest`
- `FeatureStartRequest`
- `FeatureStopRequest`

请求进入后台 worker，默认超时为 3 秒。`FeatureGetResponse` 当前映射 `name`、`title`、`group`、运行状态、描述、依赖和启动预览，不包含旧的 `stop_timeout_sec`。

## 按需 Topic Stream

声明 `topic-stream-v1` 的 bridge 可返回 ROS Master 当前有 publisher 的 topic 目录。客户端
为自己的活动会话显式订阅一个精确 topic 后，bridge 使用 `ShapeShifter` 序列化完整 ROS1
消息并定向发送 `TopicSample`。默认每会话最多 16 个、每 topic 10 Hz、单条 256 KiB；
服务端上限为 30 Hz 和 768 KiB。首个成功样本携带 ROS 类型、MD5 和消息定义，退订或
session 失效时立即清理 ROS subscription。

## 诊断

bridge diagnostics 记录：

- Runtime、peer、session 与重连状态。
- ROS topic publisher count、接收频率、消息年龄和 YunLink 发布失败计数。
- YunLink -> ROS 命令接收/发布计数。
- system service 请求、超时与响应。
- 最近转发事件和最后一次错误。

诊断用于定位转发链路，不等同于飞行任务结果。

## 当前验证范围

已验证：

- ROS Noetic 能构建 `sunray_msgs` 和 `yunlink_ros_bridge`。
- YunLink C++ protocol/runtime 全量测试可通过，包括统一命令和 topic service。
- Rust SDK 测试可通过。

尚未由当前自动测试证明：

- PX4 SITL 或真机上的完整控制闭环。
- ROS consumer 实际执行命令后的 YunLink 结果关联。
- 多地面站、多 active session 或多 UAV 竞争控制。
- 定位丢失、执行超时等飞控故障到最终 `CommandResult` 的关联。
