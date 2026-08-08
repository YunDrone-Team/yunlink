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

pub fn validate_media_endpoint_descriptor(
    endpoint: &media::MediaEndpointDescriptor,
) -> Result<(), &'static str> {
    let uri = endpoint.uri.as_str();
    let rest = uri
        .strip_prefix("rtsp://")
        .ok_or("media endpoint descriptor is invalid")?;
    let (authority, path) = rest
        .split_once('/')
        .ok_or("media endpoint descriptor is invalid")?;
    let (host, port) = authority
        .rsplit_once(':')
        .ok_or("media endpoint descriptor is invalid")?;
    let port = port
        .parse::<u16>()
        .map_err(|_| "media endpoint descriptor is invalid")?;
    let valid_host = if host.starts_with('[') && host.ends_with(']') {
        host.len() > 2
            && host[1..host.len() - 1]
                .bytes()
                .all(|byte| byte.is_ascii_hexdigit() || matches!(byte, b':' | b'.'))
    } else {
        host.bytes()
            .all(|byte| byte.is_ascii_alphanumeric() || matches!(byte, b'.' | b'-'))
    };
    (endpoint.protocol == "rtsp"
        && !host.is_empty()
        && valid_host
        && port > 0
        && !path.is_empty()
        && uri.len() <= MEDIA_MAX_SOURCE_URI_BYTES
        && !uri.contains('@')
        && !uri.contains('#')
        && !uri.bytes().any(|byte| byte < 0x20 || byte == 0x7f)
        && endpoint.username.len() <= 256
        && endpoint.password.len() <= 256
        && !endpoint
            .username
            .bytes()
            .any(|byte| byte < 0x20 || byte == 0x7f)
        && !endpoint
            .password
            .bytes()
            .any(|byte| byte < 0x20 || byte == 0x7f))
    .then_some(())
    .ok_or("media endpoint descriptor is invalid")
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
