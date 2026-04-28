# yunlink 完整测试 Todo Checklist

本文档用于规划 `yunlink` 下一阶段的完整测试体系，重点面向：

- 当前 C++ core + C ABI + Rust/Python bindings 的持续演进
- 即将开展的双机联动测试
- 后续多语言、多拓扑、多网络条件下的长期稳定性验证

本文档不是“当前已完成测试”的总结，而是“后续要逐项落地的测试工作清单”。执行原则始终保持不变：

- `runtime-first`
- `stable C ABI`
- `thin language wrappers`
- 显式生命周期
- 显式恢复
- 证据优先，不靠主观判断

---

## 1. 总体目标

- [ ] 建立一套覆盖 C++ core、C ABI、Rust、Python、单机互操作、双机联动、弱网与 soak 的完整测试体系。
- [ ] 把测试组织从“按语言零散堆 case”升级为“按层级、拓扑、故障条件、发布门禁”分层管理。
- [ ] 让双机联动测试成为正式测试域，而不是临时手工演示。
- [ ] 让每一类测试都有明确入口脚本、结果产物、日志采集和验收标准。
- [ ] 让 PR、nightly、release 三类流水线拥有不同深度但一致口径的测试门禁。

## 2. 核心思想

- [ ] 继续把协议正确性、runtime 状态机正确性、网络与恢复行为正确性放在功能测试前面。
- [ ] 不允许 SDK 层通过自动恢复掩盖 core / runtime 的真实生命周期问题。
- [ ] 不让双机测试退化成“只看能不能连上”；必须验证 authority、command、state、恢复和隔离语义。
- [ ] 不让弱网测试停留在“加一点 delay 看看”；必须定义网络 profile、观测指标和期望退化行为。
- [ ] 不让 soak 测试只看进程没崩；必须记录 CPU、内存、线程、fd/socket、延迟和恢复事件。

---

## 3. 测试大领域

### 3.1 协议与编解码测试

- [x] 覆盖 `SecureEnvelope` 头部字段 roundtrip。
- [x] 覆盖 message family / message type 编码与解码一致性。
- [x] 覆盖 source / target selector 的 entity / group / broadcast 三类语义。
- [x] 覆盖 TTL 超时行为。
- [x] 覆盖 checksum / header / payload corruption 检测。
- [x] 覆盖 protocol version mismatch 的拒绝路径。
- [x] 覆盖 payload 边界值、空字符串、长字符串、固定容量字段截断规则。
- [x] 覆盖 parser 的 partial frame、split frame、multiple frame glued together。
- [x] 覆盖 malformed frame、truncated frame、garbage prefix、garbage suffix。
- [x] 预留 codec fuzz 入口，后续引入持续 fuzz。

### 3.2 Runtime 生命周期与状态机测试

- [x] 覆盖 `start -> stop -> start` 重复生命周期。
- [x] 覆盖 session open / active / invalid / lost 的状态切换。
- [x] 覆盖 authority `claim -> renew -> release -> expire -> reacquire -> preempt` 全路径。
- [x] 覆盖 command `publish -> received -> accepted -> in_progress -> succeeded/failed/cancelled`。
- [x] 覆盖 state snapshot 与 state event 的投递顺序和来源标识。
- [x] 覆盖单端停止、双端停止、停止后调用 API 的返回码。
- [x] 覆盖 runtime restart 后 peer/session/authority 的显式恢复流程。
- [x] 覆盖无 authority 时命令应被拒绝或不执行的路径。
- [x] 覆盖 session 存在但 authority 不存在的边界条件。
- [x] 覆盖 authority lease 自然过期，而不仅是手动 release。

### 3.3 传输与网络测试

- [x] 覆盖 TCP connect / reconnect / duplicate connect。
- [ ] 覆盖 TCP half-open、RST、listener 不可达、端口被占用。
- [x] 覆盖 UDP source isolation。
- [ ] 覆盖消息乱序、重复包、丢包、延迟、抖动、短暂断网。
- [ ] 覆盖 peer identity 在连接重建前后的稳定性假设。
- [ ] 覆盖多 peer 并发连接时的路由隔离。
- [ ] 覆盖 state plane 与 command plane 在弱网下的不同退化行为。
- [ ] 覆盖 network profile 切换期间的恢复行为。

