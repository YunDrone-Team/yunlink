# sunray_advanced_monitor

`sunray_advanced_monitor` 是面向 `Sunray_v2 + YunLink` 的 Qt 调试界面，用来同时查看：

- ROS 原始话题最新值
- YunLink 收到的对应 snapshot 最新值
- 两侧字段逐项比较结果
- YunLink runtime / session / link 状态
- 下行命令发送、回执、回显和日志

当前工具已经不是占位骨架，已经具备基础可用能力。

## 当前界面内容

界面主要分四块：

- 会话状态
  - 显示 runtime、session、link 状态
  - 显示 peer id、session id、对端地址、本地监听地址
  - 显示最近说明、最近错误
- 控制面板
  - 支持 `TAKEOFF`
  - 支持 `LAND`
  - 支持 `RETURN`
  - 支持 `MOVE_POINT`
  - 支持 `MOVE_POINT_BODY`
  - 支持 `MOVE_VELOCITY`
  - 支持 `MOVE_VELOCITY_BODY`
- 命令历史
  - 显示发送时间、命令、状态、session、message id、回执、回显
- YunLink 状态页签
  - `local_odom`
  - `odom_state`
  - `uav_control_state`
  - `mavros_state`
  - `px4_state`
  - 每个页签都按“字段说明 / ROS 最新值 / YunLink 最新值 / 结果”展示
- 运行日志
  - 显示 runtime、session、command、state 等日志

## 比较对象

默认参数是：

- `agent_name=uav`
- `agent_id=1`

因此默认订阅和对比的是：

- `LocalOdomSnapshot` <-> `/uav1/sunray/localization/local_odom`
- `OdomStateSnapshot` <-> `/uav1/sunray/localization/odom_state`
- `UavControlStateSnapshot` <-> `/uav1/sunray/uav_control/control_state`
- `MavrosStateSnapshot` <-> `/uav1/mavros/state`
- `Px4StateSnapshot` <-> `/uav1/sunray/px4_state`

如果运行时修改了 `~agent_name` 或 `~agent_id`，订阅话题会自动切到对应命名空间。

## 显示语义

topic 表格当前使用以下显示规则：

- 字段不存在，且该侧还没有拿到任何数据：显示 `WAIT`
- 字段不存在，但该侧已经拿到过该 topic 的数据：显示 `--`
- 字段存在，但最终显示文本为空串或纯空白字符：显示 `<empty>`
- 字段存在且非空：显示实际值

比较结果当前使用：

- `OK`
- `DIFF`
- `WAIT`

注意：

- `<empty>` 只作用于 topic 表里的 `ROS 最新值` 和 `YunLink 最新值` 两列
- 会话状态区和命令历史区的空值仍然保持 `-` 或 `--`

## 构建依赖

该工具依赖：

- ROS Noetic
- `sunray_msgs`
- `mavros_msgs`
- `nav_msgs`
- Qt5 Widgets
- `libyunlink.a`

其中 `libyunlink.a` 默认来自：

```text
<YUNLINK_ROOT>/build/ninja-debug/libyunlink.a
```

`YUNLINK_ROOT` 默认会被识别成当前 `yunlink` 仓库根目录。

## 构建方式说明

如果 `yunlink` 是作为 `Sunray_v2` 的子模块存在，那么对最终使用者来说，默认构建位置应当是：

```text
<SUNRAY_ROOT>/thirdparty/yunlink
```

也就是：

- `YUNLINK_ROOT=<SUNRAY_ROOT>/thirdparty/yunlink`
- `sunray_advanced_monitor` 在这个子模块目录里单独构建

注意：

- 当前 `sunray_advanced_monitor` 还没有接入 `Sunray_v2` 顶层统一编译入口
- 它不是直接跟着 `Sunray_v2` 一次总编译自动产出的
- 当前做法是先构建子模块里的 `libyunlink.a`，再在子模块里的 `tools/sunray_advanced_monitor` 下单独构建这个工具

## 先构建子模块里的 YunLink 主库

推荐先在子模块根目录执行：

```bash
cd <SUNRAY_ROOT>/thirdparty/yunlink
cmake --preset ninja-debug
cmake --build --preset ninja-debug
```

确认已经生成：

```text
<SUNRAY_ROOT>/thirdparty/yunlink/build/ninja-debug/libyunlink.a
```

## 构建 sunray_advanced_monitor

先加载 ROS 与 Sunray 工作区环境：

```bash
source /opt/ros/noetic/setup.bash
source <SUNRAY_WS>/devel/setup.bash
```

