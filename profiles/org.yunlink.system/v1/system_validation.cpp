#include "org.yunlink.system/v1/system_validation.hpp"

#include <algorithm>

namespace org::yunlink::system::v1 {
namespace {
bool fail(std::string* error, const char* detail) {
    if (error != nullptr) *error = detail;
    return false;
}
bool valid_source(const std::string& source) {
    return !source.empty() && source.size() <= 64 &&
           std::all_of(source.begin(), source.end(), [](unsigned char value) {
               return (value >= 'a' && value <= 'z') || (value >= 'A' && value <= 'Z') ||
                      (value >= '0' && value <= '9') || value == '-' || value == '_' || value == '.';
           });
}
}  // namespace

bool validate_clock_sync_request(const ClockSyncRequest& request, std::string* error) {
    if (request.unix_time_ms() < kMinimumTrustedUnixTimeMs ||
        request.unix_time_ms() > kMaximumTrustedUnixTimeMs) {
        return fail(error, "clock sync time is outside the product range");
    }
    return valid_source(request.source()) ? true : fail(error, "clock sync source is invalid");
}

bool validate_clock_sync_response(const ClockSyncResponse& response, std::string* error) {
    if (response.error() <= CLOCK_SYNC_UNSPECIFIED ||
        response.error() > CLOCK_SYNC_INTERNAL_ERROR || response.message().size() > 256) {
        return fail(error, "clock sync response enum or message is invalid");
    }
    const bool ok = response.error() == CLOCK_SYNC_OK;
    if (ok && (response.previous_unix_time_ms() < kMinimumTrustedUnixTimeMs ||
               response.previous_unix_time_ms() > kMaximumTrustedUnixTimeMs ||
               response.applied_unix_time_ms() < kMinimumTrustedUnixTimeMs ||
               response.applied_unix_time_ms() > kMaximumTrustedUnixTimeMs ||
               response.delta_ms() != static_cast<int64_t>(response.applied_unix_time_ms()) -
                                          static_cast<int64_t>(response.previous_unix_time_ms()))) {
        return fail(error, "successful clock sync response has invalid timestamps");
    }
    if (!ok && (response.previous_unix_time_ms() != 0 || response.applied_unix_time_ms() != 0 ||
                response.delta_ms() != 0)) {
        return fail(error, "failed clock sync response must not contain applied timestamps");
    }
    return true;
}
}  // namespace org::yunlink::system::v1
