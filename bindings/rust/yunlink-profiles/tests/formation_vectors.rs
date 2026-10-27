use prost::Message;
use yunlink_profiles::{
    mobility, sunray, validate_formation_leader_target_request, validate_formation_set_request,
    validate_formation_state, validate_team_goal_request,
};
fn vector3(x: f64, y: f64, z: f64) -> mobility::Vector3 {
    mobility::Vector3 { x, y, z }
}
#[test]
fn formation_v27_messages_match_golden_vectors_and_validate() {
    let ring = sunray::FormationSetRequest {
        formation_type: 21,
        align_yaw_with_trajectory: true,
        ring: Some(sunray::FormationRing {
            radius_m: 3.0,
            move_speed_mps: -0.5,
        }),
        ..Default::default()
    };
    validate_formation_set_request(&ring).unwrap();
    assert_eq!(
        hex::encode(ring.encode_to_vec()),
        "081510012a1209000000000000084011000000000000e0bf"
    );

    for request in [
        sunray::FormationSetRequest {
            formation_type: 1,
            ..Default::default()
        },
        sunray::FormationSetRequest {
            formation_type: 2,
            ..Default::default()
        },
        sunray::FormationSetRequest {
            formation_type: 10,
            line: Some(sunray::FormationLine {
                spacing_m: 2.0,
                angle_deg: 90.0,
            }),
            ..Default::default()
        },
        sunray::FormationSetRequest {
            formation_type: 11,
            polygon: Some(sunray::FormationPolygon {
                side_length_m: 2.0,
                move_speed_mps: 0.0,
            }),
            ..Default::default()
        },
        sunray::FormationSetRequest {
            formation_type: 20,
            polygon: Some(sunray::FormationPolygon {
                side_length_m: 2.0,
                move_speed_mps: 0.5,
            }),
            ..Default::default()
        },
        sunray::FormationSetRequest {
            formation_type: 22,
            lemniscate: Some(sunray::FormationLemniscate {
                x_scale_m: 3.0,
                y_scale_m: 2.0,
                move_speed_mps: 0.5,
            }),
            ..Default::default()
        },
    ] {
        validate_formation_set_request(&request).unwrap();
    }

    let leader = sunray::FormationSetRequest {
        formation_type: 12,
        leader: Some(sunray::FormationLeader {
            agent_slots: [1, 2].into_iter().chain([0; 23]).collect(),
            spacing_m: 2.0,
            virtual_leader_slots: [true].into_iter().chain([false; 24]).collect(),
        }),
        ..Default::default()
    };
    validate_formation_set_request(&leader).unwrap();

    let pose = mobility::Pose {
        position: Some(vector3(1.0, 2.0, 3.0)),
        orientation: Some(mobility::Quaternion {
            w: 1.0,
            ..Default::default()
        }),
    };
    let target = sunray::FormationLeaderTargetRequest {
        target_mode: 1,
        source_stamp_ns: 42,
        frame_id: "map".into(),
        target_pose: Some(pose.clone()),
        odom_topic: String::new(),
        // 2.16 起长机目标带小队作用域；空串 = swarm 级（与 2.15 的 FormationSetRequest 同口径）。
        team_id: String::new(),
    };
    validate_formation_leader_target_request(&target).unwrap();
    assert_eq!(
        hex::encode(target.encode_to_vec()),
        "0801102a1a036d617022280a1b09000000000000f03f110000000000000040190000000000000840120921000000000000f03f"
    );
    let topic = sunray::FormationLeaderTargetRequest {
        target_mode: 2,
        odom_topic: "/tracked/leader/odom".into(),
        ..Default::default()
    };
    validate_formation_leader_target_request(&topic).unwrap();

    // 2.17 小队目标：与 formation leader target 是两条独立 action（wire 也不共享字段）。
    // 这一串必须与 C++ / Python / GCS 的向量逐字一致，防跨语言漂移。
    let team_goal = sunray::TeamGoalRequest {
        frame_id: "map".into(),
        source_stamp_ns: 42,
        target_pose: Some(pose.clone()),
        team_id: "team_1".into(),
    };
    validate_team_goal_request(&team_goal).unwrap();
    assert_eq!(
        hex::encode(team_goal.encode_to_vec()),
        "0a036d6170102a1a280a1b09000000000000f03f110000000000000040190000000000000840120921000000000000f03f22067465616d5f31"
    );
    // 空 team_id 必须被拒（不是"空 = 全部小队"——那正是被拆掉的旧通道病根）。
    let empty_team = sunray::TeamGoalRequest {
        team_id: String::new(),
        ..team_goal.clone()
    };
    assert_eq!(
        validate_team_goal_request(&empty_team),
        Err("team goal request team_id must not be empty")
    );
    let no_pose = sunray::TeamGoalRequest {
        target_pose: None,
        ..team_goal.clone()
    };
    assert!(validate_team_goal_request(&no_pose).is_err());

    let state = sunray::FormationState {
        source_stamp_ns: 42,
        frame_id: "map".into(),
        agent_id: "uav1".into(),
        task_epoch: Some(sunray::FormationTaskEpoch {
            domain_created_ns: 10,
            origin_agent_id: "uav1".into(),
            origin_sequence: 2,
        }),
        phase: 2,
        formation_type: 12,
        formation_established: true,
        virtual_leader_target_valid: true,
        virtual_leader_target: Some(pose),
        ..Default::default()
    };
    validate_formation_state(&state).unwrap();
    assert_eq!(
        hex::encode(state.encode_to_vec()),
        "082a12036d61701a0475617631220a080a12047561763118023802400c480160016a280a1b09000000000000f03f110000000000000040190000000000000840120921000000000000f03f"
    );

    // 13/23 (static / dynamic double ring) were removed. Both wire numbers are
    // reserved: an old peer sending either one must be rejected with a readable
    // reason, not silently reinterpreted as another formation.
    for removed in [13, 23] {
        let request = sunray::FormationSetRequest {
            formation_type: removed,
            ..Default::default()
        };
        assert_eq!(
            validate_formation_set_request(&request),
            Err("double-ring formations were removed")
        );
    }

    let mut invalid = ring;
    invalid.ring.as_mut().unwrap().move_speed_mps = 0.0;
    assert!(validate_formation_set_request(&invalid).is_err());
}
