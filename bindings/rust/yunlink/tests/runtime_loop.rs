use std::time::Duration;

use tokio::time::{sleep, timeout};

use yunlink::{
    AgentType, ControlSource, Event, GotoCommand, Runtime, RuntimeConfig, TargetSelector,
    VehicleCoreState,
};

#[tokio::test(flavor = "multi_thread", worker_threads = 2)]
async fn runtime_loop_roundtrip() {
    let air = Runtime::start(RuntimeConfig::new(14030, 14030, 14130, AgentType::Uav, 1)).unwrap();
    let ground = Runtime::start(RuntimeConfig::new(
        14031,
        14031,
        14131,
        AgentType::GroundStation,
        7,
    ))
    .unwrap();

    let mut events = ground.subscribe();
    let peer = ground.connect("127.0.0.1", 14130).await.unwrap();
    let session = ground.open_session(&peer, "rust-ground").await.unwrap();
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

    let command = ground
        .publish_goto(
            &peer,
            &session,
            &target,
            &GotoCommand {
                x_m: 5.0,
                y_m: 1.0,
                z_m: 3.0,
                yaw_rad: 0.25,
            },
        )
        .await
        .unwrap();
    assert_eq!(command.session_id, session.session_id);

    let authority = timeout(Duration::from_secs(3), async {
        loop {
            if let Some(lease) = air.current_authority().unwrap() {
                if lease.session_id == session.session_id {
                    return lease;
                }
            }
            sleep(Duration::from_millis(20)).await;
        }
    })
    .await
    .unwrap();
    let air_peer = authority.peer.clone();

    ground
        .renew_authority(&peer, &session, &target, ControlSource::GroundStation, 4500)
        .await
        .unwrap();

    air.publish_vehicle_core_state(
        &air_peer,
        &TargetSelector::entity(AgentType::GroundStation, 7),
        VehicleCoreState {
            armed: true,
            nav_mode: 3,
            x_m: 11.0,
            y_m: 12.0,
            z_m: 13.0,
            vx_mps: 0.1,
            vy_mps: 0.2,
            vz_mps: 0.3,
            battery_percent: 76.5,
        },
        session.session_id,
    )
    .await
    .unwrap();

    let mut saw_state = false;
    let mut result_count = 0usize;
    timeout(Duration::from_secs(3), async {
        loop {
            match events.recv().await.unwrap() {
                Event::CommandResult(result) if result.session_id == session.session_id => {
                    result_count += 1;
                }
                Event::VehicleCoreState(state)
                    if state.session_id == session.session_id
                        && (state.battery_percent - 76.5).abs() < f32::EPSILON =>
                {
                    saw_state = true;
                }
                _ => {}
            }
            if saw_state && result_count >= 4 {
                return;
            }
        }
    })
    .await
    .unwrap();

    ground
        .release_authority(&peer, &session, &target)
        .await
        .unwrap();
}
