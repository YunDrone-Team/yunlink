/** @file @brief Runtime system-service subscription registry. */

#include "../core/internal.hpp"

namespace yunlink {

size_t Runtime::subscribe_feature_list_request_internal(
    SystemServiceSubscriber::FeatureListRequestHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->feature_list_request_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_feature_list_response_internal(
    SystemServiceSubscriber::FeatureListResponseHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->feature_list_response_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_feature_get_request_internal(
    SystemServiceSubscriber::FeatureGetRequestHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->feature_get_request_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_feature_get_response_internal(
    SystemServiceSubscriber::FeatureGetResponseHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->feature_get_response_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_feature_start_request_internal(
    SystemServiceSubscriber::FeatureStartRequestHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->feature_start_request_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_feature_start_response_internal(
    SystemServiceSubscriber::FeatureStartResponseHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->feature_start_response_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_feature_stop_request_internal(
    SystemServiceSubscriber::FeatureStopRequestHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->feature_stop_request_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_feature_stop_response_internal(
    SystemServiceSubscriber::FeatureStopResponseHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->feature_stop_response_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_runtime_log_list_request_internal(
    SystemServiceSubscriber::RuntimeLogListRequestHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->runtime_log_list_request_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_runtime_log_list_response_internal(
    SystemServiceSubscriber::RuntimeLogListResponseHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->runtime_log_list_response_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_runtime_log_read_request_internal(
    SystemServiceSubscriber::RuntimeLogReadRequestHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->runtime_log_read_request_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_runtime_log_read_response_internal(
    SystemServiceSubscriber::RuntimeLogReadResponseHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->runtime_log_read_response_handlers[token] = std::move(cb);
    return token;
}

}  // namespace yunlink
