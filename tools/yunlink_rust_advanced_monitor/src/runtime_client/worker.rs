use std::sync::mpsc;
use std::time::Duration;

use yunlink::{
    AgentType, GotoCommand, Runtime, RuntimeConfig, Session, TargetSelector,
    VelocitySetpointCommand,
};

use super::actions;
use super::events::{drain_events, send_error};
use super::{RuntimeCommand, RuntimeUpdate};
use crate::model::MonitorConfig;

pub(super) async fn run(
    config: MonitorConfig,
    command_rx: mpsc::Receiver<RuntimeCommand>,
    update_tx: mpsc::Sender<RuntimeUpdate>,
) {
    // Build the safe Rust configuration first. The safe SDK turns this into
    // `yunlink_runtime_config_t`, including fixed C string buffers.
    let runtime_config = RuntimeConfig::new(
        config.udp_bind_port,
        config.udp_target_port,
        config.tcp_listen_port,
        config.agent_type(),
        config.agent_id,
    )
    .with_shared_secret(config.shared_secret.clone());
    let runtime = match Runtime::start(runtime_config) {
        Ok(runtime) => runtime,
        Err(err) => {
            let _ = update_tx.send(RuntimeUpdate::Error(format!("start failed: {err}")));
            return;
        }
    };
    let _ = update_tx.send(RuntimeUpdate::Started);

    let mut events = runtime.subscribe();
    let mut peer: Option<yunlink::PeerConnection> = None;
    let mut session: Option<Session> = None;
    let target = TargetSelector::entity(AgentType::Uav, 1);

    loop {
        drain_events(&mut events, &update_tx).await;
        match command_rx.recv_timeout(Duration::from_millis(25)) {
            Ok(RuntimeCommand::Connect) => {
                connect_peer(&runtime, &config, &mut peer, &mut session, &update_tx).await;
            }
            Ok(RuntimeCommand::RequestAuthority) => {
                actions::request_authority(
                    &runtime,
                    peer.as_ref(),
                    session.as_ref(),
                    &target,
                    config.authority_ttl_ms,
                    &update_tx,
                )
                .await;
            }
            Ok(RuntimeCommand::ReleaseAuthority) => {
                actions::release_authority(
                    &runtime,
                    peer.as_ref(),
                    session.as_ref(),
                    &target,
                    &update_tx,
                )
                .await;
            }
            Ok(RuntimeCommand::Takeoff) => {
                actions::publish_takeoff(
                    &runtime,
                    peer.as_ref(),
                    session.as_ref(),
                    &target,
                    &update_tx,
                )
                .await;
            }
            Ok(RuntimeCommand::Land) => {
                actions::publish_land(
                    &runtime,
                    peer.as_ref(),
                    session.as_ref(),
                    &target,
                    &update_tx,
                )
                .await;
            }
            Ok(RuntimeCommand::Return) => {
                actions::publish_return(
                    &runtime,
                    peer.as_ref(),
                    session.as_ref(),
                    &target,
                    &update_tx,
                )
                .await;
            }
            Ok(RuntimeCommand::Goto {
                x_m,
                y_m,
                z_m,
                yaw_rad,
            }) => {
                actions::publish_goto(
                    &runtime,
                    peer.as_ref(),
                    session.as_ref(),
                    &target,
                    GotoCommand {
                        x_m,
                        y_m,
                        z_m,
                        yaw_rad,
                    },
                    &update_tx,
                )
                .await;
            }
            Ok(RuntimeCommand::Velocity {
                vx_mps,
                vy_mps,
                vz_mps,
                yaw_rate_radps,
                body_frame,
            }) => {
                actions::publish_velocity(
                    &runtime,
                    peer.as_ref(),
                    session.as_ref(),
                    &target,
                    VelocitySetpointCommand {
                        vx_mps,
                        vy_mps,
                        vz_mps,
                        yaw_rate_radps,
                        body_frame,
                    },
                    &update_tx,
                )
                .await;
            }
            Ok(RuntimeCommand::Shutdown) => break,
            Err(mpsc::RecvTimeoutError::Timeout) => {}
            Err(mpsc::RecvTimeoutError::Disconnected) => break,
        }
    }
}

async fn connect_peer(
    runtime: &Runtime,
    config: &MonitorConfig,
    peer: &mut Option<yunlink::PeerConnection>,
    session: &mut Option<Session>,
    update_tx: &mpsc::Sender<RuntimeUpdate>,
) {
    match runtime
        .connect(&config.remote_ip, config.remote_tcp_port)
        .await
    {
        Ok(new_peer) => match runtime.open_session(&new_peer, &config.node_name).await {
            Ok(new_session) => {
                let _ = update_tx.send(RuntimeUpdate::Connected {
                    peer_id: new_peer.id.clone(),
                    session_id: new_session.session_id,
                });
                *peer = Some(new_peer);
                *session = Some(new_session);
            }
            Err(err) => {
                let _ = update_tx.send(RuntimeUpdate::Error(format!("open_session failed: {err}")));
            }
        },
        Err(err) => send_error(update_tx, err),
    }
}
