# YunLink 局域网发现（schema 1）

## 目标与范围

该发现机制用于同一二层广播域内的 YunLink 端点发现。它不改变
`SecureEnvelope`、会话协议或 schema version；现有 schema 始终为 `1`，不存在另一套发现版本。

设计目标是让新版 Monitor 在需要时主动搜索，而不是让每个飞行器持续以
高频广播完整端点信息。跨 VLAN、跨路由网络不属于本协议范围，应使用手工
地址或独立目录服务。

## 工作方式

默认模式为 `query_only`：

1. Monitor 打开设备页、手工刷新或准备重连时，生成随机非零 `nonce`。
2. Monitor 向 discovery port 发送 3 个相同 nonce 的 `YLQ1` UDP 广播，间隔约
   250 ms。
3. 端点验证 query 后，对该 `nonce + endpoint_id` 最多回复一次；回复延迟按
   nonce 与 endpoint ID 映射到 `0..response_window_ms`，默认窗口为 1000 ms。
4. 端点把 `YLR1` 单播回 query 的源地址和源端口。Monitor 只接受当前 1200 ms
   扫描窗口内、nonce 匹配的 query reply。
5. 扫描窗口结束后不再发送发现流量；端点也不会周期广播。

Monitor 按稳定 `endpoint_id` 去重。DHCP 地址或 TCP 端口变化会更新同一设备的
连接属性。当前扫描窗口内收到的 query reply 优先，并按本轮扫描窗口判断新鲜度。

不要用周期性 IP multicast 替换主动查询。在很多 Wi-Fi 网络中 multicast 会以低
基础速率发送，实际对空口的影响反而更大。

## 二进制线包

所有整数字段使用大端序。每个主动发现包最后均带 8 B 认证 tag，tag 覆盖此前所有
字节。tag 使用 YunLink 运行时既有的 keyed FNV-1a 约定，用于过滤误配端点和
无共享口令的请求；它不是加密，也不能替代会话认证、security tags 或控制权授权。

`YLQ1` query 固定为 22 B：

| 字段 | 大小 |
| --- | ---: |
| magic `YLQ1` | 4 B |
| nonce | 8 B |
| response window ms | 2 B |
| authentication tag | 8 B |

`YLR1` reply 必须不超过 128 B，包含：

| 字段 | 说明 |
| --- | --- |
| magic `YLR1`、nonce | 回显 query 的 nonce |
| endpoint ID、显示名前缀、Agent 类型、角色、node name | 稳定去重、真实端点身份与必要显示信息 |
| agent ID、TCP/UDP 端口 | 身份与连接属性 |
| capability bitmask | `state`、`commands`、`system_service`、`config-resource-v1`、`topic-stream-v1` |
| sequence、discovery period ms | 新鲜度与过期计算 |
| authentication tag | 共享口令校验 |

未知 capability bit 必须忽略。解码失败、tag 不匹配、零 nonce、非法响应窗口、
截断包或超过 128 B 的 reply 必须丢弃，且不产生响应。
编码端同样必须失败关闭：字段超过协议上限或组合后的 reply 超过 128 B 时返回编码失败，
不得发送截断字段、空数据报或伪造的默认 Agent 类型。

## 端点保护

端点不得响应来源端口为零、未指定地址、组播地址或全局广播地址的 query。每个
端点分别执行以下限制：

- 同一 nonce 在 10 s 内最多处理一次；
- 单一源地址每秒最多 `query_rate_limit` 次；
- 全局每秒最多同一上限；
- nonce 与限流窗口均在过期后回收。

主动发现包不得携带飞行状态、配置内容、控制权或命令。发现认证不授予任何会话
权限；后续 TCP 会话仍按正常认证与控制权策略执行。

## Bridge 参数

Sunray bridge 使用既有 `discovery_port`、`discovery_target_ip` 与 `shared_secret`，并
新增：

| 参数 | 默认值 | 含义 |
| --- | ---: | --- |
| `discovery_mode` | `query_only` | 默认仅响应主动查询；`hybrid`、`legacy_only` 只供诊断旧文本公告 |
| `discovery_period_ms` | `15000` | 仅在显式兼容公告模式下使用的基准周期 |
| `legacy_beacon_jitter_ms` | `2000` | 仅在显式兼容公告模式下使用的最大附加抖动 |
| `query_response_window_ms` | `1000` | query reply 的最大延迟窗口 |
| `query_rate_limit` | `6` | 单源和全局 query 上限，单位次/秒 |

由于当前没有部署中的旧端点或 Monitor，默认直接使用 `query_only`。只有在排查历史
文本公告解析时，才可临时切换为 `hybrid` 或 `legacy_only`；它们不是协议升级路径。

## 验收

一次 Monitor 扫描应观察到 3 个小型 `YLQ1` 广播，以及每个在线端点最多 1 个
`YLR1` 单播回复。空闲期不应出现周期发现广播。发现结果不代表已建立控制会话；
选择端点后仍需完成 TCP 会话认证与控制权申请。
