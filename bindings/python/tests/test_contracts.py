import yunlink


def test_public_contract_is_wire_v2_only():
    assert yunlink.Family.STREAM == 4
    assert yunlink.Target.entity("vehicle.uav.1").uids == ("vehicle.uav.1",)
    assert yunlink.TypeRef("org.example", 1, "Sample").minor == 0
    assert not hasattr(yunlink, "AgentType")


def test_event_payload_is_owned():
    payload = bytearray(b"sample")
    event = yunlink.Event(
        kind=1,
        peer_id="peer",
        link_up=True,
        error_code=0,
        message="",
        session_state=4,
        authenticated=True,
        session_id=1,
        family=yunlink.Family.STREAM,
        operation=4,
        qos=yunlink.Qos.RELIABLE_LATEST,
        message_id=2,
        correlation_id=0,
        source_endpoint_uid="endpoint.source",
        source_entity_uid="entity.source",
        target=yunlink.Target.entity("entity.target"),
        type_ref=yunlink.TypeRef("org.example", 1, "Sample"),
        payload=bytes(payload),
    )
    payload[:] = b"change"
    assert event.payload == b"sample"