### 3.4 FFI / ABI 契约测试

- [x] 覆盖 ABI version 常量与共享库命名。
- [x] 覆盖结构体布局、字段大小、对齐和 `struct_size` 契约。
- [ ] 覆盖 handle 生命周期、失效后的错误返回和重复释放防护。
- [x] 覆盖 `result code -> result name` 映射。
- [x] 覆盖无效 peer / session / target / runtime 参数。
- [x] 覆盖 runtime stopped 后的 API 行为。
- [x] 覆盖 event poll 空队列与事件顺序。
- [x] 覆盖 authority renew、release、current 的边界条件。
- [x] 覆盖共享库在 tri-platform 的加载与符号暴露预期。

### 3.5 语言绑定测试

#### Rust

- [x] 覆盖 `FfiErrorCode` 映射完整性。
- [x] 覆盖 Tokio task cancel 不影响 runtime 存活。
- [x] 覆盖 broadcast channel 背压与 `Lagged` 语义。
- [x] 覆盖 `Drop` 后线程、channel、socket 资源释放。
- [x] 覆盖 reconnect / reopen / reacquire 的显式恢复链路。
- [x] 覆盖 examples 作为 smoke 程序可运行。

#### Python

- [x] 覆盖 `YunlinkError` 及各异常子类映射。
- [x] 覆盖 polling 线程启动、停止、退出、错误传播。
- [x] 覆盖 `queue.Queue` 与 `asyncio.Queue` 共用同一 domain model。
- [x] 覆盖 `close()` 后端口和线程资源释放。
- [x] 覆盖 editable install 与 wheel install 两条路径。
- [x] 覆盖 Python helper scripts 作为 smoke 程序可运行。

### 3.6 单机多进程互操作测试

- [x] 覆盖 `Rust ground -> C++ air`。
- [x] 覆盖 `Python ground -> C++ air`。
- [x] 覆盖 `Rust ground -> Python air`。
- [x] 覆盖 `Python ground -> Rust air`。
- [x] 覆盖 stop/restart 后显式恢复。
- [ ] 覆盖 command result 与 state snapshot 同时存在的事件流。
- [ ] 覆盖 cross-language 对 authority renew 的一致行为。

### 3.7 双机联动测试

- [x] 把双机联动定义为正式测试领域，而不是 ad-hoc 手工演示。
- [ ] 覆盖双机有线 LAN 基线链路。
- [x] 覆盖双机 Wi-Fi 基线链路。
- [x] 覆盖 ground 在 Host A、air 在 Host B 的标准部署。
- [x] 覆盖 `1 GCS -> 1 UAV`。
- [x] 覆盖 `2 GCS -> 1 UAV` 控制权竞争。
- [x] 覆盖 `1 GCS -> 2 UAV` 目标路由与状态隔离。
- [ ] 覆盖 host 间时钟不同步、重启、网卡切换、短时断网。

### 3.8 故障注入与韧性测试

- [x] 覆盖 listener 未启动。
- [x] 覆盖 shared secret mismatch。
- [x] 覆盖 protocol mismatch。
- [x] 覆盖非法 target / 非法 authority source。
- [x] 覆盖 session 存在但 peer 已断开。
- [ ] 覆盖 command 发出后对端重启。
- [ ] 覆盖 authority lease 即将过期时的续租失败。
- [ ] 覆盖 state flood 导致的队列积压。
- [ ] 覆盖长连接中单边 silent hang。

### 3.9 性能与容量测试

- [ ] 建立 command latency 基线。
- [ ] 建立 state snapshot latency 基线。
- [ ] 建立 reconnect-to-ready latency 基线。
- [ ] 建立 authority acquire / renew latency 基线。
- [ ] 建立单连接 telemetry rate 基线。
- [ ] 建立双机条件下 CPU / memory / thread / fd 基线。
- [ ] 建立事件队列背压阈值与可接受退化行为。
- [ ] 预留多 UAV、多 GCS 的容量测试入口。

