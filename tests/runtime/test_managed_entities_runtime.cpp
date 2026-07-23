/** @file @brief Managed entity directory, source binding, and multi-entity state test. */

#include <algorithm>
#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <thread>

#include "yunlink/runtime/runtime.hpp"

namespace {

bool wait_until(const std::function<bool()>& predicate) {
    for (int attempt = 0; attempt < 200; ++attempt) {
        if (predicate()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return false;
}

yunlink::EndpointIdentity uav(uint32_t id) {
    yunlink::EndpointIdentity identity;
    identity.agent_type = yunlink::AgentType::kUav;
    identity.agent_id = id;
    identity.role = yunlink::EndpointRole::kVehicle;
    return identity;
}

yunlink::ManagedEntityDescriptor entity(uint32_t id) {
    yunlink::ManagedEntityDescriptor descriptor;
    descriptor.entity_uid = "uav:" + std::to_string(id);
    descriptor.identity = uav(id);
    descriptor.display_name = "UAV " + std::to_string(id);
    descriptor.hardware_id = "SIM-" + std::to_string(id);
    descriptor.capabilities = {"telemetry.px4", "control.uav"};
    descriptor.availability = yunlink::ManagedEntityAvailability::kOnline;
    return descriptor;
}

}  // namespace

int main() {
    yunlink::ManagedEntityListResponse directory;
    directory.success = true;
    directory.message = "ok";
    directory.endpoint_uid = "endpoint-test-0001";
    directory.revision = "revision-1";
    directory.primary_identity = uav(1);
    directory.entities = {entity(1), entity(2)};

    const yunlink::ByteBuffer encoded = yunlink::encode_payload(directory);
    yunlink::ManagedEntityListResponse decoded;
    if (encoded.empty() || !yunlink::decode_payload(encoded, &decoded) ||
        decoded.endpoint_uid != directory.endpoint_uid || decoded.entities.size() != 2U ||
        decoded.entities[1].identity.agent_id != 2U) {
        std::cerr << "managed entity codec round-trip failed\n";
        return 1;
    }

    yunlink::ManagedEntityAttachmentRequest attachment_request;
    attachment_request.endpoint_uid = directory.endpoint_uid;
    attachment_request.directory_revision = directory.revision;
    attachment_request.action = yunlink::ManagedEntityAttachmentAction::kAttach;
    attachment_request.entity_uids = {"uav:2"};
    const yunlink::ByteBuffer encoded_attachment_request =
        yunlink::encode_payload(attachment_request);
    yunlink::ManagedEntityAttachmentRequest decoded_attachment_request;
    if (encoded_attachment_request.empty() ||
        !yunlink::decode_payload(encoded_attachment_request, &decoded_attachment_request) ||
        decoded_attachment_request.endpoint_uid != attachment_request.endpoint_uid ||
        decoded_attachment_request.directory_revision != attachment_request.directory_revision ||
        decoded_attachment_request.action != attachment_request.action ||
        decoded_attachment_request.entity_uids != attachment_request.entity_uids) {
        std::cerr << "managed entity attachment request codec round-trip failed\n";
        return 2;
    }

    yunlink::ManagedEntityAttachmentResponse attachment_response;
    attachment_response.success = true;
    attachment_response.message = "attached";
    attachment_response.endpoint_uid = directory.endpoint_uid;
    attachment_response.directory_revision = directory.revision;
    attachment_response.results = {{"uav:2", true, "attached"}};
    attachment_response.attached_entity_uids = {"uav:2"};
    const yunlink::ByteBuffer encoded_attachment_response =
        yunlink::encode_payload(attachment_response);
    yunlink::ManagedEntityAttachmentResponse decoded_attachment_response;
    if (encoded_attachment_response.empty() ||
        !yunlink::decode_payload(encoded_attachment_response, &decoded_attachment_response) ||
        !decoded_attachment_response.success || decoded_attachment_response.results.size() != 1U ||
        !decoded_attachment_response.results.front().accepted ||
        decoded_attachment_response.attached_entity_uids !=
            attachment_response.attached_entity_uids) {
        std::cerr << "managed entity attachment response codec round-trip failed\n";
        return 3;
    }

    yunlink::Runtime air;
    yunlink::Runtime ground;
    yunlink::RuntimeConfig air_config;
    air_config.udp_bind_port = 14110;
    air_config.udp_target_port = 14110;
    air_config.tcp_listen_port = 14210;
    air_config.self_identity = uav(1);
    air_config.managed_identities = {uav(2)};
    air_config.capability_flags = yunlink::kCapabilityManagedEntities;
    air_config.shared_secret = "managed-entities-secret";

    yunlink::RuntimeConfig ground_config;
    ground_config.udp_bind_port = 14111;
    ground_config.udp_target_port = 14111;
    ground_config.tcp_listen_port = 14211;
    ground_config.self_identity.agent_type = yunlink::AgentType::kGroundStation;
    ground_config.self_identity.agent_id = 1001;
    ground_config.self_identity.role = yunlink::EndpointRole::kController;
    ground_config.shared_secret = air_config.shared_secret;

    if (air.start(air_config) != yunlink::ErrorCode::kOk ||
        ground.start(ground_config) != yunlink::ErrorCode::kOk) {
        std::cerr << "runtime start failed\n";
        return 2;
    }

    air.system_service_subscriber().subscribe_managed_entity_list_requests(
        [&](const yunlink::InboundSystemServiceRequestView<yunlink::ManagedEntityListRequest>&
                view) {
            air.system_service_publisher().publish_managed_entity_list_response(view.inbound,
                                                                                directory);
        });
    air.system_service_subscriber().subscribe_managed_entity_attachment_requests(
        [&](const yunlink::InboundSystemServiceRequestView<yunlink::ManagedEntityAttachmentRequest>&
                view) {
            yunlink::ManagedEntityAttachmentResponse response;
            response.endpoint_uid = directory.endpoint_uid;
            response.directory_revision = directory.revision;
            for (const auto& entity_uid : view.payload.entity_uids) {
                const bool known = entity_uid == "uav:1" || entity_uid == "uav:2";
                const bool revision_matches = view.payload.directory_revision == directory.revision;
                response.results.push_back(
                    {entity_uid,
                     known && revision_matches,
                     known && revision_matches ? "attached" : "directory-revision-conflict"});
                if (known && revision_matches &&
                    view.payload.action == yunlink::ManagedEntityAttachmentAction::kAttach) {
                    response.attached_entity_uids.push_back(entity_uid);
                }
            }
            response.success =
                std::all_of(response.results.begin(),
                            response.results.end(),
                            [](const yunlink::ManagedEntityAttachmentResult& result) {
                                return result.accepted;
                            });
            response.message = response.success ? "ok" : "directory-revision-conflict";
            (void)air.system_service_publisher().publish_managed_entity_attachment_response(
                view.inbound, response);
        });

    std::string peer_id;
    if (ground.tcp_clients().connect_peer("127.0.0.1", air_config.tcp_listen_port, &peer_id) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "connect failed\n";
        return 3;
    }
    const uint64_t session_id =
        ground.session_client().open_active_session(peer_id, "managed-entity-ground");
    if (session_id == 0 ||
        !wait_until([&]() { return air.session_server().has_active_session(session_id); })) {
        std::cerr << "session failed\n";
        return 4;
    }

    std::atomic<uint32_t> directory_response_count{0};
    std::atomic<uint32_t> attachment_response_count{0};
    std::atomic<bool> attachment_response_ok{false};
    std::atomic<uint32_t> last_state_source{0};
    ground.system_service_subscriber().subscribe_managed_entity_list_responses(
        [&](const yunlink::TypedMessage<yunlink::ManagedEntityListResponse>& message) {
            if (message.payload.entities.size() == 2U) {
                directory_response_count.fetch_add(1);
            }
        });
    ground.system_service_subscriber().subscribe_managed_entity_attachment_responses(
        [&](const yunlink::TypedMessage<yunlink::ManagedEntityAttachmentResponse>& message) {
            attachment_response_ok.store(message.payload.success &&
                                         message.payload.attached_entity_uids.size() == 1U &&
                                         message.payload.attached_entity_uids.front() == "uav:2");
            attachment_response_count.fetch_add(1);
        });
    ground.state_subscriber().subscribe_px4_state(
        [&](const yunlink::TypedMessage<yunlink::Px4StateSnapshot>& message) {
            last_state_source.store(message.envelope.source.agent_id);
        });

    yunlink::SystemServiceHandle handle;
    if (ground.system_service_publisher().publish_managed_entity_list_request(
            peer_id,
            session_id,
            yunlink::TargetSelector::for_entity(yunlink::AgentType::kUav, 1),
            yunlink::ManagedEntityListRequest{},
            &handle) != yunlink::ErrorCode::kOk ||
        !wait_until([&]() { return directory_response_count.load() == 1U; })) {
        std::cerr << "managed entity directory request failed\n";
        return 5;
    }

    if (ground.system_service_publisher().publish_managed_entity_attachment_request(
            peer_id,
            session_id,
            yunlink::TargetSelector::for_entity(yunlink::AgentType::kUav, 1),
            attachment_request,
            &handle) != yunlink::ErrorCode::kOk ||
        !wait_until([&]() {
            return attachment_response_count.load() == 1U && attachment_response_ok.load();
        })) {
        std::cerr << "managed entity attachment request failed\n";
        return 6;
    }

    attachment_request.directory_revision = "stale-revision";
    if (ground.system_service_publisher().publish_managed_entity_attachment_request(
            peer_id,
            session_id,
            yunlink::TargetSelector::for_entity(yunlink::AgentType::kUav, 1),
            attachment_request,
            &handle) != yunlink::ErrorCode::kOk ||
        !wait_until([&]() { return attachment_response_count.load() == 2U; }) ||
        attachment_response_ok.load()) {
        std::cerr << "managed entity attachment revision conflict was not reported\n";
        return 7;
    }

    yunlink::SessionDescriptor air_session;
    if (!air.session_server().find_active_session(&air_session)) {
        std::cerr << "air session missing\n";
        return 6;
    }
    yunlink::Px4StateSnapshot state;
    if (air.publish_state_from(
            uav(2),
            air_session.peer.id,
            yunlink::TargetSelector::for_entity(yunlink::AgentType::kGroundStation, 1001),
            state,
            session_id) != yunlink::ErrorCode::kOk ||
        !wait_until([&]() { return last_state_source.load() == 2U; })) {
        std::cerr << "managed entity state was not delivered\n";
        return 7;
    }

    if (air.publish_state_from(
            uav(99),
            air_session.peer.id,
            yunlink::TargetSelector::for_entity(yunlink::AgentType::kGroundStation, 1001),
            state,
            session_id) != yunlink::ErrorCode::kUnauthorized) {
        std::cerr << "unmanaged source was not rejected\n";
        return 8;
    }

    const auto uav2_target = yunlink::TargetSelector::for_entity(yunlink::AgentType::kUav, 2);
    if (ground.request_authority(
            peer_id, session_id, uav2_target, yunlink::ControlSource::kGroundStation, 3000) !=
            yunlink::ErrorCode::kOk ||
        !wait_until([&]() {
            yunlink::AuthorityLease lease;
            return air.current_authority_for_target(uav2_target, &lease);
        })) {
        std::cerr << "managed entity authority was not granted\n";
        return 9;
    }

    if (air.set_managed_identities({uav(3)}) != yunlink::ErrorCode::kOk) {
        std::cerr << "dynamic managed identity update failed\n";
        return 10;
    }
    yunlink::AuthorityLease removed_lease;
    if (air.current_authority_for_target(uav2_target, &removed_lease)) {
        std::cerr << "removed managed entity retained authority\n";
        return 11;
    }
    if (air.publish_state_from(
            uav(2),
            air_session.peer.id,
            yunlink::TargetSelector::for_entity(yunlink::AgentType::kGroundStation, 1001),
            state,
            session_id) != yunlink::ErrorCode::kUnauthorized) {
        std::cerr << "removed managed source was not rejected\n";
        return 12;
    }
    directory.revision = "revision-2";
    directory.entities = {entity(1), entity(3)};
    if (ground.system_service_publisher().publish_managed_entity_list_request(
            peer_id,
            session_id,
            yunlink::TargetSelector::for_entity(yunlink::AgentType::kUav, 1),
            yunlink::ManagedEntityListRequest{},
            &handle) != yunlink::ErrorCode::kOk ||
        !wait_until([&]() { return directory_response_count.load() == 2U; })) {
        std::cerr << "updated managed entity directory was not received\n";
        return 13;
    }
    last_state_source.store(0);
    if (air.publish_state_from(
            uav(3),
            air_session.peer.id,
            yunlink::TargetSelector::for_entity(yunlink::AgentType::kGroundStation, 1001),
            state,
            session_id) != yunlink::ErrorCode::kOk ||
        !wait_until([&]() { return last_state_source.load() == 3U; })) {
        std::cerr << "new managed source was not delivered\n";
        return 14;
    }

    if (air.set_managed_identities({uav(3), uav(3)}) != yunlink::ErrorCode::kInvalidArgument ||
        air.set_managed_identities({uav(0)}) != yunlink::ErrorCode::kInvalidArgument) {
        std::cerr << "invalid dynamic managed identities were accepted\n";
        return 15;
    }

    ground.stop();
    air.stop();
    return 0;
}
