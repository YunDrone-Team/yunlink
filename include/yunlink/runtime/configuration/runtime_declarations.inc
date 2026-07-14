    ErrorCode publish_configuration_service_request_payload(
        const std::string& peer_id,
        uint64_t session_id,
        const TargetSelector& target,
        uint16_t message_type,
        const ByteBuffer& payload,
        ConfigurationServiceHandle* out_handle,
        uint32_t ttl_ms);
    ErrorCode reply_configuration_service_payload(const EnvelopeEvent& inbound,
                                                  uint16_t message_type,
                                                  const ByteBuffer& payload,
                                                  uint32_t ttl_ms);
    size_t subscribe_config_resource_list_request_internal(
        ConfigurationServiceSubscriber::ResourceListRequestHandler cb);
    size_t subscribe_config_resource_list_response_internal(
        ConfigurationServiceSubscriber::ResourceListResponseHandler cb);
    size_t subscribe_config_resource_describe_request_internal(
        ConfigurationServiceSubscriber::ResourceDescribeRequestHandler cb);
    size_t subscribe_config_resource_describe_response_internal(
        ConfigurationServiceSubscriber::ResourceDescribeResponseHandler cb);
    size_t subscribe_config_resource_get_request_internal(
        ConfigurationServiceSubscriber::ResourceGetRequestHandler cb);
    size_t subscribe_config_resource_get_response_internal(
        ConfigurationServiceSubscriber::ResourceGetResponseHandler cb);
    size_t subscribe_config_resource_patch_request_internal(
        ConfigurationServiceSubscriber::ResourcePatchRequestHandler cb);
    size_t subscribe_config_resource_patch_response_internal(
        ConfigurationServiceSubscriber::ResourcePatchResponseHandler cb);
    size_t subscribe_config_resource_apply_request_internal(
        ConfigurationServiceSubscriber::ResourceApplyRequestHandler cb);
    size_t subscribe_config_resource_apply_response_internal(
        ConfigurationServiceSubscriber::ResourceApplyResponseHandler cb);
    void unsubscribe_configuration_semantic_locked(size_t token);
