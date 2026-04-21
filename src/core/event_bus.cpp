/**
 * @file src/core/event_bus.cpp
 * @brief sunray_communication_lib source file.
 */

#include "sunraycom/core/event_bus.hpp"

namespace sunraycom {

size_t EventBus::subscribe_envelope(EnvelopeHandler cb) {
    std::lock_guard<std::mutex> lock(mu_);
    const size_t token = next_token_++;
    envelope_handlers_[token] = std::move(cb);
    return token;
}

size_t EventBus::subscribe_error(ErrorHandler cb) {
    std::lock_guard<std::mutex> lock(mu_);
    const size_t token = next_token_++;
    error_handlers_[token] = std::move(cb);
    return token;
}

size_t EventBus::subscribe_link(LinkHandler cb) {
    std::lock_guard<std::mutex> lock(mu_);
    const size_t token = next_token_++;
    link_handlers_[token] = std::move(cb);
    return token;
}

void EventBus::unsubscribe(size_t token) {
    std::lock_guard<std::mutex> lock(mu_);
    envelope_handlers_.erase(token);
    error_handlers_.erase(token);
    link_handlers_.erase(token);
}

void EventBus::publish_envelope(const EnvelopeEvent& ev) const {
    std::unordered_map<size_t, EnvelopeHandler> copy;
    {
        std::lock_guard<std::mutex> lock(mu_);
        copy = envelope_handlers_;
    }
    for (const auto& item : copy) {
        if (item.second) {
            item.second(ev);
        }
    }
}

void EventBus::publish_error(const ErrorEvent& ev) const {
    std::unordered_map<size_t, ErrorHandler> copy;
    {
        std::lock_guard<std::mutex> lock(mu_);
        copy = error_handlers_;
    }
    for (const auto& item : copy) {
        if (item.second) {
            item.second(ev);
        }
    }
}

void EventBus::publish_link(const LinkEvent& ev) const {
    std::unordered_map<size_t, LinkHandler> copy;
    {
        std::lock_guard<std::mutex> lock(mu_);
        copy = link_handlers_;
    }
    for (const auto& item : copy) {
        if (item.second) {
            item.second(ev);
        }
    }
}

}  // namespace sunraycom
