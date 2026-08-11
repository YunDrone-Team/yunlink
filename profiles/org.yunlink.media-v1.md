# org.yunlink.media@1

This optional Profile carries camera control, RTSP view-session RPCs, a device
media catalog, and Bulk file transfer metadata. It does not carry video frames.
RTSP/H.264 remains the live data plane; YunLink only transports the URL returned
by the camera provider.

## RTSP view session

`CameraStartRtspRequest` opens an idempotent viewing record scoped to the
authenticated peer, Session, entity, and camera. It does not start or own the
device source. Before returning success, the Bridge refreshes the camera list
and requires an online camera whose provider reports `rtsp_running=true` and a
valid `rtsp://` URL. A successful response returns that provider-owned URL.

`CameraStopRtspRequest` closes only the caller's viewing record. It is
idempotent and must never stop or reconfigure the device RTSP source. Detach,
Session disconnect, and entity removal release the affected viewing records.
Other Sessions and viewers remain unaffected.

`CameraDescriptor.live_view_supported` and `live_view_active` are true only
when the camera is online and the provider reports an active, valid RTSP
source. `live_view_control_supported` and `live_view_autostart` are false in
this contract: they remain on the wire for schema stability, not as device
control signals.

There is no transport-level lease renewal RPC. The Bridge owns only Session
bookkeeping, while source lifecycle and concurrent RTSP consumers belong to
the camera provider. Clients keep the URL and any embedded credentials in a
private backend and must not place them in logs, UI state, or snapshots.

Catalog, media-list, RTSP view-session, and Bulk-open requests are read/view
operations. They do not grant media-control authority, but remain limited to an
authenticated attached Session and the entity visibility policy enforced by
the Bridge. `CameraTakePhoto`, `CameraStartRecording`, and
`CameraStopRecording` remain media-control operations and require
`org.yunlink.media` authority.

Endpoints implementing this behavior advertise `media-live-view-session-v1`.
The retired `media-live-rtsp-url-v1` capability must not be advertised.

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