如果是 `Sunray_v2` 子模块场景，推荐这样构建：

```bash
cd <SUNRAY_ROOT>/thirdparty/yunlink/tools/sunray_advanced_monitor
mkdir -p build
cd build
cmake .. \
  -DYUNLINK_ROOT=<SUNRAY_ROOT>/thirdparty/yunlink \
  -DYUNLINK_BUILD_DIR=<SUNRAY_ROOT>/thirdparty/yunlink/build/ninja-debug
cmake --build . -j4
```

如果你不是放在上层工程子模块里，而是单独使用 `yunlink` 仓库，也可以按独立仓库方式构建：

```bash
cd <YUNLINK_ROOT>/tools/sunray_advanced_monitor
mkdir -p build
cd build
cmake .. \
  -DYUNLINK_ROOT=<YUNLINK_ROOT> \
  -DYUNLINK_BUILD_DIR=<YUNLINK_ROOT>/build/ninja-debug
cmake --build . -j4
```

如果 `libyunlink.a` 不在默认路径，也可以直接指定：

```bash
cmake .. -DYUNLINK_LIBRARY=<PATH_TO_LIBYUNLINK_A>
```

## 运行

在 `Sunray_v2` 子模块场景下，直接运行：

```bash
source /opt/ros/noetic/setup.bash
source <SUNRAY_WS>/devel/setup.bash

<SUNRAY_ROOT>/thirdparty/yunlink/tools/sunray_advanced_monitor/build/devel/lib/sunray_advanced_monitor/sunray_advanced_monitor
```

也可以传 ROS 私有参数，例如：

```bash
<SUNRAY_ROOT>/thirdparty/yunlink/tools/sunray_advanced_monitor/build/devel/lib/sunray_advanced_monitor/sunray_advanced_monitor \
  _agent_name:=uav \
  _agent_id:=1 \
  _remote_ip:=127.0.0.1 \
  _remote_tcp_port:=9696 \
  _udp_bind_port:=9797 \
  _udp_target_port:=9898 \
  _tcp_listen_port:=9797
```

如果你是独立 `yunlink` 仓库场景，把上面的路径替换成：

```text
<YUNLINK_ROOT>/tools/sunray_advanced_monitor/build/devel/lib/sunray_advanced_monitor/sunray_advanced_monitor
```

## 常用私有参数

- `~remote_ip`
  - 默认 `127.0.0.1`
- `~remote_tcp_port`
  - 默认 `9696`
- `~udp_bind_port`
  - 默认 `9797`
- `~udp_target_port`
  - 默认 `9898`
- `~tcp_listen_port`
  - 默认 `9797`
- `~agent_name`
  - 默认 `uav`
- `~agent_id`
  - 默认 `1`
- `~shared_secret`
  - 默认 `yunlink-default-secret`
- `~node_name`
  - 默认 `sunray_advanced_monitor`
- `~log_limit`
  - 默认 `500`
- `~state_timeout_sec`
  - 默认 `2.0`
- `~authority_ttl_ms`
  - 默认 `3000`
- `~command_history_limit`
  - 默认 `32`
- `~command_timeout_ms`
  - 默认 `3000`

## 当前运行逻辑

工具启动后会：

1. 初始化 YunLink runtime
2. 绑定 link / error / session / command / state 相关事件
3. 周期性尝试连接远端 peer
4. 订阅 Sunray ROS 话题
5. 把 ROS 最新值和 YunLink 最新值分别存到 UI model
6. 以 250 ms 刷新界面

## 已知边界

- 这是调试 UI，不是完整 GCS
- topic 对比是“最新值直比”，不是时间严格对齐比对
- `WAIT` 和 `--` 有明确语义，不会被替换成 `<empty>`
- `<empty>` 只表示该字段最终显示为空，不表示字段缺失
- 如果你改了源码但界面没变化，先确认你启动的是刚编译出来的那份二进制

## 推荐使用口径

对于 `Sunray_v2` 的最终使用者，推荐口径是：

1. 把 `yunlink` 视为 `Sunray_v2` 的子模块依赖
2. 在 `<SUNRAY_ROOT>/thirdparty/yunlink` 下构建 `libyunlink.a`
3. 在 `<SUNRAY_ROOT>/thirdparty/yunlink/tools/sunray_advanced_monitor` 下单独构建 UI
4. 运行子模块里生成的 `sunray_advanced_monitor` 二进制

这样最不容易混淆源码树、构建缓存和运行产物来源。
