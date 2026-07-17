/** @file @brief Provider-neutral configuration resource registry. */

#include "yunlink/runtime/configuration_service.hpp"

#include <map>
#include <mutex>
#include <utility>

namespace yunlink {
namespace {

template <typename Response> Response failure(ConfigServiceStatus status, std::string message) {
    Response response;
    response.status = status;
    response.message = std::move(message);
    return response;
}

}  // namespace

class ConfigurationProviderRegistry::Impl {
  public:
    std::shared_ptr<ConfigurationResourceProvider> find(const std::string& id) const {
        std::lock_guard<std::mutex> lock(mutex);
        const auto iterator = providers.find(id);
        return iterator == providers.end() ? nullptr : iterator->second;
    }

    mutable std::mutex mutex;
    std::map<std::string, std::shared_ptr<ConfigurationResourceProvider>> providers;
};

ConfigurationProviderRegistry::ConfigurationProviderRegistry() : impl_(new Impl()) {}
ConfigurationProviderRegistry::~ConfigurationProviderRegistry() = default;

ErrorCode ConfigurationProviderRegistry::register_provider(
    std::shared_ptr<ConfigurationResourceProvider> provider) {
    if (!provider) {
        return ErrorCode::kInvalidArgument;
    }
    ConfigResourceDescriptor descriptor;
    try {
        descriptor = provider->descriptor();
    } catch (...) {
        return ErrorCode::kInternal;
    }
    if (descriptor.id.empty()) {
        return ErrorCode::kInvalidArgument;
    }
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->providers.emplace(descriptor.id, std::move(provider)).second
               ? ErrorCode::kOk
               : ErrorCode::kRejected;
}

bool ConfigurationProviderRegistry::unregister_provider(const std::string& resource_id) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->providers.erase(resource_id) != 0;
}

std::vector<ConfigResourceDescriptor> ConfigurationProviderRegistry::list_resources() const {
    std::vector<std::shared_ptr<ConfigurationResourceProvider>> providers;
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        providers.reserve(impl_->providers.size());
        for (const auto& item : impl_->providers) {
            providers.push_back(item.second);
        }
    }
    std::vector<ConfigResourceDescriptor> descriptors;
    descriptors.reserve(providers.size());
    for (const auto& provider : providers) {
        try {
            descriptors.push_back(provider->descriptor());
        } catch (...) {  // NOLINT(bugprone-empty-catch): isolate a failing provider from the list.
        }
    }
    return descriptors;
}

ConfigResourceDescribeResponse
ConfigurationProviderRegistry::describe(const std::string& resource_id) const {
    const auto provider = impl_->find(resource_id);
    if (!provider) {
        return failure<ConfigResourceDescribeResponse>(ConfigServiceStatus::kNotFound,
                                                       "configuration resource not found");
    }
    try {
        return provider->describe();
    } catch (...) {
        return failure<ConfigResourceDescribeResponse>(ConfigServiceStatus::kInternalError,
                                                       "configuration provider failed");
    }
}

ConfigResourceGetResponse ConfigurationProviderRegistry::get(const std::string& resource_id) const {
    const auto provider = impl_->find(resource_id);
    if (!provider) {
        return failure<ConfigResourceGetResponse>(ConfigServiceStatus::kNotFound,
                                                  "configuration resource not found");
    }
    try {
        return provider->get();
    } catch (...) {
        return failure<ConfigResourceGetResponse>(ConfigServiceStatus::kInternalError,
                                                  "configuration provider failed");
    }
}

ConfigResourcePatchResponse
ConfigurationProviderRegistry::patch(const ConfigResourcePatchRequest& request) const {
    const auto provider = impl_->find(request.resource_id);
    if (!provider) {
        return failure<ConfigResourcePatchResponse>(ConfigServiceStatus::kNotFound,
                                                    "configuration resource not found");
    }
    try {
        return provider->patch(request);
    } catch (...) {
        return failure<ConfigResourcePatchResponse>(ConfigServiceStatus::kInternalError,
                                                    "configuration provider failed");
    }
}

ConfigResourceApplyResponse
ConfigurationProviderRegistry::apply(const ConfigResourceApplyRequest& request) const {
    const auto provider = impl_->find(request.resource_id);
    if (!provider) {
        return failure<ConfigResourceApplyResponse>(ConfigServiceStatus::kNotFound,
                                                    "configuration resource not found");
    }
    try {
        return provider->apply(request);
    } catch (...) {
        return failure<ConfigResourceApplyResponse>(ConfigServiceStatus::kInternalError,
                                                    "configuration provider failed");
    }
}

}  // namespace yunlink
