/**
 * @file src/runtime/system_service/receive.cpp
 * @brief Runtime inbound system service dispatch.
 */

#include "../state/fanout.hpp"

#include <set>

namespace yunlink {
namespace {

bool valid_managed_entity_directory(const ManagedEntityListResponse& response,
                                    const SessionDescriptor& session) {
    if (!response.success || response.endpoint_uid.empty() || response.revision.empty() ||
        !runtime_same_entity(response.primary_identity, session.remote_identity) ||
        response.entities.empty() || response.entities.size() > 256U) {
        return false;
    }
    std::set<std::string> entity_uids;
    std::set<std::pair<uint8_t, uint32_t>> identities;
    bool contains_primary = false;
    for (const ManagedEntityDescriptor& entity : response.entities) {
        if (entity.entity_uid.empty() || entity.identity.agent_type == AgentType::kUnknown ||
            entity.identity.agent_id == 0 || !entity_uids.insert(entity.entity_uid).second ||
            !identities
                 .insert(
                     {static_cast<uint8_t>(entity.identity.agent_type), entity.identity.agent_id})
                 .second) {
            return false;
        }
        contains_primary =
            contains_primary || runtime_same_entity(entity.identity, response.primary_identity);
    }
    return contains_primary;
}

}  // namespace

void Runtime::handle_system_service_envelope(const EnvelopeEvent& ev) {
    if (ev.envelope.qos_class != QosClass::kReliableOrdered) {
        ErrorEvent error;
        error.code = ErrorCode::kRejected;
        error.transport = ev.transport;
        error.peer = ev.peer;
        error.message = "system-service-qos-requires-reliable-ordered";
        bus_.publish_error(error);
        return;
    }

    if (!matches_local_target(ev.envelope.target)) {
        return;
    }

    switch (static_cast<SystemServiceType>(ev.envelope.message_type)) {
    case SystemServiceType::kFeatureListRequest:
        if (!runtime_fanout_inbound_request<FeatureListRequest>(
                impl_->mu, ev, ev.envelope.payload, impl_->feature_list_request_handlers)) {
            runtime_publish_semantic_decode_error(bus_, ev);
        }
        return;
    case SystemServiceType::kFeatureListResponse:
        if (!runtime_fanout_snapshot<FeatureListResponse>(impl_->mu,
                                                          ev.envelope,
                                                          ev.envelope.payload,
                                                          impl_->feature_list_response_handlers)) {
            runtime_publish_semantic_decode_error(bus_, ev);
        }
        return;
    case SystemServiceType::kFeatureGetRequest:
        if (!runtime_fanout_inbound_request<FeatureGetRequest>(
                impl_->mu, ev, ev.envelope.payload, impl_->feature_get_request_handlers)) {
            runtime_publish_semantic_decode_error(bus_, ev);
        }
        return;
    case SystemServiceType::kFeatureGetResponse:
        if (!runtime_fanout_snapshot<FeatureGetResponse>(impl_->mu,
                                                         ev.envelope,
                                                         ev.envelope.payload,
                                                         impl_->feature_get_response_handlers)) {
            runtime_publish_semantic_decode_error(bus_, ev);
        }
        return;
    case SystemServiceType::kFeatureStartRequest:
        if (!runtime_fanout_inbound_request<FeatureStartRequest>(
                impl_->mu, ev, ev.envelope.payload, impl_->feature_start_request_handlers)) {
            runtime_publish_semantic_decode_error(bus_, ev);
        }
        return;
    case SystemServiceType::kFeatureStartResponse:
        if (!runtime_fanout_snapshot<FeatureStartResponse>(
                impl_->mu,
                ev.envelope,
                ev.envelope.payload,
                impl_->feature_start_response_handlers)) {
            runtime_publish_semantic_decode_error(bus_, ev);
        }
        return;
    case SystemServiceType::kFeatureStopRequest:
        if (!runtime_fanout_inbound_request<FeatureStopRequest>(
                impl_->mu, ev, ev.envelope.payload, impl_->feature_stop_request_handlers)) {
            runtime_publish_semantic_decode_error(bus_, ev);
        }
        return;
    case SystemServiceType::kFeatureStopResponse:
        if (!runtime_fanout_snapshot<FeatureStopResponse>(impl_->mu,
                                                          ev.envelope,
                                                          ev.envelope.payload,
                                                          impl_->feature_stop_response_handlers)) {
            runtime_publish_semantic_decode_error(bus_, ev);
        }
        return;
    case SystemServiceType::kRuntimeLogListRequest:
        if (!runtime_fanout_inbound_request<RuntimeLogListRequest>(
                impl_->mu, ev, ev.envelope.payload, impl_->runtime_log_list_request_handlers)) {
            runtime_publish_semantic_decode_error(bus_, ev);
        }
        return;
    case SystemServiceType::kRuntimeLogListResponse:
        if (!runtime_fanout_snapshot<RuntimeLogListResponse>(
                impl_->mu,
                ev.envelope,
                ev.envelope.payload,
                impl_->runtime_log_list_response_handlers)) {
            runtime_publish_semantic_decode_error(bus_, ev);
        }
        return;
    case SystemServiceType::kRuntimeLogReadRequest:
        if (!runtime_fanout_inbound_request<RuntimeLogReadRequest>(
                impl_->mu, ev, ev.envelope.payload, impl_->runtime_log_read_request_handlers)) {
            runtime_publish_semantic_decode_error(bus_, ev);
        }
        return;
    case SystemServiceType::kRuntimeLogReadResponse:
        if (!runtime_fanout_snapshot<RuntimeLogReadResponse>(
                impl_->mu,
                ev.envelope,
                ev.envelope.payload,
                impl_->runtime_log_read_response_handlers)) {
            runtime_publish_semantic_decode_error(bus_, ev);
        }
        return;
    case SystemServiceType::kTopicListRequest:
        if (!runtime_fanout_inbound_request<TopicListRequest>(
                impl_->mu, ev, ev.envelope.payload, impl_->topic_list_request_handlers)) {
            runtime_publish_semantic_decode_error(bus_, ev);
        }
        return;
    case SystemServiceType::kTopicListResponse:
        if (!runtime_fanout_snapshot<TopicListResponse>(
                impl_->mu, ev.envelope, ev.envelope.payload, impl_->topic_list_response_handlers)) {
            runtime_publish_semantic_decode_error(bus_, ev);
        }
        return;
    case SystemServiceType::kTopicSubscriptionRequest:
        if (!runtime_fanout_inbound_request<TopicSubscriptionRequest>(
                impl_->mu, ev, ev.envelope.payload, impl_->topic_subscription_request_handlers)) {
            runtime_publish_semantic_decode_error(bus_, ev);
        }
        return;
    case SystemServiceType::kTopicSubscriptionResponse:
        if (!runtime_fanout_snapshot<TopicSubscriptionResponse>(
                impl_->mu,
                ev.envelope,
                ev.envelope.payload,
                impl_->topic_subscription_response_handlers)) {
            runtime_publish_semantic_decode_error(bus_, ev);
        }
        return;
    case SystemServiceType::kManagedEntityListRequest:
        if (!runtime_fanout_inbound_request<ManagedEntityListRequest>(
                impl_->mu, ev, ev.envelope.payload, impl_->managed_entity_list_request_handlers)) {
            runtime_publish_semantic_decode_error(bus_, ev);
        }
        return;
    case SystemServiceType::kManagedEntityListResponse: {
        ManagedEntityListResponse response;
        SessionDescriptor session;
        if (!decode_payload(ev.envelope.payload, &response) ||
            (!describe_session_internal(ev.peer.id, ev.envelope.session_id, &session) &&
             !describe_session_internal(ev.envelope.session_id, &session)) ||
            !runtime_same_entity(ev.envelope.source, session.remote_identity) ||
            !valid_managed_entity_directory(response, session)) {
            ErrorEvent error;
            error.code = ErrorCode::kUnauthorized;
            error.transport = ev.transport;
            error.peer = ev.peer;
            error.message = "managed-entity-directory-invalid";
            bus_.publish_error(error);
            return;
        }
        std::lock_guard<std::mutex> lock(impl_->mu);
        auto it =
            std::find_if(impl_->sessions.begin(), impl_->sessions.end(), [&](const auto& item) {
                return item.second.session_id == ev.envelope.session_id;
            });
        if (it == impl_->sessions.end()) {
            return;
        }
        it->second.remote_managed_identities.clear();
        for (const ManagedEntityDescriptor& entity : response.entities) {
            if (!runtime_same_entity(entity.identity, response.primary_identity)) {
                it->second.remote_managed_identities.push_back(entity.identity);
            }
        }
    }
        if (!runtime_fanout_snapshot<ManagedEntityListResponse>(
                impl_->mu,
                ev.envelope,
                ev.envelope.payload,
                impl_->managed_entity_list_response_handlers)) {
            runtime_publish_semantic_decode_error(bus_, ev);
        }
        return;
    case SystemServiceType::kManagedEntityDirectoryChanged: {
        SessionDescriptor session;
        if ((!describe_session_internal(ev.peer.id, ev.envelope.session_id, &session) &&
             !describe_session_internal(ev.envelope.session_id, &session)) ||
            !runtime_same_entity(ev.envelope.source, session.remote_identity)) {
            return;
        }
        std::lock_guard<std::mutex> lock(impl_->mu);
        auto it =
            std::find_if(impl_->sessions.begin(), impl_->sessions.end(), [&](const auto& item) {
                return item.second.session_id == ev.envelope.session_id;
            });
        if (it != impl_->sessions.end()) {
            it->second.remote_managed_identities.clear();
        }
    }
        if (!runtime_fanout_snapshot<ManagedEntityDirectoryChanged>(
                impl_->mu,
                ev.envelope,
                ev.envelope.payload,
                impl_->managed_entity_directory_changed_handlers)) {
            runtime_publish_semantic_decode_error(bus_, ev);
        }
        return;
    case SystemServiceType::kManagedEntityAttachmentRequest:
        if (!runtime_fanout_inbound_request<ManagedEntityAttachmentRequest>(
                impl_->mu,
                ev,
                ev.envelope.payload,
                impl_->managed_entity_attachment_request_handlers)) {
            runtime_publish_semantic_decode_error(bus_, ev);
        }
        return;
    case SystemServiceType::kManagedEntityAttachmentResponse:
        if (!runtime_fanout_snapshot<ManagedEntityAttachmentResponse>(
                impl_->mu,
                ev.envelope,
                ev.envelope.payload,
                impl_->managed_entity_attachment_response_handlers)) {
            runtime_publish_semantic_decode_error(bus_, ev);
        }
        return;
    }
}

}  // namespace yunlink
