import pytest
from yunlink.profiles import (mobility, sunray, validate_formation_leader_target_request,
    validate_team_goal_request, validate_formation_set_request, validate_formation_state)

def test_formation_v27_messages_match_cross_language_vectors_and_validate():
    ring = sunray.FormationSetRequest(
        formation_type=sunray.FORMATION_DYNAMIC_RING,
        align_yaw_with_trajectory=True,
        ring=sunray.FormationRing(radius_m=3.0, move_speed_mps=-0.5),
    )
    validate_formation_set_request(ring)
    assert ring.SerializeToString(deterministic=True).hex() == (
        "081510012a1209000000000000084011000000000000e0bf"
    )

    # 13/23 (static / dynamic double ring) were removed. Both wire numbers are
    # reserved: an old peer sending either one must be rejected with a readable
    # reason, not silently reinterpreted as another formation.
    for removed in (13, 23):
        with pytest.raises(ValueError, match="double-ring formations were removed"):
            validate_formation_set_request(
                sunray.FormationSetRequest(formation_type=removed)
            )

    variants = [
        sunray.FormationSetRequest(formation_type=sunray.FORMATION_TAKEOFF),
        sunray.FormationSetRequest(formation_type=sunray.FORMATION_LAND),
        sunray.FormationSetRequest(
            formation_type=sunray.FORMATION_STATIC_LINE,
            line=sunray.FormationLine(spacing_m=2, angle_deg=90),
        ),
        sunray.FormationSetRequest(
            formation_type=sunray.FORMATION_STATIC_POLYGON,
            polygon=sunray.FormationPolygon(side_length_m=2),
        ),
        sunray.FormationSetRequest(
            formation_type=sunray.FORMATION_DYNAMIC_POLYGON,
            polygon=sunray.FormationPolygon(side_length_m=2, move_speed_mps=0.5),
        ),
        sunray.FormationSetRequest(
            formation_type=sunray.FORMATION_DYNAMIC_LEMNISCATE,
            lemniscate=sunray.FormationLemniscate(
                x_scale_m=3, y_scale_m=2, move_speed_mps=0.5
            ),
        ),
        sunray.FormationSetRequest(
            formation_type=sunray.FORMATION_LEADER,
            leader=sunray.FormationLeader(
                agent_slots=[1, 2] + [0] * 23,
                spacing_m=2,
                virtual_leader_slots=[True] + [False] * 24,
            ),
        ),
    ]
    for request in variants:
        validate_formation_set_request(request)

    pose = mobility.Pose(
        position=mobility.Vector3(x=1, y=2, z=3),
        orientation=mobility.Quaternion(w=1),
    )
    target = sunray.FormationLeaderTargetRequest(
        target_mode=sunray.FORMATION_LEADER_TARGET_FIXED_POSE,
        source_stamp_ns=42,
        frame_id="map",
        target_pose=pose,
    )
    validate_formation_leader_target_request(target)
    assert target.SerializeToString(deterministic=True).hex() == (
        "0801102a1a036d617022280a1b09000000000000f03f110000000000000040"
        "190000000000000840120921000000000000f03f"
    )
    validate_formation_leader_target_request(
        sunray.FormationLeaderTargetRequest(
            target_mode=sunray.FORMATION_LEADER_TARGET_ODOM_TOPIC,
            odom_topic="/tracked/leader/odom",
        )
    )

    # 2.17 小队目标：与 formation leader target 是两条独立 action（wire 也不共享字段）。
    # 这里断言与 C++ test_profile_golden 同一串字节，防跨语言漂移。
    team_goal = sunray.TeamGoalRequest(
        frame_id="map",
        source_stamp_ns=42,
        target_pose=pose,
        team_id="team_1",
    )
    validate_team_goal_request(team_goal)
    assert team_goal.SerializeToString(deterministic=True).hex() == (
        "0a036d6170102a1a280a1b09000000000000f03f110000000000000040"
        "190000000000000840120921000000000000f03f22067465616d5f31"
    )
    for invalid in (
        sunray.TeamGoalRequest(frame_id="map", target_pose=pose),  # 空 team_id
        sunray.TeamGoalRequest(team_id="team_1", target_pose=pose),  # 空 frame_id
        sunray.TeamGoalRequest(frame_id="map", team_id="team_1"),  # 缺 target_pose
    ):
        with pytest.raises(ValueError):
            validate_team_goal_request(invalid)

    state = sunray.FormationState(
        source_stamp_ns=42,
        frame_id="map",
        agent_id="uav1",
        task_epoch=sunray.FormationTaskEpoch(
            domain_created_ns=10, origin_agent_id="uav1", origin_sequence=2
        ),
        phase=sunray.FORMATION_PHASE_ACTIVE,
        formation_type=sunray.FORMATION_LEADER,
        formation_established=True,
        virtual_leader_target_valid=True,
        virtual_leader_target=pose,
    )
    validate_formation_state(state)
    assert state.SerializeToString(deterministic=True).hex() == (
        "082a12036d61701a0475617631220a080a12047561763118023802400c48016001"
        "6a280a1b09000000000000f03f1100000000000000401900000000000008401209"
        "21000000000000f03f"
    )

    ring.ring.move_speed_mps = 0
    with pytest.raises(ValueError, match="formation request is invalid"):
        validate_formation_set_request(ring)
