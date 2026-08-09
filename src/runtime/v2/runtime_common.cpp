#include "runtime_internal.hpp"

#include <chrono>

namespace yunlink::v2 {
namespace {

void append_u16(Bytes& out, uint16_t value) {
    out.push_back(static_cast<uint8_t>(value));
    out.push_back(static_cast<uint8_t>(value >> 8U));
}

bool take_u16(const Bytes& input, size_t* cursor, uint16_t* out) {
    if (*cursor + 2 > input.size()) {
        return false;
    }
    *out = static_cast<uint16_t>(input[*cursor]) | static_cast<uint16_t>(input[*cursor + 1] << 8U);
    *cursor += 2;
    return true;
}

void append_text(Bytes& out, const std::string& value) {
    append_u16(out, static_cast<uint16_t>(value.size()));
    out.insert(out.end(), value.begin(), value.end());
}

bool take_text(const Bytes& input, size_t* cursor, std::string* out) {
    uint16_t length = 0;
    if (!take_u16(input, cursor, &length) || *cursor + length > input.size()) {
        return false;
    }
    out->assign(reinterpret_cast<const char*>(input.data() + *cursor), length);
    *cursor += length;
    return true;
}

}  // namespace

uint64_t runtime_now_ms() {
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                     std::chrono::system_clock::now().time_since_epoch())
                                     .count());
}

std::string runtime_peer_id(const std::string& ip, uint16_t port) {
    return "tcp://" + ip + ":" + std::to_string(port);
}

void runtime_emit(Runtime::Impl* impl, const RuntimeEvent& event) {
    std::vector<Runtime::EventHandler> handlers;
    {
        std::lock_guard<std::mutex> lock(impl->mutex);
        handlers.reserve(impl->handlers.size());
        for (const auto& entry : impl->handlers) {
            handlers.push_back(entry.second);
        }
    }
    for (const auto& handler : handlers) {
        handler(event);
    }
}

Bytes encode_text(const std::string& value) {
    if (value.size() > UINT16_MAX) {
        return {};
    }
    Bytes out;
    append_text(out, value);
    return out;
}

bool decode_text(const Bytes& payload, std::string* value) {
    if (value == nullptr) {
        return false;
    }
    size_t cursor = 0;
    return take_text(payload, &cursor, value) && cursor == payload.size();
}

Bytes encode_profile_list(const std::vector<ProfileDescriptor>& profiles) {
    if (profiles.size() > UINT16_MAX) {
        return {};
    }
    Bytes out;
    append_u16(out, static_cast<uint16_t>(profiles.size()));
    for (const auto& profile : profiles) {
        if (!valid_profile_id(profile.profile_id) || profile.schema_digest.size() > UINT16_MAX) {
            return {};
        }
        append_text(out, profile.profile_id);
        append_u16(out, profile.major);
        append_u16(out, profile.minor);
        append_text(out, profile.schema_digest);
    }
    return out;
}

bool decode_profile_list(const Bytes& payload, std::vector<ProfileDescriptor>* profiles) {
    if (profiles == nullptr) {
        return false;
    }
    size_t cursor = 0;
    uint16_t count = 0;
    if (!take_u16(payload, &cursor, &count)) {
        return false;
    }
    std::vector<ProfileDescriptor> parsed;
    parsed.reserve(count);
    for (uint16_t i = 0; i < count; ++i) {
        ProfileDescriptor profile;
        if (!take_text(payload, &cursor, &profile.profile_id) ||
            !take_u16(payload, &cursor, &profile.major) ||
            !take_u16(payload, &cursor, &profile.minor) ||
            !take_text(payload, &cursor, &profile.schema_digest) ||
            !valid_profile_id(profile.profile_id) || profile.major == 0) {
            return false;
        }
        parsed.push_back(std::move(profile));
    }
    if (cursor != payload.size()) {
        return false;
    }
    *profiles = std::move(parsed);
    return true;
}

bool SessionInfo::has_profile(const std::string& profile_id, uint16_t major) const {
    return supports_profile(profile_id, major, 0);
}

bool SessionInfo::supports_profile(const std::string& profile_id,
                                   uint16_t major,
                                   uint16_t minimum_minor) const {
    const auto it = negotiated_profiles.find(profile_id);
    return it != negotiated_profiles.end() && it->second.major == major &&
           it->second.minor >= minimum_minor;
}

}  // namespace yunlink::v2
