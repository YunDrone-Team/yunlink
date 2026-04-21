# Sunray 核心代码解耦与 Maxline 整理 Todolist

## 背景

本清单基于两类检查结果整理：

- `check-maxline`：目标是核心代码 `maxline <= 300`
- `Linus taste`：目标是减少复制粘贴、消除特殊分支、降低嵌套、明确边界、做充分解耦

本轮只聚焦核心代码：

- `include/sunraycom`
- `src`

明确排除以下目录：

- `thirdparty`
- `compat`
- `docs`
- `examples`
- `tests`

固定检查入口：

```bash
./tools/check_core_maxline.sh
```

固定规则文件：

```text
.codex/check-maxline.json
```

## 当前超标基线

以下是当前核心目录中超过 `300` 行的文件：

- [`src/runtime/runtime.cpp`](../src/runtime/runtime.cpp) `776`
- [`src/core/semantic_messages.cpp`](../src/core/semantic_messages.cpp) `336`
- [`include/sunraycom/core/semantic_messages.hpp`](../include/sunraycom/core/semantic_messages.hpp) `329`
- [`src/transport/tcp_client_pool.cpp`](../src/transport/tcp_client_pool.cpp) `326`
- [`src/transport/tcp_server.cpp`](../src/transport/tcp_server.cpp) `320`

## 主要异味判断

### `runtime.cpp`

- 一个文件同时承担 runtime 启停、会话推进、控制权处理、命令发布、状态分发、结果分发
- `handle_envelope()` 吞掉多个消息族，职责过重
- `publish_takeoff()` 到 `publish_formation_task()` 基本是复制粘贴
- `publish_command_result_sequence()` 把命令结果策略和 runtime 主循环耦在一起

### `semantic_messages.*`

- 同一组头/源文件同时承担：
  - 消息模型定义
  - traits 注册
  - typed helper
  - 所有 payload 编解码
- 宏式编解码定义读起来不直观
- `AuthorityStatus.session_id` 当前编码为 `uint32_t`，存在截断风险

### `tcp_client_pool.cpp` / `tcp_server.cpp`

- client/server 各自维护一套非常相近的读循环与拆包逻辑
- `make_peer_id()`、`to_int_bytes()`、link event 发布、envelope 发布逻辑重复
- transport 层边界可以继续收紧，减少重复 glue code

## 执行清单

- [x] 建立仓库内固定的核心代码 `maxline` 检查入口，只扫描 `include/sunraycom` 和 `src`，阈值固定为 `300`
- [x] 将 `maxline` 约束文档化，明确本轮不对 `thirdparty`、`compat` 做整改
- [x] 重构 [`src/runtime/runtime.cpp`](../src/runtime/runtime.cpp)，将 runtime 主实现拆分为多个独立实现单元，每个文件必须 `<= 300` 行
- [x] 将 [`src/runtime/runtime.cpp`](../src/runtime/runtime.cpp) 中的命令发布路径抽成通用 helper，消除 `publish_takeoff()` 到 `publish_formation_task()` 的复制逻辑
- [x] 将 [`src/runtime/runtime.cpp`](../src/runtime/runtime.cpp) 中的 `handle_envelope()` 按 `message_family` 拆成独立处理函数
- [x] 将 [`src/runtime/runtime.cpp`](../src/runtime/runtime.cpp) 中的 `publish_command_result_sequence()` 剥离为独立结果发射模块
- [x] 为 runtime 引入内部实现头，承接 `Impl`、共享 helper、family dispatch 声明，减少实现细节散落
- [x] 重构 [`include/sunraycom/core/semantic_messages.hpp`](../include/sunraycom/core/semantic_messages.hpp)，把“消息枚举/模型定义”“traits 注册”“typed helper”拆开
- [x] 确保 [`include/sunraycom/core/semantic_messages.hpp`](../include/sunraycom/core/semantic_messages.hpp) 不再同时承担四种职责
- [x] 重构 [`src/core/semantic_messages.cpp`](../src/core/semantic_messages.cpp)，把共享二进制读写工具与具体消息族 codec 分离
- [x] 将 [`src/core/semantic_messages.cpp`](../src/core/semantic_messages.cpp) 至少拆为 `session_authority`、`command`、`state_bulk` 三类 codec 实现
- [x] 收缩宏式 codec 定义的使用范围，优先改为小而直接的 helper 或按 family 的显式实现
- [x] 修复 [`src/core/semantic_messages.cpp`](../src/core/semantic_messages.cpp) 中 `AuthorityStatus.session_id` 被按 `uint32_t` 编解码的问题
- [x] 重构 [`src/transport/tcp_client_pool.cpp`](../src/transport/tcp_client_pool.cpp) 与 [`src/transport/tcp_server.cpp`](../src/transport/tcp_server.cpp)，抽取共享 TCP stream peer helper
- [x] 统一 client/server 的 read loop、拆包、事件发布、断链收尾逻辑，避免双份维护
- [x] 抽取 transport 层共享 helper，统一 `make_peer_id()`、`to_int_bytes()`、link event 发布和 envelope 发布逻辑
- [x] 检查所有新拆出的核心文件，要求没有函数超过 `100` 行
- [x] 检查所有新拆出的核心函数，要求缩进层级不超过 `3` 层
- [x] 优先使用 early return 消灭深层分支
- [x] 减少“先构造对象，后置补字段”的写法，让 helper 尽量一次返回最终结果
- [x] 梳理 runtime 内部锁与状态访问边界，避免同一实现单元同时负责锁管理、状态迁移、分发策略和 transport reply
- [x] 严格控制本轮范围为“结构性解耦 + 明确 bugfix + maxline 收敛”，不把新功能扩展混进来
- [x] 删除不再被主路径引用的 legacy/frame/payload mapper 旧文件

