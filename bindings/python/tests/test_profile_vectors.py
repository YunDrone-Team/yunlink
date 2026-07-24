from google.protobuf.message import DecodeError
import pytest

from yunlink.profiles import mobility, sunray


GOTO_GOLDEN = bytes.fromhex(
    "0a036d6170121b09000000000000f03f1100000000000000c0"
    "19000000000000e03f19000000000000d03f"
)


def test_profile_payloads_match_cross_language_golden_vectors():
    goal = mobility.GotoGoal(frame_id="map", yaw_rad=0.25)
    goal.position.x = 1.0
    goal.position.y = -2.0
    goal.position.z = 0.5
    assert goal.SerializeToString(deterministic=True) == GOTO_GOLDEN

    request = sunray.FeatureStartRequest(name="mapping")
    assert request.SerializeToString(deterministic=True) == b"\x0a\x07mapping"

    with pytest.raises(DecodeError):
        sunray.FeatureStartRequest.FromString(b"\x0a\x08mapping")
