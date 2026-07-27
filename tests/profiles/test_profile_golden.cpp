#include <cassert>
#include <limits>
#include <iomanip>
#include <sstream>
#include <string>

#include "com.yundrone.sunray/v1/sunray.pb.h"
#include "com.yundrone.sunray/v1/control_validation.hpp"
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

void valid_yaw(com::yundrone::sunray::v1::UavDirectControlGoal* goal) {
    goal->mutable_yaw()->set_mode(com::yundrone::sunray::v1::UAV_YAW_KEEP);
    goal->set_controller(com::yundrone::sunray::v1::UAV_CONTROLLER_DEFAULT);
}

template <typename Message> void assert_round_trip(const Message& source) {
    Message decoded;
    assert(decoded.ParseFromString(source.SerializeAsString()));
    assert(decoded.SerializeAsString() == source.SerializeAsString());
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

    using namespace com::yundrone::sunray::v1;
    UavDirectControlGoal world_position;
    valid_yaw(&world_position);
    world_position.mutable_world_position()->set_frame_id("map");
    world_position.mutable_world_position()->mutable_position_m()->set_z(1.0);
    assert(validate_uav_direct_control_goal(world_position, &validation_error));
    assert_round_trip(world_position);

    UavDirectControlGoal body_position;
    valid_yaw(&body_position);
    body_position.mutable_body_position()->mutable_body_xy_position_m()->set_x(1.0);
    body_position.mutable_body_position()->set_fixed_height_m(2.0);
    assert(validate_uav_direct_control_goal(body_position, &validation_error));
    assert_round_trip(body_position);

    UavDirectControlGoal trajectory;
    trajectory.mutable_trajectory_setpoint()->set_frame_id("map");
    auto* trajectory_position = trajectory.mutable_trajectory_setpoint()->mutable_position_m();
    trajectory_position->set_x(1.0);
    trajectory_position->set_y(-2.0);
    trajectory_position->set_z(0.5);
    auto* trajectory_velocity = trajectory.mutable_trajectory_setpoint()->mutable_velocity_mps();
    trajectory_velocity->set_x(0.1);
    trajectory_velocity->set_y(0.2);
    trajectory_velocity->set_z(-0.3);
    auto* trajectory_acceleration =
        trajectory.mutable_trajectory_setpoint()->mutable_acceleration_mps2();
    trajectory_acceleration->set_x(0.01);
    trajectory_acceleration->set_y(0.02);
    trajectory_acceleration->set_z(0.03);
    trajectory.mutable_yaw()->set_mode(UAV_YAW_SET_ANGLE);
    trajectory.mutable_yaw()->set_value(0.25);
    trajectory.set_controller(UAV_CONTROLLER_POSITION);
    trajectory.set_lease_ms(750);
    assert(validate_uav_direct_control_goal(trajectory, &validation_error));
    assert_round_trip(trajectory);
    assert(hex(trajectory.SerializeAsString()) ==
           "1a5c0a036d6170121b09000000000000f03f1100000000000000c019000000000000e03f1a1b099a"
           "9999999999b93f119a9999999999c93f19333333333333d3bf221b097b14ae47e17a843f117b14ae"
           "47e17a943f19b81e85eb51b89e3f320b080111000000000000d03f380140ee05");

    UavDirectControlGoal world_velocity;
    valid_yaw(&world_velocity);
    world_velocity.mutable_world_velocity()->set_frame_id("map");
    world_velocity.mutable_world_velocity()->mutable_velocity_mps()->set_x(0.5);
    world_velocity.mutable_world_velocity()->mutable_height_lock()->set_height_m(2.0);
    world_velocity.set_lease_ms(kMinDirectControlLeaseMs);
    assert(validate_uav_direct_control_goal(world_velocity, &validation_error));
    assert_round_trip(world_velocity);

    UavDirectControlGoal body_velocity;
    valid_yaw(&body_velocity);
    body_velocity.mutable_body_velocity()->mutable_body_xy_velocity_mps()->set_y(0.5);
    body_velocity.mutable_body_velocity()->set_fixed_height_m(2.0);
    body_velocity.set_lease_ms(kMaxDirectControlLeaseMs);
    assert(validate_uav_direct_control_goal(body_velocity, &validation_error));
    assert_round_trip(body_velocity);

    body_velocity.set_lease_ms(kMinDirectControlLeaseMs - 1);
    assert(!validate_uav_direct_control_goal(body_velocity, &validation_error));
    body_position.mutable_body_position()->set_fixed_height_m(0.0);
    assert(!validate_uav_direct_control_goal(body_position, &validation_error));
    trajectory_acceleration->set_z(std::numeric_limits<double>::infinity());
    assert(!validate_uav_direct_control_goal(trajectory, &validation_error));

    UavWaypointMissionGoal mission;
    mission.set_frame_id("map");
    auto* first = mission.add_waypoints();
    first->mutable_position_m()->set_x(1.0);
    first->mutable_position_m()->set_y(2.0);
    first->mutable_position_m()->set_z(3.0);
    first->set_yaw_rad(0.5);
    first->set_hold_time_s(1.5);
    auto* second = mission.add_waypoints();
    second->mutable_position_m()->set_x(-1.0);
    second->mutable_position_m()->set_y(-2.0);
    second->mutable_position_m()->set_z(4.0);
    second->set_yaw_rad(-0.25);
    mission.set_interrupt_current_task(true);
    assert(validate_uav_waypoint_mission_goal(mission, &validation_error));
    assert_round_trip(mission);
    assert(hex(mission.SerializeAsString()) ==
           "0a036d6170122f0a1b09000000000000f03f1100000000000000401900000000000008401100000000"
           "0000e03f19000000000000f83f12260a1b09000000000000f0bf1100000000000000c0190000000000"
           "00104011000000000000d0bf1801");

    UavWaypointMissionGoal empty_mission;
    empty_mission.set_frame_id("map");
    assert(!validate_uav_waypoint_mission_goal(empty_mission, &validation_error));
    for (int index = 0; index <= kMaxWaypointCount; ++index) {
        empty_mission.add_waypoints()->mutable_position_m();
    }
    assert(!validate_uav_waypoint_mission_goal(empty_mission, &validation_error));
    return 0;
}
