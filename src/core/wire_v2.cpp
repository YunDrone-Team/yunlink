/** @file src/core/wire_v2.cpp */

#include "yunlink/core/wire_v2.hpp"

#include <algorithm>
#include <cctype>
#include <utility>

namespace yunlink::v2 {
namespace {

TargetSelector target(TargetScope scope, std::string uid) {
    TargetSelector out;
    out.scope = scope;
    out.uids.push_back(std::move(uid));
    return out;
}

bool contains(const std::vector<std::string>& values, const std::string& value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}

}  // namespace

TargetSelector TargetSelector::endpoint(std::string uid) {
    return target(TargetScope::kEndpoint, std::move(uid));
}

TargetSelector TargetSelector::entity(std::string uid) {
    return target(TargetScope::kEntity, std::move(uid));
}

TargetSelector TargetSelector::group(std::string uid) {
    return target(TargetScope::kGroup, std::move(uid));
}

TargetSelector TargetSelector::broadcast() {
    return {};
}

bool TargetSelector::matches(const std::string& endpoint_uid,
                             const std::vector<std::string>& entity_uids,
                             const std::vector<std::string>& group_uids) const {
    if (scope == TargetScope::kBroadcast) {
        return uids.empty();
    }
    if (uids.empty()) {
        return false;
    }
    if (scope == TargetScope::kEndpoint) {
        return contains(uids, endpoint_uid);
    }
    if (scope == TargetScope::kEntity) {
        return std::any_of(
            uids.begin(), uids.end(), [&](const auto& uid) { return contains(entity_uids, uid); });
    }
    return std::any_of(
        uids.begin(), uids.end(), [&](const auto& uid) { return contains(group_uids, uid); });
}

bool valid_uid(const std::string& value) {
    if (value.empty() || value.size() > kMaxUidBytes) {
        return false;
    }
    return std::all_of(value.begin(), value.end(), [](unsigned char ch) {
        return std::isalnum(ch) != 0 || ch == '.' || ch == '_' || ch == ':' || ch == '-';
    });
}

bool valid_profile_id(const std::string& value) {
    return valid_uid(value) && value.size() <= kMaxProfileIdBytes;
}

bool valid_type_ref(const TypeRef& value) {
    return valid_profile_id(value.profile_id) && !value.type_name.empty() &&
           value.type_name.size() <= kMaxTypeNameBytes && value.major > 0;
}

}  // namespace yunlink::v2
