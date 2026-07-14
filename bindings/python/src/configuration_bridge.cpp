#include "configuration_bridge_internal.hpp"

namespace nb = nanobind;
using namespace yunlink_python_config;

void ConfigurationBridge::Impl::push(OwnedResponse response) noexcept {
    try {
        std::lock_guard<std::mutex> lock(mutex);
        if (responses.size() >= 256) {
            responses.pop_front();
        }
        responses.push_back(std::move(response));
    } catch (...) {
    }
}

ConfigurationBridge::ConfigurationBridge() : impl_(new Impl()) {}
ConfigurationBridge::~ConfigurationBridge() = default;

void ConfigurationBridge::attach(yunlink_runtime_t* runtime) {
    const auto subscribe = [&](auto function, auto callback) {
        size_t token = 0;
        throwIfError(function(runtime, callback, this, &token));
        impl_->tokens.push_back(token);
    };
    subscribe(yunlink_configuration_subscribe_resource_list_responses, &onList);
    subscribe(yunlink_configuration_subscribe_resource_describe_responses, &onDescribe);
    subscribe(yunlink_configuration_subscribe_resource_get_responses, &onGet);
    subscribe(yunlink_configuration_subscribe_resource_patch_responses, &onPatch);
    subscribe(yunlink_configuration_subscribe_resource_apply_responses, &onApply);
}

void ConfigurationBridge::detach(yunlink_runtime_t* runtime) noexcept {
    for (size_t token : impl_->tokens) {
        (void)yunlink_configuration_unsubscribe(runtime, token);
    }
    impl_->tokens.clear();
}

nb::object ConfigurationBridge::poll() {
    OwnedResponse response;
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        if (impl_->responses.empty()) {
            return nb::none();
        }
        response = std::move(impl_->responses.front());
        impl_->responses.pop_front();
    }
    return nb::cast(responseToPython(response));
}
