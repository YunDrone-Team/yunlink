# Wire v2 Contract

Every frame uses `protocol_major=2`, `header_version=2`, and
`schema_version=2`. A receiver rejects other protocol or header versions.

## Envelope

An envelope carries:

- family and operation
- QoS, creation time, TTL, flags, checksum, and optional security tag
- session, message, and correlation IDs
- source `{endpoint_uid, entity_uid}`
- target `{scope, uids}`
- `TypeRef {profile_id, major, minor, type_name}`
- opaque payload bytes

UIDs are 1-128 ASCII characters from `[A-Za-z0-9._:-]`. Target scopes are
endpoint, entity, group, and broadcast. Broadcast has no UID list; all other
scopes require one or more UIDs.

## Families

| ID | Family | Purpose |
| --- | --- | --- |
| 1 | Session | hello, authentication, Profile negotiation, ready |
| 2 | Authority | claim, renew, release, status |
| 3 | EntityDirectory | list, changes, attach, detach |
| 4 | Stream | catalog, subscribe, sample, unsubscribe |
| 5 | Action | goal, update, cancel |
| 6 | RPC | typed request and response |
| 7 | Configuration | list, describe, get, patch, apply |
| 8 | Log | list and read |
| 9 | Bulk | open, chunk, close, status |

QoS policy keys are `(family, profile_id, type_name)`. Profile payload meaning
is never inferred from a transport connection or a numeric vehicle type.

## Session Profiles

Profiles negotiate on `{profile_id, major, minor, schema_digest}`. Matching
major versions select the lower minor. Equal versions with different digests
reject that Profile. An unsupported optional Profile does not fail the Core
session; a missing required Profile prevents the session from becoming usable.

## Actions

Core checks session, target, TTL, and authority scope. A Profile handler checks
domain payload fields. Updates use the lifecycle `received -> accepted ->
running -> succeeded|failed|cancelled|expired` and correlate to the goal's
message ID.
