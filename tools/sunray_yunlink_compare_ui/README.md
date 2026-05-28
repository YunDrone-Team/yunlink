# sunray_yunlink_compare_ui

该工具用于对比：

- Yunlink 收到的状态快照
- Sunray ROS 原始话题

检查它们是否与 `Sunray_v2/communication/yunros/src/yunros_node.cpp` 当前实际上行到 Yunlink 的字段映射保持一致。

默认参数为 `agent_name=uav`、`agent_id=1`，因此默认比对的话题是：

- `LocalOdomSnapshot` <-> `/uav1/sunray/localization/local_odom`
- `OdomStateSnapshot` <-> `/uav1/sunray/localization/odom_state`
- `UavControlStateSnapshot` <-> `/uav1/sunray/uav_control/control_state`
- `MavrosStateSnapshot` <-> `/uav1/mavros/state`
- `Px4StateSnapshot` <-> `/uav1/sunray/px4_state`

如果运行时修改了私有参数 `~agent_name` 或 `~agent_id`，UI 订阅的话题也会跟着切到对应的 `/<agent_name><agent_id>/...` 命名空间。

比对原则：

- 每个页面只比较对应 ROS 话题、Yunlink snapshot 和 UI 显示字段三者共同定义的字段。
- 当前 `yunros_node.cpp` 对上述 5 个上行话题执行逐字段映射，UI 不再把这些话题的 `.msg` 字段列为“未覆盖”。
- UI 同时提供“最新值直比”和“时间对齐比”两种视图，时间对齐窗口默认是 `300 ms`；超出窗口时仍保留最近样本显示，避免短暂采样不同步导致整页闪成 `WAIT`。

## 构建

### `libyunlink.a` 是什么

`libyunlink.a` 是 `yunlink` 仓库顶层 CMake 生成的主静态库产物，对应顶层 `CMakeLists.txt` 里的：

```cmake
add_library(yunlink STATIC ${YUNLINK_LIBRARY_SOURCES})
```

这个 compare UI 会直接链接这份静态库，以复用 `yunlink` 的协议类型、编解码和 runtime 能力。

默认情况下，这个静态库会出现在：

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

- `CMakeLists.txt` 默认会把当前工具所在仓库根目录识别为 `YUNLINK_ROOT`，因此标准目录下通常直接 `cmake ..` 即可。
- 如果 `libyunlink.a` 不在默认的 `<YUNLINK_ROOT>/build/ninja-debug/`，请显式指定：

```bash
cmake .. -DYUNLINK_BUILD_DIR=<YUNLINK_BUILD_DIR>
```

或者直接指定静态库路径：

```bash
cmake .. -DYUNLINK_LIBRARY=<PATH_TO_LIBYUNLINK_A>
```

## 运行

先确保已经有一套可用的对比环境：

1. `roscore` 已启动
2. Sunray 侧原始 ROS 话题已经在发布
3. `yunros_node` 已启动，并且正在把同一架飞机的数据上行到 Yunlink

### 直接运行 UI 二进制

也可以直接运行：

```bash
source /opt/ros/noetic/setup.bash
source <SUNRAY_WS>/devel/setup.bash

cd <YUNLINK_ROOT>/tools/sunray_yunlink_compare_ui
./build/devel/lib/sunray_yunlink_compare_ui/sunray_yunlink_compare_ui
```

如果需要切换到其它飞机命名空间，可直接传 ROS 私有参数，例如：

```bash
./build/devel/lib/sunray_yunlink_compare_ui/sunray_yunlink_compare_ui \
  _agent_name:=uav \
  _agent_id:=2 \
  _align_window_ms:=300
```

常用私有参数：

- `~agent_name`，默认 `uav`
- `~agent_id`，默认 `1`
- `~align_window_ms`，默认 `300`
- `~shared_secret`，默认 `yunlink-default-secret`

## 本地辅助脚本示例

下面两个脚本是“本地测试辅助脚本”的示例写法，便于你在自己的工作区快速拉起环境。

注意：

- 它们不是 `yunlink` 仓库的必需组成部分。
- 不应把你个人机器上的绝对路径直接写成公共 README 的唯一使用方式。
- 如果你想在自己的工作区复用，可以按自己的目录结构调整后另存为本地脚本。

### 示例 1：启动一套本地测试环境

这个脚本会启动：

- `roscore`
- `sunray_simulator`
- `localization_fusion`
- `sunray_uav_control`
- `yunros_node`

它不会启动本 UI。

```bash
#!/bin/bash

gnome-terminal --window -e 'bash -c "roscore; exec bash"' \
--tab -e 'bash -c "sleep 5.0; roslaunch sunray_simulator sunray_sim_uav_planning.launch; exec bash"' \
--tab -e 'bash -c "sleep 5.0; roslaunch localization_fusion localization_fusion.launch; exec bash"' \
--tab -e 'bash -c "sleep 5.0; roslaunch sunray_uav_control uav_control.launch; exec bash"' \
--tab -e 'bash -c "sleep 6.0; roslaunch yunros yunros.launch; exec bash"'
```

### 示例 2：启动 compare UI

假设你把这个脚本放在 `<SUNRAY_WS>/scripts_test/yunlink_ui.sh`，可以写成：

```bash
#!/bin/bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SUNRAY_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
YUNLINK_ROOT="<YUNLINK_ROOT>"
COMPARE_UI_BIN="${YUNLINK_ROOT}/tools/sunray_yunlink_compare_ui/build/devel/lib/sunray_yunlink_compare_ui/sunray_yunlink_compare_ui"

source /opt/ros/noetic/setup.bash
source "${SUNRAY_ROOT}/devel/setup.bash"

"${COMPARE_UI_BIN}"
```
