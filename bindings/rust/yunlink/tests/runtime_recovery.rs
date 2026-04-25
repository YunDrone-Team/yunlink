use std::time::Duration;

use tokio::time::{sleep, timeout};

use yunlink::{AgentType, AuthorityState, ControlSource, Runtime, RuntimeConfig, TargetSelector};

#[tokio::test(flavor = "multi_thread", worker_threads = 2)]
async fn explicit_reconnect_session_reopen_and_authority_reacquire() {
    let air = Runtime::start(RuntimeConfig::new(14050, 14050, 14150, AgentType::Uav, 1)).unwrap();
    let target = TargetSelector::entity(AgentType::Uav, 1);

    let session_one_id = {
        let ground = Runtime::start(RuntimeConfig::new(
            14051,
            14051,
            14151,
            AgentType::GroundStation,
            7,
        ))
        .unwrap();
        let peer = ground.connect("127.0.0.1", 14150).await.unwrap();
        let session = ground.open_session(&peer, "rust-ground-1").await.unwrap();
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
                        assert_eq!(lease.state, AuthorityState::Controller);
                        break;
                    }
                }
                sleep(Duration::from_millis(20)).await;
            }
        })
        .await
        .unwrap();

        ground
            .release_authority(&peer, &session, &target)
            .await
            .unwrap();
        timeout(Duration::from_secs(3), async {
            loop {
                if air.current_authority().unwrap().is_none() {
                    break;
                }
                sleep(Duration::from_millis(20)).await;
            }
        })
        .await
        .unwrap();
        session.session_id
    };

    let ground = Runtime::start(RuntimeConfig::new(
        14052,
        14052,
        14152,
        AgentType::GroundStation,
        7,
    ))
    .unwrap();
    let peer = ground.connect("127.0.0.1", 14150).await.unwrap();
    let session = ground.open_session(&peer, "rust-ground-2").await.unwrap();
    assert!(session.session_id > 0);
    assert!(session_one_id > 0);

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
                    assert_eq!(lease.state, AuthorityState::Controller);
                    break;
                }
            }
            sleep(Duration::from_millis(20)).await;
        }
    })
    .await
    .unwrap();
}