### 3.10 安全与拒绝路径测试

- [x] 覆盖 shared secret mismatch 必须拒绝。
- [x] 覆盖 protocol version mismatch 必须拒绝。
- [x] 覆盖 authority 未授予时 command 不能形成成功语义。
- [x] 覆盖错误 target 不应污染非目标实体的状态面。
- [ ] 覆盖错误 peer 不应借由旧 session 越权。
- [ ] 覆盖错误输入只返回稳定错误码，不触发崩溃或未定义行为。

### 3.11 可观测性与结果采集测试

- [x] 为双机测试定义统一日志命名和目录布局。
- [x] 为每轮测试采集 runtime stdout/stderr、时间戳、host 标识、网络 profile。
- [x] 采集 connect、session、authority、command、state、recovery 五类关键时间点。
- [x] 生成统一 JSON / Markdown 测试摘要。
- [ ] 在失败时输出最小可定位信息，而不是只报“timeout”。

---

## 4. 双机联动测试分阶段路线

### 4.1 Phase A: 双机基线联通

- [x] 准备两台固定角色主机：`host-ground`、`host-air`。
- [x] 固定 IP、固定端口、固定 shared secret、固定测试用户。
- [x] 在两台主机上都准备统一的 repo 路径和构建产物位置。
- [x] 在两台主机上准备统一的 Python venv、Rust toolchain、CMake preset。
- [x] 验证 `C++ air + Rust ground` 双机链路。
- [x] 验证 `C++ air + Python ground` 双机链路。
- [x] 验证 connect、session open、authority request、renew、goto、state、release。
- [x] 定义基线验收：全链路稳定通过至少 10 次。

### 4.2 Phase B: 双机生命周期恢复

- [x] 覆盖 ground 重启，air 保持运行。
- [x] 覆盖 air 重启，ground 保持运行。
- [x] 覆盖 release 后 ground 手动 reconnect / reopen / reacquire。
- [x] 覆盖 authority 自然过期后手动恢复。
- [x] 覆盖 TCP 断链后手动恢复。
- [ ] 为恢复测试记录每一步耗时。
- [x] 定义恢复验收：不依赖自动恢复，脚本显式步骤成功。

### 4.3 Phase C: 双机竞争与隔离

- [x] 覆盖 `2 GCS -> 1 UAV` claim 冲突。
- [x] 覆盖 `2 GCS -> 1 UAV` preempt 行为。
- [x] 覆盖一个 GCS release 后另一个 GCS 接管。
- [x] 覆盖 `1 GCS -> 2 UAV` 的 target routing 正确性。
- [x] 覆盖 UAV-A 的 state 不应泄漏到 UAV-B 订阅链路。
- [x] 覆盖 command result 的 correlation 必须只回到正确 ground。

### 4.4 Phase D: 双机弱网与网络劣化

- [x] 准备标准化 `tc netem` profile。
- [ ] 覆盖 `delay` profile。
- [ ] 覆盖 `delay + jitter` profile。
- [ ] 覆盖 `loss` profile。
- [ ] 覆盖 `loss + reorder` profile。
- [ ] 覆盖 `duplication` profile。
- [ ] 覆盖短时完全断网后恢复。
- [ ] 记录不同 profile 下 command/state/recovery 的退化表现。

### 4.5 Phase E: 双机 Soak 与发布门

- [ ] 准备 30min soak。
- [ ] 准备 2h soak。
- [ ] 预留 8h nightly soak。
- [ ] 在 soak 中周期性执行 authority renew、goto、state publish。
- [ ] 统计内存增长、线程数、fd/socket 数、平均延迟、P95/P99 延迟。
- [ ] 统计 reconnect 次数和恢复耗时。
- [ ] 定义 release 前必须通过的 soak 门槛。

---

## 5. 测试拓扑矩阵

### 5.1 角色矩阵

