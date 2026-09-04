#include "org.yunlink.media/v1/media_validation.hpp"

#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace org::yunlink::media::v1 {
namespace {

bool fail(const char* message, std::string* error) {
    if (error != nullptr) {
        *error = message;
    }
    return false;
}

bool ascii_alphanumeric(unsigned char character) {
    return (character >= 'a' && character <= 'z') ||
           (character >= 'A' && character <= 'Z') ||
           (character >= '0' && character <= '9');
}

bool valid_token(const std::string& value, std::size_t max_bytes) {
    if (value.empty() || value.size() > max_bytes) {
        return false;
    }
    for (const unsigned char character : value) {
        if (!(ascii_alphanumeric(character) || character == '-' || character == '_' ||
              character == '.')) {
            return false;
        }
    }
    return true;
}

bool valid_page_token(const std::string& value) {
    return value.size() <= kMediaMaxPageTokenBytes &&
           std::all_of(value.begin(), value.end(), [](const unsigned char character) {
               return ascii_alphanumeric(character) || character == '-' || character == '_' ||
                      character == '=';
           });
}

bool valid_error(MediaError error) {
    return error >= MEDIA_OK && error <= MEDIA_INTEGRITY_ERROR;
}

bool contains_control(const std::string& value) {
    return std::any_of(value.begin(), value.end(), [](const unsigned char character) {
        return character < 0x20 || character == 0x7f;
    });
}

bool valid_relative_path(const std::string& value, bool allow_empty) {
    if (value.empty()) return allow_empty;
    if (value.front() == '/' || value.find('\\') != std::string::npos) return false;
    size_t start = 0;
    while (start <= value.size()) {
        const size_t end = value.find('/', start);
        const auto segment = value.substr(start, end == std::string::npos ? std::string::npos : end - start);
        if (segment.empty() || segment == "." || segment == "..") return false;
        if (end == std::string::npos) break;
        start = end + 1;
    }
    return true;
}

}  // namespace

bool validate_camera_request(const std::string& camera_uid, std::string* error) {
    if (!valid_token(camera_uid, kMediaMaxCameraUidBytes)) {
        return fail("camera uid is invalid", error);
    }
    if (error != nullptr) {
        error->clear();
    }
    return true;
}

bool validate_camera_descriptor(const CameraDescriptor& camera, std::string* error) {
    if (!valid_token(camera.camera_uid(), kMediaMaxCameraUidBytes)) {
        return fail("camera uid is invalid", error);
    }
    if (camera.name().size() > 128 || camera.image_topic().size() > 256 ||
        camera.camera_info_topic().size() > 256 || camera.encoding().size() > 64 ||
        camera.error_message().size() > kMediaMaxMessageBytes ||
        camera.rtsp_url().size() > kMediaMaxSourceUriBytes ||
        contains_control(camera.rtsp_url()) ||
        (camera.live_view_active() &&
         (!camera.live_view_supported() || camera.rtsp_url().empty())) ||
        (!camera.live_view_supported() &&
         (camera.live_view_control_supported() || !camera.rtsp_url().empty() ||
          camera.live_view_autostart())) ||
        !std::isfinite(camera.frame_rate_hz()) || camera.frame_rate_hz() < 0.0) {
        return fail("camera descriptor is invalid", error);
    }
    if (error != nullptr) {
        error->clear();
    }
    return true;
}

bool validate_camera_catalog_snapshot(const CameraCatalogSnapshot& snapshot, std::string* error) {
    if (snapshot.cameras_size() > static_cast<int>(kMediaMaxCameras)) {
        return fail("camera catalog is invalid", error);
    }
    std::unordered_set<std::string> camera_uids;
    for (const auto& camera : snapshot.cameras()) {
        if (!validate_camera_descriptor(camera, error)) {
            return false;
        }
        if (!camera_uids.insert(camera.camera_uid()).second) {
            return fail("duplicate camera uid", error);
        }
    }
    if (error != nullptr) {
        error->clear();
    }
    return true;
}

