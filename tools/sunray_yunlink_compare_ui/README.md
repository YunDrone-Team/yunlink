# sunray_yunlink_compare_ui

该工具用于对比两路数据是否一致：

- Yunlink 收到的状态快照
- Sunray ROS 原始话题

它检查这些数据是否与 `Sunray_v2/communication/yunlink_ros_bridge/src/yunlink_ros_bridge_node.cpp` 当前实际上行到 Yunlink 的字段映射保持一致。

当前 UI 只覆盖 `yunlink_ros_bridge` 的 5 个上行 state snapshot：

- `LocalOdomSnapshot` <-> `/uav1/sunray/localization/local_odom`
- `OdomStateSnapshot` <-> `/uav1/sunray/localization/odom_state`
- `UavControlStateSnapshot` <-> `/uav1/sunray/uav_control/control_state`
- `MavrosStateSnapshot` <-> `/uav1/mavros/state`
- `Px4StateSnapshot` <-> `/uav1/sunray/px4_state`

它不校验 Yunlink 下行命令链路；`yunlink_ros_bridge` 当前还额外处理：

- `/uav1/sunray/uav_control/control_cmd` 发布
- `takeoff`
- `land`
- `return`
- `goto`
- `velocity_setpoint`

默认参数为 `agent_name=uav`、`agent_id=1`，因此默认订阅 `uav1` 命名空间。若运行时修改 `~agent_name` 或 `~agent_id`，UI 订阅的话题会切到对应的 `/<agent_name><agent_id>/...` 命名空间。

## 比对原则

- 每个页面只比较对应 ROS 话题、Yunlink snapshot 和 UI 显示字段三者共同定义的字段。
- 当前 `yunlink_ros_bridge_node.cpp` 对上述 5 个上行话题执行逐字段映射，UI 不再把这些话题的 `.msg` 字段列为“未覆盖”。
- UI 同时提供“最新值直比”和“时间对齐比”两种视图。
- 时间对齐窗口默认是 `300 ms`；超出窗口时仍保留最近样本显示，避免短暂采样不同步导致整页闪成 `WAIT`。

## 构建

### 依赖关系

该 UI 会直接链接 `yunlink` 顶层 CMake 生成的主静态库：

```cmake
add_library(yunlink STATIC ${YUNLINK_LIBRARY_SOURCES})
```

默认库路径为：

```text
<YUNLINK_ROOT>/build/ninja-debug/libyunlink.a
```

### 先生成 `libyunlink.a`

推荐先在 `yunlink` 仓库根目录执行：

```bash
cd <YUNLINK_ROOT>
cmake --preset ninja-debug
cmake --build --preset ninja-debug
```

### 在 `yunlink` 仓库内独立构建 UI

先确保：

1. 已经生成 `<YUNLINK_ROOT>/build/ninja-debug/libyunlink.a`
2. 当前 shell 已经能找到 ROS 与 `sunray_msgs` 相关依赖
3. `<YUNLINK_ROOT>` 指向当前 `yunlink` 仓库根目录
4. `<SUNRAY_WS>` 指向能提供 `sunray_msgs` 的 Sunray ROS 工作区根目录

```bash
source /opt/ros/noetic/setup.bash
source <SUNRAY_WS>/devel/setup.bash

cd <YUNLINK_ROOT>/tools/sunray_yunlink_compare_ui
mkdir -p build
cd build
cmake ..
make -j4
```

说明：

- `CMakeLists.txt` 默认会把当前工具所在仓库根目录识别为 `YUNLINK_ROOT`，标准目录下通常直接 `cmake ..` 即可。
- 如果 `libyunlink.a` 不在默认的 `<YUNLINK_ROOT>/build/ninja-debug/`，可显式指定：

```bash
cmake .. -DYUNLINK_BUILD_DIR=<YUNLINK_BUILD_DIR>
```

或直接指定静态库路径：

```bash
cmake .. -DYUNLINK_LIBRARY=<PATH_TO_LIBYUNLINK_A>
```

### 在 `Sunray_v2` 工作区内运行 UI

当前本机工作流里，UI 更常见的运行方式是直接使用 `Sunray_v2` 工作区产物：

```bash
source /opt/ros/noetic/setup.bash
source <SUNRAY_WS>/devel/setup.bash

<SUNRAY_WS>/devel/lib/sunray_yunlink_compare_ui/sunray_yunlink_compare_ui
```

如果你使用当前工作区里的本地辅助脚本，默认入口是：

