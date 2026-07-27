#include <cassert>
#include <limits>
#include <iomanip>
#include <sstream>
#include <string>

#include "com.yundrone.sunray/v1/sunray.pb.h"
#include "org.yunlink.mobility/v1/mobility.pb.h"
#include "org.yunlink.telemetry/v1/summary_validation.hpp"

namespace {

std::string hex(const std::string& value) {
    std::ostringstream stream;
    stream << std::hex << std::setfill('0');
    for (unsigned char byte : value) {
        stream << std::setw(2) << static_cast<unsigned>(byte);
    }
    return stream.str();
}

}  // namespace

int main() {
    org::yunlink::mobility::v1::GotoGoal goal;
    goal.set_frame_id("map");
    goal.mutable_position()->set_x(1.0);
    goal.mutable_position()->set_y(-2.0);
    goal.mutable_position()->set_z(0.5);
    goal.set_yaw_rad(0.25);
    assert(hex(goal.SerializeAsString()) ==
           "0a036d6170121b09000000000000f03f1100000000000000c019000000000000e03f190000000000"
           "00d03f");

    com::yundrone::sunray::v1::FeatureStartRequest request;
    request.set_name("mapping");
    assert(hex(request.SerializeAsString()) == "0a076d617070696e67");

    const std::string damaged("\x0a\x08mapping", 9);
    assert(!request.ParseFromString(damaged));

    org::yunlink::telemetry::v1::SummarySnapshot summary;
    summary.set_generated_at_ns(1);
    auto* metric = summary.add_metrics();
    metric->set_key("org.test.ready");
    metric->mutable_value()->set_bool_value(true);
    metric->set_quality(org::yunlink::telemetry::v1::METRIC_VALID);
    metric->set_source_timestamp_ns(2);
    assert(org::yunlink::telemetry::v1::validate_summary_snapshot(summary));
    assert(hex(summary.SerializeAsString()) ==
           "080112180a0e6f72672e746573742e72656164791202080120012802");

    *summary.add_metrics() = *metric;
    std::string validation_error;
    assert(!org::yunlink::telemetry::v1::validate_summary_snapshot(summary, &validation_error));
    assert(validation_error == "duplicate metric key");

    summary.mutable_metrics()->RemoveLast();
    metric->mutable_value()->set_double_value(std::numeric_limits<double>::infinity());
    assert(!org::yunlink::telemetry::v1::validate_summary_snapshot(summary, &validation_error));
    assert(validation_error == "metric double is not finite");

    metric->set_key("Org.test.invalid");
    metric->mutable_value()->set_text_value("diagnostic");
    assert(!org::yunlink::telemetry::v1::validate_summary_snapshot(summary, &validation_error));
    assert(validation_error == "invalid metric key");
    return 0;
}
