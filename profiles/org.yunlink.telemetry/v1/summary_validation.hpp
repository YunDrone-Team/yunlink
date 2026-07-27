#pragma once

#include <cstddef>
#include <string>

#include "org.yunlink.telemetry/v1/telemetry.pb.h"

namespace org::yunlink::telemetry::v1 {

constexpr std::size_t kSummaryMaxMetrics = 64;
constexpr std::size_t kSummaryMaxPayloadBytes = 16 * 1024;
constexpr std::size_t kMetricMaxKeyBytes = 128;
constexpr std::size_t kMetricMaxUnitBytes = 16;
constexpr std::size_t kMetricMaxEnumBytes = 64;
constexpr std::size_t kMetricMaxTextBytes = 256;

bool valid_metric_key(const std::string& key);
bool validate_summary_snapshot(const SummarySnapshot& snapshot, std::string* error = nullptr);

}  // namespace org::yunlink::telemetry::v1
