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
    return 0;
}
