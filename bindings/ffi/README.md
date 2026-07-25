# C ABI 2

The stable entrypoint is `include/yunlink/c/yunlink_c.h`. ABI 2 exposes the
generic runtime, UID targets, Profile negotiation, `TypeRef`, and opaque byte
payloads.

String, target, type, and payload views received by a callback are valid only
for the callback duration. Language bindings must copy them before returning.
The shared library uses `SOVERSION=2`.
