use std::env;
use std::time::Duration;

use tokio::time::{sleep, timeout};

use yunlink::{AgentType, Runtime, RuntimeConfig, TargetSelector, VehicleCoreState};

fn parse_u16(value: &str) -> u16 {
    value.parse::<u16>().expect("invalid u16")
}

#[tokio::main(flavor = "multi_thread", worker_threads = 2)]
async fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() != 4 {
        eprintln!("usage: {} <udp-bind> <udp-target> <tcp-listen>", args[0]);
        std::process::exit(2);
    }

    let runtime = Runtime::start(RuntimeConfig::new(
        parse_u16(&args[1]),
        parse_u16(&args[2]),
        parse_u16(&args[3]),
        AgentType::Uav,
        1,
    ))
    .expect("start runtime");

    let lease = timeout(Duration::from_secs(4), async {
        loop {
            if let Some(lease) = runtime.current_authority().expect("current authority") {
                return lease;
            }
            sleep(Duration::from_millis(20)).await;
        }
    })
    .await
    .expect("authority timeout");

    runtime
        .publish_vehicle_core_state(
            &lease.peer,
            &TargetSelector::broadcast(AgentType::GroundStation),
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
            lease.session_id,
        )
        .await
        .expect("publish vehicle core state");

    sleep(Duration::from_millis(300)).await;
    println!("rust-air-roundtrip ok");
}
