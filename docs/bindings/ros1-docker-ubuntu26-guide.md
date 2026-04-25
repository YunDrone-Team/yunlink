# ROS1 Docker Deployment on Ubuntu 26.04

This guide deploys ROS1 Noetic on an Ubuntu 26.04 host by running the official `ros:noetic-ros-base` Docker image. It avoids installing ROS1 directly on Ubuntu 26.04, because Noetic targets Ubuntu 20.04/Focal.

## Target

- Host: Ubuntu 26.04 LTS
- Container image: `ros:noetic-ros-base`
- Container name: `ros1-noetic`
- Network mode: host
- ROS master: `http://127.0.0.1:11311`
- Workspace mount: `~/ros1_ws:/root/ros1_ws`

## Prerequisites

Docker must already be installed and usable without `sudo`:

```bash
docker --version
docker ps
```

If `docker ps` reports a permission error, add the current user to the `docker` group and log in again:

```bash
sudo usermod -aG docker "$USER"
```

## Pull ROS1 Noetic

```bash
docker pull ros:noetic-ros-base
```

## Start ROS1

```bash
mkdir -p "$HOME/ros1_ws"

docker run -d \
  --name ros1-noetic \
  --restart unless-stopped \
  --network host \
  --ipc host \
  -e ROS_MASTER_URI=http://127.0.0.1:11311 \
  -e ROS_IP=127.0.0.1 \
  -v "$HOME/ros1_ws:/root/ros1_ws" \
  -w /root/ros1_ws \
  ros:noetic-ros-base \
  bash -lc "source /opt/ros/noetic/setup.bash && roscore"
```

If the container already exists:

```bash
docker start ros1-noetic
```

## Verify

```bash
docker exec ros1-noetic bash -lc \
  "source /opt/ros/noetic/setup.bash && echo ROS_DISTRO=\$ROS_DISTRO && rosversion -d && rosnode list"
```

Expected output includes:

```text
ROS_DISTRO=noetic
noetic
/rosout
```

## Install Build Dependencies

The base image is intentionally small. Install the catkin and ROS message dependencies needed by the Sunray Yunlink bridge:

```bash
docker exec ros1-noetic bash -lc \
  "apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    python3-catkin-tools python3-rosdep python3-vcstool \
    build-essential cmake git \
    ros-noetic-geographic-msgs ros-noetic-mavros-msgs \
    ros-noetic-tf ros-noetic-tf2-ros ros-noetic-tf2-geometry-msgs \
    ros-noetic-visualization-msgs ros-noetic-gazebo-msgs"
```

## Build The Sunray Bridge Workspace

Mirror the local repositories into the mounted host workspace:

```bash
rsync -az --delete \
  --exclude='.git/' --exclude='build/' --exclude='devel/' --exclude='install/' \
  --exclude='log/' --exclude='output/' --exclude='.cache/' \
  /path/to/yunlink/ \
  <host>:~/ros1_ws/src/yunlink/

rsync -az --delete \
  --exclude='.git/' --exclude='build/' --exclude='devel/' --exclude='install/' \
  --exclude='log/' --exclude='.cache/' \
  /path/to/sunray_v2/ \
  <host>:~/ros1_ws/src/sunray_v2/
```

Create a minimal catkin workspace that contains only the Sunray message/common packages and the bridge package. Use relative symlinks so the links resolve both on the Ubuntu host and inside the Docker mount:

```bash
mkdir -p "$HOME/ros1_ws/bridge_ws/src"
cd "$HOME/ros1_ws/bridge_ws/src"

ln -s ../../src/sunray_v2/common/sunray_msgs sunray_msgs
ln -s ../../src/sunray_v2/common/sunray_common sunray_common
ln -s ../../src/sunray_v2/communication/sunray_yunlink_bridge sunray_yunlink_bridge
```

Build and test inside the ROS1 container:

```bash
docker exec ros1-noetic bash -lc \
  "source /opt/ros/noetic/setup.bash && \
   cd /root/ros1_ws/bridge_ws && \
   catkin_make"

docker exec ros1-noetic bash -lc \
  "source /opt/ros/noetic/setup.bash && \
   source /root/ros1_ws/bridge_ws/devel/setup.bash && \
   cd /root/ros1_ws/bridge_ws && \
   catkin_make run_tests && catkin_test_results"
```

Expected test summary:

```text
Summary: 24 tests, 0 errors, 0 failures, 0 skipped
```

## Bridge Launch Smoke Test

Check that ROS can find the package and parse the launch file:

```bash
docker exec ros1-noetic bash -lc \
  "source /opt/ros/noetic/setup.bash && \
   source /root/ros1_ws/bridge_ws/devel/setup.bash && \
   rospack find sunray_yunlink_bridge && \
   roslaunch sunray_yunlink_bridge sunray_yunlink_bridge.launch --nodes"
```

Start the bridge briefly with fake UAV identity parameters. A `timeout` exit is expected; the important signal is that the node starts and logs the resolved UAV namespace:

```bash
docker exec ros1-noetic bash -lc \
  "source /opt/ros/noetic/setup.bash && \
   source /root/ros1_ws/bridge_ws/devel/setup.bash && \
   rosparam set /uav_name uav && \
   rosparam set /uav_id 1 && \
   timeout 5s roslaunch sunray_yunlink_bridge sunray_yunlink_bridge.launch; \
   code=\$?; if [ \$code -eq 124 ]; then echo ROSLAUNCH_TIMEOUT_OK; exit 0; fi; exit \$code"
```

Expected output includes:

```text
sunray_yunlink_bridge started for /uav1
ROSLAUNCH_TIMEOUT_OK
```

## Useful Commands

Open a shell inside the ROS1 container:

```bash
docker exec -it ros1-noetic bash -lc "source /opt/ros/noetic/setup.bash && exec bash"
```

Stop ROS1:

```bash
docker stop ros1-noetic
```

Restart ROS1:

```bash
docker restart ros1-noetic
```

Remove the container:

```bash
docker rm -f ros1-noetic
```
