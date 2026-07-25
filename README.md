# YunLink

YunLink 2.0 is a provider-neutral communication runtime. It supplies discovery,
sessions, UID routing, authority, typed streams, actions, RPC, configuration,
logs, and bulk-transfer coordination without depending on a robotics framework
or a product data model.

## Architecture

YunLink is split into three boundaries:

1. **Core** owns the Wire v2 envelope, transport, session negotiation, routing,
   authority, and generic message families. Core is C++17 and does not depend on
   Protobuf or any product Profile.
2. **Profiles** define optional typed payloads. This repository currently ships
   `org.yunlink.mobility@1.0` and `com.yundrone.sunray@1.0`. Profiles are built
   only with `YUNLINK_BUILD_PROFILES=ON`.
3. **Adapters** live in the integrating product. They translate external
   framework data into Profile payloads or framework-neutral Stream samples.

Core treats Profile payloads as `TypeRef + bytes`. A Profile may describe a
domain, but it must not make Core depend on that domain or on its source
middleware.

See [docs/architecture.md](docs/architecture.md) for the dependency rules.

## Wire v2

Wire v2 is intentionally incompatible with Wire v1:

- `protocol_major=2`, `header_version=2`, `schema_version=2`
- UID source and target routing, with endpoint, entity, group, and broadcast
- Profile negotiation by `profile_id`, major, minor, and schema digest
- nine generic families: Session, Authority, EntityDirectory, Stream, Action,
  RPC, Configuration, Log, and Bulk
- typed payloads identified by `TypeRef`; Core never interprets Profile bytes
- action lifecycle: received, accepted, running, succeeded, failed, cancelled,
  expired

The public C++ entrypoint is `include/yunlink/yunlink.hpp`. The stable ABI 2
entrypoint is `include/yunlink/c/yunlink_c.h`.

## Build

Core only:

```bash
git submodule update --init --recursive
cmake -S . -B build/core -DYUNLINK_BUILD_PROFILES=OFF -DYUNLINK_BUILD_TESTS=ON
cmake --build build/core -j
ctest --test-dir build/core --output-on-failure
```

Core plus the optional Protobuf Profiles:

```bash
cmake -S . -B build/profiles -DYUNLINK_BUILD_PROFILES=ON -DYUNLINK_BUILD_TESTS=ON
cmake --build build/profiles -j
ctest --test-dir build/profiles --output-on-failure
```

Bindings:

```bash
cargo test --workspace --manifest-path bindings/rust/Cargo.toml
tools/bindings/run_all.sh
```

## Repository Map

- `include/yunlink/core/`: Wire v2 types and deterministic Core payload codecs
- `include/yunlink/runtime/`: provider-neutral runtime facade
- `include/yunlink/c/`: ABI 2 callback/view contract
- `src/`: Core, runtime, discovery, and ABI implementation
- `profiles/`: optional Protobuf schemas and C++ generated targets
- `bindings/rust/`: raw ABI, owned Rust facade, and generated Profiles
- `bindings/python/`: owned Python facade and generated Profiles
- `tests/`: Wire v2, runtime, ABI, discovery, and architecture boundaries
- `docs/`: maintained v2 architecture and integration documents

## Compatibility

Version 2.0.0 is a hard major-version cut. Wire v1, numeric vehicle routing,
fixed vehicle command/state unions, and the old C/Rust/Python APIs are not part
of the v2 public contract. Applications must migrate both endpoints together.
