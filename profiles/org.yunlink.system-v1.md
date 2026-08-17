# org.yunlink.system/v1

Endpoint-scoped clock synchronization for an authenticated YunLink session.
Minor version 1 defines one operation, `ClockSyncRequest`/`ClockSyncResponse`.

- The target must be the current endpoint and the session must be authenticated.
- `unix_time_ms` is restricted to 2024-01-01 through 2100-01-01.
- `source` is a bounded ASCII token; GCS sends `sunray-gcs`.
- An `OK` response includes previous and applied UTC plus their delta. The applied UTC must be in the product range; the previous UTC may be untrusted because reporting that value is required when recovering a device from 1970 or another invalid wall clock. Failed responses contain no timestamp result.
- The endpoint may reject synchronization while a vehicle is armed or its control state is unknown.
- YunLink Core uses local monotonic clocks for deadlines and freshness. Wire wall-clock fields remain audit metadata only.

The deterministic request vector is in [`v1/golden/system-v1-vectors.txt`](org.yunlink.system/v1/golden/system-v1-vectors.txt).
