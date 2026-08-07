# Sunray v2 Summary Metrics

This registry defines the `com.yundrone.sunray.*` metrics currently emitted in
`org.yunlink.telemetry/v1/SummarySnapshot`. It does not change the protobuf
schema, YunLink wire schema, or any profile version.

## Transport Boundary

`com.yundrone.sunray/v2/FlightControlState` is a high-rate controller-state
stream used for action readiness and operational safety. It carries only facts
published by the vehicle controller: arm state, landed state, controller mode
and state, battery values, and manual override. It is not a MAVROS or PX4
mirror.

`org.yunlink.telemetry/v1/SummarySnapshot` is a compact display stream. The
Bridge normally emits it at 5 Hz after an attached peer subscribes to
`<entity_uid>.status_summary`. It may be stale between updates and must not be
used as the source of high-rate pose, velocity, or command feedback. Mobility
`Odometry`, `OdomState`, `UavControlState`, and `FlightControlState` remain
their respective authoritative streams.

Every metric has an independent `MetricQuality`:

| Quality | Meaning |
| --- | --- |
| `METRIC_VALID` | A source supplied a current value. |
| `METRIC_STALE` | A source supplied a value but it exceeded its configured freshness timeout. Consumers may display it only as stale. |
| `METRIC_INVALID` | A source supplied an invalid value. The current Bridge does not synthesize substitute values. |
| `METRIC_UNAVAILABLE` | The source is absent, has not published, or has no value for this metric. The metric has no `MetricValue`; consumers must not convert it to `false`, `0`, or `unknown`. |

Source timestamps are preserved when the upstream message supplies one. A zero
timestamp means that the source does not provide a timestamp; freshness is then
calculated from the Bridge receive time.

## Registered Metrics

All keys below are per logical entity and use the same `SummarySnapshot` rate
unless stated otherwise.

| Key | Value type / unit | Source | Missing and quality semantics |
| --- | --- | --- | --- |
| `com.yundrone.sunray.odom.source` | enum token | `OdomState.external_source` | Unavailable until `OdomState` arrives; stale after the vehicle timeout. |
| `com.yundrone.sunray.odom.valid` | bool | `OdomState.odometry_valid` | The boolean is emitted only when the source is present. Missing is not `false`. |
| `com.yundrone.sunray.odom.rate_hz` | double / `Hz` | `OdomState.odometry_update_hz` | Raw source measurement, not the 5 Hz summary rate. |
| `com.yundrone.sunray.battery.voltage_v` | double / `V` | UAV controller state or UGV power voltage | Unavailable if the selected voltage source is absent or invalid. |
| `com.yundrone.sunray.uav.control.state` | enum token | `UAVControlState.control_state` | UAV only; unavailable or stale with the controller source. |
| `com.yundrone.sunray.uav.control.rc_takeover` | bool | `UAVControlState.manual_override` | UAV only. This is manual takeover, not an inference from control mode or RC channel data. |
| `com.yundrone.sunray.uav.control.mode` | enum token | `UAVControlState.fcu_control_state` | UAV only; a controller mode, not MAVROS/PX4 mode. |
| `com.yundrone.sunray.uav.takeoff.relative_height_m` | double / `m` | `UAVControlState.takeoff_relative_height` | UAV only; unavailable or stale with the controller source. |
| `com.yundrone.sunray.flight_control.armed` | bool | `UAVControlState.fcu_arm_state` | UAV only. `true` and `landed=true` are independently valid. |
| `com.yundrone.sunray.flight_control.landed` | bool | `UAVControlState.fcu_land_state` | UAV only; missing is not an airborne assertion. |
| `com.yundrone.sunray.flight_control.battery_percent` | signed integer / `%` | `UAVControlState.agent_battery_percent` | UAV only; unavailable until controller state arrives, never defaulted to zero. |
| `com.yundrone.sunray.flight_controller.connected` | bool | MAVROS `State.connected` | UAV only. A `false` value is emitted only after a real MAVROS state message; no message is `unavailable`, not disconnected. |
| `com.yundrone.sunray.flight_controller.mode` | enum token | MAVROS `State.mode` | UAV only. Empty or absent mode is `unavailable`; the Bridge never emits a valid `unknown` placeholder. |
| `com.yundrone.sunray.ugv.control.state` | enum token | `UGVControlState.state` | UGV only; unavailable or stale with the UGV controller source. |

The default freshness timeout for vehicle controller, odometry, and MAVROS
state metrics is 1500 ms. UGV power voltage defaults to 3000 ms. Deployments
may configure these timeouts, but changing a timeout does not change the
meaning of the quality values.
