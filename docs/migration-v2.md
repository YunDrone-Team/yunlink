# Migration To Wire v2

Wire v2 is a coordinated breaking migration. It does not negotiate or decode
Wire v1.

Replace numeric agent and vehicle routing with endpoint, entity, and group
UIDs. Replace fixed command/state/system-service enums with a generic family,
operation, `TypeRef`, and payload. Move product messages to a Profile. Move
middleware serialization and lifecycle code to an external adapter.

Applications should migrate in this order:

1. update both endpoints to the ABI/runtime v2 API;
2. define stable endpoint and entity UIDs;
3. offer and require the Profiles needed by the workflow;
4. project typed product data through those Profiles;
5. project dynamic sources through generic self-describing Streams;
6. remove all v1 includes and language APIs from the application.

The repository intentionally does not ship legacy v1 headers, libraries,
examples, or compatibility bindings under the 2.0.0 release.
