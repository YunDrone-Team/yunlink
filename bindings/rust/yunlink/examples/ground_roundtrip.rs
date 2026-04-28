use std::env;
use std::time::{Duration, Instant};

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
    let started_at = Instant::now();
    let peer = runtime
        .connect(&args[1], parse_u16(&args[2]))
        .await
        .expect("connect peer");
    let connected_at = Instant::now();
    let session = runtime
        .open_session(&peer, "rust-ground")
        .await
        .expect("open session");
    let session_ready_at = Instant::now();
    let target = TargetSelector::entity(AgentType::Uav, 1);

    let authority_started_at = Instant::now();
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
    let command_started_at = Instant::now();

    let mut saw_state = false;
    let mut result_count = 0usize;
    let mut first_state_at: Option<Instant> = None;
    let mut first_result_at: Option<Instant> = None;
    timeout(Duration::from_secs(4), async {
        loop {
            match events.recv().await.expect("receive event") {
                Event::CommandResult(result) if result.session_id == session.session_id => {
                    result_count += 1;
                    if first_result_at.is_none() {
                        first_result_at = Some(Instant::now());
                    }
                }
                Event::VehicleCoreState(state)
                    if state.session_id == session.session_id
                        && (state.battery_percent - 76.5).abs() < f32::EPSILON =>
                {
                    saw_state = true;
                    if first_state_at.is_none() {
                        first_state_at = Some(Instant::now());
                    }
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
    let first_state_at = first_state_at.unwrap_or(session_ready_at);
    let first_result_at = first_result_at.unwrap_or(command_started_at);
    println!(
        "YUNLINK_METRICS {{\"connect_ms\":{:.4},\"session_ready_ms\":{:.4},\"authority_acquire_ms\":{:.4},\"command_result_ms\":{:.4},\"state_first_seen_ms\":{:.4},\"recovery_ms\":0.0}}",
        connected_at.duration_since(started_at).as_secs_f64() * 1000.0,
        session_ready_at.duration_since(connected_at).as_secs_f64() * 1000.0,
        first_state_at.duration_since(authority_started_at).as_secs_f64() * 1000.0,
        first_result_at.duration_since(command_started_at).as_secs_f64() * 1000.0,
        first_state_at.duration_since(authority_started_at).as_secs_f64() * 1000.0,
    );
    println!("rust-ground-roundtrip ok");
}
