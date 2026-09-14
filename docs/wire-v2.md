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

## QoS failure isolation

`BestEffort` and `Bulk` frames may be dropped when a send is congested or a
receive buffer overflows. Those drops must not close the TCP peer or the
session. Only `ReliableOrdered` (and a hard socket error) may tear down the
connection. Send loops write without holding the enqueue mutex so control
frames can still be queued while a large lossy frame is on the wire.

After `session.ready`, a client may open a second TCP connection to the same
listen port and send `session.lane_bind` (operation 5) with the existing
session id and shared secret. The server replies `session.lane_ready`
(operation 6). BestEffort/Bulk `publish` then uses that lossy lane. Closing the
lossy lane must not mark the session lost. Peers that ignore lane bind keep the
single-TCP Phase 0 path.

## Actions

Core checks session, target, TTL, and authority scope. A Profile handler checks
domain payload fields. Updates use the lifecycle `received -> accepted ->
running -> succeeded|failed|cancelled|expired` and correlate to the goal's
message ID.
