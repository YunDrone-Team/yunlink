/**
 * @file include/yunlink/runtime/system_service.hpp
 * @brief Runtime system service publish and subscription facades.
 */

#ifndef YUNLINK_RUNTIME_SYSTEM_SERVICE_HPP
#define YUNLINK_RUNTIME_SYSTEM_SERVICE_HPP

#include <cstddef>
#include <functional>
#include <string>

#include "yunlink/core/semantic_messages.hpp"

namespace yunlink {

class Runtime;

template <typename T> struct InboundSystemServiceRequestView {
    EnvelopeEvent inbound;
    T payload;
};

class SystemServicePublisher {
  public:
    explicit SystemServicePublisher(Runtime* runtime = nullptr);

    ErrorCode publish_feature_list_request(const std::string& peer_id,
                                           uint64_t session_id,
                                           const TargetSelector& target,
                                           const FeatureListRequest& payload,
                                           SystemServiceHandle* out_handle = nullptr);
    ErrorCode publish_feature_get_request(const std::string& peer_id,
                                          uint64_t session_id,
                                          const TargetSelector& target,
                                          const FeatureGetRequest& payload,
                                          SystemServiceHandle* out_handle = nullptr);
    ErrorCode publish_feature_start_request(const std::string& peer_id,
                                            uint64_t session_id,
                                            const TargetSelector& target,
                                            const FeatureStartRequest& payload,
                                            SystemServiceHandle* out_handle = nullptr);
    ErrorCode publish_feature_stop_request(const std::string& peer_id,
                                           uint64_t session_id,
                                           const TargetSelector& target,
                                           const FeatureStopRequest& payload,
                                           SystemServiceHandle* out_handle = nullptr);
    ErrorCode publish_feature_list_response(const EnvelopeEvent& inbound,
                                            const FeatureListResponse& payload,
                                            uint32_t ttl_ms = 3000);
    ErrorCode publish_feature_get_response(const EnvelopeEvent& inbound,
                                           const FeatureGetResponse& payload,
                                           uint32_t ttl_ms = 3000);
    ErrorCode publish_feature_start_response(const EnvelopeEvent& inbound,
                                             const FeatureStartResponse& payload,
                                             uint32_t ttl_ms = 3000);
    ErrorCode publish_feature_stop_response(const EnvelopeEvent& inbound,
                                            const FeatureStopResponse& payload,
                                            uint32_t ttl_ms = 3000);
    ErrorCode publish_runtime_log_list_request(const std::string& peer_id,
                                               uint64_t session_id,
                                               const TargetSelector& target,
                                               const RuntimeLogListRequest& payload,
                                               SystemServiceHandle* out_handle = nullptr);
    ErrorCode publish_runtime_log_read_request(const std::string& peer_id,
                                               uint64_t session_id,
                                               const TargetSelector& target,
                                               const RuntimeLogReadRequest& payload,
                                               SystemServiceHandle* out_handle = nullptr);
    ErrorCode publish_runtime_log_list_response(const EnvelopeEvent& inbound,
                                                const RuntimeLogListResponse& payload,
                                                uint32_t ttl_ms = 5000);
    ErrorCode publish_runtime_log_read_response(const EnvelopeEvent& inbound,
                                                const RuntimeLogReadResponse& payload,
                                                uint32_t ttl_ms = 5000);
    ErrorCode publish_topic_list_request(const std::string& peer_id,
                                         uint64_t session_id,
                                         const TargetSelector& target,
                                         const TopicListRequest& payload,
                                         SystemServiceHandle* out_handle = nullptr);
    ErrorCode publish_topic_list_response(const EnvelopeEvent& inbound,
                                          const TopicListResponse& payload,
                                          uint32_t ttl_ms = 3000);
    ErrorCode publish_topic_subscription_request(const std::string& peer_id,
                                                 uint64_t session_id,
                                                 const TargetSelector& target,
                                                 const TopicSubscriptionRequest& payload,
                                                 SystemServiceHandle* out_handle = nullptr);
    ErrorCode publish_topic_subscription_response(const EnvelopeEvent& inbound,
                                                  const TopicSubscriptionResponse& payload,
                                                  uint32_t ttl_ms = 3000);
    ErrorCode publish_managed_entity_list_request(
        const std::string& peer_id,
        uint64_t session_id,
        const TargetSelector& target,
        const ManagedEntityListRequest& payload,
        SystemServiceHandle* out_handle = nullptr);
    ErrorCode publish_managed_entity_list_response(
        const EnvelopeEvent& inbound,
        const ManagedEntityListResponse& payload,
        uint32_t ttl_ms = 3000);
    ErrorCode publish_managed_entity_directory_changed(
        const std::string& peer_id,
        uint64_t session_id,
        const TargetSelector& target,
        const ManagedEntityDirectoryChanged& payload,
        SystemServiceHandle* out_handle = nullptr);
    void bind(Runtime* runtime);

