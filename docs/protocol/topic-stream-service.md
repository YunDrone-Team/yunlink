# Topic Stream Service

Topic Stream 是 schema 1 的只读观察通道。它让客户端先查询端点当前活跃的 topic，
再为自己的活动会话显式订阅少量原始样本。协议只定义稳定 topic 名、类型标识、字节样本
和资源限制，不认识 ROS、DDS 或其他中间件 API。

## Discovery

支持该能力的端点在局域网发现结果中声明字符串能力 `topic-stream-v1`。它不占用 session
capability flag，也不改变 `schema_version = 1`。客户端未看到该能力时不得显示订阅入口。

## 消息

| Family | Type | Payload | QoS |
| --- | ---: | --- | --- |
| `SystemService` | 13 | `TopicListRequest` | `ReliableOrdered` |
| `SystemService` | 14 | `TopicListResponse` | `ReliableOrdered` |
| `SystemService` | 15 | `TopicSubscriptionRequest` | `ReliableOrdered` |
| `SystemService` | 16 | `TopicSubscriptionResponse` | `ReliableOrdered` |
| `StateSnapshot` | 14 | `TopicSample` | `ReliableOrdered` |

目录响应包含 `revision` 和完整 `TopicDescriptor[]`。每个 descriptor 只有 `name`、
`type_name` 和当前 publisher 数量；目录不携带样本、控制权限或配置内容。

订阅请求包含精确 `topic_name`、订阅/退订动作、最高频率和最大样本字节数。提供方可以
降低请求值，必须在响应中返回实际限制。退订是幂等操作。

## Sample

`TopicSample` 必须定向发送到发起订阅的 `peer_id + session_id`，不能广播。每条样本包含：

- topic 名、类型名、接收时间和单订阅递增 sequence；
- `encoding` 和具体中间件的原始完整字节；
- 首个成功样本中的类型 hash 与消息定义，后续样本可省略重复元数据。

原始消息必须完整发送或整条丢弃，不得为满足上限截断。多个 topic 共用同一个
`message_type`，因此 QoS 使用 `ReliableOrdered`，不能使用会跨 topic 共享 latest watermark
的 `ReliableLatest`。

## 生命周期与安全

- 目录和订阅请求只接受已认证的活动会话，envelope source 必须与 session identity 一致。
- 只能订阅提供方刚从本地中间件目录确认仍活跃的精确 topic。
- 提供方必须限制每会话订阅数量、单 topic 频率和整包大小。
- 会话失效时必须清理它创建的所有本地订阅。
- 图像、点云等高带宽 topic 只能由客户端显式选择，不能因目录发现自动订阅。
- 本服务只读；不得扩展成任意 topic publish、任意 service 或远程 shell。
