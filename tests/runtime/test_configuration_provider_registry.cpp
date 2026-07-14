/** @file @brief C++ configuration provider registration contract. */

#include <memory>

#include "yunlink/runtime/configuration_service.hpp"

namespace {

class TestProvider final : public yunlink::ConfigurationResourceProvider {
  public:
    yunlink::ConfigResourceDescriptor descriptor() const override {
        return {"vendor.test", "Test", "Test provider", true, true, true};
    }
    yunlink::ConfigResourceDescribeResponse describe() const override {
        yunlink::ConfigResourceDescribeResponse response;
        response.status = yunlink::ConfigServiceStatus::kOk;
        response.resource = descriptor();
        return response;
    }
    yunlink::ConfigResourceGetResponse get() override {
        yunlink::ConfigResourceGetResponse response;
        response.status = yunlink::ConfigServiceStatus::kOk;
        response.snapshot.resource_id = descriptor().id;
        response.snapshot.revision = "r1";
        return response;
    }
    yunlink::ConfigResourcePatchResponse
    patch(const yunlink::ConfigResourcePatchRequest& request) override {
        yunlink::ConfigResourcePatchResponse response;
        response.status = yunlink::ConfigServiceStatus::kOk;
        response.snapshot.resource_id = request.resource_id;
        response.snapshot.revision = "r2";
        return response;
    }
    yunlink::ConfigResourceApplyResponse
    apply(const yunlink::ConfigResourceApplyRequest& request) override {
        yunlink::ConfigResourceApplyResponse response;
        response.status = yunlink::ConfigServiceStatus::kOk;
        response.applied_revision = request.expected_revision;
        response.outcome = yunlink::ConfigApplyOutcome::kApplied;
        return response;
    }
};

}  // namespace

int main() {
    yunlink::ConfigurationProviderRegistry registry;
    auto provider = std::make_shared<TestProvider>();
    if (registry.register_provider(provider) != yunlink::ErrorCode::kOk ||
        registry.register_provider(provider) != yunlink::ErrorCode::kRejected ||
        registry.list_resources().size() != 1) {
        return 1;
    }
    if (registry.describe("missing").status != yunlink::ConfigServiceStatus::kNotFound ||
        registry.get("vendor.test").snapshot.revision != "r1") {
        return 2;
    }
    yunlink::ConfigResourcePatchRequest patch;
    patch.resource_id = "vendor.test";
    if (registry.patch(patch).snapshot.revision != "r2") {
        return 3;
    }
    yunlink::ConfigResourceApplyRequest apply{"vendor.test", "r2"};
    if (registry.apply(apply).applied_revision != "r2" ||
        !registry.unregister_provider("vendor.test") ||
        registry.unregister_provider("vendor.test")) {
        return 4;
    }
    return 0;
}
