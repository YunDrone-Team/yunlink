/** @file @brief Configuration service handler registry. */

#include "../core/internal.hpp"

namespace yunlink {

#define YUNLINK_DEFINE_CONFIG_SUBSCRIBE(METHOD, HANDLER, MAP)                                      \
    size_t Runtime::METHOD(ConfigurationServiceSubscriber::HANDLER cb) {                           \
        std::lock_guard<std::mutex> lock(impl_->mu);                                               \
        const size_t token = impl_->next_token++;                                                  \
        impl_->MAP[token] = std::move(cb);                                                         \
        return token;                                                                              \
    }

YUNLINK_DEFINE_CONFIG_SUBSCRIBE(subscribe_config_resource_list_request_internal,
                                ResourceListRequestHandler,
                                config_resource_list_request_handlers)
YUNLINK_DEFINE_CONFIG_SUBSCRIBE(subscribe_config_resource_list_response_internal,
                                ResourceListResponseHandler,
                                config_resource_list_response_handlers)
YUNLINK_DEFINE_CONFIG_SUBSCRIBE(subscribe_config_resource_describe_request_internal,
                                ResourceDescribeRequestHandler,
                                config_resource_describe_request_handlers)
YUNLINK_DEFINE_CONFIG_SUBSCRIBE(subscribe_config_resource_describe_response_internal,
                                ResourceDescribeResponseHandler,
                                config_resource_describe_response_handlers)
YUNLINK_DEFINE_CONFIG_SUBSCRIBE(subscribe_config_resource_get_request_internal,
                                ResourceGetRequestHandler,
                                config_resource_get_request_handlers)
YUNLINK_DEFINE_CONFIG_SUBSCRIBE(subscribe_config_resource_get_response_internal,
                                ResourceGetResponseHandler,
                                config_resource_get_response_handlers)
YUNLINK_DEFINE_CONFIG_SUBSCRIBE(subscribe_config_resource_patch_request_internal,
                                ResourcePatchRequestHandler,
                                config_resource_patch_request_handlers)
YUNLINK_DEFINE_CONFIG_SUBSCRIBE(subscribe_config_resource_patch_response_internal,
                                ResourcePatchResponseHandler,
                                config_resource_patch_response_handlers)
YUNLINK_DEFINE_CONFIG_SUBSCRIBE(subscribe_config_resource_apply_request_internal,
                                ResourceApplyRequestHandler,
                                config_resource_apply_request_handlers)
YUNLINK_DEFINE_CONFIG_SUBSCRIBE(subscribe_config_resource_apply_response_internal,
                                ResourceApplyResponseHandler,
                                config_resource_apply_response_handlers)

#undef YUNLINK_DEFINE_CONFIG_SUBSCRIBE

void Runtime::unsubscribe_configuration_semantic_locked(size_t token) {
    impl_->config_resource_list_request_handlers.erase(token);
    impl_->config_resource_list_response_handlers.erase(token);
    impl_->config_resource_describe_request_handlers.erase(token);
    impl_->config_resource_describe_response_handlers.erase(token);
    impl_->config_resource_get_request_handlers.erase(token);
    impl_->config_resource_get_response_handlers.erase(token);
    impl_->config_resource_patch_request_handlers.erase(token);
    impl_->config_resource_patch_response_handlers.erase(token);
    impl_->config_resource_apply_request_handlers.erase(token);
    impl_->config_resource_apply_response_handlers.erase(token);
}

ConfigurationServiceSubscriber::ConfigurationServiceSubscriber(Runtime* runtime)
    : runtime_(runtime) {}

void ConfigurationServiceSubscriber::bind(Runtime* runtime) {
    runtime_ = runtime;
}

#define YUNLINK_DEFINE_CONFIG_SUBSCRIBER(METHOD, HANDLER, INTERNAL)                                \
    size_t ConfigurationServiceSubscriber::METHOD(HANDLER cb) {                                    \
        return runtime_ ? runtime_->INTERNAL(std::move(cb)) : 0;                                   \
    }

YUNLINK_DEFINE_CONFIG_SUBSCRIBER(subscribe_resource_list_requests,
                                 ResourceListRequestHandler,
                                 subscribe_config_resource_list_request_internal)
YUNLINK_DEFINE_CONFIG_SUBSCRIBER(subscribe_resource_list_responses,
                                 ResourceListResponseHandler,
                                 subscribe_config_resource_list_response_internal)
YUNLINK_DEFINE_CONFIG_SUBSCRIBER(subscribe_resource_describe_requests,
                                 ResourceDescribeRequestHandler,
                                 subscribe_config_resource_describe_request_internal)
YUNLINK_DEFINE_CONFIG_SUBSCRIBER(subscribe_resource_describe_responses,
                                 ResourceDescribeResponseHandler,
                                 subscribe_config_resource_describe_response_internal)
YUNLINK_DEFINE_CONFIG_SUBSCRIBER(subscribe_resource_get_requests,
                                 ResourceGetRequestHandler,
                                 subscribe_config_resource_get_request_internal)
YUNLINK_DEFINE_CONFIG_SUBSCRIBER(subscribe_resource_get_responses,
                                 ResourceGetResponseHandler,
                                 subscribe_config_resource_get_response_internal)
YUNLINK_DEFINE_CONFIG_SUBSCRIBER(subscribe_resource_patch_requests,
                                 ResourcePatchRequestHandler,
                                 subscribe_config_resource_patch_request_internal)
YUNLINK_DEFINE_CONFIG_SUBSCRIBER(subscribe_resource_patch_responses,
                                 ResourcePatchResponseHandler,
                                 subscribe_config_resource_patch_response_internal)
YUNLINK_DEFINE_CONFIG_SUBSCRIBER(subscribe_resource_apply_requests,
                                 ResourceApplyRequestHandler,
                                 subscribe_config_resource_apply_request_internal)
YUNLINK_DEFINE_CONFIG_SUBSCRIBER(subscribe_resource_apply_responses,
                                 ResourceApplyResponseHandler,
                                 subscribe_config_resource_apply_response_internal)

#undef YUNLINK_DEFINE_CONFIG_SUBSCRIBER

void ConfigurationServiceSubscriber::unsubscribe(size_t token) {
    if (runtime_) {
        runtime_->unsubscribe_semantic(token);
    }
}

}  // namespace yunlink
