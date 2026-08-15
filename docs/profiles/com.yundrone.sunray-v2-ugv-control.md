# Sunray UGV Single-Vehicle Control

`com.yundrone.sunray@2.5` defines the typed payloads used to control one UGV.
The actions target an attached UGV entity and require
`org.yunlink.mobility` Authority. They do not represent swarm control or a
hardware emergency-stop facility.

## Capabilities

- `com.yundrone.sunray.ugv.move-point.v1`
- `com.yundrone.sunray.ugv.velocity-lease.v1`
- `com.yundrone.sunray.ugv.hold.v1`
- `com.yundrone.sunray.ugv.control-state.v1`

New clients use these capabilities as a hard gate and do not use
`org.yunlink.mobility.goto.v1` for UGV control.

## Move Point

`UgvMovePointGoal` preserves the controller's native frame semantics.

- `UGV_MOVE_LOCAL` requires `local_frame_id` to identify the current local
  controller frame.
- `UGV_MOVE_BODY` requires an empty `local_frame_id`; the point is a body-frame
  displacement and must not be presented as a world coordinate.
- `UGV_YAW_KEEP` preserves the current yaw. `UGV_YAW_SET` uses
  `desired_yaw_rad` as an absolute target.
- `point_m.z` is always zero and every numeric field must be finite.

## Velocity Lease

`UgvVelocityGoal` contains exactly one target and a `250..2000 ms` lease.
Repeated goals with the same root correlation refresh that lease instead of
creating another action.

- `local` carries local-frame `x/y` velocity plus absolute desired yaw.
- `body` carries body-frame `x/y` velocity plus yaw rate.

The vehicle adapter may further restrict these targets by drive type. A
differential drive accepts only body-frame forward velocity and yaw rate;
mecanum drive may accept local/body targets and lateral velocity.

When the lease expires or its Session, attachment, or Authority disappears,
the adapter stops refreshing velocity and invokes the UGV Hold service.

## Hold And State

`UgvHoldGoal` requests parking hold. It first cancels the caller's active UGV
move/velocity action for the entity and is idempotent when the controller is
already in `HOLD`.

`UgvControlState.odom_ready` reports whether the UGV controller currently has
usable odometry. Its `source_stamp_ns` remains a nanosecond source timestamp;
drive type, FSM state, and diagnostic detail are reported from the controller
without fabricated fallback values.
