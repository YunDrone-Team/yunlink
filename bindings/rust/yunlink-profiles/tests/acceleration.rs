use prost::Message;
use yunlink_profiles::{mobility, sunray, validate_uav_acceleration_compensation_goal};

#[test]
fn acceleration_golden_and_validation() {
    let mut goal = sunray::UavAccelerationCompensationGoal {
        acceleration_ned_mps2: Some(mobility::Vector3 { x: 0.25, y: -0.5, z: 0.125 }),
    };
    assert!(validate_uav_acceleration_compensation_goal(&goal).is_ok());
    assert_eq!(hex::encode(goal.encode_to_vec()),
        "0a1b09000000000000d03f11000000000000e0bf19000000000000c03f");
    goal.acceleration_ned_mps2.as_mut().unwrap().z = f64::NAN;
    assert!(validate_uav_acceleration_compensation_goal(&goal).is_err());
    assert!(validate_uav_acceleration_compensation_goal(&Default::default()).is_err());
}
