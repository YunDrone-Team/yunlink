# 双机实验室准备指南

## 目标

为 `yunlink` 的双机联动测试准备稳定、可复现的实验室环境。

## 推荐最小环境

- `host-ground`
  地面站测试机，负责运行 Rust / Python ground helper、日志采集与结果汇总。
- `host-air`
  机载侧测试机，负责运行 C++ / Rust / Python air helper。

## 基础要求

- 两台主机固定主机名与固定网卡名。
- 双方具备固定 IP，至少一条有线 LAN 链路。
- 两台主机上的 repo 路径保持一致，例如 `/opt/yunlink`。
- 两台主机统一使用同一构建 preset、同一 shared secret、同一测试用户。

## 准备步骤

1. 拉取同一版本代码。
2. 用 `tools/testing/dual_host/sync_repo.sh` 把本地 repo 同步到远端固定路径。
3. 用 `tools/testing/dual_host/bootstrap_host.sh` 或 `prepare_hosts.sh` 在两台主机上准备 `.venv`、Rust toolchain、`cmake --preset ninja-debug`。
4. 使用 `tools/testing/dual_host/hosts.example.yaml` 或环境专用配置文件。
5. 优先先跑 dry-run，确认命令路径和 host 配置无误。

## 当前 Office Wi-Fi 约定

- `host-ground`
  本机 macOS，repo 路径 `/Users/groove/Project/work/YunDrone/yunlink`
- `host-air`
  `OfficeUbuntu26`，repo 路径 `/home/groove/Project/work/YunDrone/yunlink`
- 环境配置文件：
  `tools/testing/dual_host/office_wifi_lab.yaml`

推荐先执行：

```bash
tools/testing/dual_host/prepare_hosts.sh \
  --remote-host OfficeUbuntu26 \
  --remote-repo-dir /home/groove/Project/work/YunDrone/yunlink \
  --preset ninja-debug

python3 tools/testing/dual_host/run_suite.py \
  --config tools/testing/dual_host/office_wifi_lab.yaml \
  --suite dual-host-baseline \
  --dry-run
```

当前 Office Wi-Fi 已落地并稳定运行的 suite：

- `dual-host-baseline`
  `C++ air + Rust ground`、`C++ air + Python ground`
- `dual-host-recovery`
  `Python air + Rust ground` 显式恢复
  `Python air restart + Rust ground` air 重启恢复
- `dual-host-competition`
  `Python air + Python ground` 的 `2 GCS -> 1 UAV` 控制权竞争矩阵
- `dual-host-routing`
  `Python air + Python ground` 的 `1 GCS -> 2 UAV` 路由与状态隔离矩阵

截至 2026 年 4 月 24 日，Office Wi-Fi 双机环境已经得到下面的稳定性证据：

- `dual-host-competition`
  首次通过产物位于 `output/testing/20260424-232756/dual-host-competition/`，随后在 `20260424-232841` 到 `20260424-232901` 间连续通过 10 次。
- `dual-host-routing`
  首次通过产物位于 `output/testing/20260424-232756/dual-host-routing/`，随后在 `20260424-232841` 到 `20260424-232856` 间连续通过 10 次。

当前这两类 suite 的语义边界如下：

- `dual-host-competition`
  已验证 A 先 claim、B 非抢占 claim 不得形成成功命令、B preempt 接管、A 旧 session 不再形成成功命令、B release 后 A 可重新接管。
- `dual-host-routing`
  已验证一个 ground 可同时连接两个 UAV、错误 target 不形成成功命令、错误定向 state 不应泄漏、正确 target 的状态与命令结果回到正确 session。

当前仍未覆盖：

- A release 后 B 接管的镜像顺序用例
- broadcast target 下多个 UAV 同时回包时的来源区分

## 日志建议

- 每台主机单独保留 `stdout.log`、`stderr.log`。
- 每个 case 记录开始/结束时间、suite、profile、角色组合。
- 所有结果统一汇总到 `output/testing/...`。

## 注意事项

- 双机测试的恢复链路必须显式执行 `reconnect -> reopen -> reacquire`。
- 当验证 air 重启恢复时，优先让 ground 保持单进程 runtime，不要把“两个 ground 进程串起来”误当成 air restart 成功。
- 当前阶段不要把自动恢复加入 SDK 后再用双机测试掩盖生命周期问题。