bool validate_camera_start_rtsp_response(const CameraStartRtspResponse& response,
                                         std::string* error) {
    if (!valid_error(response.error()) || response.message().size() > kMediaMaxMessageBytes ||
        response.rtsp_url().size() > kMediaMaxSourceUriBytes ||
        contains_control(response.rtsp_url()) ||
        (response.error() == MEDIA_OK && response.rtsp_url().empty()) ||
        (response.error() != MEDIA_OK && !response.rtsp_url().empty())) {
        return fail("camera start RTSP response is invalid", error);
    }
    if (error != nullptr) {
        error->clear();
    }
    return true;
}

bool validate_media_asset_ref(const MediaAssetRef& asset, std::string* error) {
    if (!valid_token(asset.asset_id(), kMediaMaxAssetIdBytes) || asset.kind() < MEDIA_PHOTO ||
        asset.kind() > MEDIA_VIDEO || asset.mime_type().empty() || asset.mime_type().size() > 96 ||
        asset.size_bytes() == 0 || asset.sha256().size() != kMediaSha256Bytes ||
        !valid_token(asset.camera_uid(), kMediaMaxCameraUidBytes) ||
        asset.display_name().size() > 160) {
        return fail("media asset reference is invalid", error);
    }
    if (error != nullptr) {
        error->clear();
    }
    return true;
}

bool validate_media_asset_item(const MediaAssetItem& item, std::string* error) {
    if (!item.has_asset() || !validate_media_asset_ref(item.asset(), error) ||
        item.asset().kind() == MEDIA_THUMBNAIL || item.width() > kMediaMaxDimensionPixels ||
        item.height() > kMediaMaxDimensionPixels) {
        return fail("media asset item is invalid", error);
    }
    if (item.has_thumbnail()) {
        if (!validate_media_asset_ref(item.thumbnail(), error) ||
            item.thumbnail().kind() != MEDIA_THUMBNAIL ||
            item.thumbnail().camera_uid() != item.asset().camera_uid() ||
            item.thumbnail().asset_id() == item.asset().asset_id()) {
            return fail("media asset thumbnail relation is invalid", error);
        }
    }
    if (error != nullptr) {
        error->clear();
    }
    return true;
}

bool validate_media_asset_list_request(const MediaAssetListRequest& request, std::string* error) {
    if ((!request.camera_uid().empty() &&
         !valid_token(request.camera_uid(), kMediaMaxCameraUidBytes)) ||
        request.page_size() == 0 || request.page_size() > kMediaMaxAssetPageSize ||
        !valid_page_token(request.page_token()) ||
        (request.created_after_ns() != 0 && request.created_before_ns() != 0 &&
         request.created_after_ns() > request.created_before_ns())) {
        return fail("media asset list request is invalid", error);
    }
    std::unordered_set<int> kinds;
    for (const auto kind : request.kinds()) {
        if ((kind != MEDIA_PHOTO && kind != MEDIA_VIDEO) ||
            !kinds.insert(static_cast<int>(kind)).second) {
            return fail("media asset list kind is invalid", error);
        }
    }
    if (error != nullptr) {
        error->clear();
    }
    return true;
}

bool validate_media_asset_list_response(const MediaAssetListResponse& response,
                                        std::string* error) {
    if (!valid_error(response.error()) || response.message().size() > kMediaMaxMessageBytes ||
        response.items_size() > static_cast<int>(kMediaMaxAssetPageSize) ||
        !valid_page_token(response.next_page_token())) {
        return fail("media asset list response is invalid", error);
    }
    if (response.error() != MEDIA_OK &&
        (response.items_size() != 0 || !response.next_page_token().empty())) {
        return fail("failed media asset list response contains data", error);
    }
    std::unordered_set<std::string> asset_ids;
    for (const auto& item : response.items()) {
        if (!validate_media_asset_item(item, error) ||
            !asset_ids.insert(item.asset().asset_id()).second) {
            return fail("media asset list response contains duplicate or invalid asset", error);
        }
    }
    if (error != nullptr) {
        error->clear();
    }
    return true;
}

