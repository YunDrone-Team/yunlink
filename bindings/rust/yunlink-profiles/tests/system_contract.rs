use prost::Message;
use yunlink_profiles::{system, validate_clock_sync_request, validate_clock_sync_response};

#[test]
fn clock_sync_matches_golden_vector() {
    let request = system::ClockSyncRequest {
        unix_time_ms: 1_767_225_600_123,
        source: "sunray-gcs".into(),
    };
    validate_clock_sync_request(&request).unwrap();
    assert_eq!(
        request.encode_to_vec(),
        hex::decode("08fbd0eab6b733120a73756e7261792d676373").unwrap()
    );

    let response = system::ClockSyncResponse {
        error: system::ClockSyncError::ClockSyncOk as i32,
        message: "synchronized".into(),
        previous_unix_time_ms: 1_767_225_600_000,
        applied_unix_time_ms: 1_767_225_600_123,
        delta_ms: 123,
    };
    validate_clock_sync_response(&response).unwrap();
}

#[test]
fn clock_sync_rejects_invalid_values() {
    assert!(validate_clock_sync_request(&system::ClockSyncRequest {
        unix_time_ms: 1_000,
        source: "sunray-gcs".into(),
    })
    .is_err());
    assert!(validate_clock_sync_request(&system::ClockSyncRequest {
        unix_time_ms: yunlink_profiles::MINIMUM_TRUSTED_UNIX_TIME_MS,
        source: "sunray gcs".into(),
    })
    .is_err());
    assert!(validate_clock_sync_response(&system::ClockSyncResponse {
        error: system::ClockSyncError::ClockSyncOk as i32,
        message: "bad".into(),
        ..Default::default()
    })
    .is_err());
}

#[test]
fn clock_sync_accepts_an_untrusted_previous_device_time() {
    validate_clock_sync_response(&system::ClockSyncResponse {
        error: system::ClockSyncError::ClockSyncOk as i32,
        message: "synchronized".into(),
        previous_unix_time_ms: 31_449_600_000,
        applied_unix_time_ms: 1_767_225_600_123,
        delta_ms: 1_735_776_000_123,
    })
    .unwrap();

    validate_clock_sync_response(&system::ClockSyncResponse {
        error: system::ClockSyncError::ClockSyncOk as i32,
        message: "synchronized".into(),
        previous_unix_time_ms: 0,
        applied_unix_time_ms: 1_767_225_600_123,
        delta_ms: 1_767_225_600_123,
    })
    .unwrap();
}
