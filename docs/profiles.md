# Profiles

Profiles are optional typed contracts carried by YunLink Core. They are not
part of the Core library target and are disabled by default.

## Included Profiles

`org.yunlink.mobility@1.0` defines geometry, odometry, goto, velocity, and
trajectory payloads.

`com.yundrone.sunray@1.0` defines Sunray telemetry, vehicle actions, formation
actions, diagnostics, events, and Feature RPC payloads. It imports the Mobility
Profile for common geometry.

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