## 回测清单

- [x] 增加 `AuthorityStatus.session_id` 64 位 roundtrip 回归测试，锁死截断问题
- [x] 为 Session 分发路径补回归测试，覆盖 `Hello -> Authenticate -> Capabilities -> Ready -> Active`
- [x] 为 Authority 分发路径补回归测试，覆盖 `Claim`、`Renew`、`Release`、`Preempt`
- [x] 为命令发布通用 helper 补回归测试，确认 `message_family`、`message_type`、`message_id`、`correlation_id` 填充行为不变
- [x] 为命令结果流补回归测试，确认仍保持 `Received -> Accepted -> InProgress -> Succeeded`
- [x] 为 TCP client/server 共用 stream helper 补回归测试，确认两侧仍能正确拆包并发布 `EnvelopeEvent`
- [x] 保持 [`tests/test_protocol.cpp`](../tests/test_protocol.cpp) 通过
- [x] 保持 [`tests/test_parser.cpp`](../tests/test_parser.cpp) 通过
- [x] 保持 [`tests/test_transport_loop.cpp`](../tests/test_transport_loop.cpp) 通过
- [x] 保持 [`tests/test_udp_source_isolation.cpp`](../tests/test_udp_source_isolation.cpp) 通过
- [x] 保持 [`tests/test_compat_roundtrip.cpp`](../tests/test_compat_roundtrip.cpp) 通过
- [x] 增加一次核心目录 `maxline` 回测，确认 `include/sunraycom` 和 `src` 下所有核心文件都满足 `<= 300`
- [x] 增加一次完整构建回测，确认拆文件后没有漏加到构建系统
- [x] 增加一次 examples 编译回测，至少覆盖 [`examples/tcp_command_client/main.cpp`](../examples/tcp_command_client/main.cpp) 和 [`examples/telemetry_receiver/main.cpp`](../examples/telemetry_receiver/main.cpp)

## 最终验收标准

- [x] `include/sunraycom` 与 `src` 下不再有任何超过 `300` 行的核心文件
- [x] runtime 不再存在一个吞掉所有协议语义的大分发函数
- [x] TCP client/server 不再各自维护一套复制粘贴的 read loop
- [x] `semantic_messages` 不再把模型、traits、工厂、所有 codec 全塞进单一头/源文件
- [x] `AuthorityStatus.session_id` 截断问题被测试锁死
- [x] 现有协议行为与现有测试结果保持一致

## 建议执行顺序

- [x] 先拆 `runtime.cpp`
- [x] 再拆 `semantic_messages.hpp/.cpp`
- [x] 再抽 transport 公共 TCP stream helper
- [x] 最后统一补回测、跑 `maxline`、跑构建、跑现有测试
