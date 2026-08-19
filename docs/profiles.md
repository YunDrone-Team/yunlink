# Profiles

Profiles are optional typed contracts carried by YunLink Core. They are not
part of the Core library target and are disabled by default.

## Included Profiles

`org.yunlink.mobility@1.0` defines geometry, odometry, goto, velocity, and
trajectory payloads.

`com.yundrone.sunray@2.6` defines Sunray telemetry, typed UAV and UGV actions,
formation actions, diagnostics, events, and Feature RPC payloads. It imports
the Mobility Profile for common geometry. The UGV single-vehicle action and
control-state contract is specified in
[`profiles/com.yundrone.sunray-v2-ugv-control.md`](profiles/com.yundrone.sunray-v2-ugv-control.md).

`org.yunlink.visual@1` defines a neutral StreamSample contract for bounded
point-cloud, marker-array, and image data. Its complete wire, metadata, and
golden-vector specification is in
[`profiles/org.yunlink.visual-v1.md`](profiles/org.yunlink.visual-v1.md).

Sunray's registered `SummarySnapshot` metric keys, their quality semantics,
and the boundary between the display summary and high-rate flight state are in
[`profiles/com.yundrone.sunray-v2-summary-metrics.md`](profiles/com.yundrone.sunray-v2-summary-metrics.md).

`org.yunlink.media@1` defines camera control, live-view leases, a cursor-paged
media catalog, and Bulk file metadata. RTSP/H.264 frames remain outside
YunLink; see [`profiles/org.yunlink.media-v1.md`](../profiles/org.yunlink.media-v1.md)
for the catalog and thumbnail relationship rules.

Both schemas use proto3 syntax compatible with Protobuf 3.6 and avoid newer
optional-field syntax.

## Build And Generated Code

C++ Profile code is generated only when `YUNLINK_BUILD_PROFILES=ON`. Rust uses
`prost` with a vendored `protoc`. Python ships generated modules under
`yunlink.profiles`.

Core does not link the generated targets. Applications explicitly link or
import the Profiles they understand, encode the message, then publish it with
the matching `TypeRef`.

Profile schema files must not describe their source middleware. Field names and
semantics represent the domain value on the network, regardless of whether an
adapter obtained it from a local bus, a file, a simulator, or another API.