bool validate_media_asset_chunk(const MediaAssetChunkResponse& chunk, std::string* error) {
    if (!valid_error(chunk.error())) {
        return fail("media error is invalid", error);
    }
    if (chunk.message().size() > kMediaMaxMessageBytes ||
        chunk.data().size() > kMediaMaxChunkBytes ||
        (!chunk.transfer_id().empty() &&
         !valid_token(chunk.transfer_id(), kMediaMaxTransferIdBytes))) {
        return fail("media asset chunk is invalid", error);
    }
    if (chunk.error() == MEDIA_OK &&
        (chunk.transfer_id().empty() || (chunk.data().empty() && !chunk.eof()))) {
        return fail("empty non-final media chunk", error);
    }
    if (error != nullptr) {
        error->clear();
    }
    return true;
}

bool validate_media_file_entry(const MediaFileEntry& entry, std::string* error) {
    const bool directory = entry.entry_type() == MEDIA_DIRECTORY;
    if (!valid_token(entry.file_id(), kMediaMaxAssetIdBytes) ||
        (entry.entry_type() != MEDIA_FILE && !directory) ||
        entry.storage_id().empty() || entry.storage_id().size() > kMediaMaxStorageIdBytes ||
        entry.relative_path().empty() || entry.relative_path().size() > kMediaMaxRelativePathBytes ||
        entry.name().empty() || entry.name().size() > 160 || contains_control(entry.relative_path()) ||
        !valid_relative_path(entry.relative_path(), false) ||
        contains_control(entry.name()) || (!directory && entry.size_bytes() == 0) ||
        (!directory && entry.sha256().size() != kMediaSha256Bytes) ||
        (directory && (!entry.mime_type().empty() || entry.size_bytes() != 0 || !entry.sha256().empty())) ||
        entry.mime_type().size() > 96) {
        return fail("media file entry is invalid", error);
    }
    if (error != nullptr) error->clear();
    return true;
}

bool validate_media_file_list_request(const MediaFileListRequest& request, std::string* error) {
    if (request.storage_id().empty() || request.storage_id().size() > kMediaMaxStorageIdBytes ||
        request.path_prefix().size() > kMediaMaxRelativePathBytes ||
        contains_control(request.path_prefix()) || !valid_relative_path(request.path_prefix(), true) || request.page_size() == 0 ||
        request.page_size() > kMediaMaxFileEntries || !valid_page_token(request.page_token())) {
        return fail("media file list request is invalid", error);
    }
    if (error != nullptr) error->clear();
    return true;
}

bool validate_media_file_list_response(const MediaFileListResponse& response, std::string* error) {
    if (!valid_error(response.error()) || response.message().size() > kMediaMaxMessageBytes ||
        response.entries_size() > static_cast<int>(kMediaMaxFileEntries) ||
        !valid_page_token(response.next_page_token())) {
        return fail("media file list response is invalid", error);
    }
    if (response.error() != MEDIA_OK &&
        (response.entries_size() != 0 || !response.next_page_token().empty())) {
        return fail("failed media file list response contains data", error);
    }
    std::unordered_set<std::string> ids;
    for (const auto& entry : response.entries()) {
        if (!validate_media_file_entry(entry, error) || !ids.insert(entry.file_id()).second) {
            return fail("media file list response contains duplicate or invalid entry", error);
        }
    }
    if (error != nullptr) error->clear();
    return true;
}

bool validate_media_file_chunk(const MediaFileChunkResponse& chunk, std::string* error) {
    if (!valid_error(chunk.error()) || chunk.message().size() > kMediaMaxMessageBytes ||
        chunk.data().size() > kMediaMaxChunkBytes ||
        (!chunk.transfer_id().empty() && !valid_token(chunk.transfer_id(), kMediaMaxTransferIdBytes)) ||
        (chunk.error() == MEDIA_OK &&
         (chunk.transfer_id().empty() || (chunk.data().empty() && !chunk.eof())))) {
        return fail("media file chunk is invalid", error);
    }
    if (error != nullptr) error->clear();
    return true;
}

}  // namespace org::yunlink::media::v1
