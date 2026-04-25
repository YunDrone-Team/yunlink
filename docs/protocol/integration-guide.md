# Yunlink 协议接入指南

本文档面向“按当前仓库实际能力接入”的读者。协议字段、消息族与线包约束请以 [yunlink-protocol-spec.md](yunlink-protocol-spec.md) 为准；当前 runtime 的边界和实现缺口请同时参考 [implementation-status.md](implementation-status.md)。

## 1. 适用对象

- 地面站应用
- 无人机机载计算机
- 无人车车载计算单元
- 集群协调或执行节点

## 2. 构建前提与依赖

### 2.1 推荐工作流

```bash
git submodule update --init --recursive
cmake --preset ninja-debug
python3 tools/build_fast.py --preset ninja-debug
ctest --test-dir build/ninja-debug --output-on-failure
```

说明：

- `cmake --preset ...` 需要 `CMake 3.25+`。
- 推荐使用 `Ninja`。
- `tools/build_fast.py` 只是对 `cmake --build --preset ... --parallel ...` 的轻量封装。

### 2.2 传统配置入口

如果你不能使用 presets，可以直接走普通 CMake 配置路径：

```bash
cmake -S . -B build/manual -G Ninja \
  -DYUNLINK_BUILD_EXAMPLES=ON \
  -DYUNLINK_BUILD_TESTS=ON
cmake --build build/manual --parallel
ctest --test-dir build/manual --output-on-failure
```

顶层 `CMakeLists.txt` 的最低要求是 `CMake 3.16`，但推荐工作流仍然是 presets。

### 2.3 关键依赖

- `standalone Asio 1.30.2`
  路径：`thirdparty/asio/asio/include`
- `CMake + Ninja`
  默认开发工作流

### 2.4 质量检查

```bash
python3 tools/build_fast.py --preset ninja-debug --target lint
```

## 3. 公开 API 入口

- C++ 总入口头：
  `include/yunlink/yunlink.hpp`
- C++ runtime 入口：
  `include/yunlink/runtime/runtime.hpp`
- C ABI 入口：
  `include/yunlink/c/yunlink_c.h`

当前最常用的 C++ 接口是：

- `Runtime`
- `SessionClient` / `SessionServer`
- `CommandPublisher`
- `StateSubscriber`
- `EventSubscriber`
- `Runtime::request_authority()` / `Runtime::release_authority()` / `Runtime::current_authority()`

需要注意：

- C++ 接口是当前仓库的主接入面。
- C ABI 目前更像“原始 transport + 事件轮询层”，不是完整的语义 SDK。

## 4. 最小接入路径

### 4.1 地面站侧

建议顺序：

1. 配置 `RuntimeConfig.self_identity` 与双方一致的 `shared_secret`
2. `Runtime::start()` 启动本地 runtime
3. 通过 `tcp_clients().connect_peer()` 建立 TCP peer
4. 用 `session_client().open_active_session(peer_id, node_name)` 发起会话
5. 订阅 `CommandResult`、状态快照或事件
6. 通过 `request_authority()` 申请控制权
7. 通过 `command_publisher()` 发送 `GotoCommand`、`VelocitySetpointCommand` 等控制消息

### 4.2 机载/车载侧

建议顺序：

1. 配置自身 `self_identity`，启动 runtime
2. 接收 `Session` 消息并在本地推进会话状态
3. 接收 `AuthorityRequest` 并更新本地租约
4. 对控制命令做目标匹配和控制权检查
5. 周期发布 `StateSnapshot`
6. 按需发布 `VehicleEvent`

如果你要做真实执行器集成，需要明确一点：默认 runtime 只负责协议边界和默认 auto-result，不会替你接入飞控、车辆底盘或群组执行器。若执行器侧需要自己拥有正式结果流，例如 `sunray_yunlink_bridge` 这种外部 bridge，则应显式切到 `CommandHandlingMode::kExternalHandler`，通过 typed inbound command subscribe + `reply_command_result(...)` 自行回包。

外部 executor 的 contract 固定为：

