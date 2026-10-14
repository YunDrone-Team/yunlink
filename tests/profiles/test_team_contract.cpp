#include <cassert>
#include <fstream>
#include <iomanip>
#include <limits>
#include <map>
#include <sstream>
#include <string>

#include "com.yundrone.sunray/v2/control_validation.hpp"
#include "yunlink/core/core_messages_v2.hpp"

namespace {
std::map<std::string, std::string> vectors() {
    std::ifstream input(std::string(YUNLINK_SOURCE_DIR) +
                        "/profiles/com.yundrone.sunray/v2/golden/team-contract-vectors.txt");
    assert(input.good());
    std::map<std::string, std::string> result;
    for (std::string line; std::getline(input, line);) {
        if (line.empty() || line.front() == '#')
            continue;
        const auto separator = line.find('=');
        assert(separator != std::string::npos);
        result.emplace(line.substr(0, separator), line.substr(separator + 1));
    }
    return result;
}

std::string hex(const std::string& bytes) {
    std::ostringstream output;
    output << std::hex << std::setfill('0');
    for (unsigned char byte : bytes)
        output << std::setw(2) << static_cast<unsigned>(byte);
    return output.str();
}

yunlink::v2::Bytes unhex(const std::string& value) {
    yunlink::v2::Bytes result;
    for (size_t i = 0; i < value.size(); i += 2)
        result.push_back(static_cast<uint8_t>(std::stoul(value.substr(i, 2), nullptr, 16)));
    return result;
}

template <typename T>
void check(const std::map<std::string, std::string>& values, const char* name, const T& message) {
    const auto bytes = message.SerializeAsString();
    assert(hex(bytes) == values.at(std::string(name) + ".hex"));
    T decoded;
    assert(decoded.ParseFromString(bytes));
    assert(decoded.SerializeAsString() == bytes);
}
}  // namespace

int main() {
    using namespace com::yundrone::sunray::v2;
    const auto values = vectors();
    org::yunlink::mobility::v1::Pose pose;
    pose.mutable_position()->set_x(1);
    pose.mutable_position()->set_y(2);
    pose.mutable_position()->set_z(3);
    pose.mutable_orientation()->set_w(1);
    FormationSetRequest set;
    set.set_formation_type(FORMATION_TAKEOFF);
    set.set_team_id("team_1");
    assert(validate_formation_set_request(set));
    check(values, "formation_set", set);
    set.clear_team_id();
    assert(validate_formation_set_request(set));
    FormationLeaderTargetRequest target;
    target.set_target_mode(FORMATION_LEADER_TARGET_FIXED_POSE);
    target.set_source_stamp_ns(42);
    target.set_frame_id("map");
    *target.mutable_target_pose() = pose;
    target.set_team_id("team_1");
    assert(validate_formation_leader_target_request(target));
    check(values, "formation_leader_target", target);
    target.clear_team_id();
    assert(validate_formation_leader_target_request(target));
    FormationState state;
    state.set_virtual_leader_target_valid(true);
    *state.mutable_virtual_leader_target() = pose;
    state.set_virtual_leader_team_id("team_1");
    assert(validate_formation_state(state));
    check(values, "formation_state", state);
    TeamGoalRequest goal;
    goal.set_frame_id("map");
    goal.set_source_stamp_ns(42);
    *goal.mutable_target_pose() = pose;
    goal.set_team_id("team_1");
    assert(validate_team_goal_request(goal));
    check(values, "team_goal", goal);
    for (int field : {1, 3, 4}) {
        auto invalid = goal;
        invalid.GetReflection()->ClearField(&invalid,
                                            invalid.GetDescriptor()->FindFieldByNumber(field));
        assert(!validate_team_goal_request(invalid));
    }
    auto invalid = goal;
    invalid.mutable_target_pose()->mutable_orientation()->set_w(0);
    assert(!validate_team_goal_request(invalid));
    for (double value : {std::numeric_limits<double>::quiet_NaN(),
                         std::numeric_limits<double>::infinity(),
                         -std::numeric_limits<double>::infinity()}) {
        invalid = goal;
        invalid.mutable_target_pose()->mutable_position()->set_x(value);
        assert(!validate_team_goal_request(invalid));
        invalid = goal;
        invalid.mutable_target_pose()->mutable_orientation()->set_x(value);
        assert(!validate_team_goal_request(invalid));
    }
    TeamGoalResponse response;
    response.set_success(true);
    response.set_message("ok");
    response.set_team_id("team_1");
    response.set_planner_agent_id("uav1");
    check(values, "team_goal_response", response);
    TeamDemoStartRequest demo;
    demo.set_demo_id("demo");
    demo.set_target_agent_name("uav1");
    demo.set_target_agent_id(1);
    check(values, "team_demo_start", demo);
    TeamDemoStartResponse demo_response;
    demo_response.set_success(true);
    demo_response.set_message("ok");
    demo_response.set_demo_id("demo");
    demo_response.set_target_agent_name("uav1");
    demo_response.set_target_agent_id(1);
    check(values, "team_demo_response", demo_response);
    FeatureDescriptor feature;
    feature.set_name("demo");
    feature.set_demo_id("demo");
    feature.set_lifecycle_mode(TEAM_DEMO_MANAGED_TAKEOFF_THEN_GOAL);
    feature.set_target_scope("team");
    check(values, "feature", feature);
    for (int value = 0; value <= 3; ++value)
        assert(TeamDemoLifecycleMode_IsValid(value));
    LocalizationState localization;
    localization.set_status(LOCALIZATION_PUBLISHER_MISSING);
    localization.set_reason("missing");
    check(values, "localization", localization);
    StartupQueueItem item;
    item.set_auto_start_locked(true);
    check(values, "startup_queue_item", item);

    yunlink::ConfigResourceDescribeRequest describe;
    describe.resource_id = "sunray.params.flight";
    describe.locale = "zh-CN";
    const auto request_bytes = unhex(values.at("describe.zh-CN.hex"));
    assert(yunlink::v2::encode(describe) == request_bytes);
    yunlink::ConfigResourceDescribeRequest decoded_request;
    assert(yunlink::v2::decode(request_bytes, &decoded_request));
    assert(decoded_request.locale == "zh-CN");
    const auto response_bytes = unhex(values.at("describe.advanced.hex"));
    yunlink::ConfigResourceDescribeResponse decoded_response;
    assert(yunlink::v2::decode(response_bytes, &decoded_response));
    assert(decoded_response.fields.size() == 2);
    assert(decoded_response.fields[0].advanced);
    assert(!decoded_response.fields[1].advanced);
    assert(yunlink::v2::encode(decoded_response) == response_bytes);
    auto truncated = response_bytes;
    truncated.pop_back();
    assert(!yunlink::v2::decode(truncated, &decoded_response));
}
