use std::time::Duration;

use tokio::time::timeout;

use yunlink::{AgentType, Event, Runtime, RuntimeConfig};

#[tokio::test(flavor = "multi_thread", worker_threads = 2)]
async fn drop_closes_event_channel() {
    let receiver = {
        let runtime = Runtime::start(RuntimeConfig::new(
            14060,
            14060,
            14160,
            AgentType::GroundStation,
            7,
        ))
        .unwrap();
        runtime.subscribe()
    };

    match timeout(Duration::from_secs(1), receiver.resubscribe().recv()).await {
        Ok(Err(tokio::sync::broadcast::error::RecvError::Closed)) => {}
        other => panic!("expected closed receiver after runtime drop, got {other:?}"),
    }
}

#[tokio::test(flavor = "multi_thread", worker_threads = 2)]
async fn drop_releases_ports_for_restart() {
    {
        let runtime = Runtime::start(RuntimeConfig::new(
            14061,
            14061,
            14161,
            AgentType::GroundStation,
            7,
        ))
        .unwrap();
        let mut rx = runtime.subscribe();
        assert!(matches!(
            timeout(Duration::from_millis(50), rx.recv()).await,
            Err(_)
                | Ok(Err(_))
                | Ok(Ok(Event::Link(_)))
                | Ok(Ok(Event::Error(_)))
                | Ok(Ok(Event::CommandResult(_)))
                | Ok(Ok(Event::VehicleCoreState(_)))
        ));
    }

    Runtime::start(RuntimeConfig::new(
        14061,
        14061,
        14161,
        AgentType::GroundStation,
        7,
    ))
    .unwrap();
}
