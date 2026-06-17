use std::sync::mpsc;

use yunlink::{
    ControlSource, GotoCommand, LandCommand, PeerConnection, ReturnCommand, Runtime, Session,
    TakeoffCommand, TargetSelector, VelocitySetpointCommand,
};

use super::events::{send_authority, send_command_sent, send_error};
use super::RuntimeUpdate;

pub(super) async fn request_authority(
    runtime: &Runtime,
    peer: Option<&PeerConnection>,
    session: Option<&Session>,
    target: &TargetSelector,
    ttl_ms: u32,
    update_tx: &mpsc::Sender<RuntimeUpdate>,
) {
    if let (Some(peer), Some(session)) = (peer, session) {
        if let Err(err) = runtime
            .request_authority(
                peer,
                session,
                target,
                ControlSource::GroundStation,
                ttl_ms,
                false,
            )
            .await
        {
            send_error(update_tx, err);
        }
    } else {
        let _ = update_tx.send(RuntimeUpdate::Error(
            "connect and open a session before this action".to_string(),
        ));
    }
    send_authority(runtime, update_tx);
}

pub(super) async fn release_authority(
    runtime: &Runtime,
    peer: Option<&PeerConnection>,
    session: Option<&Session>,
    target: &TargetSelector,
    update_tx: &mpsc::Sender<RuntimeUpdate>,
) {
    if let (Some(peer), Some(session)) = (peer, session) {
        if let Err(err) = runtime.release_authority(peer, session, target).await {
            send_error(update_tx, err);
        }
    } else {
        let _ = update_tx.send(RuntimeUpdate::Error(
            "connect and open a session before this action".to_string(),
        ));
    }
    send_authority(runtime, update_tx);
}

pub(super) async fn publish_takeoff(
    runtime: &Runtime,
    peer: Option<&PeerConnection>,
    session: Option<&Session>,
    target: &TargetSelector,
    height_m: f32,
    max_velocity_mps: f32,
    update_tx: &mpsc::Sender<RuntimeUpdate>,
) {
    let Some((peer, session)) = peer.zip(session) else {
        return;
    };
    match runtime
        .publish_takeoff(
            peer,
            session,
            target,
            &TakeoffCommand {
                relative_height_m: height_m,
                max_velocity_mps,
            },
        )
        .await
    {
        Ok(handle) => send_command_sent(
            update_tx,
            "TAKEOFF",
            format!("height={height_m:.1} max_v={max_velocity_mps:.1}"),
            handle,
        ),
        Err(err) => send_error(update_tx, err),
    }
}

pub(super) async fn publish_land(
    runtime: &Runtime,
    peer: Option<&PeerConnection>,
    session: Option<&Session>,
    target: &TargetSelector,
    max_velocity_mps: f32,
    update_tx: &mpsc::Sender<RuntimeUpdate>,
) {
    let Some((peer, session)) = peer.zip(session) else {
        return;
    };
    match runtime
        .publish_land(peer, session, target, &LandCommand { max_velocity_mps })
        .await
    {
        Ok(handle) => send_command_sent(
            update_tx,
            "LAND",
            format!("max_v={max_velocity_mps:.1}"),
            handle,
        ),
        Err(err) => send_error(update_tx, err),
    }
}

pub(super) async fn publish_return(
    runtime: &Runtime,
    peer: Option<&PeerConnection>,
    session: Option<&Session>,
    target: &TargetSelector,
    loiter_s: f32,
    update_tx: &mpsc::Sender<RuntimeUpdate>,
) {
    let Some((peer, session)) = peer.zip(session) else {
        return;
    };
    match runtime
        .publish_return(
            peer,
            session,
            target,
            &ReturnCommand {
                loiter_before_return_s: loiter_s,
            },
        )
        .await
    {
        Ok(handle) => {
            send_command_sent(update_tx, "RETURN", format!("loiter={loiter_s:.1}"), handle)
        }
        Err(err) => send_error(update_tx, err),
    }
}

pub(super) async fn publish_goto(
    runtime: &Runtime,
    peer: Option<&PeerConnection>,
    session: Option<&Session>,
    target: &TargetSelector,
    command: GotoCommand,
    update_tx: &mpsc::Sender<RuntimeUpdate>,
) {
    let Some((peer, session)) = peer.zip(session) else {
        return;
    };
    match runtime.publish_goto(peer, session, target, &command).await {
        Ok(handle) => send_command_sent(
            update_tx,
            "GOTO",
            format!(
                "x={:.1} y={:.1} z={:.1} yaw={:.2}",
                command.x_m, command.y_m, command.z_m, command.yaw_rad
            ),
            handle,
        ),
        Err(err) => send_error(update_tx, err),
    }
}

pub(super) async fn publish_velocity(
    runtime: &Runtime,
    peer: Option<&PeerConnection>,
    session: Option<&Session>,
    target: &TargetSelector,
    command: VelocitySetpointCommand,
    update_tx: &mpsc::Sender<RuntimeUpdate>,
) {
    let Some((peer, session)) = peer.zip(session) else {
        return;
    };
    match runtime
        .publish_velocity_setpoint(peer, session, target, &command)
        .await
    {
        Ok(handle) => send_command_sent(
            update_tx,
            "VELOCITY",
            format!(
                "vx={:.1} vy={:.1} vz={:.1}",
                command.vx_mps, command.vy_mps, command.vz_mps
            ),
            handle,
        ),
        Err(err) => send_error(update_tx, err),
    }
}
