"""Sunray 2.18 startup queue wire contract (not device execution policy)."""

from yunlink.profiles import sunray


def test_get_request_matches_golden_vector():
    request = sunray.StartupQueueRequest(action="get")
    assert request.SerializeToString().hex() == "0a03676574"
    decoded = sunray.StartupQueueRequest.FromString(bytes.fromhex("0a03676574"))
    assert decoded.action == "get"
    assert decoded.expected_revision == 0
    assert decoded.expected_run_id == ""
    assert len(decoded.items) == 0


def test_save_preserves_u64_revision_and_complete_item_order():
    request = sunray.StartupQueueRequest(
        action="save", expected_revision=(1 << 64) - 1, expected_run_id=""
    )
    request.items.add(
        feature_name="bridge", enabled=True, completion_mode="delay",
        delay_sec=3.0, timeout_sec=60.0,
    )
    request.items.add(
        feature_name="localization", enabled=False, completion_mode="health",
        delay_sec=0.0, timeout_sec=120.0,
    )
    decoded = sunray.StartupQueueRequest.FromString(request.SerializeToString())
    assert decoded == request
    assert decoded.expected_revision == (1 << 64) - 1
    assert [item.feature_name for item in decoded.items] == ["bridge", "localization"]
    assert not decoded.items[1].enabled


def test_response_distinguishes_missing_snapshot_and_preserves_runtime_fields():
    rejected = sunray.StartupQueueResponse(success=False, message="Access denied")
    decoded = sunray.StartupQueueResponse.FromString(rejected.SerializeToString())
    assert not decoded.HasField("snapshot")

    response = sunray.StartupQueueResponse(success=True)
    snapshot = response.snapshot
    snapshot.schema_version = 1
    snapshot.revision = (1 << 53) + 1
    snapshot.run_id = "run-1"
    snapshot.state = "PAUSED"
    snapshot.current_index = -1
    snapshot.config_warning = "Fallback configuration"
    snapshot.items.add(
        feature_name="localization", enabled=True, completion_mode="health",
        delay_sec=3.0, timeout_sec=60.0, display_name="Localization",
        depends_on=["bridge"], health_supported=True, pinned=False,
        is_protected=True, status="WAIT_COMPLETE", message="Waiting for probe",
        started_at=1700000000.5, finished_at=0.0, elapsed_sec=2.5,
    )
    decoded = sunray.StartupQueueResponse.FromString(response.SerializeToString())
    assert decoded.HasField("snapshot")
    assert decoded == response
    assert decoded.snapshot.current_index == -1
    assert decoded.snapshot.revision == (1 << 53) + 1
    assert decoded.snapshot.items[0].status == "WAIT_COMPLETE"
