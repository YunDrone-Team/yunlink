use prost::Message;
use yunlink_profiles::{
    mobility, sunray, validate_emergency_kill_goal, validate_flight_control_state,
    validate_gimbal_angle_goal, validate_gimbal_rate_goal, validate_gimbal_zoom_absolute_goal,
    validate_land_goal, validate_planner_set_home_request, validate_takeoff_goal,
    validate_uav_direct_control_goal, validate_uav_waypoint_mission_goal,
    validate_ugv_move_point_goal, validate_ugv_velocity_goal,
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
fn flight_control_state_round_trips_and_rejects_invalid_battery_values() {
    let mut state = sunray::FlightControlState {
        source_stamp_ns: 42,
        armed: true,
        landed: false,
        control_mode: 1,
        control_state: 3,
        battery_voltage_v: 15.2,
        battery_percent: 88,
        manual_override: false,
    };
    validate_flight_control_state(&state).unwrap();
    assert_eq!(
        sunray::FlightControlState::decode(state.encode_to_vec().as_slice()).unwrap(),
        state
    );
    state.battery_percent = 101;
    assert_eq!(
        validate_flight_control_state(&state),
        Err("flight control state is invalid")
    );
}

#[test]
fn flight_goal_parameters_validate_and_empty_payloads_remain_decodable() {
    let takeoff = sunray::TakeoffGoal {
        takeoff_relative_height_m: 1.2,
        takeoff_max_velocity_mps: 0.5,
    };
    validate_takeoff_goal(&takeoff).unwrap();
    assert_eq!(
        sunray::TakeoffGoal::decode([].as_slice()).unwrap(),
        sunray::TakeoffGoal::default()
    );
    assert!(validate_takeoff_goal(&sunray::TakeoffGoal {
        takeoff_relative_height_m: -1.0,
        takeoff_max_velocity_mps: 0.0,
    })
    .is_err());
    validate_land_goal(&sunray::LandGoal {
        land_max_velocity_mps: 0.4,
    })
    .unwrap();
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
        waypoints: vec![
            sunray::UavWaypoint {
                position_m: Some(vector3(1.0, 2.0, 3.0)),
                yaw_rad: 0.5,
                hold_time_s: 1.5,
                arrival_action: sunray::UavWaypointArrivalAction::UavWaypointHoldSetYaw as i32,
            },
            sunray::UavWaypoint {
                position_m: Some(vector3(-1.0, -2.0, 4.0)),
                yaw_rad: -0.25,
                hold_time_s: 0.0,
                arrival_action: sunray::UavWaypointArrivalAction::UavWaypointNext as i32,
            },
        ],
        task_name: "yunlink-task-42".into(),
        completion_action: sunray::UavMissionCompletionAction::UavMissionFinishHover as i32,
    };
    validate_uav_waypoint_mission_goal(&mission).unwrap();
    let bytes = mission.encode_to_vec();
    assert_eq!(
        bytes,
        hex::decode(
            "0a036d617012310a1b09000000000000f03f11000000000000004019000000000000084011000000000000e03f19000000000000f83f200212260a1b09000000000000f0bf1100000000000000c019000000000000104011000000000000d0bf1a0f79756e6c696e6b2d7461736b2d3432"
        )
        .unwrap()
    );
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
            arrival_action: sunray::UavWaypointArrivalAction::UavWaypointNext as i32,
        },
    );
    assert!(validate_uav_waypoint_mission_goal(&mission).is_err());
}

#[test]
fn planner_v22_messages_round_trip_and_validate() {
    assert!(sunray::UavReturnHomeGoal::decode([].as_slice()).is_ok());
    assert!(sunray::PlannerCancelTaskRequest::decode([].as_slice()).is_ok());
    let mut request = sunray::PlannerSetHomeRequest {
        home_m: Some(vector3(1.0, -2.0, 0.5)),
        frame_id: "map".into(),
    };
    validate_planner_set_home_request(&request).unwrap();
    assert_eq!(
        request.encode_to_vec(),
        vec![
            0x0a, 0x1b, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x3f, 0x11, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe0,
            0x3f, 0x12, 0x03, 0x6d, 0x61, 0x70,
        ]
    );
    request.home_m.as_mut().unwrap().z = f64::NAN;
    assert!(validate_planner_set_home_request(&request).is_err());
}