- [x] `C++ air` ↔ `Rust ground`
- [x] `C++ air` ↔ `Python ground`
- [x] `Rust air` ↔ `Python ground`
- [x] `Python air` ↔ `Rust ground`
- [x] `Rust air` ↔ `Rust ground`
- [x] `Python air` ↔ `Python ground`

### 5.2 规模矩阵

- [x] `1 GCS -> 1 UAV`
- [x] `2 GCS -> 1 UAV`
- [x] `1 GCS -> 2 UAV`
- [x] `2 GCS -> 2 UAV`
- [ ] 预留 `1 GCS -> N UAV`

### 5.3 网络矩阵

- [x] localhost 单机多进程
- [ ] 双机有线 LAN
- [x] 双机 Wi-Fi
- [ ] 双机有线 + netem
- [ ] 双机 Wi-Fi + netem

### 5.4 操作系统矩阵

- [x] Linux 作为全量主平台。
- [x] macOS 作为开发平台 smoke。
- [x] Windows 作为 bindings 与安装 smoke。
- [ ] 预留 Linux ground + Linux air 的正式双机主线。
- [ ] 预留 Linux ground + macOS air 的实验组合。

---

## 6. 目录与脚本布局

### 6.1 建议目录

- [x] 新增 `tests/core/`，承接纯协议与 codec 级测试。
- [x] 新增 `tests/runtime/`，承接 session / authority / command 状态机测试。
- [x] 新增 `tests/transport/`，承接 TCP / UDP / parser / source isolation 测试。
- [x] 新增 `tests/ffi/`，承接共享 ABI 契约与平台加载测试。
- [x] 保持 `tests/bindings/` 聚焦 bindings 与 interop。
- [x] 新增 `tests/integration/single_host/`。
- [x] 新增 `tests/integration/dual_host/`。
- [x] 新增 `tests/network/`，承接 netem profile 与弱网测试。
- [x] 新增 `tests/soak/`。
- [x] 新增 `tests/perf/`。
- [x] 新增 `tests/security/`。

### 6.2 建议脚本布局

- [x] 新增 `tools/testing/README.md`，说明测试脚本体系。
- [x] 新增 `tools/testing/dual_host/hosts.example.yaml`。
- [x] 新增 `tools/testing/dual_host/run_suite.py`。
- [x] 新增 `tools/testing/dual_host/deploy_artifacts.sh`。
- [x] 新增 `tools/testing/dual_host/collect_logs.sh`。
- [x] 新增 `tools/testing/netem/apply_profile.sh`。
- [x] 新增 `tools/testing/netem/clear_profile.sh`。
- [x] 新增 `tools/testing/netem/profiles/`。
- [x] 新增 `tools/testing/report/render_summary.py`。
- [x] 新增 `tools/testing/perf/run_perf_suite.py`。

### 6.3 建议结果目录

- [x] 使用 `output/testing/<date>/<suite>/` 保存测试产物。
- [x] 每轮保存 `summary.json`。
- [x] 每轮保存 `summary.md`。
- [ ] 每个 host 保存独立日志目录。
- [ ] 每个 case 保存开始时间、结束时间、profile、角色组合、结果码。

---

## 7. 每类测试应包含的检查点

### 7.1 会话与 authority

- [ ] connect latency
- [ ] session open latency
- [ ] authority grant latency
- [ ] authority renew latency
- [ ] authority release latency
- [ ] authority expire latency
- [ ] preempt latency
- [x] release 后 current_authority 清空

### 7.2 命令平面

- [x] command publish 返回 handle 合法
- [ ] `message_id` 唯一
- [x] `correlation_id` 与结果流匹配
- [x] result phase 顺序稳定
- [x] result detail 可追踪
- [x] 无 authority 时命令不应形成成功结果

### 7.3 状态平面

- [x] state snapshot 来源正确
- [x] target routing 正确
- [x] 非目标实体不应收到状态
- [ ] state flood 时退化行为符合预期
- [ ] state plane 不污染 command plane 的验证结果

### 7.4 恢复流程

