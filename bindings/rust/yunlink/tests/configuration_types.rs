use yunlink::{ConfigFieldValue, ConfigSnapshot, ConfigValue};

#[test]
fn configuration_snapshot_owns_nested_string_values() {
    let mut source = String::from("/opt/sunray/profile");
    let snapshot = ConfigSnapshot {
        resource_id: "sunray.device.identity".to_string(),
        revision: "r1".to_string(),
        applied_revision: "r0".to_string(),
        values: vec![ConfigFieldValue {
            path: "profile_dirs".to_string(),
            value: ConfigValue::StringList(vec![source.clone()]),
        }],
    };
    source.clear();

    assert_eq!(
        snapshot.values[0].value,
        ConfigValue::StringList(vec!["/opt/sunray/profile".to_string()])
    );
}

#[test]
fn configuration_snapshot_owns_numeric_list_values() {
    let mut source = vec![1.0, 0.5, 0.25];
    let snapshot = ConfigSnapshot {
        resource_id: "sunray.params.sunray_uav_control".to_string(),
        revision: "r1".to_string(),
        applied_revision: "r0".to_string(),
        values: vec![ConfigFieldValue {
            path: "coefficients".to_string(),
            value: ConfigValue::DoubleList(source.clone()),
        }],
    };
    source.clear();

    assert_eq!(
        snapshot.values[0].value,
        ConfigValue::DoubleList(vec![1.0, 0.5, 0.25])
    );
}
