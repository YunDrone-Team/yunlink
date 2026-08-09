# org.yunlink.media@1

This optional Profile carries camera control, RTSP start/stop RPCs, a device
media catalog, and Bulk file transfer metadata. It does not carry video frames.
RTSP/H.264 remains the live data plane; YunLink only transports the URL returned
by the camera provider.

## RTSP live source

`CameraStartRtspRequest` and `CameraStopRtspRequest` directly mirror the device
camera-management operations. A successful start returns a non-empty
`rtsp_url`. YunLink validates only the payload size and the absence of control
characters; it does not parse, normalize, redact, or reconstruct the URL. The
Bridge must copy the provider response byte-for-byte, including userinfo, port,
path, query, and escaping.

There is no transport-level viewer lease or renewal RPC. Source reachability,
idempotent start/stop behavior, and concurrent viewer handling belong to the
camera provider. Clients keep credentials private and must not place the full
URL in logs or UI state.

`CameraDescriptor` reports `live_view_active`, `live_view_autostart`, and the
provider-owned `rtsp_url`. An active source must have a non-empty URL. A client
may consume that URL without calling Start. Clients must not stop an autostart
source when their local viewer closes. This supports device boot-time autostart
without turning a read-only viewer into the owner of the source.

Catalog, media-list, RTSP start/stop, and Bulk-open requests are read/view
operations. They do not grant media-control authority, but remain limited to an
authenticated attached Session and the entity visibility policy enforced by
the Bridge. `CameraTakePhoto`, `CameraStartRecording`, and
`CameraStopRecording` remain media-control operations and require
`org.yunlink.media` authority.

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

The shared deterministic catalog and RTSP vectors are in
[`v1/golden/media-library-v1-vectors.txt`](v1/golden/media-library-v1-vectors.txt).
