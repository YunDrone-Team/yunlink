use prost::Message;
use yunlink_profiles::{
    mobility, sunray, validate_formation_leader_target_request, validate_formation_set_request,
    validate_formation_state, validate_team_goal_request,
};

fn golden(name: &str) -> Vec<u8> {
    let text = include_str!(
        "../../../../profiles/com.yundrone.sunray/v2/golden/team-contract-vectors.txt"
    );
    let value = text
        .lines()
        .filter_map(|line| line.split_once('='))
        .find(|(key, _)| *key == format!("{name}.hex"))
        .unwrap()
        .1;
    hex::decode(value).unwrap()
}

fn check<T: Message + Default + PartialEq + std::fmt::Debug>(name: &str, value: T) {
    let bytes = golden(name);
    assert_eq!(value.encode_to_vec(), bytes, "{name}");
    assert_eq!(T::decode(bytes.as_slice()).unwrap(), value);
}

fn pose() -> mobility::Pose {
    mobility::Pose {
        position: Some(mobility::Vector3 {
            x: 1.0,
            y: 2.0,
            z: 3.0,
        }),
        orientation: Some(mobility::Quaternion {
            w: 1.0,
            ..Default::default()
        }),
    }
}

#[test]
fn team_contracts_match_shared_golden() {
    let mut set = sunray::FormationSetRequest {
        formation_type: 1,
        team_id: "team_1".into(),
        ..Default::default()
    };
    validate_formation_set_request(&set).unwrap();
    check("formation_set", set.clone());
    set.team_id.clear();
    validate_formation_set_request(&set).unwrap();
    let mut target = sunray::FormationLeaderTargetRequest {
        target_mode: 1,
        source_stamp_ns: 42,
        frame_id: "map".into(),
        target_pose: Some(pose()),
        team_id: "team_1".into(),
        ..Default::default()
    };
    validate_formation_leader_target_request(&target).unwrap();
    check("formation_leader_target", target.clone());
    target.team_id.clear();
    validate_formation_leader_target_request(&target).unwrap();
    let state = sunray::FormationState {
        virtual_leader_target_valid: true,
        virtual_leader_target: Some(pose()),
        virtual_leader_team_id: "team_1".into(),
        ..Default::default()
    };
    validate_formation_state(&state).unwrap();
    check("formation_state", state);
    let goal = sunray::TeamGoalRequest {
        frame_id: "map".into(),
        source_stamp_ns: 42,
        target_pose: Some(pose()),
        team_id: "team_1".into(),
    };
    validate_team_goal_request(&goal).unwrap();
    check("team_goal", goal);
    check(
        "team_goal_response",
        sunray::TeamGoalResponse {
            success: true,
            message: "ok".into(),
            team_id: "team_1".into(),
            planner_agent_id: "uav1".into(),
        },
    );
    check(
        "team_demo_start",
        sunray::TeamDemoStartRequest {
            demo_id: "demo".into(),
            target_agent_name: "uav1".into(),
            target_agent_id: 1,
        },
    );
    check(
        "team_demo_response",
        sunray::TeamDemoStartResponse {
            success: true,
            message: "ok".into(),
            demo_id: "demo".into(),
            target_agent_name: "uav1".into(),
            target_agent_id: 1,
        },
    );
    check(
        "feature",
        sunray::FeatureDescriptor {
            name: "demo".into(),
            demo_id: "demo".into(),
            lifecycle_mode: 3,
            target_scope: "team".into(),
            ..Default::default()
        },
    );
    for value in 0..=3 {
        assert!(sunray::TeamDemoLifecycleMode::try_from(value).is_ok());
    }
    check(
        "localization",
        sunray::LocalizationState {
            status: 2,
            reason: "missing".into(),
            ..Default::default()
        },
    );
    check(
        "startup_queue_item",
        sunray::StartupQueueItem {
            auto_start_locked: true,
            ..Default::default()
        },
    );
}

#[test]
fn team_goal_rejects_invalid_requests() {
    let valid = sunray::TeamGoalRequest {
        frame_id: "map".into(),
        target_pose: Some(pose()),
        team_id: "team_1".into(),
        ..Default::default()
    };
    for invalid in [
        sunray::TeamGoalRequest {
            team_id: String::new(),
            ..valid.clone()
        },
        sunray::TeamGoalRequest {
            frame_id: String::new(),
            ..valid.clone()
        },
        sunray::TeamGoalRequest {
            target_pose: None,
            ..valid.clone()
        },
    ] {
        assert!(validate_team_goal_request(&invalid).is_err());
    }
    let mut invalid = valid.clone();
    invalid
        .target_pose
        .as_mut()
        .unwrap()
        .orientation
        .as_mut()
        .unwrap()
        .w = 0.0;
    assert!(validate_team_goal_request(&invalid).is_err());
    for value in [f64::NAN, f64::INFINITY, f64::NEG_INFINITY] {
        let mut invalid = valid.clone();
        invalid
            .target_pose
            .as_mut()
            .unwrap()
            .position
            .as_mut()
            .unwrap()
            .x = value;
        assert!(validate_team_goal_request(&invalid).is_err());
        let mut invalid = valid.clone();
        invalid
            .target_pose
            .as_mut()
            .unwrap()
            .orientation
            .as_mut()
            .unwrap()
            .x = value;
        assert!(validate_team_goal_request(&invalid).is_err());
    }
}