- [x] reconnect 前旧 peer 句柄是否可继续使用要有稳定错误结果
- [x] reopen 前旧 session 不应被默默复活
- [x] reacquire 前 current_authority 不应伪装为有效
- [x] 恢复链路必须按显式步骤完成
- [x] 恢复后 command 与 state 再次恢复正常

---

## 8. 双机联动必测场景

### 8.1 基线场景

- [x] `ground connect -> open_session -> request_authority -> renew -> goto -> receive_result -> receive_state -> release`
- [x] 同一条基线场景连续跑 10 次。
- [x] 同一条基线场景换 Rust / Python ground 各跑 10 次。

### 8.2 控制权竞争场景

- [x] GCS-A claim 成功。
- [x] GCS-B claim 失败或保持非 controller。
- [x] GCS-B preempt 成功。
- [ ] GCS-A release 后 GCS-B 接管。
- [x] GCS-A 旧 session 不应继续发送成功命令。

### 8.3 双 UAV 隔离场景

- [x] GCS 对 UAV-1 发命令。
- [ ] UAV-2 不应收到该命令。
- [x] UAV-1 状态只回到正确订阅方。
- [ ] 广播 target 时多个 UAV 的回包仍可区分来源。

### 8.4 恢复场景

- [x] ground 进程重启。
- [x] air 进程重启。
- [ ] ground 所在 host 网络断开后恢复。
- [ ] air 所在 host 网络断开后恢复。
- [ ] authority 过期后重新 claim。
- [x] release 后重新 claim。

### 8.5 弱网场景

- [ ] 高延迟下 command 仍可完成。
- [ ] 高丢包下 state plane 可以退化但不能污染 authority 语义。
- [ ] reorder 下 parser / envelope 不应混乱。
- [ ] duplication 下不应制造错误重复语义。
- [ ] 断网恢复后不应自动偷偷恢复 authority。

---

## 9. 性能、资源与稳定性指标

### 9.1 建议观测指标

- [ ] `connect_to_session_ready_ms`
- [ ] `authority_acquire_ms`
- [ ] `authority_renew_ms`
- [ ] `command_publish_to_first_result_ms`
- [ ] `command_publish_to_success_ms`
- [ ] `state_snapshot_end_to_end_ms`
- [ ] `reconnect_to_ready_ms`
- [ ] `process_rss_mb`
- [ ] `cpu_percent`
- [ ] `thread_count`
- [ ] `fd_count`
- [ ] `socket_count`
- [ ] `event_queue_depth`
- [ ] `lagged_event_count`

### 9.2 建议验收方式

- [ ] 先建立基线，不急着先定死所有数值。
- [ ] 所有指标先记录 1 周。
- [ ] 再基于真实数据给出 P50 / P95 / P99 门槛。
- [ ] release 门禁以相对回归检测为主，避免过早硬编码不合理阈值。

---

## 10. CI / Nightly / Release 门禁

### 10.1 PR Gate

- [ ] 运行 core 单测。
- [ ] 运行 FFI 合约测试。
- [ ] 运行 Rust 单测。
- [ ] 运行 Python 单测。
- [ ] 运行单机 interop smoke。
- [ ] 运行 bindings build + wheel smoke。

### 10.2 Nightly Gate

- [ ] 运行 PR Gate 全部内容。
- [ ] 运行双机基线套件。
- [ ] 运行双机恢复套件。
- [ ] 运行双机竞争套件。
- [ ] 运行至少一个弱网 profile。
- [ ] 运行 30min soak。

### 10.3 Release Gate

- [ ] 运行 Nightly Gate 全部内容。
- [ ] 运行 2h soak。
- [ ] 运行 shared secret mismatch / protocol mismatch / invalid target 全部拒绝路径。
- [ ] 运行 tri-platform 产物验证。
- [ ] 生成 release 前测试报告并归档。

---

## 11. 文档与测试报告

