from pathlib import Path
import pytest
import yunlink
from yunlink.configuration_codec import decode, encode, ConfigurationCodecError
from yunlink.profiles import (
    mobility, sunray, validate_team_goal_request,
    validate_formation_set_request, validate_formation_leader_target_request,
    validate_formation_state,
)

VECTORS = dict(line.split("=", 1) for line in (
    Path(__file__).resolve().parents[3] /
    "profiles/com.yundrone.sunray/v2/golden/team-contract-vectors.txt"
).read_text().splitlines() if line and not line.startswith("#"))


def pose():
    return mobility.Pose(position=mobility.Vector3(x=1, y=2, z=3),
                         orientation=mobility.Quaternion(w=1))


def test_team_contracts_match_shared_golden():
    messages = {
        "formation_set": sunray.FormationSetRequest(formation_type=1, team_id="team_1"),
        "formation_leader_target": sunray.FormationLeaderTargetRequest(
            target_mode=1, source_stamp_ns=42, frame_id="map", target_pose=pose(),
            team_id="team_1"),
        "formation_state": sunray.FormationState(
            virtual_leader_target_valid=True, virtual_leader_target=pose(),
            virtual_leader_team_id="team_1"),
        "team_goal": sunray.TeamGoalRequest(
            frame_id="map", source_stamp_ns=42, target_pose=pose(), team_id="team_1"),
        "team_goal_response": sunray.TeamGoalResponse(
            success=True, message="ok", team_id="team_1", planner_agent_id="uav1"),
        "team_demo_start": sunray.TeamDemoStartRequest(
            demo_id="demo", target_agent_name="uav1", target_agent_id=1),
        "team_demo_response": sunray.TeamDemoStartResponse(
            success=True, message="ok", demo_id="demo",
            target_agent_name="uav1", target_agent_id=1),
        "feature": sunray.FeatureDescriptor(
            name="demo", demo_id="demo", lifecycle_mode=3, target_scope="team"),
        "localization": sunray.LocalizationState(status=2, reason="missing"),
        "startup_queue_item": sunray.StartupQueueItem(auto_start_locked=True),
    }
    for name, message in messages.items():
        golden = bytes.fromhex(VECTORS[name + ".hex"])
        assert message.SerializeToString(deterministic=True) == golden, name
        assert type(message).FromString(golden) == message, name
    for request, validator in (
        (messages["formation_set"], validate_formation_set_request),
        (messages["formation_leader_target"], validate_formation_leader_target_request),
    ):
        validator(request)
        request.team_id = ""
        validator(request)
    validate_formation_state(messages["formation_state"])
    validate_team_goal_request(messages["team_goal"])


@pytest.mark.parametrize("value", [float("nan"), float("inf"), -float("inf")])
def test_team_goal_rejects_nonfinite_pose(value):
    for field in ("position", "orientation"):
        request = sunray.TeamGoalRequest(frame_id="map", target_pose=pose(), team_id="team_1")
        getattr(request.target_pose, field).x = value
        with pytest.raises(ValueError, match="pose"):
            validate_team_goal_request(request)


def test_team_goal_rejects_zero_quaternion():
    request = sunray.TeamGoalRequest(frame_id="map", target_pose=pose(), team_id="team_1")
    request.target_pose.orientation.w = 0
    with pytest.raises(ValueError, match="pose"):
        validate_team_goal_request(request)


def test_field_numbers_and_lifecycle_values():
    for message, fields in (
        (sunray.FormationSetRequest, {"team_id": 11}),
        (sunray.FormationLeaderTargetRequest, {"team_id": 6}),
        (sunray.FormationState, {"virtual_leader_team_id": 24}),
        (sunray.FeatureDescriptor, {"demo_id": 12, "lifecycle_mode": 13, "target_scope": 14}),
        (sunray.StartupQueueItem, {"auto_start_locked": 16}),
    ):
        for name, number in fields.items():
            assert message.DESCRIPTOR.fields_by_name[name].number == number
    assert list(sunray.TeamDemoLifecycleMode.values()) == [0, 1, 2, 3]


def test_configuration_matches_actual_old_rust_bytes():
    request = yunlink.ConfigResourceDescribeRequest("sunray.params.flight", "zh-CN")
    golden = bytes.fromhex(VECTORS["describe.zh-CN.hex"])
    assert encode(request) == golden
    assert decode(type(request), golden) == request


def test_current_python_schema_cannot_decode_old_rust_advanced_fields():
    # Python currently omits default-value and advanced schema fields.
    # Keep the actual shipment fixture authoritative in C++ and Rust.
    response_bytes = bytes.fromhex(VECTORS["describe.advanced.hex"])
    with pytest.raises(ConfigurationCodecError, match="truncated configuration payload"):
        decode(yunlink.ConfigResourceDescribeResponse, response_bytes)
