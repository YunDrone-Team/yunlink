import math

from .org.yunlink.media.v1 import media_pb2 as media


def _valid_media_token(value: str, max_bytes: int) -> bool:
    return (
        bool(value)
        and len(value.encode()) <= max_bytes
        and all(char.isascii() and (char.isalnum() or char in "-_.") for char in value)
    )


def _valid_media_page_token(value: str) -> bool:
    return len(value.encode()) <= 512 and all(
        char.isascii() and (char.isalnum() or char in "-_") or char == "=" for char in value
    )


def validate_camera_descriptor(camera: media.CameraDescriptor) -> None:
    if not (
        _valid_media_token(camera.camera_uid, 96)
        and len(camera.name.encode()) <= 128
        and len(camera.image_topic.encode()) <= 256
        and len(camera.camera_info_topic.encode()) <= 256
        and len(camera.encoding.encode()) <= 64
        and len(camera.error_message.encode()) <= 256
        and len(camera.rtsp_url.encode()) <= 2048
        and not any(ord(char) < 0x20 or ord(char) == 0x7F for char in camera.rtsp_url)
        and (
            not camera.live_view_active
            or (camera.live_view_supported and bool(camera.rtsp_url))
        )
        and (
            camera.live_view_supported
            or (
                not camera.live_view_control_supported
                and not camera.rtsp_url
                and not camera.live_view_autostart
            )
        )
        and math.isfinite(camera.frame_rate_hz)
        and camera.frame_rate_hz >= 0
    ):
        raise ValueError("camera descriptor is invalid")


def validate_camera_catalog_snapshot(snapshot: media.CameraCatalogSnapshot) -> None:
    if len(snapshot.cameras) > 32:
        raise ValueError("camera catalog is invalid")
    camera_uids: set[str] = set()
    for camera in snapshot.cameras:
        validate_camera_descriptor(camera)
        if camera.camera_uid in camera_uids:
            raise ValueError("duplicate camera uid")
        camera_uids.add(camera.camera_uid)


def validate_camera_start_rtsp_response(response: media.CameraStartRtspResponse) -> None:
    if not (
        response.error in set(range(media.MEDIA_OK, media.MEDIA_INTEGRITY_ERROR + 1))
        and len(response.message.encode()) <= 256
        and len(response.rtsp_url.encode()) <= 2048
        and not any(ord(char) < 0x20 or ord(char) == 0x7F for char in response.rtsp_url)
        and (
            (response.error == media.MEDIA_OK and bool(response.rtsp_url))
            or (response.error != media.MEDIA_OK and not response.rtsp_url)
        )
    ):
        raise ValueError("camera start RTSP response is invalid")


def validate_media_asset_ref(asset: media.MediaAssetRef) -> None:
    if not (
        _valid_media_token(asset.asset_id, 128)
        and asset.kind in {media.MEDIA_PHOTO, media.MEDIA_THUMBNAIL, media.MEDIA_VIDEO}
        and 0 < len(asset.mime_type.encode()) <= 96
        and asset.size_bytes > 0
        and len(asset.sha256) == 32
        and _valid_media_token(asset.camera_uid, 96)
        and len(asset.display_name.encode()) <= 160
    ):
        raise ValueError("media asset reference is invalid")


def validate_media_asset_item(item: media.MediaAssetItem) -> None:
    if not item.HasField("asset"):
        raise ValueError("media asset item is invalid")
    try:
        validate_media_asset_ref(item.asset)
    except ValueError as exc:
        raise ValueError("media asset item is invalid") from exc
    if item.asset.kind == media.MEDIA_THUMBNAIL or item.width > 32768 or item.height > 32768:
        raise ValueError("media asset item is invalid")
    if item.HasField("thumbnail"):
        try:
            validate_media_asset_ref(item.thumbnail)
        except ValueError as exc:
            raise ValueError("media asset thumbnail relation is invalid") from exc
        if (
            item.thumbnail.kind != media.MEDIA_THUMBNAIL
            or item.thumbnail.camera_uid != item.asset.camera_uid
            or item.thumbnail.asset_id == item.asset.asset_id
        ):
            raise ValueError("media asset thumbnail relation is invalid")


def validate_media_asset_list_request(request: media.MediaAssetListRequest) -> None:
    if not (
        (not request.camera_uid or _valid_media_token(request.camera_uid, 96))
        and 1 <= request.page_size <= 100
        and _valid_media_page_token(request.page_token)
        and not (
            request.created_after_ns
            and request.created_before_ns
            and request.created_after_ns > request.created_before_ns
        )
    ):
        raise ValueError("media asset list request is invalid")
    if len(set(request.kinds)) != len(request.kinds) or any(
        kind not in {media.MEDIA_PHOTO, media.MEDIA_VIDEO} for kind in request.kinds
    ):
        raise ValueError("media asset list kind is invalid")


def validate_media_asset_list_response(response: media.MediaAssetListResponse) -> None:
    if not (
        response.error in {
            media.MEDIA_OK, media.MEDIA_INVALID_REQUEST, media.MEDIA_CAMERA_UNAVAILABLE,
            media.MEDIA_UNSUPPORTED, media.MEDIA_BUSY, media.MEDIA_OPERATION_FAILED,
            media.MEDIA_TIMEOUT, media.MEDIA_INTERNAL_ERROR, media.MEDIA_PERMISSION_DENIED,
            media.MEDIA_NOT_FOUND, media.MEDIA_INTEGRITY_ERROR,
        }
        and len(response.message.encode()) <= 256
        and len(response.items) <= 100
        and _valid_media_page_token(response.next_page_token)
    ):
        raise ValueError("media asset list response is invalid")
    if response.error != media.MEDIA_OK and (response.items or response.next_page_token):
        raise ValueError("failed media asset list response contains data")
    asset_ids: set[str] = set()
    for item in response.items:
        try:
            validate_media_asset_item(item)
        except ValueError as exc:
            raise ValueError("media asset list response contains duplicate or invalid asset") from exc
        if item.asset.asset_id in asset_ids:
            raise ValueError("media asset list response contains duplicate or invalid asset")
        asset_ids.add(item.asset.asset_id)


def validate_media_asset_chunk(chunk: media.MediaAssetChunkResponse) -> None:
    if chunk.error not in {
        media.MEDIA_OK,
        media.MEDIA_INVALID_REQUEST,
        media.MEDIA_CAMERA_UNAVAILABLE,
        media.MEDIA_UNSUPPORTED,
        media.MEDIA_BUSY,
        media.MEDIA_OPERATION_FAILED,
        media.MEDIA_TIMEOUT,
        media.MEDIA_INTERNAL_ERROR,
        media.MEDIA_PERMISSION_DENIED,
        media.MEDIA_NOT_FOUND,
        media.MEDIA_INTEGRITY_ERROR,
    }:
        raise ValueError("media error is invalid")
    if not (
        len(chunk.message.encode()) <= 256
        and len(chunk.data) <= 256 * 1024
        and (not chunk.transfer_id or _valid_media_token(chunk.transfer_id, 128))
        and (
            chunk.error != media.MEDIA_OK
            or (bool(chunk.transfer_id) and (bool(chunk.data) or chunk.eof))
        )
    ):
        raise ValueError("media asset chunk is invalid")
