import pytest

from yunlink.profiles import (
    media,
    validate_camera_catalog_snapshot,
    validate_camera_descriptor,
    validate_camera_start_rtsp_response,
    validate_media_asset_chunk,
    validate_media_asset_item,
    validate_media_asset_list_request,
    validate_media_asset_list_response,
    validate_media_asset_ref,
)


def test_media_profile_matches_cross_language_golden_and_rejects_invalid_assets():
    request = media.CameraTakePhotoRequest(camera_uid="front")
    assert request.SerializeToString(deterministic=True) == bytes.fromhex("0a0566726f6e74")

    camera = media.CameraDescriptor(
        camera_uid="front", camera_id=1, name="Front camera", online=True,
        frame_rate_hz=30.0, live_view_supported=True, live_view_active=True,
        live_view_autostart=True,
        rtsp_url="rtsp://192.168.10.38:8554/front",
    )
    validate_camera_descriptor(camera)
    catalog = media.CameraCatalogSnapshot(
        generated_at_ns=42, cameras=[camera], camera_manager_available=True,
    )
    validate_camera_catalog_snapshot(catalog)
    catalog.cameras[0].rtsp_url = ""
    with pytest.raises(ValueError, match="camera descriptor is invalid"):
        validate_camera_catalog_snapshot(catalog)
    catalog.cameras[0].CopyFrom(camera)
    catalog.cameras.add().CopyFrom(camera)
    with pytest.raises(ValueError, match="duplicate camera uid"):
        validate_camera_catalog_snapshot(catalog)

    asset = media.MediaAssetRef(
        asset_id="asset-01", kind=media.MEDIA_PHOTO, mime_type="image/png",
        size_bytes=8, sha256=b"\x01" * 32, camera_uid="front",
    )
    validate_media_asset_ref(asset)
    asset.sha256 = b"\x01" * 31
    with pytest.raises(ValueError, match="media asset reference is invalid"):
        validate_media_asset_ref(asset)

    chunk = media.MediaAssetChunkResponse(
        error=media.MEDIA_OK, transfer_id="transfer-01", eof=True,
    )
    validate_media_asset_chunk(chunk)
    validate_media_asset_chunk(
        media.MediaAssetChunkResponse(error=media.MEDIA_BUSY, message="queue is full")
    )


def test_media_rtsp_start_response_preserves_the_provider_url_byte_for_byte():
    response = media.CameraStartRtspResponse(
        error=media.MEDIA_OK,
        message="ready",
        rtsp_url="rtsp://viewer:secret@192.168.10.60:8554/front/main?profile=high&token=a%2Fb",
    )
    validate_camera_start_rtsp_response(response)
    assert response.SerializeToString(deterministic=True).hex() == (
        "0801120572656164791a4b727473703a2f2f7669657765723a736563726574403139322e"
        "3136382e31302e36303a383535342f66726f6e742f6d61696e3f70726f66696c653d6869"
        "676826746f6b656e3d6125324662"
    )

    response.rtsp_url = ""
    with pytest.raises(ValueError, match="camera start RTSP response is invalid"):
        validate_camera_start_rtsp_response(response)
    response.error = media.MEDIA_OPERATION_FAILED
    response.message = "provider rejected start"
    validate_camera_start_rtsp_response(response)


def test_media_asset_list_contract_is_paged_and_keeps_thumbnail_relation_explicit():
    request = media.MediaAssetListRequest(
        camera_uid="front", kinds=[media.MEDIA_PHOTO, media.MEDIA_VIDEO],
        created_after_ns=10, created_before_ns=20, page_size=25, page_token="Y3Vyc29y",
    )
    validate_media_asset_list_request(request)
    assert request.SerializeToString(deterministic=True).hex() == (
        "0a0566726f6e7412020103180a2014281932085933567963323979"
    )
    asset = media.MediaAssetRef(
        asset_id="photo-1", kind=media.MEDIA_PHOTO, mime_type="image/png", size_bytes=8,
        sha256=b"\x01" * 32, created_at_ns=42, camera_uid="front", display_name="photo.png",
    )
    thumbnail = media.MediaAssetRef(
        asset_id="thumb-1", kind=media.MEDIA_THUMBNAIL, mime_type="image/png", size_bytes=4,
        sha256=b"\x02" * 32, created_at_ns=42, camera_uid="front", display_name="thumb.png",
    )
    response = media.MediaAssetListResponse(
        error=media.MEDIA_OK, items=[media.MediaAssetItem(
            asset=asset, thumbnail=thumbnail, width=1920, height=1080,
        )], next_page_token="next", catalog_revision=7,
    )
    validate_media_asset_item(response.items[0])
    validate_media_asset_list_response(response)
    request.page_size = 101
    with pytest.raises(ValueError, match="media asset list request is invalid"):
        validate_media_asset_list_request(request)
