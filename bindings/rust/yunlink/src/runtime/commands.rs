use yunlink_sys as sys;

use super::Runtime;
use crate::error::{ensure, Result};
use crate::types::{
    CommandHandle, GotoCommand, LandCommand, LocalOdom, PeerConnection, ReturnCommand, Session,
    TakeoffCommand, TargetSelector, UavControlCommand, VehicleCoreState, VelocitySetpointCommand,
};

impl Runtime {
    /// Publish a takeoff command through `yunlink_command_publish_takeoff`.
    pub async fn publish_takeoff(
        &self,
        peer: &PeerConnection,
        session: &Session,
        target: &TargetSelector,
        _command: &TakeoffCommand,
    ) -> Result<CommandHandle> {
        let session = session.to_native();
        let payload = sys::yunlink_takeoff_command_t { reserved: 0 };
        let mut handle = sys::yunlink_command_handle_t::default();
        ensure(unsafe {
            sys::yunlink_command_publish_takeoff(
                self.raw_ptr(),
                &peer.raw,
                &session,
                &target.raw,
                &payload,
                &mut handle,
            )
        })?;
        Ok(Self::command_handle_from_native(handle))
    }

    /// Publish a land command through `yunlink_command_publish_land`.
    pub async fn publish_land(
        &self,
        peer: &PeerConnection,
        session: &Session,
        target: &TargetSelector,
        _command: &LandCommand,
    ) -> Result<CommandHandle> {
        let session = session.to_native();
        let payload = sys::yunlink_land_command_t { reserved: 0 };
        let mut handle = sys::yunlink_command_handle_t::default();
        ensure(unsafe {
            sys::yunlink_command_publish_land(
                self.raw_ptr(),
                &peer.raw,
                &session,
                &target.raw,
                &payload,
                &mut handle,
            )
        })?;
        Ok(Self::command_handle_from_native(handle))
    }

    /// Publish a return command through `yunlink_command_publish_return`.
    pub async fn publish_return(
        &self,
        peer: &PeerConnection,
        session: &Session,
        target: &TargetSelector,
        _command: &ReturnCommand,
    ) -> Result<CommandHandle> {
        let session = session.to_native();
        let payload = sys::yunlink_return_command_t { reserved: 0 };
        let mut handle = sys::yunlink_command_handle_t::default();
        ensure(unsafe {
            sys::yunlink_command_publish_return(
                self.raw_ptr(),
                &peer.raw,
                &session,
                &target.raw,
                &payload,
                &mut handle,
            )
        })?;
        Ok(Self::command_handle_from_native(handle))
    }

    /// Publish a goto command through `yunlink_command_publish_goto`.
    pub async fn publish_goto(
        &self,
        peer: &PeerConnection,
        session: &Session,
        target: &TargetSelector,
        command: &GotoCommand,
    ) -> Result<CommandHandle> {
        let session = session.to_native();
        let payload = sys::yunlink_goto_command_t {
            x_m: command.x_m,
            y_m: command.y_m,
            z_m: command.z_m,
            yaw_rad: command.yaw_rad,
        };
        let mut handle = sys::yunlink_command_handle_t::default();
        ensure(unsafe {
            sys::yunlink_command_publish_goto(
                self.raw_ptr(),
                &peer.raw,
                &session,
                &target.raw,
                &payload,
                &mut handle,
            )
        })?;
        Ok(Self::command_handle_from_native(handle))
    }

    /// Publish a velocity setpoint through `yunlink_command_publish_velocity_setpoint`.
    pub async fn publish_velocity_setpoint(
        &self,
        peer: &PeerConnection,
        session: &Session,
        target: &TargetSelector,
        command: &VelocitySetpointCommand,
    ) -> Result<CommandHandle> {
        let session = session.to_native();
        let payload = sys::yunlink_velocity_setpoint_command_t {
            vx_mps: command.vx_mps,
            vy_mps: command.vy_mps,
            vz_mps: command.vz_mps,
            yaw_rate_radps: command.yaw_rate_radps,
            body_frame: if command.body_frame { 1 } else { 0 },
        };
        let mut handle = sys::yunlink_command_handle_t::default();
        ensure(unsafe {
            sys::yunlink_command_publish_velocity_setpoint(
                self.raw_ptr(),
                &peer.raw,
                &session,
                &target.raw,
                &payload,
                &mut handle,
            )
        })?;
        Ok(Self::command_handle_from_native(handle))
    }

