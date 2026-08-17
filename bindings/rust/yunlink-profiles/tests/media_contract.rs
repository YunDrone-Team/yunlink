use prost::Message;
use yunlink_profiles::{
    media, validate_camera_catalog_snapshot, validate_camera_descriptor,
    validate_camera_start_rtsp_response, validate_media_asset_chunk,
    validate_media_asset_list_request, validate_media_asset_list_response,
    validate_media_asset_ref,
};

#[test]
fn camera_request_matches_cross_language_golden_vector() {
    let request = media::CameraTakePhotoRequest {
        camera_uid: "front".into(),
    };
    assert_eq!(
        request.encode_to_vec(),
        hex::decode("0a0566726f6e74").unwrap()
    );
}

#[test]
fn asset_list_contract_is_paged_and_keeps_thumbnail_relation_explicit() {
    let request = media::MediaAssetListRequest {
        camera_uid: "front".into(),
        kinds: vec![
            media::MediaAssetKind::MediaPhoto as i32,
            media::MediaAssetKind::MediaVideo as i32,
        ],
        created_after_ns: 10,
        created_before_ns: 20,
        page_size: 25,
        page_token: "Y3Vyc29y".into(),
    };
    validate_media_asset_list_request(&request).unwrap();
    assert_eq!(
        request.encode_to_vec(),
        hex::decode("0a0566726f6e7412020103180a2014281932085933567963323979").unwrap()
    );

    let asset = media::MediaAssetRef {
        asset_id: "photo-1".into(),
        kind: media::MediaAssetKind::MediaPhoto as i32,
        mime_type: "image/png".into(),
        size_bytes: 8,
        sha256: vec![1; 32],
        created_at_ns: 42,
        camera_uid: "front".into(),
        display_name: "photo.png".into(),
    };
    let thumbnail = media::MediaAssetRef {
        asset_id: "thumb-1".into(),
        kind: media::MediaAssetKind::MediaThumbnail as i32,
        mime_type: "image/png".into(),
        size_bytes: 4,
        sha256: vec![2; 32],
        created_at_ns: 42,
        camera_uid: "front".into(),
        display_name: "thumb.png".into(),
    };
    let response = media::MediaAssetListResponse {
        error: media::MediaError::MediaOk as i32,
        items: vec![media::MediaAssetItem {
            asset: Some(asset),
            thumbnail: Some(thumbnail),
            width: 1920,
            height: 1080,
            duration_ms: 0,
        }],
        next_page_token: "next".into(),
        catalog_revision: 7,
        ..Default::default()
    };
    validate_media_asset_list_response(&response).unwrap();
    let mut invalid = request.clone();
    invalid.page_size = 101;
    assert!(validate_media_asset_list_request(&invalid).is_err());
}

#[test]
fn rtsp_start_response_preserves_the_provider_url_byte_for_byte() {
    let response = media::CameraStartRtspResponse {
        error: media::MediaError::MediaOk as i32,
        message: "ready".into(),
        rtsp_url: "rtsp://viewer:secret@192.168.10.60:8554/front/main?profile=high&token=a%2Fb"
            .into(),
    };
    validate_camera_start_rtsp_response(&response).unwrap();
    assert_eq!(
        response.encode_to_vec(),
        hex::decode(
            "0801120572656164791a4b727473703a2f2f7669657765723a736563726574403139322e3136382e31302e36303a383535342f66726f6e742f6d61696e3f70726f66696c653d6869676826746f6b656e3d6125324662"
        )
        .unwrap()
    );

    let mut invalid = response.clone();
    invalid.rtsp_url.clear();
    assert!(validate_camera_start_rtsp_response(&invalid).is_err());
    invalid.error = media::MediaError::MediaOperationFailed as i32;
    invalid.message = "provider rejected start".into();
    assert!(validate_camera_start_rtsp_response(&invalid).is_ok());
}

#[test]
fn catalog_and_asset_contracts_reject_ambiguous_data() {
    let camera = media::CameraDescriptor {
        camera_uid: "front".into(),
        camera_id: 1,
        name: "Front camera".into(),
        online: true,
        frame_rate_hz: 30.0,
        live_view_supported: true,
        live_view_active: true,
        live_view_autostart: true,
        rtsp_url: "rtsp://192.168.10.38:8554/front".into(),
        ..Default::default()
    };
    validate_camera_descriptor(&camera).unwrap();
    let mut catalog = media::CameraCatalogSnapshot {
        generated_at_ns: 42,
        cameras: vec![camera.clone()],
        camera_manager_available: true,
        ..Default::default()
    };
    validate_camera_catalog_snapshot(&catalog).unwrap();
    catalog.cameras[0].rtsp_url.clear();
    assert_eq!(
        validate_camera_catalog_snapshot(&catalog),
        Err("camera descriptor is invalid")
    );
    catalog.cameras[0] = camera.clone();
    catalog.cameras.push(camera);
    assert_eq!(
        validate_camera_catalog_snapshot(&catalog),
        Err("duplicate camera uid")
    );

    let asset = media::MediaAssetRef {
        asset_id: "asset-01".into(),
        kind: media::MediaAssetKind::MediaPhoto as i32,
        mime_type: "image/png".into(),
        size_bytes: 8,
        sha256: vec![1; 32],
        camera_uid: "front".into(),
        ..Default::default()
    };
    validate_media_asset_ref(&asset).unwrap();
    let invalid = media::MediaAssetRef {
        sha256: vec![1; 31],
        ..asset
    };
    assert!(validate_media_asset_ref(&invalid).is_err());
}

#[test]
fn asset_chunks_are_bounded_and_final_empty_chunk_is_explicit() {
    let chunk = media::MediaAssetChunkResponse {
        error: media::MediaError::MediaOk as i32,
        transfer_id: "transfer-01".into(),
        eof: true,
        ..Default::default()
    };
    validate_media_asset_chunk(&chunk).unwrap();
    let invalid = media::MediaAssetChunkResponse {
        eof: false,
        ..chunk
    };
    assert!(validate_media_asset_chunk(&invalid).is_err());

    let rejected = media::MediaAssetChunkResponse {
        error: media::MediaError::MediaBusy as i32,
        message: "queue is full".into(),
        ..Default::default()
    };
    validate_media_asset_chunk(&rejected).unwrap();
    let malformed_error = media::MediaAssetChunkResponse {
        transfer_id: "not valid!".into(),
        ..rejected
    };
    assert!(validate_media_asset_chunk(&malformed_error).is_err());
}
