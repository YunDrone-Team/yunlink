/**
 * @file include/yunlink/runtime/configuration_service.hpp
 * @brief Runtime configuration resource publish and subscription facades.
 */

#ifndef YUNLINK_RUNTIME_CONFIGURATION_SERVICE_HPP
#define YUNLINK_RUNTIME_CONFIGURATION_SERVICE_HPP

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "yunlink/core/semantic_messages.hpp"
#include "yunlink/core/event_bus.hpp"
#include "yunlink/core/types.hpp"

namespace yunlink {

class Runtime;

template <typename T> struct InboundConfigurationServiceRequestView {
    EnvelopeEvent inbound;
    T payload;
};

class ConfigurationServicePublisher {
  public:
    explicit ConfigurationServicePublisher(Runtime* runtime = nullptr);

    ErrorCode publish_resource_list_request(const std::string& peer_id,
                                            uint64_t session_id,
                                            const TargetSelector& target,
                                            const ConfigResourceListRequest& payload,
                                            ConfigurationServiceHandle* out_handle = nullptr);
    ErrorCode publish_resource_describe_request(const std::string& peer_id,
                                                uint64_t session_id,
                                                const TargetSelector& target,
                                                const ConfigResourceDescribeRequest& payload,
                                                ConfigurationServiceHandle* out_handle = nullptr);
    ErrorCode publish_resource_get_request(const std::string& peer_id,
                                           uint64_t session_id,
                                           const TargetSelector& target,
                                           const ConfigResourceGetRequest& payload,
                                           ConfigurationServiceHandle* out_handle = nullptr);
    ErrorCode publish_resource_patch_request(const std::string& peer_id,
                                             uint64_t session_id,
                                             const TargetSelector& target,
                                             const ConfigResourcePatchRequest& payload,
                                             ConfigurationServiceHandle* out_handle = nullptr);
    ErrorCode publish_resource_apply_request(const std::string& peer_id,
                                             uint64_t session_id,
                                             const TargetSelector& target,
                                             const ConfigResourceApplyRequest& payload,
                                             ConfigurationServiceHandle* out_handle = nullptr);

    ErrorCode publish_resource_list_response(const EnvelopeEvent& inbound,
                                             const ConfigResourceListResponse& payload,
                                             uint32_t ttl_ms = 3000);
    ErrorCode publish_resource_describe_response(const EnvelopeEvent& inbound,
                                                 const ConfigResourceDescribeResponse& payload,
                                                 uint32_t ttl_ms = 3000);
    ErrorCode publish_resource_get_response(const EnvelopeEvent& inbound,
                                            const ConfigResourceGetResponse& payload,
                                            uint32_t ttl_ms = 3000);
    ErrorCode publish_resource_patch_response(const EnvelopeEvent& inbound,
                                              const ConfigResourcePatchResponse& payload,
                                              uint32_t ttl_ms = 3000);
    ErrorCode publish_resource_apply_response(const EnvelopeEvent& inbound,
                                              const ConfigResourceApplyResponse& payload,
                                              uint32_t ttl_ms = 3000);
    void bind(Runtime* runtime);

  private:
    Runtime* runtime_ = nullptr;
};

class ConfigurationServiceSubscriber {
  public:
    using ResourceListRequestHandler = std::function<void(
        const InboundConfigurationServiceRequestView<ConfigResourceListRequest>&)>;
    using ResourceListResponseHandler =
        std::function<void(const TypedMessage<ConfigResourceListResponse>&)>;
    using ResourceDescribeRequestHandler = std::function<void(
        const InboundConfigurationServiceRequestView<ConfigResourceDescribeRequest>&)>;
    using ResourceDescribeResponseHandler =
        std::function<void(const TypedMessage<ConfigResourceDescribeResponse>&)>;
    using ResourceGetRequestHandler = std::function<void(
        const InboundConfigurationServiceRequestView<ConfigResourceGetRequest>&)>;
    using ResourceGetResponseHandler =
        std::function<void(const TypedMessage<ConfigResourceGetResponse>&)>;
    using ResourcePatchRequestHandler = std::function<void(
        const InboundConfigurationServiceRequestView<ConfigResourcePatchRequest>&)>;
    using ResourcePatchResponseHandler =
        std::function<void(const TypedMessage<ConfigResourcePatchResponse>&)>;
    using ResourceApplyRequestHandler = std::function<void(
        const InboundConfigurationServiceRequestView<ConfigResourceApplyRequest>&)>;
    using ResourceApplyResponseHandler =
        std::function<void(const TypedMessage<ConfigResourceApplyResponse>&)>;

    explicit ConfigurationServiceSubscriber(Runtime* runtime = nullptr);

    size_t subscribe_resource_list_requests(ResourceListRequestHandler cb);
    size_t subscribe_resource_list_responses(ResourceListResponseHandler cb);
    size_t subscribe_resource_describe_requests(ResourceDescribeRequestHandler cb);
    size_t subscribe_resource_describe_responses(ResourceDescribeResponseHandler cb);
    size_t subscribe_resource_get_requests(ResourceGetRequestHandler cb);
    size_t subscribe_resource_get_responses(ResourceGetResponseHandler cb);
    size_t subscribe_resource_patch_requests(ResourcePatchRequestHandler cb);
    size_t subscribe_resource_patch_responses(ResourcePatchResponseHandler cb);
    size_t subscribe_resource_apply_requests(ResourceApplyRequestHandler cb);
    size_t subscribe_resource_apply_responses(ResourceApplyResponseHandler cb);
    void unsubscribe(size_t token);
    void bind(Runtime* runtime);

  private:
    Runtime* runtime_ = nullptr;
};

class ConfigurationResourceProvider {
  public:
    virtual ~ConfigurationResourceProvider() = default;
    virtual ConfigResourceDescriptor descriptor() const = 0;
    virtual ConfigResourceDescribeResponse describe() const = 0;
    virtual ConfigResourceGetResponse get() = 0;
    virtual ConfigResourcePatchResponse patch(const ConfigResourcePatchRequest& request) = 0;
    virtual ConfigResourceApplyResponse apply(const ConfigResourceApplyRequest& request) = 0;
};

/**
 * Thread-safe provider registry. Authentication, authorization and unsafe-state
 * checks remain application policy and run before mutation dispatch.
 */
class ConfigurationProviderRegistry {
  public:
    ConfigurationProviderRegistry();
    ~ConfigurationProviderRegistry();
    ConfigurationProviderRegistry(const ConfigurationProviderRegistry&) = delete;
    ConfigurationProviderRegistry& operator=(const ConfigurationProviderRegistry&) = delete;

    ErrorCode register_provider(std::shared_ptr<ConfigurationResourceProvider> provider);
    bool unregister_provider(const std::string& resource_id);
    std::vector<ConfigResourceDescriptor> list_resources() const;
    ConfigResourceDescribeResponse describe(const std::string& resource_id) const;
    ConfigResourceGetResponse get(const std::string& resource_id) const;
    ConfigResourcePatchResponse patch(const ConfigResourcePatchRequest& request) const;
    ConfigResourceApplyResponse apply(const ConfigResourceApplyRequest& request) const;

  private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace yunlink

#endif  // YUNLINK_RUNTIME_CONFIGURATION_SERVICE_HPP
