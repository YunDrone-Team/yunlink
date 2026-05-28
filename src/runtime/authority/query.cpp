/**
 * @file src/runtime/authority/query.cpp
 * @brief Runtime authority lease query operations.
 */

#include "../core/internal.hpp"

namespace yunlink {

bool Runtime::current_authority(AuthorityLease* out) const {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const uint64_t now_ms = runtime_now_millis();
    for (const auto& item : impl_->authorities) {
        const AuthorityLease& lease = item.second;
        if (lease.state != AuthorityState::kController) {
            continue;
        }
        if (lease.expires_at_ms > 0 && lease.expires_at_ms < now_ms) {
            continue;
        }
        if (out != nullptr) {
            *out = lease;
        }
        return true;
    }
    return false;
}

bool Runtime::current_authority_for_target(const TargetSelector& target,
                                           AuthorityLease* out) const {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const auto it = impl_->authorities.find(runtime_target_key(target));
    if (it == impl_->authorities.end()) {
        return false;
    }
    const AuthorityLease& lease = it->second;
    if (lease.state != AuthorityState::kController) {
        return false;
    }
    if (lease.expires_at_ms > 0 && lease.expires_at_ms < runtime_now_millis()) {
        return false;
    }
    if (out != nullptr) {
        *out = lease;
    }
    return true;
}

}  // namespace yunlink