  private:
    Runtime* runtime_ = nullptr;
};

class SystemServiceSubscriber {
  public:
    using FeatureListRequestHandler =
        std::function<void(const InboundSystemServiceRequestView<FeatureListRequest>&)>;
    using FeatureListResponseHandler =
        std::function<void(const TypedMessage<FeatureListResponse>&)>;
    using FeatureGetRequestHandler =
        std::function<void(const InboundSystemServiceRequestView<FeatureGetRequest>&)>;
    using FeatureGetResponseHandler = std::function<void(const TypedMessage<FeatureGetResponse>&)>;
    using FeatureStartRequestHandler =
        std::function<void(const InboundSystemServiceRequestView<FeatureStartRequest>&)>;
    using FeatureStartResponseHandler =
        std::function<void(const TypedMessage<FeatureStartResponse>&)>;
    using FeatureStopRequestHandler =
        std::function<void(const InboundSystemServiceRequestView<FeatureStopRequest>&)>;
    using FeatureStopResponseHandler =
        std::function<void(const TypedMessage<FeatureStopResponse>&)>;
    using RuntimeLogListRequestHandler =
        std::function<void(const InboundSystemServiceRequestView<RuntimeLogListRequest>&)>;
    using RuntimeLogListResponseHandler =
        std::function<void(const TypedMessage<RuntimeLogListResponse>&)>;
    using RuntimeLogReadRequestHandler =
        std::function<void(const InboundSystemServiceRequestView<RuntimeLogReadRequest>&)>;
    using RuntimeLogReadResponseHandler =
        std::function<void(const TypedMessage<RuntimeLogReadResponse>&)>;
    using TopicListRequestHandler =
        std::function<void(const InboundSystemServiceRequestView<TopicListRequest>&)>;
    using TopicListResponseHandler = std::function<void(const TypedMessage<TopicListResponse>&)>;
    using TopicSubscriptionRequestHandler =
        std::function<void(const InboundSystemServiceRequestView<TopicSubscriptionRequest>&)>;
    using TopicSubscriptionResponseHandler =
        std::function<void(const TypedMessage<TopicSubscriptionResponse>&)>;
    using ManagedEntityListRequestHandler =
        std::function<void(const InboundSystemServiceRequestView<ManagedEntityListRequest>&)>;
    using ManagedEntityListResponseHandler =
        std::function<void(const TypedMessage<ManagedEntityListResponse>&)>;
    using ManagedEntityDirectoryChangedHandler =
        std::function<void(const TypedMessage<ManagedEntityDirectoryChanged>&)>;

    explicit SystemServiceSubscriber(Runtime* runtime = nullptr);

    size_t subscribe_feature_list_requests(FeatureListRequestHandler cb);
    size_t subscribe_feature_list_responses(FeatureListResponseHandler cb);
    size_t subscribe_feature_get_requests(FeatureGetRequestHandler cb);
    size_t subscribe_feature_get_responses(FeatureGetResponseHandler cb);
    size_t subscribe_feature_start_requests(FeatureStartRequestHandler cb);
    size_t subscribe_feature_start_responses(FeatureStartResponseHandler cb);
    size_t subscribe_feature_stop_requests(FeatureStopRequestHandler cb);
    size_t subscribe_feature_stop_responses(FeatureStopResponseHandler cb);
    size_t subscribe_runtime_log_list_requests(RuntimeLogListRequestHandler cb);
    size_t subscribe_runtime_log_list_responses(RuntimeLogListResponseHandler cb);
    size_t subscribe_runtime_log_read_requests(RuntimeLogReadRequestHandler cb);
    size_t subscribe_runtime_log_read_responses(RuntimeLogReadResponseHandler cb);
    size_t subscribe_topic_list_requests(TopicListRequestHandler cb);
    size_t subscribe_topic_list_responses(TopicListResponseHandler cb);
    size_t subscribe_topic_subscription_requests(TopicSubscriptionRequestHandler cb);
    size_t subscribe_topic_subscription_responses(TopicSubscriptionResponseHandler cb);
    size_t subscribe_managed_entity_list_requests(ManagedEntityListRequestHandler cb);
    size_t subscribe_managed_entity_list_responses(ManagedEntityListResponseHandler cb);
    size_t subscribe_managed_entity_directory_changed(ManagedEntityDirectoryChangedHandler cb);
    void unsubscribe(size_t token);
    void bind(Runtime* runtime);

  private:
    Runtime* runtime_ = nullptr;
};

}  // namespace yunlink

#endif  // YUNLINK_RUNTIME_SYSTEM_SERVICE_HPP
