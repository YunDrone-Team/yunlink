# Sunray Protocol 迁移说明

本文档用于帮助已有通信认知迁移到当前统一协议模型。这里允许集中谈历史模型与迁移建议；其它首页或导航文档不再承担这部分叙事。

## 1. 迁移核心

当前模型的核心变化不是“换一组字段名”，而是从“消息编号驱动的点对点包模型”迁移到“稳定信封总线 + 语义消息族 + 会话与控制权”的统一协议模型。

## 2. 认知映射

| 历史认知 | 当前认知 |
| --- | --- |
| `seq` 编号决定消息意义 | `message_family + message_type + schema_version` 决定消息意义 |
| `robot_id` 决定目标 | `source + target` 共同定义身份与目标域 |
| 线包直接代表业务 | `SecureEnvelope` 承载业务 payload，语义由 schema 决定 |
| 控制是否生效由业务层自行约定 | 会话、认证、能力协商与控制权在协议层定义 |
| 单播是默认主模型 | 单体、组、广播都应通过统一目标域表达 |
| 结果常靠业务侧自定义回包 | `CommandResult` 是统一结果流 |

## 3. 常见迁移误区

- 误区 1：
  还试图通过 transport 类型或历史消息编号反推协议语义
- 误区 2：
  把 group control 退化为应用层拆多条单播
- 误区 3：
  把状态快照和事件混成一条上行流
- 误区 4：
  继续把视频、点云等大流内容塞进主控制链路
- 误区 5：
  认为“能发命令”就等于“有控制权”

## 4. 迁移建议

建议按以下顺序迁移：

1. 先接受 `SecureEnvelope` 作为唯一线包主体
2. 再接受 `Session` 与 `Authority` 是控制命令的前置条件
3. 再把命令、结果、快照、事件按消息族拆开
4. 最后处理群组目标与 bulk 旁路能力

## 5. 面向当前仓库 API 的落点

- 如果你需要 typed C++ 接入，优先从 `Runtime`、`SessionClient`、`CommandPublisher`、`StateSubscriber`、`EventSubscriber` 入手。
- 如果你需要嵌入式或跨语言接入，当前 C ABI 更接近“transport + 事件轮询层”，而不是完整语义 SDK。
- 不要再寻找旧的 `FrameHeader` / `Frame` / `PayloadMapper` 主路径；当前仓库已经把主模型收敛到 `SecureEnvelope + semantic messages`。

## 6. 对代码接入者的提示

- 如果你在旧代码里寻找“哪一个字段等价于 `robot_id`”，应改为思考目标域如何通过 `TargetSelector` 表达
- 如果你在旧代码里寻找“哪一个数字等价于旧 `seq`”，应改为从消息族和消息类型目录入手
- 如果你想保留历史桥接逻辑，应把桥接视为边缘适配问题，而不是协议主模型的一部分

## 7. 推荐联读

- 主规范：
  [sunray-unified-protocol-spec.md](sunray-unified-protocol-spec.md)
- 接入指南：
  [integration-guide.md](integration-guide.md)
- 实现状态：
  [implementation-status.md](implementation-status.md)
