import queue
import time

import yunlink


PROFILE = yunlink.Profile("org.example.telemetry", 1, 0, "example-v1")


def test_runtime_v2_stream_roundtrip():
    source = yunlink.Runtime(
        yunlink.RuntimeConfig("endpoint.source", 15130, profiles=(PROFILE,))
    )
    sink = yunlink.Runtime(
        yunlink.RuntimeConfig(
            "endpoint.sink", 15131, profiles=(PROFILE,), required_profiles=(PROFILE,)
        )
    )
    try:
        peer = sink.connect("127.0.0.1", 15130)
        session_id = sink.open_session(peer)
        deadline = time.time() + 3
        while time.time() < deadline and not sink.session_has_profile(
            peer, session_id, PROFILE.profile_id, PROFILE.major
        ):
            time.sleep(0.02)
        assert sink.session_has_profile(peer, session_id, PROFILE.profile_id, PROFILE.major)
        assert sink.session_supports_profile(
            peer, session_id, PROFILE.profile_id, PROFILE.major, PROFILE.minor
        )
        assert not sink.session_supports_profile(
            peer, session_id, PROFILE.profile_id, PROFILE.major, PROFILE.minor + 1
        )
        assert sink.session_endpoint_uid(peer, session_id) == "endpoint.source"

        handle = sink.publish(
            peer,
            session_id,
            yunlink.Family.STREAM,
            4,
            yunlink.Target.endpoint("endpoint.source"),
            yunlink.TypeRef(PROFILE.profile_id, 1, "Sample"),
            b"owned-payload",
            qos=yunlink.Qos.RELIABLE_LATEST,
        )
        assert handle.session_id == session_id

        deadline = time.time() + 3
        received = None
        while time.time() < deadline:
            try:
                event = source.events.get(timeout=0.1)
            except queue.Empty:
                continue
            if event.kind == 1 and event.family == yunlink.Family.STREAM:
                received = event
                break
        assert received is not None
        assert received.payload == b"owned-payload"
        sink.close_peer(peer)
    finally:
        sink.close()
        source.close()
