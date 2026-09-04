use crate::{
    media, MEDIA_MAX_ASSET_PAGE_SIZE, MEDIA_MAX_CAMERAS, MEDIA_MAX_CHUNK_BYTES,
    MEDIA_MAX_DIMENSION_PIXELS, MEDIA_MAX_PAGE_TOKEN_BYTES, MEDIA_MAX_SOURCE_URI_BYTES,
};

fn valid_token(value: &str, max_bytes: usize) -> bool {
    !value.is_empty()
        && value.len() <= max_bytes
        && value
            .bytes()
            .all(|byte| byte.is_ascii_alphanumeric() || matches!(byte, b'-' | b'_' | b'.'))
}

fn valid_page_token(value: &str) -> bool {
    value.len() <= MEDIA_MAX_PAGE_TOKEN_BYTES
        && value
            .bytes()
            .all(|byte| byte.is_ascii_alphanumeric() || matches!(byte, b'-' | b'_' | b'='))
}

pub fn validate_camera_descriptor(camera: &media::CameraDescriptor) -> Result<(), &'static str> {
    (valid_token(&camera.camera_uid, 96)
        && camera.name.len() <= 128
        && camera.image_topic.len() <= 256
        && camera.camera_info_topic.len() <= 256
        && camera.encoding.len() <= 64
        && camera.error_message.len() <= 256
        && camera.rtsp_url.len() <= MEDIA_MAX_SOURCE_URI_BYTES
        && !camera
            .rtsp_url
            .bytes()
            .any(|byte| byte < 0x20 || byte == 0x7f)
        && (!camera.live_view_active
            || (camera.live_view_supported && !camera.rtsp_url.is_empty()))
        && (camera.live_view_supported
            || (!camera.live_view_control_supported
                && camera.rtsp_url.is_empty()
                && !camera.live_view_autostart))
        && camera.frame_rate_hz.is_finite()
        && camera.frame_rate_hz >= 0.0)
        .then_some(())
        .ok_or("camera descriptor is invalid")
}

pub fn validate_camera_catalog_snapshot(
    snapshot: &media::CameraCatalogSnapshot,
) -> Result<(), &'static str> {
    use std::collections::HashSet;

    if snapshot.cameras.len() > MEDIA_MAX_CAMERAS {
        return Err("camera catalog is invalid");
    }
    let mut camera_uids = HashSet::new();
    for camera in &snapshot.cameras {
        validate_camera_descriptor(camera)?;
        if !camera_uids.insert(camera.camera_uid.as_str()) {
            return Err("duplicate camera uid");
        }
    }
    Ok(())
}

pub fn validate_camera_start_rtsp_response(
    response: &media::CameraStartRtspResponse,
) -> Result<(), &'static str> {
    let error = media::MediaError::try_from(response.error)
        .map_err(|_| "camera start RTSP response is invalid")?;
    (error != media::MediaError::Unspecified
        && response.message.len() <= 256
        && response.rtsp_url.len() <= MEDIA_MAX_SOURCE_URI_BYTES
        && !response
            .rtsp_url
            .bytes()
            .any(|byte| byte < 0x20 || byte == 0x7f)
        && ((error == media::MediaError::MediaOk && !response.rtsp_url.is_empty())
            || (error != media::MediaError::MediaOk && response.rtsp_url.is_empty())))
    .then_some(())
    .ok_or("camera start RTSP response is invalid")
}

pub fn validate_media_asset_ref(asset: &media::MediaAssetRef) -> Result<(), &'static str> {
    (valid_token(&asset.asset_id, 128)
        && (media::MediaAssetKind::MediaPhoto as i32..=media::MediaAssetKind::MediaVideo as i32)
            .contains(&asset.kind)
        && !asset.mime_type.is_empty()
        && asset.mime_type.len() <= 96
        && asset.size_bytes > 0
        && asset.sha256.len() == 32
        && valid_token(&asset.camera_uid, 96)
        && asset.display_name.len() <= 160)
        .then_some(())
        .ok_or("media asset reference is invalid")
}

pub fn validate_media_asset_item(item: &media::MediaAssetItem) -> Result<(), &'static str> {
    let asset = item.asset.as_ref().ok_or("media asset item is invalid")?;
    validate_media_asset_ref(asset).map_err(|_| "media asset item is invalid")?;
    if asset.kind == media::MediaAssetKind::MediaThumbnail as i32
        || item.width > MEDIA_MAX_DIMENSION_PIXELS
        || item.height > MEDIA_MAX_DIMENSION_PIXELS
    {
        return Err("media asset item is invalid");
    }
    if let Some(thumbnail) = &item.thumbnail {
        validate_media_asset_ref(thumbnail)
            .map_err(|_| "media asset thumbnail relation is invalid")?;
        if thumbnail.kind != media::MediaAssetKind::MediaThumbnail as i32
            || thumbnail.camera_uid != asset.camera_uid
            || thumbnail.asset_id == asset.asset_id
        {
            return Err("media asset thumbnail relation is invalid");
        }
    }
    Ok(())
}

