# Yunlink 测试真相矩阵

本表是仓库内唯一权威的测试真相表。每个测试期望必须落入以下状态之一：

- `covered-in-repo`
  已有自动化证据，能在本仓库内执行。
- `implemented-now`
  本轮补齐的新自动化或回归校验，能在本仓库内执行。
- `blocked-external`
  需要外部环境；repo 内已具备 suite 规格、产物约定和验收标准，但当前不宣称已验证。

`gate_tier` 只允许使用：`pr`、`ci`、`nightly-local`、`manual-external`、`release-external`。

## Repo-local Coverage

| domain | scenario | status | evidence | gate_tier | env_requirement |
| --- | --- | --- | --- | --- | --- |
| protocol | SecureEnvelope roundtrip / checksum / TTL codec | `covered-in-repo` | [test_protocol](../../tests/test_protocol.cpp) | `pr` | `repo-only` |
| protocol | header_len / payload_len / variable-header / checksum corruption | `implemented-now` | [test_protocol_corruption](../../tests/core/test_protocol_corruption.cpp) | `pr` | `repo-only` |
| protocol | parser partial / truncated / malformed / garbage recovery | `covered-in-repo` | [test_parser](../../tests/test_parser.cpp), [test_parser_resilience](../../tests/core/test_parser_resilience.cpp) | `pr` | `repo-only` |
| protocol | target selector entity / group / broadcast semantics | `covered-in-repo` | [test_target_selector](../../tests/core/test_target_selector.cpp) | `pr` | `repo-only` |
| protocol | payload boundary / truncation / malformed decode / unknown enum | `covered-in-repo` | [test_semantic_payload_policy](../../tests/runtime/test_semantic_payload_policy.cpp) | `pr` | `repo-only` |
| protocol | protocol / header / schema mismatch rejection | `covered-in-repo` | [test_runtime_version_rejection](../../tests/runtime/test_runtime_version_rejection.cpp), [test_session_security](../../tests/security/test_session_security.cpp) | `pr` | `repo-only` |
| protocol | fuzz-style decode harness | `implemented-now` | `yunlink_codec_fuzz_harness` in [CMakeLists.txt](../../CMakeLists.txt) | `nightly-local` | `repo-only` |
| runtime | single-UAV roundtrip command/state baseline | `covered-in-repo` | [test_compat_roundtrip](../../tests/test_compat_roundtrip.cpp), [test_transport_loop](../../tests/test_transport_loop.cpp) | `pr` | `repo-only` |
| runtime | session `open -> active -> invalid -> lost` convergence | `implemented-now` | [test_session_lifecycle](../../tests/runtime/test_session_lifecycle.cpp), [test_capability_negotiation](../../tests/runtime/test_capability_negotiation.cpp), [test_session_security](../../tests/security/test_session_security.cpp) | `pr` | `repo-only` |
| runtime | explicit restart / reconnect / reopen / reacquire | `covered-in-repo` | [test_runtime_restart](../../tests/runtime/test_runtime_restart.cpp), [test_remote_reconnect](../../tests/runtime/test_remote_reconnect.cpp) | `ci` | `repo-only` |
| runtime | authority `claim -> renew -> release -> expire -> reacquire -> preempt` | `covered-in-repo` | [test_runtime_control_paths](../../tests/test_runtime_control_paths.cpp), [test_authority_edges](../../tests/runtime/test_authority_edges.cpp), [test_authority_status](../../tests/runtime/test_authority_status.cpp) | `pr` | `repo-only` |
| runtime | no-authority / missing-session / expired command rejection | `covered-in-repo` | [test_authority_edges](../../tests/runtime/test_authority_edges.cpp), [test_runtime_ttl_freshness](../../tests/runtime/test_runtime_ttl_freshness.cpp) | `pr` | `repo-only` |
| runtime | explicit command phase flow including cancelled | `implemented-now` | [test_command_result_edges](../../tests/runtime/test_command_result_edges.cpp) | `pr` | `repo-only` |
| runtime | external command handler disables auto-result | `covered-in-repo` | [test_external_command_handling](../../tests/runtime/test_external_command_handling.cpp) | `pr` | `repo-only` |
| runtime | state snapshot / state event source identity and ordering | `implemented-now` | [test_state_plane_semantics](../../tests/runtime/test_state_plane_semantics.cpp) | `pr` | `repo-only` |
| runtime | QoS policy for command/state/event/bulk | `covered-in-repo` | [test_qos_runtime](../../tests/runtime/test_qos_runtime.cpp) | `pr` | `repo-only` |
| runtime | trajectory chunk / formation task / bulk channel runtime contracts | `covered-in-repo` | [test_trajectory_chunk_runtime](../../tests/runtime/test_trajectory_chunk_runtime.cpp), [test_formation_task_runtime](../../tests/runtime/test_formation_task_runtime.cpp), [test_bulk_channel_runtime](../../tests/runtime/test_bulk_channel_runtime.cpp) | `ci` | `repo-only` |
| transport | TCP local loop / duplicate connect / unavailable listener / bind conflict | `covered-in-repo` | [test_transport_loop](../../tests/test_transport_loop.cpp), [test_tcp_resilience](../../tests/transport/test_tcp_resilience.cpp) | `pr` | `repo-only` |
| transport | UDP source isolation / routing isolation | `covered-in-repo` | [test_udp_source_isolation](../../tests/test_udp_source_isolation.cpp), [test_routing_and_source_validation](../../tests/security/test_routing_and_source_validation.cpp) | `pr` | `repo-only` |
| security | shared secret mismatch / replay / key epoch / wrong target-source / broadcast reject | `covered-in-repo` | [test_session_security](../../tests/security/test_session_security.cpp), [test_security_tags](../../tests/security/test_security_tags.cpp), [test_broadcast_policy](../../tests/runtime/test_broadcast_policy.cpp), [test_routing_and_source_validation](../../tests/security/test_routing_and_source_validation.cpp) | `ci` | `repo-only` |
| ffi | C ABI runtime loop, contract, loader/symbol smoke | `covered-in-repo` | [test_c_ffi_v1](../../tests/bindings/test_c_ffi_v1.cpp), [test_c_ffi_contract](../../tests/bindings/test_c_ffi_contract.cpp), [test_c_ffi_loader](../../tests/ffi/test_c_ffi_loader.py) | `ci` | `repo-only` |
| bindings-rust | runtime loop / recovery / lagged backpressure / drop cleanup | `covered-in-repo` | [runtime_loop.rs](../../bindings/rust/yunlink/tests/runtime_loop.rs), [runtime_recovery.rs](../../bindings/rust/yunlink/tests/runtime_recovery.rs), [runtime_behaviors.rs](../../bindings/rust/yunlink/tests/runtime_behaviors.rs), [runtime_contracts.rs](../../bindings/rust/yunlink/tests/runtime_contracts.rs) | `ci` | `repo-only` |
| bindings-python | contract / poll-thread / sync-async queue / runtime loop | `implemented-now` | [test_contracts.py](../../bindings/python/tests/test_contracts.py), [test_runtime_loop.py](../../bindings/python/tests/test_runtime_loop.py) | `ci` | `repo-only` |
| docs | documentation / matrix / drift consistency | `implemented-now` | [test_docs_consistency.py](../../tests/test_docs_consistency.py), [test_dev_tooling_config.cmake](../../tests/test_dev_tooling_config.cmake) | `pr` | `repo-only` |
| tooling | dual-host runner / report renderer / perf aggregator / netem helper dry-run | `covered-in-repo` | [test_testing_tooling.py](../../tests/test_testing_tooling.py), [test_ci_regressions.py](../../tests/test_ci_regressions.py) | `ci` | `repo-only` |

