#pragma once

#include <cstdint>
#include <string>

#include "org.yunlink.system/v1/system.pb.h"

namespace org::yunlink::system::v1 {
constexpr uint64_t kMinimumTrustedUnixTimeMs = 1704067200000ULL;
constexpr uint64_t kMaximumTrustedUnixTimeMs = 4102444800000ULL;
bool validate_clock_sync_request(const ClockSyncRequest&, std::string* error = nullptr);
bool validate_clock_sync_response(const ClockSyncResponse&, std::string* error = nullptr);
}  // namespace org::yunlink::system::v1
