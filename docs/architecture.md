# Architecture Boundary

YunLink is a generic communication platform. Product and middleware adapters
depend on YunLink; YunLink does not depend on them.

```text
external framework <-> product adapter <-> YunLink Core <-> network
                               |               ^
                               +-> Profiles ---+
```

The dependency rules are:

- Core may depend only on the standard toolchain and standalone Asio.
- Core does not depend on Protobuf or any Profile target.
- Profiles contain domain schemas and may depend on other Profiles.
- Profiles do not contain middleware headers, serialization layouts, hashes, or
  message definitions. Source identifiers remain opaque descriptor metadata.
- Adapters own all middleware APIs and translate before calling YunLink.
- Consumers receive Profile payloads or self-describing generic Stream data;
  they do not decode the source middleware wire format.

For a dynamically discovered publish/subscribe source, the adapter must emit a
framework-neutral stream descriptor and payload. A suitable contract is:

```text
stream_uid: stable UID chosen by the adapter
encoding: application/json
metadata:
  source.kind: pubsub
  source.system: source adapter system, for example ros1, dds, or mavlink
  source.name: external source name
  source.type: external source type identifier
  source.definition_digest: optional stable digest of the source definition
  stream.label: human-readable label, normally source.name
  schema.name: YunLink payload schema label
data: bounded UTF-8 JSON
```

Known product telemetry and controls should use a Profile Protobuf message.
Unknown or operator-selected source data may use the generic Stream contract.
Raw external serialization must terminate inside the adapter.

`source.*` values identify an adapter-owned external source; they never ask a
YunLink consumer to deserialize the source middleware wire format. Consumers
must use `stream_uid` for subscriptions and sample matching, and use source
metadata only for operator-facing identification.

Architecture tests enforce these rules for Core, common bindings, and Profiles.
