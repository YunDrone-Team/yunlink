#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "org.yunlink.media/v1/media.pb.h"

namespace org::yunlink::media::v1 {

constexpr std::size_t kMediaMaxCameras = 32;
constexpr std::size_t kMediaMaxCameraUidBytes = 96;
constexpr std::size_t kMediaMaxMessageBytes = 256;
constexpr std::size_t kMediaMaxAssetIdBytes = 128;
constexpr std::size_t kMediaMaxTransferIdBytes = 128;
constexpr std::size_t kMediaMaxChunkBytes = 256 * 1024;
constexpr std::size_t kMediaSha256Bytes = 32;
constexpr std::size_t kMediaMaxSourceUriBytes = 2048;
constexpr std::size_t kMediaMaxAssetPageSize = 100;
constexpr std::size_t kMediaMaxPageTokenBytes = 512;
constexpr std::uint32_t kMediaMaxDimensionPixels = 32768;
constexpr std::size_t kMediaMaxStorageIdBytes = 64;
constexpr std::size_t kMediaMaxRelativePathBytes = 1024;
constexpr std::size_t kMediaMaxFileEntries = 256;

bool validate_camera_descriptor(const CameraDescriptor& camera, std::string* error = nullptr);
bool validate_camera_catalog_snapshot(const CameraCatalogSnapshot& snapshot,
                                      std::string* error = nullptr);
bool validate_camera_start_rtsp_response(const CameraStartRtspResponse& response,
                                         std::string* error = nullptr);
bool validate_media_asset_ref(const MediaAssetRef& asset, std::string* error = nullptr);
bool validate_media_asset_item(const MediaAssetItem& item, std::string* error = nullptr);
bool validate_media_asset_list_request(const MediaAssetListRequest& request,
                                       std::string* error = nullptr);
bool validate_media_asset_list_response(const MediaAssetListResponse& response,
                                        std::string* error = nullptr);
bool validate_camera_request(const std::string& camera_uid, std::string* error = nullptr);
bool validate_media_asset_chunk(const MediaAssetChunkResponse& chunk, std::string* error = nullptr);
bool validate_media_file_entry(const MediaFileEntry& entry, std::string* error = nullptr);
bool validate_media_file_list_request(const MediaFileListRequest& request,
                                      std::string* error = nullptr);
bool validate_media_file_list_response(const MediaFileListResponse& response,
                                       std::string* error = nullptr);
bool validate_media_file_chunk(const MediaFileChunkResponse& chunk, std::string* error = nullptr);

}  // namespace org::yunlink::media::v1
