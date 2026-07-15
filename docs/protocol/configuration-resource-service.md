# YunLink 类型化配置资源服务

本文定义 schema 1 中的 `ConfigurationService`。它提供资源发现、字段描述、读取、字段级校验更新和显式生效，不定义文件、环境变量、ROS 参数、远程 shell 或任意嵌套文档。

## 协议身份

- `message_family = 9 (ConfigurationService)`
- `schema_version = 1`
- 默认 QoS 为 `ReliableOrdered`
- Discovery 字符串能力为 `config-resource-v1`
- 该能力不占用 session capability bit；未知 family/type 可由旧端点忽略

消息类型固定为：

| Type | Payload |
| ---: | --- |
| 1 | `ConfigResourceListRequest` |
| 2 | `ConfigResourceListResponse` |
| 3 | `ConfigResourceDescribeRequest` |
| 4 | `ConfigResourceDescribeResponse` |
| 5 | `ConfigResourceGetRequest` |
| 6 | `ConfigResourceGetResponse` |
| 7 | `ConfigResourcePatchRequest` |
| 8 | `ConfigResourcePatchResponse` |
| 9 | `ConfigResourceApplyRequest` |
| 10 | `ConfigResourceApplyResponse` |

响应的 `correlation_id` 必须等于对应请求的 `message_id`。

## 类型模型

`ConfigValue` 只允许六种显式 tag：`bool`、`int64`、`double`、`string`、`string_list` 和
`double_list`。`double_list` 用于平坦的数值序列，例如参数数组；它不是任意嵌套文档。
未知 tag、截断 payload、超过 1024 bytes 的字符串和超过 256 项的配置数组必须编码失败或
解码失败，不能静默截断。

字段使用 provider 定义的稳定 `path`。协议不解释 path，也不允许客户端借 path 指定文件名、命令或任意嵌套对象。

`ConfigFieldSchema` 包含：

- `path/title/description/type`
- `required/read_only/sensitive`
- 可选 `minimum/maximum`
- `validation_pattern`
- 类型化 `choices` 与显示 label

Monitor 必须按 schema 动态生成控件。资源字段标题和说明由 provider 提供；数据值、协议帧与原始诊断文本不翻译。

## Snapshot 与 revision

`ConfigSnapshot` 返回：

- `resource_id`
- `revision`
- `applied_revision`
- 完整 `values`

revision 是 provider 生成的不可解释字符串。客户端只能原样保存、比较和回传，不能推断哈希算法、文件时间或版本序号。

## Patch

Patch 必须携带 `resource_id`、非空 `expected_revision`、字段级 `updates` 和 `validate_only`。

- provider 必须在写入锁内重新读取当前 revision。
- revision 不匹配时返回 `conflict` 和当前完整 snapshot，不覆盖新值。
- `validate_only=true` 只做授权、安全状态、revision 和字段校验，不持久化。
- 成功或校验失败都应尽量返回规范化后的完整 snapshot。
- 字段错误用 `path/code/message` 返回。
- `effects` 返回生效需求、受影响组件和 `reconnect_expected`。

Patch 只保存，不自动 Apply。

## Apply

Apply 必须携带待生效 `expected_revision`，只能生效该 revision。结果 outcome 为：

- `applied`
- `restart_scheduled`
- `manual_action_required`
- `failed`

部署不允许自动重启时，应以 `status=ok` 和 `outcome=manual_action_required` 返回，不接受远端命令或服务名。预计身份变化会断线时，`effects.reconnect_expected=true`；客户端随后重新发现端点并建立新会话。

## 公共状态码

状态码固定为：`ok`、`not_found`、`unsupported`、`unauthenticated`、`unauthorized`、`conflict`、`invalid`、`unsafe_state`、`internal_error`。

transport/runtime 错误仍使用 `ErrorCode`；业务响应不得拿 transport 错误码代替字段错误或 revision 冲突。

## Provider 与授权边界

C++ provider 通过 `ConfigurationResourceProvider` 和 `ConfigurationProviderRegistry` 注册。Registry 只按资源 ID 路由，不代替应用授权策略。

应用必须分别裁决读、写和 Apply。对安全敏感资源，推荐同时要求：

- 活动且已认证的 session；
- 本地启用并成功验证的 security tag；
- 本地管理员白名单；
- provider 特有的安全状态检查。

对端自报的 discovery capability 或 session capability 不能作为权限凭证。`security_tags_enabled` 可在不占用 required capability bit 的情况下签名并验证消息，因此旧客户端仍可建立原有会话；缺少已验证 tag 的请求不能执行受保护 mutation。

## C ABI 与语言绑定

C ABI 的请求函数在调用期间借用输入 view；五类响应通过 callback 提供只读 view。view 中所有字符串、数组和嵌套指针只在 callback 返回前有效，调用方不得保存。

Rust safe SDK 和 Python high-level SDK 必须在 callback 内立即深拷贝：

- Rust 暴露 owned enum/struct 和 `subscribe_configuration()`。
- Python native 层先拷贝到受锁队列，高层再转换为 dataclass；IO callback 不持有 Python 借用指针。

这套生命周期合同独立于普通 runtime event polling，不能把 callback view 塞进原有 tagged event union。

## Sunray adapter 边界

Sunray 首个资源为 `sunray.device.identity`。`.sunray_env`、四个环境键、飞行状态检查、原子文件写入和固定服务重启都属于 `yunlink_ros_bridge` adapter，不进入 YunLink 公共协议。
