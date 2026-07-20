use std::net::{TcpListener, UdpSocket};
use std::time::Duration;

use tokio::time::{sleep, timeout};
use yunlink::{
    AgentType, ControlSource, Event, LandCommand, ReturnCommand, Runtime, RuntimeConfig,
    TakeoffCommand, TargetSelector, VelocitySetpointCommand,
};

fn free_udp_port() -> u16 {
    UdpSocket::bind(("127.0.0.1", 0))
        .expect("reserve free UDP port")
        .local_addr()
        .expect("read UDP local addr")
        .port()
}

fn free_tcp_port() -> u16 {
    TcpListener::bind(("127.0.0.1", 0))
        .expect("reserve free TCP port")
        .local_addr()
        .expect("read TCP local addr")
        .port()
}

fn start_runtime_with_retry(agent_type: AgentType, agent_id: u32) -> (Runtime, u16) {
    let mut last_error = None;
    for _ in 0..8 {
        let udp_port = free_udp_port();
        let tcp_port = free_tcp_port();
        let config = RuntimeConfig {
            udp_bind_port: udp_port,
            udp_target_port: udp_port,
            tcp_listen_port: tcp_port,
            agent_type,
            agent_id,
            shared_secret: "yunlink-default-secret".into(),
            multicast_group: "224.1.1.1".into(),
            capability_flags: 0,
            required_peer_capability_flags: 0,
            managed_identities: Vec::new(),
        };
        match Runtime::start(config) {
            Ok(runtime) => return (runtime, tcp_port),
            Err(error) => last_error = Some(error),
        }
    }
    panic!("failed to start runtime with free ports: {last_error:?}");
}

#[test]
fn advanced_monitor_config_matches_cpp_defaults() {
    let config = RuntimeConfig::advanced_monitor_ground(1);

    assert_eq!(config.udp_bind_port, 9797);
    assert_eq!(config.udp_target_port, 9898);
    assert_eq!(config.tcp_listen_port, 9797);
    assert_eq!(config.agent_type, AgentType::GroundStation);
    assert_eq!(config.agent_id, 1001);
    assert_eq!(config.shared_secret, "yunlink-default-secret");
    assert_eq!(config.multicast_group, "224.1.1.1");
}

#[tokio::test(flavor = "multi_thread", worker_threads = 2)]
async fn advanced_monitor_command_publishers_emit_results() {
    let (air, air_tcp_port) = start_runtime_with_retry(AgentType::Uav, 1);
    let (ground, _) = start_runtime_with_retry(AgentType::GroundStation, 1001);

    let peer = ground.connect("127.0.0.1", air_tcp_port).await.unwrap();
    let session = ground
        .open_session(&peer, "yunlink_advanced_monitor")
        .await
        .unwrap();
    let target = TargetSelector::entity(AgentType::Uav, 1);

    ground
        .request_authority(
            &peer,
            &session,
            &target,
            ControlSource::GroundStation,
            3000,
            false,
        )
        .await
        .unwrap();

    timeout(Duration::from_secs(3), async {
        loop {
            if let Some(lease) = air.current_authority().unwrap() {
                if lease.session_id == session.session_id {
                    break;
                }
            }
            sleep(Duration::from_millis(20)).await;
        }
    })
    .await
    .unwrap();

    let mut events = ground.subscribe();
    let handles = [
        ground
            .publish_takeoff(&peer, &session, &target, &TakeoffCommand)
            .await
            .unwrap(),
        ground
            .publish_land(&peer, &session, &target, &LandCommand)
            .await
            .unwrap(),
        ground
            .publish_return(&peer, &session, &target, &ReturnCommand)
            .await
            .unwrap(),
        ground
            .publish_velocity_setpoint(
                &peer,
                &session,
                &target,
                &VelocitySetpointCommand {
                    vx_mps: 0.1,
                    vy_mps: 0.2,
                    vz_mps: -0.1,
                    yaw_rate_radps: 0.05,
                    body_frame: true,
                },
            )
            .await
            .unwrap(),
    ];

    timeout(Duration::from_secs(3), async {
        let mut seen = 0usize;
        loop {
            if let Event::CommandResult(result) = events.recv().await.unwrap() {
                if handles
                    .iter()
                    .any(|handle| handle.correlation_id == result.correlation_id)
                {
                    seen += 1;
                }
            }
            if seen >= handles.len() {
                break;
            }
        }
    })
    .await
    .unwrap();
}
