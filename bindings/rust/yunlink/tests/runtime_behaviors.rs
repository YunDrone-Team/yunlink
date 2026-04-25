use std::time::Duration;

use tokio::time::{sleep, timeout};

use yunlink::{
    AgentType, ControlSource, Event, FfiErrorCode, Runtime, RuntimeConfig, TargetSelector,
    VehicleCoreState, YunlinkError, EVENT_CHANNEL_CAPACITY,
};

#[tokio::test(flavor = "multi_thread", worker_threads = 2)]
async fn ffi_errors_map_to_typed_codes() {
    let runtime = Runtime::start(RuntimeConfig::new(
        14070,
        14070,
        14170,
        AgentType::GroundStation,
        7,
    ))
    .unwrap();

    let err = runtime.connect("256.0.0.1", 14171).await.unwrap_err();
    assert!(matches!(
        err,
        YunlinkError::Ffi(FfiErrorCode::InvalidArgument)
    ));
}

#[tokio::test(flavor = "multi_thread", worker_threads = 2)]
async fn cancelling_receiver_task_does_not_break_runtime() {
    let air = Runtime::start(RuntimeConfig::new(14071, 14071, 14171, AgentType::Uav, 1)).unwrap();
    let ground = Runtime::start(RuntimeConfig::new(
        14072,
        14072,
        14172,
        AgentType::GroundStation,
        7,
    ))
    .unwrap();

    let mut cancelled_rx = ground.subscribe();
    let cancelled_task = tokio::spawn(async move { cancelled_rx.recv().await });
    cancelled_task.abort();
    assert!(cancelled_task.await.unwrap_err().is_cancelled());

    let mut live_rx = ground.subscribe();
    let peer = ground.connect("127.0.0.1", 14171).await.unwrap();
    let session = ground
        .open_session(&peer, "rust-cancelled-rx")
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

    let lease = timeout(Duration::from_secs(3), async {
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

    air.publish_vehicle_core_state(
        &lease.peer,
        &TargetSelector::entity(AgentType::GroundStation, 7),
        VehicleCoreState {
            armed: true,
            nav_mode: 3,
            x_m: 1.0,
            y_m: 2.0,
            z_m: 3.0,
            vx_mps: 0.1,
            vy_mps: 0.2,
            vz_mps: 0.3,
            battery_percent: 77.0,
        },
        session.session_id,
    )
    .await
    .unwrap();

    timeout(Duration::from_secs(3), async {
        loop {
            match live_rx.recv().await.unwrap() {
                Event::VehicleCoreState(state)
                    if state.session_id == session.session_id
                        && (state.battery_percent - 77.0).abs() < f32::EPSILON =>
                {
                    break;
                }
                _ => {}
            }
        }
    })
    .await
    .unwrap();
}

#[tokio::test(flavor = "multi_thread", worker_threads = 2)]
async fn slow_receivers_observe_lagged_backpressure() {
    let air = Runtime::start(RuntimeConfig::new(14073, 14073, 14173, AgentType::Uav, 1)).unwrap();
    let ground = Runtime::start(RuntimeConfig::new(
        14074,
        14074,
        14174,
        AgentType::GroundStation,
        7,
    ))
    .unwrap();

    let mut rx = ground.subscribe();
    let peer = ground.connect("127.0.0.1", 14173).await.unwrap();
    let session = ground
        .open_session(&peer, "rust-backpressure")
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

    let lease = timeout(Duration::from_secs(3), async {
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

    let burst = EVENT_CHANNEL_CAPACITY + 16;
    for index in 0..burst {
        air.publish_vehicle_core_state(
            &lease.peer,
            &TargetSelector::entity(AgentType::GroundStation, 7),
            VehicleCoreState {
                armed: true,
                nav_mode: 3,
                x_m: index as f32,
                y_m: 1.0,
                z_m: 3.0,
                vx_mps: 0.1,
                vy_mps: 0.2,
                vz_mps: 0.3,
                battery_percent: index as f32,
            },
            session.session_id,
        )
        .await
        .unwrap();
    }

    sleep(Duration::from_millis(100)).await;

    let lagged = timeout(Duration::from_secs(4), async {
        loop {
            match rx.recv().await {
                Err(tokio::sync::broadcast::error::RecvError::Lagged(skipped)) => {
                    return skipped > 0;
                }
                Err(tokio::sync::broadcast::error::RecvError::Closed) => return false,
                Ok(Event::VehicleCoreState(_)) => {}
                Ok(_) => {}
            }
        }
    })
    .await
    .unwrap();

    assert!(
        lagged,
        "expected lagged receiver after burst beyond channel capacity {EVENT_CHANNEL_CAPACITY}"
    );
}
