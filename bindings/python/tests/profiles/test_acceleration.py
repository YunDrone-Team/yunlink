import pytest

from yunlink.profiles import mobility, sunray, validate_uav_acceleration_compensation_goal


def test_acceleration_golden_and_validation():
    goal = sunray.UavAccelerationCompensationGoal(
        acceleration_ned_mps2=mobility.Vector3(x=0.25, y=-0.5, z=0.125)
    )
    validate_uav_acceleration_compensation_goal(goal)
    assert goal.SerializeToString().hex() == (
        "0a1b09000000000000d03f11000000000000e0bf19000000000000c03f"
    )
    for value in (float("nan"), float("inf"), -float("inf")):
        goal.acceleration_ned_mps2.z = value
        with pytest.raises(ValueError):
            validate_uav_acceleration_compensation_goal(goal)
    with pytest.raises(ValueError):
        validate_uav_acceleration_compensation_goal(sunray.UavAccelerationCompensationGoal())
    goal.acceleration_ned_mps2.Clear()
    validate_uav_acceleration_compensation_goal(goal)  # Explicit zero is a valid sample.
