#include "org.yunlink.telemetry/v1/summary_validation.hpp"

#include <cmath>
#include <unordered_set>

namespace org::yunlink::telemetry::v1 {
namespace {

bool fail(const char* message, std::string* error) {
    if (error != nullptr) {
        *error = message;
    }
    return false;
}

}  // namespace

bool valid_metric_key(const std::string& key) {
    if (key.empty() || key.size() > kMetricMaxKeyBytes) {
        return false;
    }
    std::size_t segment_count = 1;
    bool segment_start = true;
    for (const unsigned char character : key) {
        if (character == '.') {
            if (segment_start) {
                return false;
            }
            segment_start = true;
            ++segment_count;
            continue;
        }
        if (segment_start) {
            if (character < 'a' || character > 'z') {
                return false;
            }
            segment_start = false;
            continue;
        }
        const bool lowercase = character >= 'a' && character <= 'z';
        const bool digit = character >= '0' && character <= '9';
        if (!lowercase && !digit && character != '_') {
            return false;
        }
    }
    return !segment_start && segment_count >= 3;
}

bool validate_summary_snapshot(const SummarySnapshot& snapshot, std::string* error) {
    if (snapshot.metrics_size() > static_cast<int>(kSummaryMaxMetrics)) {
        return fail("too many metrics", error);
    }
    if (snapshot.ByteSizeLong() > kSummaryMaxPayloadBytes) {
        return fail("summary payload exceeds limit", error);
    }
    std::unordered_set<std::string> keys;
    for (const auto& metric : snapshot.metrics()) {
        if (!valid_metric_key(metric.key())) {
            return fail("invalid metric key", error);
        }
        if (!keys.insert(metric.key()).second) {
            return fail("duplicate metric key", error);
        }
        if (metric.unit().size() > kMetricMaxUnitBytes) {
            return fail("metric unit exceeds limit", error);
        }
        if (metric.quality() == METRIC_QUALITY_UNSPECIFIED ||
            metric.quality() > METRIC_UNAVAILABLE) {
            return fail("invalid metric quality", error);
        }
        const auto value_case = metric.value().value_case();
        if ((metric.quality() == METRIC_VALID || metric.quality() == METRIC_STALE) &&
            value_case == MetricValue::VALUE_NOT_SET) {
            return fail("metric value is required", error);
        }
        if (value_case == MetricValue::kDoubleValue &&
            !std::isfinite(metric.value().double_value())) {
            return fail("metric double is not finite", error);
        }
        if (value_case == MetricValue::kEnumToken &&
            metric.value().enum_token().size() > kMetricMaxEnumBytes) {
            return fail("metric enum token exceeds limit", error);
        }
        if (value_case == MetricValue::kTextValue &&
            metric.value().text_value().size() > kMetricMaxTextBytes) {
            return fail("metric text exceeds limit", error);
        }
    }
    if (error != nullptr) {
        error->clear();
    }
    return true;
}

}  // namespace org::yunlink::telemetry::v1
