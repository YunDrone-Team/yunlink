# Acceleration compensation (Sunray 2.20)

`UavAccelerationCompensationGoal@2.20` carries one `acceleration_ned_mps2`
Vector3 in world NED axes, m/s², without gravity. Zero means no compensation.
It adds acceleration feedforward to the existing position/velocity reference;
it does not select a controller or start a flight/motion task.

Use Entity-targeted Action Goal to open, Action Update with the same correlation
ID for each new sample, and Cancel to clear. Each sample is consumed once.
Receivers must not refresh old samples. The action expires after 300 ms without
a new sample; loss of attachment, authority, or session clears compensation.
Removal must not issue Hover or cancel the vehicle's original motion task.

Requires Sunray authority and a session supporting Sunray minor >= 20. Existing
types keep their original TypeRef minors. Only SunrayPosController consumes the
compensation. The controller applies its configured magnitude limit (default
0.5 m/s²) and input timeout (default 0.3 s). Send new samples at 100 Hz for
closed-loop algorithms; network transport is not a real-time scheduler.

The ROS adapter stamps samples using onboard ROS time and converts NED to ENU
exactly once at the existing controller input. Clients need no ROS clock sync.
No Core, Wire, Session, or Authority changes are required.