## External-Gated Suites

| domain | scenario | status | evidence | gate_tier | env_requirement |
| --- | --- | --- | --- | --- | --- |
| dual-host | wired LAN baseline | `blocked-external` | [hosts.example.yaml](../../tools/testing/dual_host/hosts.example.yaml), [run_suite.py](../../tools/testing/dual_host/run_suite.py) | `manual-external` | `2-host-lan-lab` |
| dual-host | Office Wi-Fi baseline / recovery / competition / routing | `blocked-external` | [office_wifi_lab.yaml](../../tools/testing/dual_host/office_wifi_lab.yaml), [tools/testing/README.md](../../tools/testing/README.md) | `manual-external` | `office-wifi-lab` |
| network | Wi-Fi + netem `delay` / `delay+jitter` / `loss` / `loss+reorder` / `duplication` / short disconnect | `blocked-external` | [profiles](../../tools/testing/netem/profiles), [README](../../tests/network/README.md) | `manual-external` | `linux-netem-lab` |
| scale | `2 GCS -> 2 UAV` | `blocked-external` | [test-world-map.md](./test-world-map.md), [tools/testing/README.md](../../tools/testing/README.md) | `manual-external` | `2-gcs-2-uav-lab` |
| scale | `1 GCS -> N UAV` capacity entry | `blocked-external` | [test-world-map.md](./test-world-map.md), [tests/perf/README.md](../../tests/perf/README.md) | `nightly-local` | `multi-uav-lab` |
| perf | `connect_ms` / `session_ready_ms` / `authority_acquire_ms` / `command_result_ms` / `state_first_seen_ms` / `recovery_ms` reports | `blocked-external` | [run_perf_suite.py](../../tools/testing/perf/run_perf_suite.py), [render_summary.py](../../tools/testing/report/render_summary.py) | `nightly-local` | `metric-producing-suites` |
| soak | `30min` / `2h` / `overnight` soak | `blocked-external` | [tests/soak/README.md](../../tests/soak/README.md) | `release-external` | `long-running-lab` |
| bridge | real Sunray graph integration | `blocked-external` | [ros-sunray-bridge-overview.md](./ros-sunray-bridge-overview.md) | `manual-external` | `external-bridge-repo` |
| sitl | PX4 SITL + bridge + Yunlink ground | `blocked-external` | [test-world-map.md](./test-world-map.md), [ros-sunray-bridge-overview.md](./ros-sunray-bridge-overview.md) | `release-external` | `px4-sitl-lab` |
| hil | HIL / 真机 release gate | `blocked-external` | [test-world-map.md](./test-world-map.md) | `release-external` | `hil-or-vehicle-lab` |
