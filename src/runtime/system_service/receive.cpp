/**
 * @file src/runtime/system_service/receive.cpp
 * @brief Runtime inbound system service dispatch.
 */

#include "../state/fanout.hpp"

namespace yunlink {

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

    if (!ev.envelope.target.matches(config_.self_identity)) {
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
                impl_->mu,
                ev,
                ev.envelope.payload,
                impl_->topic_subscription_request_handlers)) {
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
    }
}

}  // namespace yunlink