pub fn validate_media_asset_list_request(
    request: &media::MediaAssetListRequest,
) -> Result<(), &'static str> {
    use std::collections::HashSet;

    if (!request.camera_uid.is_empty() && !valid_token(&request.camera_uid, 96))
        || request.page_size == 0
        || request.page_size as usize > MEDIA_MAX_ASSET_PAGE_SIZE
        || !valid_page_token(&request.page_token)
        || (request.created_after_ns != 0
            && request.created_before_ns != 0
            && request.created_after_ns > request.created_before_ns)
    {
        return Err("media asset list request is invalid");
    }
    let mut kinds = HashSet::new();
    for kind in &request.kinds {
        if !matches!(
            media::MediaAssetKind::try_from(*kind),
            Ok(media::MediaAssetKind::MediaPhoto | media::MediaAssetKind::MediaVideo)
        ) || !kinds.insert(*kind)
        {
            return Err("media asset list kind is invalid");
        }
    }
    Ok(())
}

pub fn validate_media_asset_list_response(
    response: &media::MediaAssetListResponse,
) -> Result<(), &'static str> {
    use std::collections::HashSet;

    let error = media::MediaError::try_from(response.error)
        .map_err(|_| "media asset list response is invalid")?;
    if error == media::MediaError::Unspecified
        || response.message.len() > 256
        || response.items.len() > MEDIA_MAX_ASSET_PAGE_SIZE
        || !valid_page_token(&response.next_page_token)
    {
        return Err("media asset list response is invalid");
    }
    if error != media::MediaError::MediaOk
        && (!response.items.is_empty() || !response.next_page_token.is_empty())
    {
        return Err("failed media asset list response contains data");
    }
    let mut asset_ids = HashSet::new();
    for item in &response.items {
        validate_media_asset_item(item)
            .map_err(|_| "media asset list response contains duplicate or invalid asset")?;
        if !asset_ids.insert(item.asset.as_ref().unwrap().asset_id.as_str()) {
            return Err("media asset list response contains duplicate or invalid asset");
        }
    }
    Ok(())
}

pub fn validate_media_asset_chunk(
    chunk: &media::MediaAssetChunkResponse,
) -> Result<(), &'static str> {
    let error = media::MediaError::try_from(chunk.error).map_err(|_| "media error is invalid")?;
    if error == media::MediaError::Unspecified {
        return Err("media error is invalid");
    }
    (chunk.message.len() <= 256
        && chunk.data.len() <= MEDIA_MAX_CHUNK_BYTES
        && (chunk.transfer_id.is_empty() || valid_token(&chunk.transfer_id, 128))
        && (error != media::MediaError::MediaOk
            || (!chunk.transfer_id.is_empty() && (!chunk.data.is_empty() || chunk.eof))))
        .then_some(())
        .ok_or("media asset chunk is invalid")
}

pub fn validate_media_file_entry(entry: &media::MediaFileEntry) -> Result<(), &'static str> {
    let directory = entry.entry_type == media::MediaFileEntryType::MediaDirectory as i32;
    (valid_token(&entry.file_id, 128)
        && matches!(
            media::MediaFileEntryType::try_from(entry.entry_type),
            Ok(media::MediaFileEntryType::MediaFile | media::MediaFileEntryType::MediaDirectory)
        )
        && !entry.storage_id.is_empty()
        && entry.storage_id.len() <= 64
        && !entry.relative_path.is_empty()
        && entry.relative_path.len() <= 1024
        && !entry
            .relative_path
            .bytes()
            .any(|byte| byte < 0x20 || byte == 0x7f)
        && !entry.name.is_empty()
        && entry.name.len() <= 160
        && !entry.name.bytes().any(|byte| byte < 0x20 || byte == 0x7f)
        && entry.mime_type.len() <= 96
        && (directory
            || (entry.size_bytes > 0 && entry.sha256.len() == 32))
        && (!directory
            || (entry.mime_type.is_empty() && entry.size_bytes == 0 && entry.sha256.is_empty())))
        .then_some(())
        .ok_or("media file entry is invalid")
}

pub fn validate_media_file_list_request(
    request: &media::MediaFileListRequest,
) -> Result<(), &'static str> {
    ( !request.storage_id.is_empty()
        && request.storage_id.len() <= 64
        && request.path_prefix.len() <= 1024
        && !request.path_prefix.bytes().any(|byte| byte < 0x20 || byte == 0x7f)
        && request.page_size > 0
        && request.page_size as usize <= 256
        && valid_page_token(&request.page_token))
        .then_some(())
        .ok_or("media file list request is invalid")
}

pub fn validate_media_file_list_response(
    response: &media::MediaFileListResponse,
) -> Result<(), &'static str> {
    let error = media::MediaError::try_from(response.error).map_err(|_| "media file list response is invalid")?;
    if error == media::MediaError::Unspecified
        || response.message.len() > 256
        || response.entries.len() > 256
        || !valid_page_token(&response.next_page_token)
    {
        return Err("media file list response is invalid");
    }
    if error != media::MediaError::MediaOk
        && (!response.entries.is_empty() || !response.next_page_token.is_empty())
    {
        return Err("failed media file list response contains data");
    }
    let mut ids = std::collections::HashSet::new();
    for entry in &response.entries {
        validate_media_file_entry(entry)?;
        if !ids.insert(entry.file_id.as_str()) {
            return Err("media file list response contains duplicate entry");
        }
    }
    Ok(())
}

pub fn validate_media_file_chunk(
    chunk: &media::MediaFileChunkResponse,
) -> Result<(), &'static str> {
    validate_media_asset_chunk(&media::MediaAssetChunkResponse {
        error: chunk.error,
        message: chunk.message.clone(),
        transfer_id: chunk.transfer_id.clone(),
        offset: chunk.offset,
        data: chunk.data.clone(),
        eof: chunk.eof,
    })
}
