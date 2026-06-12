# yunlink_advanced_monitor

`yunlink_advanced_monitor` 现在定位为一个纯 `Qt + libyunlink` 的 Ground Station UI。

它现在主要做这些事：

- 显示 YunLink runtime / session / link / authority 状态
- 通过 YunLink 下发控制命令
- 展示通过 YunLink 收到的状态 snapshot
- 通过 YunLink system service 通道查询和操作 feature
- 展示命令回执、system service 请求结果和运行日志

它不再承担这些职责：

- 直接订阅 ROS 原始 topic
- 直接比较 ROS 值与 YunLink 值
- 直接读取 ROS `/sunray/system_info`
- 直接调用 ROS service 形式的 `list_features / get_features / start_feature / stop_feature`

如果后续还需要做 ROS 原始值和桥接结果的核对，应当放到 `Sunray_v2` 侧单独调试工具，不再塞回本工具。

## 当前界面

当前界面改成左侧便签式导航 + 右侧内容区切换：

- 左侧顶层导航
  - `Commands`
  - `System`
  - `State`
  - `Logs`
- `Commands`
  - 会话状态
  - `runtime / session / link / authority`
  - `peer id / session id / 对端地址 / 本地监听地址`
  - 最近 3 条 `WARN / ERROR`
  - 最近说明与最近错误
  - `立即重连`
  - 控制面板
  - `TAKEOFF / LAND / RETURN / MOVE_POINT / MOVE_VELOCITY`
  - 命令历史
  - 发送时间、命令、状态、session、message id、回执
- `System`
  - 页面标题仍为 `System Service`
  - `FeatureList`
  - `FeatureGet`
  - `FeatureStart`
  - `FeatureStop`
  - `feature_name / override_args / restart_if_running / start_with_terminal / force_stop`
  - feature 列表、feature 详情、请求历史、结果状态
- `State`
  - 页内仍按 topic tab 分页
  - `local_odom`
  - `odom_state`
  - `uav_control_cmd`
  - `uav_control_state`
  - `px4_state`
  - 每个页签只展示“字段说明 / YunLink 最新值”
- `Logs`
  - 运行日志
  - `全部日志 / Warn+ / Connection / Authority / Command / System`
  - `自动跟随`
  - `清空日志`

## 显示语义

- 字段不存在，且该 topic 还没有收到任何 YunLink 数据：显示 `WAIT`
- 字段不存在，但该 topic 已经收到过 YunLink 数据：显示 `--`
- 字段存在，但显示文本为空串或纯空白：显示 `<empty>`
- 其他情况：显示实际值

命令历史语义：

- `SENT`
  - monitor 已经把 YunLink command 发出
- `RECEIVED`
  - 已收到 YunLink command result
- `TIMEOUT`
  - monitor 在超时时间内没有等到 YunLink command result

System Service 请求历史语义：

- `PENDING`
  - 请求已经发出，仍在等待响应
- `SUCCEEDED`
  - 已收到成功响应
- `FAILED`
  - 已收到失败响应
- `TIMEOUT`
  - monitor 在超时时间内没有等到响应

## 构建依赖

- Qt5 Widgets
- `libyunlink.a`

## 构建

先构建 YunLink 主库：

```bash
cd <YUNLINK_ROOT>
cmake --preset ninja-debug
cmake --build --preset ninja-debug
```

再构建 monitor：

```bash
cd <YUNLINK_ROOT>/tools/yunlink_advanced_monitor
mkdir -p build
cd build
cmake .. \
  -DYUNLINK_ROOT=<YUNLINK_ROOT> \
  -DYUNLINK_BUILD_DIR=<YUNLINK_ROOT>/build/ninja-debug
cmake --build . -j4
```

在 `Sunray_v2` 子模块场景下，`YUNLINK_ROOT` 通常就是：

```text
<SUNRAY_ROOT>/thirdparty/yunlink
```

## 运行

```bash
<YUNLINK_ROOT>/tools/yunlink_advanced_monitor/build/yunlink_advanced_monitor
```

也可以传命令行参数：

```bash
<YUNLINK_ROOT>/tools/yunlink_advanced_monitor/build/yunlink_advanced_monitor \
  --agent-name=uav \
  --agent-id=1 \
  --remote-ip=127.0.0.1 \
  --remote-tcp-port=9696 \
  --udp-bind-port=9797 \
  --udp-target-port=9898 \
  --tcp-listen-port=9797
```

## 常用参数

- `--remote-ip`
- `--remote-tcp-port`
- `--udp-bind-port`
- `--udp-target-port`
- `--tcp-listen-port`
- `--agent-name`
- `--agent-id`
- `--shared-secret`
- `--node-name`
- `--log-limit`
- `--authority-ttl-ms`
- `--command-history-limit`
- `--command-timeout-ms`

## 当前运行逻辑

1. 初始化 YunLink runtime
2. 绑定 YunLink link / error / authority / command / state / system service 事件
3. 由 Qt 定时轮询 discovery，刷新发现设备列表
4. 用户在 `Devices` 页面点击 `连接` 后，才主动连接选中的远端 peer
5. 如果显式传入 `--remote-ip / --remote-tcp-port`，保留直接连接远端 peer 的调试入口
6. 通过 YunLink 接收 snapshot 和 system service 响应并刷新 UI model
7. 以 `500 ms` 刷新界面

## 后续方向

- 如果 UI 需要新按钮，先确认是否需要扩 YunLink command
- 如果 UI 需要新状态，先确认是否需要扩 YunLink snapshot / event
- 如果还要做 ROS 对比或 bridge 映射核对，单独放到 `Sunray_v2` 侧工具