    /// Publish the complete UAV control payload used by the current Bridge.
    pub async fn publish_uav_control(
        &self,
        peer: &PeerConnection,
        session: &Session,
        target: &TargetSelector,
        command: &UavControlCommand,
    ) -> Result<CommandHandle> {
        let session = session.to_native();
        let payload = sys::yunlink_uav_control_command_t {
            control_cmd: command.control_cmd,
            desired_position_x_m: command.desired_position[0],
            desired_position_y_m: command.desired_position[1],
            desired_position_z_m: command.desired_position[2],
            desired_velocity_x_mps: command.desired_velocity[0],
            desired_velocity_y_mps: command.desired_velocity[1],
            desired_velocity_z_mps: command.desired_velocity[2],
            desired_acceleration_x_mps2: command.desired_acceleration[0],
            desired_acceleration_y_mps2: command.desired_acceleration[1],
            desired_acceleration_z_mps2: command.desired_acceleration[2],
            desired_body_xy_position_x_m: command.desired_body_xy_position[0],
            desired_body_xy_position_y_m: command.desired_body_xy_position[1],
            desired_body_xy_velocity_x_mps: command.desired_body_xy_velocity[0],
            desired_body_xy_velocity_y_mps: command.desired_body_xy_velocity[1],
            fixed_height_m: command.fixed_height_m,
            yaw_mode: command.yaw_mode,
            desired_yaw_rad: command.desired_yaw_rad,
            desired_yaw_rate_radps: command.desired_yaw_rate_radps,
            controller_type: command.controller_type,
        };
        let mut handle = sys::yunlink_command_handle_t::default();
        ensure(unsafe {
            sys::yunlink_command_publish_uav_control(
                self.raw_ptr(),
                &peer.raw,
                &session,
                &target.raw,
                &payload,
                &mut handle,
            )
        })?;
        Ok(Self::command_handle_from_native(handle))
    }

    /// Publish VehicleCoreState through the state-plane C ABI helper.
    pub async fn publish_vehicle_core_state(
        &self,
        peer: &PeerConnection,
        target: &TargetSelector,
        state: VehicleCoreState,
        session_id: u64,
    ) -> Result<()> {
        let payload = sys::yunlink_vehicle_core_state_t {
            armed: if state.armed { 1 } else { 0 },
            nav_mode: state.nav_mode,
            x_m: state.x_m,
            y_m: state.y_m,
            z_m: state.z_m,
            vx_mps: state.vx_mps,
            vy_mps: state.vy_mps,
            vz_mps: state.vz_mps,
            battery_percent: state.battery_percent,
        };
        ensure(unsafe {
            sys::yunlink_publish_vehicle_core_state(
                self.raw_ptr(),
                &peer.raw,
                &target.raw,
                &payload,
                session_id,
            )
        })
    }

    /// Publish local odometry through the state-plane C ABI helper.
    pub async fn publish_local_odom(
        &self,
        peer: &PeerConnection,
        target: &TargetSelector,
        odom: &LocalOdom,
        session_id: u64,
    ) -> Result<()> {
        let mut payload = sys::yunlink_local_odom_t {
            source_stamp_ns: odom.source_stamp_ns,
            x_m: odom.x_m,
            y_m: odom.y_m,
            z_m: odom.z_m,
            orientation_x: odom.orientation_x,
            orientation_y: odom.orientation_y,
            orientation_z: odom.orientation_z,
            orientation_w: odom.orientation_w,
            vx_mps: odom.vx_mps,
            vy_mps: odom.vy_mps,
            vz_mps: odom.vz_mps,
            angular_x_radps: odom.angular_x_radps,
            angular_y_radps: odom.angular_y_radps,
            angular_z_radps: odom.angular_z_radps,
            ..Default::default()
        };
        crate::ffi_util::write_c_buffer(&mut payload.frame_id, &odom.frame_id);
        crate::ffi_util::write_c_buffer(&mut payload.child_frame_id, &odom.child_frame_id);
        ensure(unsafe {
            sys::yunlink_publish_local_odom(
                self.raw_ptr(),
                &peer.raw,
                &target.raw,
                &payload,
                session_id,
            )
        })
    }
}
