use std::env;
use std::time::Duration;

use tokio::time::{sleep, timeout};

use yunlink::{
    AgentType, ControlSource, Event, GotoCommand, Runtime, RuntimeConfig, TargetSelector,
};

fn parse_u16(value: &str) -> u16 {
    value.parse::<u16>().expect("invalid u16")
}

#[tokio::main(flavor = "multi_thread", worker_threads = 2)]
async fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() != 6 {
        eprintln!(
            "usage: {} <ip> <port> <udp-bind> <udp-target> <tcp-listen>",
            args[0]
        );
        std::process::exit(2);
    }

    let runtime = Runtime::start(RuntimeConfig::new(
        parse_u16(&args[3]),
        parse_u16(&args[4]),
        parse_u16(&args[5]),
        AgentType::GroundStation,
        7,
    ))
    .expect("start runtime");

    let mut events = runtime.subscribe();
    let peer = runtime
        .connect(&args[1], parse_u16(&args[2]))
        .await
        .expect("connect peer");
    let session = runtime
        .open_session(&peer, "rust-ground")
        .await
        .expect("open session");
    let target = TargetSelector::entity(AgentType::Uav, 1);

    runtime
        .request_authority(
            &peer,
            &session,
            &target,
            ControlSource::GroundStation,
            3000,
            false,
        )
        .await
        .expect("request authority");
    runtime
        .renew_authority(
            &peer,
            &session,
            &target,
            ControlSource::GroundStation,
            4500,
        )
        .await
        .expect("renew authority");
    runtime
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
        .expect("publish goto");

    let mut saw_state = false;
    let mut result_count = 0usize;
    timeout(Duration::from_secs(4), async {
        loop {
            match events.recv().await.expect("receive event") {
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
    .expect("roundtrip timeout");

    runtime
        .release_authority(&peer, &session, &target)
        .await
        .expect("release authority");
    sleep(Duration::from_millis(100)).await;
    println!("rust-ground-roundtrip ok");
}
