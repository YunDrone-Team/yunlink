# Sunray Protocol 接入指南

本文档面向接入者，描述如何把当前 repo 用作统一协议通信库的最小落地入口。协议字段、消息族与线包约束请以 [sunray-unified-protocol-spec.md](sunray-unified-protocol-spec.md) 为准。

## 1. 适用对象

- 地面站应用
- 无人机机载计算机
- 无人车车载计算单元
- 集群协调或执行节点

## 2. 构建与依赖

### 2.1 初始化

```bash
git submodule update --init --recursive
cmake --preset ninja-debug
python3 tools/build_fast.py --preset ninja-debug
```

### 2.2 关键依赖

- `standalone Asio 1.30.2`
  路径：`thirdparty/asio/asio/include`
- `CMake + Ninja`
  默认开发工作流

### 2.3 质量检查

```bash
python3 tools/build_fast.py --preset ninja-debug --target lint
```

## 3. 最小接入路径

### 3.1 地面站侧

建议顺序：

1. 配置 `self_identity` 与共享认证信息
2. 建立底层 peer route
3. 通过 `SessionClient` 或 runtime 打开会话并等待 `Active`
4. 申请 `Authority`
5. 发布一条最小命令
6. 订阅 `CommandResult`
7. 并行订阅 `VehicleCoreState` 与 `VehicleEvent`

### 3.2 机载/车载侧

建议顺序：

1. 启动 runtime 并绑定自身身份
2. 接收 `Session` 并推进会话状态
3. 对 `Authority` 做显式租约管理
4. 对控制命令做目标匹配、控制权检查和 TTL 检查
5. 把执行器反馈映射为 `CommandResult`
6. 周期发布 `VehicleCoreState`
7. 按需发布 `VehicleEvent`

## 4. 最小联调闭环

建议优先跑通以下闭环：

1. 建立 TCP peer
2. 建立 `Session`
3. 申请 `Authority`
4. 发送一条 `GotoCommand` 或 `VelocitySetpointCommand`
5. 看到完整 `CommandResult`
6. 收到至少一条 `VehicleCoreState`

## 5. SDK 入口

常见接口映射如下：

- `SessionClient` / `SessionServer`
  会话建立与状态推进
- `Runtime::request_authority()`
  控制权租约申请
- `CommandPublisher`
  控制命令发布
- `StateSubscriber`
  周期快照订阅
- `EventSubscriber`
  事件与命令结果订阅

## 6. 日志与排障观察点

联调日志至少应包含：

- `peer_id`
- `session_id`
- `message_id`
- `correlation_id`
- `message_family`
- `message_type`
- `target.scope`
- `target.group_id`
- `source`
- `ttl_ms`

## 7. 常见问题

- 看到状态但控制命令不生效：
  优先检查 `Session` 是否为 `Active`、是否持有 `Authority`、以及 `ttl_ms` 是否过短
- 群组命令无效：
  优先检查 `target.scope` 是否为 `kGroup`，以及 `group_id` 在 envelope 与 payload 内是否一致
- 想接大流：
  先通过 `BulkChannelDescriptor` 做发现和授权，不要把视频或点云直接塞进主控制链路

## 8. 参考入口

- 协议约束：
  [sunray-unified-protocol-spec.md](sunray-unified-protocol-spec.md)
- 场景 walkthrough：
  [scenario-walkthroughs.md](scenario-walkthroughs.md)
- 当前实现覆盖：
  [implementation-status.md](implementation-status.md)
