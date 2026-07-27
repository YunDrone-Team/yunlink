use prost::Message;
use yunlink_profiles::{
    mobility, sunray, validate_emergency_kill_goal, validate_uav_direct_control_goal,
    validate_uav_waypoint_mission_goal,
};

const DIRECT_CONTROL_GOLDEN: &[u8] = &[
    0x1a, 0x5c, 0x0a, 0x03, 0x6d, 0x61, 0x70, 0x12, 0x1b, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xf0, 0x3f, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x19, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xe0, 0x3f, 0x1a, 0x1b, 0x09, 0x9a, 0x99, 0x99, 0x99, 0x99, 0x99, 0xb9, 0x3f, 0x11,
    0x9a, 0x99, 0x99, 0x99, 0x99, 0x99, 0xc9, 0x3f, 0x19, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0xd3,
    0xbf, 0x22, 0x1b, 0x09, 0x7b, 0x14, 0xae, 0x47, 0xe1, 0x7a, 0x84, 0x3f, 0x11, 0x7b, 0x14, 0xae,
    0x47, 0xe1, 0x7a, 0x94, 0x3f, 0x19, 0xb8, 0x1e, 0x85, 0xeb, 0x51, 0xb8, 0x9e, 0x3f, 0x32, 0x0b,
    0x08, 0x01, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x3f, 0x38, 0x01, 0x40, 0xee, 0x05,
];

fn vector3(x: f64, y: f64, z: f64) -> mobility::Vector3 {
    mobility::Vector3 { x, y, z }
}

fn base_goal(
    target: sunray::uav_direct_control_goal::Target,
    lease_ms: u32,
) -> sunray::UavDirectControlGoal {
    sunray::UavDirectControlGoal {
        target: Some(target),
        yaw: Some(sunray::YawTarget {
            mode: 0,
            value: 0.0,
        }),
        controller: 0,
        lease_ms,
    }
}

#[test]
fn emergency_kill_requires_confirmation_and_matches_golden_vector() {
    let mut goal = sunray::EmergencyKillGoal { confirmed: false };
    assert_eq!(
        validate_emergency_kill_goal(&goal),
        Err("emergency kill requires explicit confirmation")
    );
    goal.confirmed = true;
    validate_emergency_kill_goal(&goal).unwrap();
    assert_eq!(goal.encode_to_vec(), [0x08, 0x01]);
    assert_eq!(
        sunray::EmergencyKillGoal::decode(goal.encode_to_vec().as_slice()).unwrap(),
        goal
    );
}

#[test]
fn all_direct_control_variants_round_trip_and_validate() {
    use sunray::uav_direct_control_goal::Target;
    let goals = [
        base_goal(
            Target::WorldPosition(sunray::WorldPositionTarget {
                frame_id: "map".into(),
                position_m: Some(vector3(0.0, 0.0, 1.0)),
            }),
            0,
        ),
        base_goal(
            Target::BodyPosition(sunray::BodyPositionTarget {
                body_xy_position_m: Some(mobility::Vector2 { x: 1.0, y: 0.0 }),
                fixed_height_m: 2.0,
            }),
            0,
        ),
        base_goal(
            Target::TrajectorySetpoint(sunray::TrajectorySetpointTarget {
                frame_id: "map".into(),
                position_m: Some(vector3(0.0, 0.0, 1.0)),
                velocity_mps: Some(vector3(0.1, 0.0, 0.0)),
                acceleration_mps2: Some(vector3(0.0, 0.1, 0.0)),
            }),
            750,
        ),
        base_goal(
            Target::WorldVelocity(sunray::WorldVelocityTarget {
                frame_id: "map".into(),
                velocity_mps: Some(vector3(0.5, 0.0, 0.0)),
                height_lock: None,
            }),
            250,
        ),
        base_goal(
            Target::BodyVelocity(sunray::BodyVelocityTarget {
                body_xy_velocity_mps: Some(mobility::Vector2 { x: 0.0, y: 0.5 }),
                fixed_height_m: 2.0,
            }),
            2000,
        ),
    ];
    for goal in goals {
        validate_uav_direct_control_goal(&goal).unwrap();
        let bytes = goal.encode_to_vec();
        let decoded = sunray::UavDirectControlGoal::decode(bytes.as_slice()).unwrap();
        assert_eq!(decoded.target, goal.target);
    }
}

#[test]
fn direct_control_golden_and_validation_failures() {
    use sunray::uav_direct_control_goal::Target;
    let mut goal = base_goal(
        Target::TrajectorySetpoint(sunray::TrajectorySetpointTarget {
            frame_id: "map".into(),
            position_m: Some(vector3(1.0, -2.0, 0.5)),
            velocity_mps: Some(vector3(0.1, 0.2, -0.3)),
            acceleration_mps2: Some(vector3(0.01, 0.02, 0.03)),
        }),
        750,
    );
    goal.yaw = Some(sunray::YawTarget {
        mode: 1,
        value: 0.25,
    });
    goal.controller = 1;
    assert_eq!(goal.encode_to_vec(), DIRECT_CONTROL_GOLDEN);

    goal.lease_ms = 249;
    assert!(validate_uav_direct_control_goal(&goal).is_err());
    goal.lease_ms = 750;
    if let Some(Target::TrajectorySetpoint(target)) = goal.target.as_mut() {
        target.acceleration_mps2.as_mut().unwrap().z = f64::NAN;
    }
    assert!(validate_uav_direct_control_goal(&goal).is_err());
}

#[test]
fn waypoint_limits_and_round_trip() {
    let mut mission = sunray::UavWaypointMissionGoal {
        frame_id: "map".into(),
        waypoints: vec![sunray::UavWaypoint {
            position_m: Some(vector3(1.0, 2.0, 3.0)),
            yaw_rad: 0.5,
            hold_time_s: 1.5,
        }],
        interrupt_current_task: true,
    };
    validate_uav_waypoint_mission_goal(&mission).unwrap();
    let bytes = mission.encode_to_vec();
    assert_eq!(
        sunray::UavWaypointMissionGoal::decode(bytes.as_slice()).unwrap(),
        mission
    );
    mission.waypoints.clear();
    assert!(validate_uav_waypoint_mission_goal(&mission).is_err());
    mission.waypoints.resize(
        257,
        sunray::UavWaypoint {
            position_m: Some(vector3(0.0, 0.0, 0.0)),
            yaw_rad: 0.0,
            hold_time_s: 0.0,
        },
    );
    assert!(validate_uav_waypoint_mission_goal(&mission).is_err());
}
