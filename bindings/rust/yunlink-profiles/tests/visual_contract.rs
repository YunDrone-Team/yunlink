use std::{collections::BTreeMap, fs, path::PathBuf};

fn vectors() -> BTreeMap<String, String> {
    let path = PathBuf::from(env!("CARGO_MANIFEST_DIR"))
        .join("../../../profiles/org.yunlink.visual/v1/golden/visual-v1-vectors.txt");
    fs::read_to_string(path)
        .unwrap()
        .lines()
        .filter(|line| !line.is_empty() && !line.starts_with('#'))
        .map(|line| line.split_once('=').unwrap())
        .map(|(key, value)| (key.to_owned(), value.to_owned()))
        .collect()
}

fn hex(value: &str) -> Vec<u8> {
    (0..value.len())
        .step_by(2)
        .map(|index| u8::from_str_radix(&value[index..index + 2], 16).unwrap())
        .collect()
}

fn valid_ylpc(bytes: &[u8]) -> bool {
    if bytes.len() < 16 || &bytes[..4] != b"YLPC" {
        return false;
    }
    let u16_at = |offset| u16::from_le_bytes([bytes[offset], bytes[offset + 1]]);
    let u32_at = |offset| {
        u32::from_le_bytes([
            bytes[offset],
            bytes[offset + 1],
            bytes[offset + 2],
            bytes[offset + 3],
        ])
    };
    u16_at(4) == 1
        && u16_at(6) & !1 == 0
        && u32_at(12) == 16
        && bytes.len() == 16 + u32_at(8) as usize * 16
}

#[test]
fn shared_visual_vectors_define_the_v1_contract() {
    let values = vectors();
    assert!(valid_ylpc(&hex(&values["point_cloud.valid.hex"])));
    assert!(!valid_ylpc(&hex(&values["point_cloud.invalid_stride.hex"])));
    assert!(!valid_ylpc(&hex(
        &values["point_cloud.invalid_truncated.hex"]
    )));
    assert_eq!(hex(&values["image.raw.hex"]), vec![0, 1, 2, 3, 4, 5]);
    assert!(values["marker.add.json"].contains("\"frame_locked\""));
    assert!(values["marker.delete.json"].contains("\"action\":2"));
    assert!(values["marker.invalid_quaternion.json"].contains("[0,0,0,0]"));
}