#[test]
fn gimbal_v24_messages_match_golden_vectors_and_validate() {
    let state = sunray::GimbalParams {
        roll_rad: 1.0,
        pitch_rad: -0.5,
        yaw_rad: 0.25,
        zoom: 3.5,
        connected: true,
        roll_rate_rad_s: 0.1,
        pitch_rate_rad_s: -0.2,
        yaw_rate_rad_s: 0.3,
    };
    assert_eq!(
        hex::encode(state.encode_to_vec()),
        "11000000000000f03f19000000000000e0bf21000000000000d03f290000000000000c403801419a9999999999b93f499a9999999999c9bf51333333333333d33f"
    );

    let angle = sunray::GimbalAngleGoal {
        yaw_rad: std::f64::consts::FRAC_PI_2,
        pitch_rad: -std::f64::consts::FRAC_PI_4,
    };
    validate_gimbal_angle_goal(&angle).unwrap();
    assert_eq!(
        hex::encode(angle.encode_to_vec()),
        "09182d4454fb21f93f11182d4454fb21e9bf"
    );

    let rate = sunray::GimbalRateGoal {
        yaw_control: 45,
        pitch_control: -30,
    };
    validate_gimbal_rate_goal(&rate).unwrap();
    assert_eq!(
        hex::encode(rate.encode_to_vec()),
        "082d10e2ffffffffffffffff01"
    );

    let zoom = sunray::GimbalZoomAbsoluteGoal { zoom: 3.5 };
    validate_gimbal_zoom_absolute_goal(&zoom).unwrap();
    assert_eq!(hex::encode(zoom.encode_to_vec()), "090000000000000c40");
    assert!(sunray::GimbalCenterGoal::decode([].as_slice()).is_ok());

    assert!(validate_gimbal_rate_goal(&sunray::GimbalRateGoal {
        yaw_control: 101,
        pitch_control: 0,
    })
    .is_err());
}

#[test]
fn ugv_v25_messages_match_golden_vectors_and_validate() {
    let mut move_goal = sunray::UgvMovePointGoal {
        frame: sunray::UgvMovePointFrame::UgvMoveLocal as i32,
        point_m: Some(vector3(1.0, -2.0, 0.0)),
        yaw_mode: sunray::UgvYawMode::UgvYawSet as i32,
        desired_yaw_rad: 0.25,
        local_frame_id: "map".into(),
    };
    validate_ugv_move_point_goal(&move_goal).unwrap();
    assert_eq!(
        hex::encode(move_goal.encode_to_vec()),
        "121209000000000000f03f1100000000000000c0180121000000000000d03f2a036d6170"
    );
    move_goal.point_m.as_mut().unwrap().z = 0.1;
    assert!(validate_ugv_move_point_goal(&move_goal).is_err());

    let mut velocity = sunray::UgvVelocityGoal {
        target: Some(sunray::ugv_velocity_goal::Target::Body(
            sunray::UgvBodyVelocityTarget {
                linear_mps: Some(mobility::Vector2 { x: 0.5, y: 0.1 }),
                yaw_rate_radps: -0.2,
            },
        )),
        lease_ms: 750,
    };
    validate_ugv_velocity_goal(&velocity).unwrap();
    assert_eq!(
        hex::encode(velocity.encode_to_vec()),
        "121d0a1209000000000000e03f119a9999999999b93f119a9999999999c9bf18ee05"
    );
    velocity.lease_ms = 249;
    assert!(validate_ugv_velocity_goal(&velocity).is_err());

    let state = sunray::UgvControlState {
        source_stamp_ns: 42,
        drive_type: 1,
        diagnostic_message: "hold".into(),
        fsm_state: 1,
        odom_ready: true,
        ..Default::default()
    };
    assert_eq!(
        hex::encode(state.encode_to_vec()),
        "082a28014a04686f6c6450017801"
    );
    assert!(sunray::UgvHoldGoal {}.encode_to_vec().is_empty());
}
