import yunlink
from yunlink.core_codec import (
    decode_action_update,
    decode_attachment_request,
    decode_attachment_response,
    decode_authority_request,
    decode_authority_status,
    decode_entity_directory,
    decode_stream_catalog,
    decode_stream_sample,
    decode_stream_subscription,
    decode_stream_subscription_status,
)


def test_directory_and_attachment_round_trip():
    entity = yunlink.EntityDescriptor(
        "uav1", "sunray.uav", "UAV 1", "sim-1",
        {"sunray.system_mode": "sim"}, ("flight",), yunlink.Availability.ONLINE,
    )
    directory = yunlink.EntityDirectory("bridge", "r1", (entity,))
    assert decode_entity_directory(yunlink.encode_core(directory)) == directory
    request = yunlink.AttachmentRequest("r1", ("uav1",))
    assert decode_attachment_request(yunlink.encode_core(request)) == request
    response = yunlink.AttachmentResponse(True, "r1", ("uav1",), "attached")
    assert decode_attachment_response(yunlink.encode_core(response)) == response


def test_authority_stream_and_action_round_trip():
    request = yunlink.AuthorityRequest("com.yundrone.sunray", 5000, False)
    assert decode_authority_request(yunlink.encode_core(request)) == request
    authority = yunlink.AuthorityStatus("com.yundrone.sunray", "controller", 5000, 0, "")
    assert decode_authority_status(yunlink.encode_core(authority)) == authority
    descriptor = yunlink.StreamDescriptor(
        "uav1.odometry", yunlink.TypeRef("org.yunlink.mobility", 1, "Odometry"), "protobuf", {"rate": "10"}
    )
    catalog = yunlink.StreamCatalog("r2", (descriptor,))
    assert decode_stream_catalog(yunlink.encode_core(catalog)) == catalog
    subscription = yunlink.StreamSubscription("uav1.odometry", 10.0, 65536)
    assert decode_stream_subscription(yunlink.encode_core(subscription)) == subscription
    status = yunlink.StreamSubscriptionStatus(True, True, "uav1.odometry", 10.0, 65536, "")
    assert decode_stream_subscription_status(yunlink.encode_core(status)) == status
    sample = yunlink.StreamSample("uav1.odometry", "protobuf", {}, 123, 4, b"payload")
    assert decode_stream_sample(yunlink.encode_core(sample)) == sample
    update = yunlink.ActionUpdate(yunlink.ActionPhase.RUNNING, 0, 45, "moving")
    assert decode_action_update(yunlink.encode_core(update)) == update
