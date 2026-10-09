use prost::Message;
use yunlink_profiles::sunray;

#[test]
fn get_request_matches_python_golden_vector() {
    let request = sunray::StartupQueueRequest {
        action: "get".into(),
        ..Default::default()
    };
    assert_eq!(hex::encode(request.encode_to_vec()), "0a03676574");
    assert_eq!(
        sunray::StartupQueueRequest::decode(request.encode_to_vec().as_slice()).unwrap(),
        request
    );
}

#[test]
fn revision_and_negative_index_survive_round_trip() {
    let request = sunray::StartupQueueRequest {
        action: "save".into(),
        expected_revision: u64::MAX,
        expected_run_id: String::new(),
        items: vec![sunray::StartupQueueItem {
            feature_name: "bridge".into(),
            enabled: true,
            completion_mode: "delay".into(),
            delay_sec: 3.0,
            timeout_sec: 60.0,
            ..Default::default()
        }],
    };
    assert_eq!(
        sunray::StartupQueueRequest::decode(request.encode_to_vec().as_slice()).unwrap(),
        request
    );
    let response = sunray::StartupQueueResponse {
        success: true,
        message: String::new(),
        snapshot: Some(sunray::StartupQueueSnapshot {
            schema_version: 1,
            revision: u64::MAX,
            current_index: -1,
            state: "IDLE".into(),
            items: request.items,
            ..Default::default()
        }),
    };
    assert_eq!(
        sunray::StartupQueueResponse::decode(response.encode_to_vec().as_slice()).unwrap(),
        response
    );
}

#[test]
fn rejection_without_snapshot_is_decodable() {
    let response = sunray::StartupQueueResponse {
        success: false,
        message: "Access denied".into(),
        snapshot: None,
    };
    let decoded =
        sunray::StartupQueueResponse::decode(response.encode_to_vec().as_slice()).unwrap();
    assert_eq!(decoded, response);
    assert!(decoded.snapshot.is_none());
}
