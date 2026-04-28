# tools/testing

这里放 `yunlink` 下一阶段测试体系的统一脚手架：

- `dual_host/`
  双机部署、编排、日志采集相关脚本。
- `netem/`
  弱网 profile 与 `tc netem` 操作脚本。
- `report/`
  测试结果聚合与摘要渲染。
- `perf/`
  指标汇总与性能基线分析。

当前这些脚本优先保证：

- 单机可 dry-run
- 输出结构稳定
- 后续可逐步接入真实双机实验室

正式结果目录约定为 `output/testing/<date>/<suite>/`。

双机/弱网/soak 套件的 case 规格统一支持以下元数据字段：

- `required_env`
  运行该 case 所需的外部环境标签列表。
- `network_profile`
  `none`、`delay`、`delay+jitter`、`loss`、`loss+reorder`、`duplication`、`short-disconnect` 等固定网络画像。
- `manual_gate`
  `nightly-local`、`manual-external`、`release-external` 等门禁口径。
- `metrics`
  当前统一指标键为 `connect_ms`、`session_ready_ms`、`authority_acquire_ms`、`command_result_ms`、`state_first_seen_ms`、`recovery_ms`。
- `artifacts`
  该 case 期望生成的日志、报告、pcap 或摘要产物路径。

双机常用入口：

- `tools/testing/dual_host/sync_repo.sh`
  将当前 workspace 增量同步到远端 repo，并自动过滤 build、venv、Rust target 等不应同步内容。
- `tools/testing/dual_host/bootstrap_host.sh`
  在单个 host 上准备 `.venv`、editable Python 绑定、CMake preset、C++ air helper、Rust examples。
- `tools/testing/dual_host/prepare_hosts.sh`
  组合执行本机 bootstrap、远端同步、远端 bootstrap。
- `tools/testing/dual_host/office_wifi_lab.yaml`
  当前 Office Wi-Fi 双机实验配置，约定本机为 `host-ground`、`OfficeUbuntu26` 为 `host-air`。
- `tools/testing/dual_host/run_suite.py`
  真正执行双机 suite，支持 air 进程先启动、ground 前台运行、TCP ready probe、case JSON 与 summary 产物。

当前双机 suite 已实测覆盖：

- `dual-host-baseline`
  `C++ air ↔ Rust ground`、`C++ air ↔ Python ground` 的 Office Wi-Fi 基线链路。
- `dual-host-recovery`
  `Python air ↔ Rust ground` 的显式 `release -> reconnect -> reopen -> reacquire` 恢复链路，以及 `Python air restart ↔ Rust ground` 的 air 真重启恢复链路。
- `dual-host-competition`
  `Python air ↔ Python ground` 的 `2 GCS -> 1 UAV` 控制权竞争矩阵，验证 claim 冲突、preempt、旧 session 失效、release 后再次接管。
- `dual-host-routing`
  `Python air ↔ Python ground` 的 `1 GCS -> 2 UAV` 路由与状态隔离矩阵，验证多 UAV 同时连接、错误 target 拒绝、定向 state 不泄漏。

截至 2026 年 4 月 24 日：

- `dual-host-competition` 已在 Office Wi-Fi 双机环境连续稳定通过 10 次。
- `dual-host-routing` 已在 Office Wi-Fi 双机环境连续稳定通过 10 次。