- runtime 先统一执行 TTL、target、session、authority gate。
- gate 失败时 runtime 直接回 `CommandResult.Failed` 或 `CommandResult.Expired`，detail 使用稳定字符串，例如 `wrong-target`、`no-active-session`、`no-authority`、`session-lost`、`runtime-ttl-expired`。
- 只有 gate 通过的 command 才会进入 typed inbound command handler。
- `CommandHandlingMode::kExternalHandler` 关闭默认 `runtime-auto-result`，后续 `Received / Accepted / InProgress / Succeeded / Failed` 由 bridge 或 executor 使用 `reply_command_result(...)` 显式回包。
- executor 必须保留 inbound route/envelope 元信息，尤其是 `session_id`、`message_id`、`correlation_id` 和 transport peer，以保证结果能原路返回。

## 5. 当前 runtime 的最小现实模型

下面这些行为是“当前仓库确实已经实现”的接入前提，不是规范层的理想化描述：

- 会话由发起方发送 `SessionHello -> SessionAuthenticate -> SessionCapabilities -> SessionReady`，接收方校验后推进到 `Active` 并回 `SessionReady` ack；发起方收到 ack 后也能在本地观察到 `Active`。
- 认证当前只是 `shared_secret` 的字符串比较。
- capability flags 会作为 required bitset 做裁决，peer flags 不覆盖本端 required flags 时 session 进入 `Invalid`。
- `Authority` 按 target 分片管理，并会主动发送 `AuthorityStatus` 回执。
- 控制命令只有在 TTL 未过期、target 命中、会话为 `Active` 且对应 target 租约属于该 `session_id` 时才会进入默认 auto-result 或 external handler 路径。
- 默认命令处理路径会自动发出 `Received -> Accepted -> InProgress -> Succeeded` 四段结果流。
- 如果切到 `CommandHandlingMode::kExternalHandler`，runtime 会在 gate 通过后把命令分发给外部 handler，并关闭默认 `runtime-auto-result`。
- runtime 收包入口会执行 TTL freshness；过期 command 会返回 `CommandResult.Expired(detail=runtime-ttl-expired)`。

## 6. 最小联调闭环

建议优先跑通以下闭环：

1. 建立 TCP peer
2. 建立 `Session`
3. 申请 `Authority`
4. 发送一条 `GotoCommand` 或 `VelocitySetpointCommand`
5. 看到完整 `CommandResult`
6. 收到至少一条 `VehicleCoreState`

当前仓库最容易直接复用的联调样例有：

- `example_tcp_command_client` + `example_telemetry_receiver`
- `example_discovery_udp` + `example_udp_tcp_bridge`
- `examples/smoke_local/run_smoke_local.py`

## 7. 日志与排障观察点

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

对当前 repo 来说，额外建议再记录：

- `SessionState`
- 当前租约对应的 `session_id`
- transport 方向（`TcpClient` / `TcpServer` / `Udp*`）

## 8. 常见问题

- 看到状态但控制命令不生效：
  优先检查 `Session` 是否为 `Active`、当前租约是否属于该 `session_id`。当前 runtime 不会主动给客户端发送 `AuthorityStatus`，所以不能把“申请已发送”当成“租约已生效”。
- 会话一直没有进入 `Active`：
  先检查 `shared_secret` 是否一致。当前最小实现没有双向协商回执，排障时更应该看受控端是否把该 `session_id` 推进到了 `Active`。
- 命令发出后没有结果流：
  当前默认结果流只会在命令真正进入 runtime 命中路径时触发；如果会话未激活、租约不属于该 `session_id`，或者目标不匹配，就不会出现显式 `Rejected`。
- 群组命令无效：
  先检查 `target.scope` 是否为 `kGroup`，以及 `group_id` 在 envelope 与 payload 内是否一致。还要注意：当前 repo 还没有真实组成员路由与群组执行器，`kGroup` 仍主要停留在线包与语义建模层。
- TTL 看起来没有生效：
  当前 runtime 收包路径不会自动做 TTL 过期拒绝；如果你依赖 TTL，请把这件事视为待补齐能力，而不是现成功能。
- 想接大流：
  先通过 `BulkChannelDescriptor` 做发现建模，不要把视频或点云直接塞进主控制链路。当前 repo 还没有 bulk consumer/runtime。

## 9. 参考入口

- 协议约束：
  [yunlink-protocol-spec.md](yunlink-protocol-spec.md)
- 当前实现覆盖：
  [implementation-status.md](implementation-status.md)
- 场景 walkthrough：
  [scenario-walkthroughs.md](scenario-walkthroughs.md)
- 迁移说明：
  [migration-notes.md](migration-notes.md)
