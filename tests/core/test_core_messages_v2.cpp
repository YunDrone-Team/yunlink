#include <cassert>

#include "yunlink/core/core_messages_v2.hpp"

int main() {
    using namespace yunlink::v2;
    EntityDirectory directory;
    directory.endpoint_uid = "endpoint.bridge";
    directory.revision = "42";
    EntityDescriptor entity;
    entity.entity_uid = "e-endpoint.bridge-device-1";
    entity.kind = "mobile_robot";
    entity.display_name = "Device 1";
    entity.hardware_id = "serial-1";
    entity.attributes = {{"provider.name", "alpha"}, {"provider.id", "1"}};
    entity.capabilities = {"stream", "action"};
    entity.availability = Availability::kOnline;
    directory.entities.push_back(entity);
    EntityDirectory decoded;
    assert(decode(encode(directory), &decoded));
    assert(decoded.endpoint_uid == directory.endpoint_uid);
    assert(decoded.entities.size() == 1);
    assert(decoded.entities.front().attributes == entity.attributes);

    ActionUpdate update{ActionPhase::kRunning, 0, 75, "executing"};
    ActionUpdate decoded_update;
    assert(decode(encode(update), &decoded_update));
    assert(decoded_update.phase == ActionPhase::kRunning);
    assert(decoded_update.progress_percent == 75);

    StreamCatalog catalog;
    catalog.revision = "1";
    catalog.streams.push_back({"entity.stream.pose",
                               {"org.yunlink.mobility", 1, 0, "Odometry"},
                               "protobuf",
                               {{"frame", "map"}}});
    StreamCatalog decoded_catalog;
    assert(decode(encode(catalog), &decoded_catalog));
    assert(decoded_catalog.streams.front().type.type_name == "Odometry");

    StreamSample sample;
    sample.stream_uid = "entity.stream.raw";
    sample.encoding = "application/json";
    sample.metadata = {{"schema.name", "example/PointCloud"}, {"schema.digest", "abcd"}};
    sample.source_timestamp_ns = 123456789;
    sample.sequence = 42;
    sample.data = {0, 1, 2, 255};
    StreamSample decoded_sample;
    const Bytes encoded_sample = encode(sample);
    assert(decode(encoded_sample, &decoded_sample));
    assert(decoded_sample.stream_uid == sample.stream_uid);
    assert(decoded_sample.metadata == sample.metadata);
    assert(decoded_sample.data == sample.data);
    Bytes truncated_sample = encoded_sample;
    truncated_sample.pop_back();
    assert(!decode(truncated_sample, &decoded_sample));

    yunlink::ConfigResourceGetResponse configuration;
    configuration.status = yunlink::ConfigServiceStatus::kOk;
    configuration.message = "ok";
    configuration.snapshot.resource_id = "bridge.runtime";
    configuration.snapshot.revision = "7";
    configuration.snapshot.values = {
        {"endpoint_uid", yunlink::ConfigValue::from_string("endpoint.bridge")},
        {"tcp_listen_port", yunlink::ConfigValue::from_int64(9696)},
    };
    yunlink::ConfigResourceGetResponse decoded_configuration;
    assert(decode(encode(configuration), &decoded_configuration));
    assert(decoded_configuration.snapshot.values.size() == 2);
    assert(decoded_configuration.snapshot.values.front().value.string_value == "endpoint.bridge");

    LogListResponse logs;
    logs.success = true;
    logs.message = "ok";
    logs.logs.push_back({"bridge.runtime",
                         "endpoint.bridge",
                         "Bridge Runtime",
                         "running",
                         1,
                         0,
                         false,
                         0,
                         {{"component", "bridge"}},
                         "active"});
    LogListResponse decoded_logs;
    assert(decode(encode(logs), &decoded_logs));
    assert(decoded_logs.logs.size() == 1);
    assert(decoded_logs.logs.front().owner_uid == "endpoint.bridge");

    LogReadResponse log_read{true, "ok", "bridge.runtime", "line\n", 5, false, true};
    LogReadResponse decoded_log_read;
    assert(decode(encode(log_read), &decoded_log_read));
    assert(decoded_log_read.chunk == "line\n");
    return 0;
}