- [x] 在 `docs/bindings/test-matrix.md` 保持高层矩阵。
- [x] 在本文件维护执行清单与推进顺序。
- [x] 新增 `docs/bindings/dual-host-lab-guide.md`，说明双机实验室准备方法。
- [x] 新增 `docs/bindings/network-profiles.md`，说明 netem profile。
- [x] 新增 `docs/bindings/test-report-template.md`，沉淀回归报告模板。
- [x] 每次新增测试域后同步更新 docs 入口。

---

## 12. 推荐执行顺序

### 12.1 立即优先

- [x] 先把测试目录和脚本体系整理出来。
- [x] 先把双机基线链路跑通并标准化脚本。
- [x] 先补 authority expiry、shared secret mismatch、protocol mismatch。
- [x] 先补 `2 GCS -> 1 UAV` 与 `1 GCS -> 2 UAV`。
- [x] 先补弱网 profile 脚手架。

### 12.2 第二优先级

- [ ] 再做双机恢复矩阵。
- [ ] 再做弱网行为矩阵。
- [ ] 再做 30min soak。
- [ ] 再把 nightly 流水线接起来。

### 12.3 第三优先级

- [ ] 最后做 2h / 8h soak。
- [ ] 最后做 perf 基线与回归阈值。
- [ ] 最后预留 HIL / 真机飞控闭环接口。

---

## 13. 近期缺失项清单

下面这些项建议优先纳入下一轮实现：

- [x] authority 自然过期测试
- [x] shared secret mismatch 测试
- [x] protocol mismatch 测试
- [x] invalid target / invalid source 测试
- [x] `2 GCS -> 1 UAV` 抢占矩阵
- [x] `1 GCS -> 2 UAV` 路由隔离矩阵
- [ ] 双机有线基线脚本
- [x] 双机恢复脚本
- [x] `tc netem` profile 脚本
- [ ] 30min soak 脚本
- [x] 测试结果采集与 summary 渲染

---

## 14. 完成判定

当且仅当满足下面条件，才能认为 `yunlink` 测试体系进入“可支撑双机联动开发”的阶段：

- [x] 核心测试域已按目录分层。
- [x] 单机多语言互操作矩阵稳定运行。
- [x] 双机基线套件稳定运行。
- [x] 双机恢复套件稳定运行。
- [x] 双机竞争套件稳定运行。
- [ ] 至少一个弱网 profile 已纳入自动化。
- [ ] 30min soak 已纳入 nightly。
- [ ] PR / nightly / release 三层门禁已定义并落地。
- [ ] 失败时可自动收集足够日志并复盘。
- [ ] 文档、脚本、测试入口保持一致，不存在“只有人记得怎么跑”的隐性流程。

---

## 15. 2026-04-24 实测进展

以下回填基于 2026 年 4 月 24 日 Office Wi-Fi 双机实测产物：

- [x] `dual-host-competition` 首次通过产物位于 `output/testing/20260424-232756/dual-host-competition/`。
- [x] `dual-host-routing` 首次通过产物位于 `output/testing/20260424-232756/dual-host-routing/`。
- [x] `dual-host-competition` 已在 `20260424-232841` 到 `20260424-232901` 之间连续稳定通过 10 次。
- [x] `dual-host-routing` 已在 `20260424-232841` 到 `20260424-232856` 之间连续稳定通过 10 次。
- [x] `2 GCS -> 1 UAV` 当前已验证：
  GCS-A claim 成功、GCS-B 非抢占 claim 不应形成成功命令、GCS-B preempt 成功、GCS-A 旧 session 不应继续形成成功命令、GCS-B release 后 GCS-A 可显式重新接管。
- [x] `1 GCS -> 2 UAV` 当前已验证：
  一个 ground 可同时连接两个 UAV、错误 target 不应形成成功命令、错误定向 state 不应泄漏到 ground 7、正确 target 的状态和命令结果按各自 session 回到正确 ground。
- [ ] 当前尚未补齐“GCS-A release 后 GCS-B 接管”的镜像顺序用例；现有竞争脚本覆盖的是 “GCS-B release 后 GCS-A reacquire”。
- [ ] 当前尚未补齐广播 target 下多个 UAV 同时回包时的来源区分。