- [yunlink_ui.sh](/home/taolin/Sunray_v2/scripts_test/yunlink_ui.sh)

该脚本当前默认：

- `YUNLINK_ROOT=${SUNRAY_ROOT}/thirdparty/yunlink`
- `COMPARE_UI_BIN=${SUNRAY_ROOT}/devel/lib/sunray_yunlink_compare_ui/sunray_yunlink_compare_ui`

也就是说，README 里的 `tools/.../build/devel/...` 路径只适用于“独立构建 UI”那条路径，不等同于 `Sunray_v2` 工作区里的默认运行方式。

## 运行前提

先确保已经有一套可用的对比环境：

1. `roscore` 已启动
2. Sunray 侧原始 ROS 话题已经在发布
3. `yunlink_ros_bridge_node` 已启动，并且正在把同一架飞机的数据上行到 Yunlink

当前工作区里可直接参考：

- [yunlink_test.sh](/home/taolin/Sunray_v2/scripts_test/yunlink_test.sh)
- [yunlink_ros_bridge.launch](/home/taolin/Sunray_v2/communication/yunlink_ros_bridge/launch/yunlink_ros_bridge.launch)

## 端口配对

UI 和 `yunlink_ros_bridge` 默认是一对互相配套的端口设置。

### UI 默认值

- `~remote_ip=127.0.0.1`
- `~remote_tcp_port=9696`
- `~udp_bind_port=9797`
- `~udp_target_port=9898`
- `~tcp_listen_port=9797`

### `yunlink_ros_bridge` 默认值

- `udp_bind_port=9696`
- `udp_target_port=9898`
- `tcp_listen_port=9696`
- `remote_ip=127.0.0.1`
- `remote_tcp_port=9797`

含义上可以理解为：

- UI 主动连接 bridge 的 TCP 监听端口 `9696`
- bridge 回连 UI 的 TCP 监听端口 `9797`

如果你修改一侧端口，另一侧也要同步修改，否则会出现能启动但连不上会话的情况。

## 直接运行 UI 二进制

### 独立构建产物

```bash
source /opt/ros/noetic/setup.bash
source <SUNRAY_WS>/devel/setup.bash

cd <YUNLINK_ROOT>/tools/sunray_yunlink_compare_ui
./build/devel/lib/sunray_yunlink_compare_ui/sunray_yunlink_compare_ui
```

### `Sunray_v2` 工作区产物

```bash
source /opt/ros/noetic/setup.bash
source <SUNRAY_WS>/devel/setup.bash

<SUNRAY_WS>/devel/lib/sunray_yunlink_compare_ui/sunray_yunlink_compare_ui
```

## 常用参数

可直接通过 ROS 私有参数传入，例如：

```bash
<SUNRAY_WS>/devel/lib/sunray_yunlink_compare_ui/sunray_yunlink_compare_ui \
  _agent_name:=uav \
  _agent_id:=2 \
  _align_window_ms:=300 \
  _remote_ip:=127.0.0.1 \
  _remote_tcp_port:=9696
```

常用私有参数如下：

- `~agent_name`，默认 `uav`
- `~agent_id`，默认 `1`
- `~align_window_ms`，默认 `300`
- `~shared_secret`，默认 `yunlink-default-secret`
- `~remote_ip`，默认 `127.0.0.1`
- `~remote_tcp_port`，默认 `9696`
- `~udp_bind_port`，默认 `9797`
- `~udp_target_port`，默认 `9898`
- `~tcp_listen_port`，默认 `9797`
- `~node_name`，默认 `sunray_yunlink_compare_ui`
- `~history_limit`，默认 `200`
- `~log_limit`，默认 `500`

其中：

- `history_limit` 控制每个 topic 保留多少历史样本，用于“时间对齐比”
- `log_limit` 控制 UI 内日志面板最多保留多少条记录

## 本地辅助脚本

下面这些脚本属于当前 `Sunray_v2` 工作区的本地辅助入口，不属于 `yunlink` 仓库协议核心的一部分，但在当前开发环境里是最直接的启动方式：

- [yunlink_test.sh](/home/taolin/Sunray_v2/scripts_test/yunlink_test.sh)：启动 `roscore`、仿真、定位、控制、`yunlink_ros_bridge`
- [yunlink_ui.sh](/home/taolin/Sunray_v2/scripts_test/yunlink_ui.sh)：启动 compare UI

如果你是在别的工作区复用这个 UI，建议按自己的目录结构另写本地脚本，而不是把个人绝对路径写回公共 README。
