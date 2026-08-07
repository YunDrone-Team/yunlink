# `org.yunlink.visual@1`

`org.yunlink.visual@1` transports bounded, read-only visual samples through the
generic Wire v2 `StreamSample` envelope. It deliberately does not contain ROS
message definitions, ROS topic names, TF graph data, package paths, or renderer
implementation details.

## Catalog contract

Each visual stream has a stable opaque `stream_uid`. Entity-owned streams must
declare `owner.entity_uid`; an Endpoint-owned visual stream must not be treated
as vehicle scene data by a client.

Required metadata for scene data:

```text
owner.entity_uid
visual.kind = point_cloud | marker_array | image
visual.role = scene.markers | scene.point_cloud.world | scene.point_cloud.sensor
visual.reference_frame_policy = entity_controller_frame
stream.max_rate_hz
stream.max_payload_bytes
stream.session_bandwidth_bytes_per_sec
```

The `stream.label` is display-only. It must not contain a ROS topic path and is
not a routing key.

Samples are only valid for an authenticated session that has negotiated this
profile and has attached the owner entity. Visual streams are read-only and do
not require Authority. Implementations must use bounded, latest-only queues;
dropping a visual sample must never delay a control message.

## Common sample rules

- `StreamSample.source_timestamp_ns` is the upstream sample timestamp when it
  exists, otherwise `0`.
- `sequence` is monotonically increasing per `stream_uid` within one Bridge
  process lifetime. A client must discard a sequence rollback until it receives
  a newer Stream Catalog revision or reconnects.
- A sample larger than the catalog or negotiated payload limit is rejected.
- All numeric values in JSON and binary floats must be finite. `NaN` and
  infinities are invalid; producers must drop invalid points rather than send
  them.
- Metadata strings are UTF-8 and bounded to 256 bytes. Stream metadata must not
  expose host paths or untrusted resource URIs.

## Point cloud: `YLPC v1`

Encoding:

```text
application/vnd.yunlink.pointcloud.xyz-i-f32-le;version=1
```

The payload is little-endian:

| Offset | Field | Type |
| --- | --- | --- |
| 0 | magic `YLPC` | 4 bytes |
| 4 | version | `u16`, value `1` |
| 6 | flags | `u16`, bit 0 means intensity is present |
| 8 | point count | `u32` |
| 12 | stride | `u32`, value `16` |
| 16 | points | `count * 16` bytes |

Each point is `[x, y, z, intensity]` in `f32`. If bit 0 is clear, intensity is
written as `0`; the fixed stride keeps GPU upload simple. The declared count and
payload length must match exactly. A decoder must reject unknown flags, an
unsupported version, a non-16 stride, truncated data, trailing data, or
non-finite coordinates.

Required sample metadata:

```text
visual.kind = point_cloud
visual.format = xyz_i_f32_le
visual.point_count
visual.source_frame
visual.reference_frame
visual.reference_from_source.translation_xyz = [x,y,z]
visual.reference_from_source.rotation_xyzw = [x,y,z,w]
```

The matrix semantics are `T(reference <- source)`. Point bytes remain in the
source frame. A client applies the one transform to its render group, never a
per-point JavaScript coordinate conversion.

## Marker array JSON v1

Encoding:

```text
application/vnd.yunlink.scene.marker-array+json;version=1
```

The root object contains `version: 1`, `reference_frame`, and `markers`. Marker
strings are UTF-8. `ADD`/`MODIFY` markers include a finite pose, scale, color,
`source_stamp_ns`, `lifetime_ns`, and `frame_locked`; their `frame_id` is the
normalized reference frame. `points` are local marker geometry and are not
pre-transformed. `DELETE` only needs `namespace`, `id`, and `action`; `DELETEALL`
clears only the current `(owner entity, stream_uid)` scope.

Marker types use the ROS numeric values for compatibility with existing
operators. This profile explicitly supports CUBE, SPHERE, CYLINDER, ARROW,
LINE_STRIP, LINE_LIST, CUBE_LIST, SPHERE_LIST, POINTS, TEXT_VIEW_FACING,
MESH_RESOURCE, and TRIANGLE_LIST. Unknown marker types are valid transport data
but clients render a diagnostic placeholder.

`mesh_resource` is never forwarded. Producers may emit `mesh.policy = none` or
`mesh.policy = builtin` with a stable `mesh.asset_id`. `package://`, `file://`,
and arbitrary HTTP(S) URIs are not load instructions and must not be exposed to
clients.

## Image

Encoding:

```text
application/vnd.yunlink.image.raw;version=1
```

Required metadata is `visual.kind = image`, width, height, step, pixel format,
source frame, reference frame, and `T(reference <- source)`. The byte layout is
the declared pixel format; unknown formats are transport-valid but require a
client diagnostic rather than a guessed decoder.

## Golden vectors

Language bindings and adapters consume the shared vectors in
`profiles/org.yunlink.visual/v1/golden/visual-v1-vectors.txt`. They cover a
valid two-point cloud, malformed point-cloud headers, marker add/delete cases,
an invalid quaternion, and a tiny raw image. The vectors define the contract;
they do not encode ROS serialization.
