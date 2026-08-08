# org.yunlink.media@1

This optional Profile carries camera control, live-view lease control, a
device media catalog, and Bulk file transfer metadata. It does not carry video
frames. RTSP/H.264 remains the live data plane; YunLink carries only the lease
and endpoint information needed to open and close it.

## Media catalog

`MediaAssetListRequest` is a read-only, cursor-paged query. `page_size` is
between 1 and 100. An empty `camera_uid` means all cameras. `page_token` is an
opaque URL-safe token and must not be interpreted by GCS. Results are ordered
by the device catalog and return `next_page_token` for the next page.

Each `MediaAssetItem` contains the primary photo or video and, when available,
an explicitly related thumbnail. A thumbnail must have kind `MEDIA_THUMBNAIL`
and the same `camera_uid`; clients must never guess a thumbnail by camera or
filename.

`asset_id` is stable across Bridge restart and YunLink reconnect. The catalog
is read-only and does not grant permission to download an asset. Bulk open
re-checks the entity, session, path, size, and SHA-256 before sending bytes.

The shared deterministic vectors are in
[`v1/golden/media-library-v1-vectors.txt`](v1/golden/media-library-v1-vectors.txt).
