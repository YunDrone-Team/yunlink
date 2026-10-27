#include <cassert>
#include <limits>
#include <iomanip>
#include <sstream>
#include <string>
#include "com.yundrone.sunray/v2/sunray.pb.h"
#include "com.yundrone.sunray/v2/control_validation.hpp"
namespace {
std::string hex(const std::string& value) {
    std::ostringstream stream;
    stream << std::hex << std::setfill('0');
    for (unsigned char byte : value) {
        stream << std::setw(2) << static_cast<unsigned>(byte);
    }
    return stream.str();
}

template <typename Message> void assert_round_trip(const Message& source) {
    Message decoded;
    assert(decoded.ParseFromString(source.SerializeAsString()));
    assert(decoded.SerializeAsString() == source.SerializeAsString());
}

}  // namespace
int main() {
    using namespace com::yundrone::sunray::v2;
    std::string validation_error;
    FormationSetRequest ring;
    ring.set_formation_type(FORMATION_DYNAMIC_RING);
    ring.set_align_yaw_with_trajectory(true);
    ring.mutable_ring()->set_radius_m(3.0);
    ring.mutable_ring()->set_move_speed_mps(-0.5);
    assert(validate_formation_set_request(ring, &validation_error));
    assert(hex(ring.SerializeAsString()) == "081510012a1209000000000000084011000000000000e0bf");

    // 13/23 (static / dynamic double ring) were removed. Both wire numbers are
    // reserved: an old peer sending either one must be rejected, not reinterpreted.
    FormationSetRequest removed_ring_type;
    removed_ring_type.set_formation_type(static_cast<FormationType>(13));
    assert(!validate_formation_set_request(removed_ring_type, &validation_error));
    assert(validation_error.find("double-ring formations were removed") != std::string::npos);
    removed_ring_type.set_formation_type(static_cast<FormationType>(23));
    assert(!validate_formation_set_request(removed_ring_type, &validation_error));
    assert(validation_error.find("double-ring formations were removed") != std::string::npos);

    FormationSetRequest leader;
    leader.set_formation_type(FORMATION_LEADER);
    for (int index = 0; index < 25; ++index) {
        leader.mutable_leader()->add_agent_slots(index < 2 ? index + 1 : 0);
        leader.mutable_leader()->add_virtual_leader_slots(index == 0);
    }
    leader.mutable_leader()->set_spacing_m(2.0);
    assert(validate_formation_set_request(leader, &validation_error));

    FormationLeaderTargetRequest target;
    target.set_target_mode(FORMATION_LEADER_TARGET_FIXED_POSE);
    target.set_source_stamp_ns(42);
    target.set_frame_id("map");
    target.mutable_target_pose()->mutable_position()->set_x(1.0);
    target.mutable_target_pose()->mutable_position()->set_y(2.0);
    target.mutable_target_pose()->mutable_position()->set_z(3.0);
    target.mutable_target_pose()->mutable_orientation()->set_w(1.0);
    assert(validate_formation_leader_target_request(target, &validation_error));
    assert(hex(target.SerializeAsString()) ==
           "0801102a1a036d617022280a1b09000000000000f03f110000000000000040190000000000000840"
           "120921000000000000f03f");

    // 2.17 小队目标：与 FormationLeaderTargetRequest 是**两条独立 action**（wire 也不共享字段）。
    // team_id 必填非空是本消息的**安全契约**：空值必须在协议层被拒。
    TeamGoalRequest team_goal;
    team_goal.set_frame_id("map");
    team_goal.set_source_stamp_ns(42);
    team_goal.mutable_target_pose()->mutable_position()->set_x(1.0);
    team_goal.mutable_target_pose()->mutable_position()->set_y(2.0);
    team_goal.mutable_target_pose()->mutable_position()->set_z(3.0);
    team_goal.mutable_target_pose()->mutable_orientation()->set_w(1.0);
    team_goal.set_team_id("team_1");
    assert(validate_team_goal_request(team_goal, &validation_error));
    assert(hex(team_goal.SerializeAsString()) ==
           "0a036d6170102a1a280a1b09000000000000f03f110000000000000040190000000000000840120921"
           "000000000000f03f22067465616d5f31");
    // 空 team_id：拒绝（不是"空 = 全部小队"——那正是被拆掉的旧通道病根）。
    TeamGoalRequest team_goal_missing_team = team_goal;
    team_goal_missing_team.clear_team_id();
    assert(!validate_team_goal_request(team_goal_missing_team, &validation_error));
    assert(validation_error.find("team_id") != std::string::npos);
    // 空 frame_id / 缺 target_pose / 四元数范数为零：逐一拒绝。
    TeamGoalRequest team_goal_missing_frame = team_goal;
    team_goal_missing_frame.clear_frame_id();
    assert(!validate_team_goal_request(team_goal_missing_frame, &validation_error));
    TeamGoalRequest team_goal_missing_pose = team_goal;
    team_goal_missing_pose.clear_target_pose();
    assert(!validate_team_goal_request(team_goal_missing_pose, &validation_error));
    TeamGoalRequest team_goal_zero_quaternion = team_goal;
    team_goal_zero_quaternion.mutable_target_pose()->mutable_orientation()->set_w(0.0);
    assert(!validate_team_goal_request(team_goal_zero_quaternion, &validation_error));

    TeamGoalResponse team_goal_response;
    team_goal_response.set_success(true);
    team_goal_response.set_message("accepted by uav1");
    team_goal_response.set_team_id("team_1");
    team_goal_response.set_planner_agent_id("uav1");
    assert_round_trip(team_goal_response);

    FormationState formation_state;
    formation_state.set_source_stamp_ns(42);
    formation_state.set_frame_id("map");
    formation_state.set_agent_id("uav1");
    formation_state.mutable_task_epoch()->set_domain_created_ns(10);
    formation_state.mutable_task_epoch()->set_origin_agent_id("uav1");
    formation_state.mutable_task_epoch()->set_origin_sequence(2);
    formation_state.set_phase(FORMATION_PHASE_ACTIVE);
    formation_state.set_formation_type(FORMATION_LEADER);
    formation_state.set_formation_established(true);
    formation_state.set_virtual_leader_target_valid(true);
    *formation_state.mutable_virtual_leader_target() = target.target_pose();
    assert(validate_formation_state(formation_state, &validation_error));
    assert(hex(formation_state.SerializeAsString()) ==
           "082a12036d61701a0475617631220a080a12047561763118023802400c480160016a280a1b09000000"
           "000000f03f110000000000000040190000000000000840120921000000000000f03f");

    FormationState spatial_state;
    spatial_state.set_source_stamp_ns(42);
    spatial_state.set_agent_id("uav1");
    spatial_state.set_formation_type(FORMATION_DYNAMIC_RING);
    spatial_state.set_phase(FORMATION_PHASE_ACTIVE);
    spatial_state.set_state_sequence(7);
    assert(validate_formation_state(spatial_state, &validation_error));
    assert(hex(spatial_state.SerializeAsString()) == "082a1a047561763138024015980107");
    assert_round_trip(spatial_state);

    FormationState bad_spatial_type = spatial_state;
    bad_spatial_type.set_formation_type(static_cast<FormationType>(14));
    assert(!validate_formation_state(bad_spatial_type, &validation_error));

    // 2.10：逐成员槽位目标回显（字段 22/23）。valid ⇒ 位姿必须存在且有限；
    // valid == false 时字段整体缺省必须放行（这是"当前没有已受理目标"的唯一表达）。
    FormationState target_echo = spatial_state;
    target_echo.set_formation_target_valid(true);
    assert(!validate_formation_state(target_echo, &validation_error));  // valid 却没有位姿
    target_echo.mutable_formation_target()->mutable_position()->set_x(1.0);
    target_echo.mutable_formation_target()->mutable_position()->set_y(2.0);
    target_echo.mutable_formation_target()->mutable_position()->set_z(3.0);
    target_echo.mutable_formation_target()->mutable_orientation()->set_z(0.5);
    target_echo.mutable_formation_target()->mutable_orientation()->set_w(0.8660254037844386);
    assert(validate_formation_state(target_echo, &validation_error));
    // 字段 22/23 的 wire 编号在这里锁死：b0 01 = field 22(varint)、ba 01 = field 23(LEN)。
    assert(hex(target_echo.SerializeAsString()) ==
           "082a1a047561763138024015980107b00101ba01310a1b09"
           "000000000000f03f110000000000000040190000000000000840121219000000000000e03f21"
           "aa4c58e87ab6eb3f");

    FormationState nan_target = target_echo;
    nan_target.mutable_formation_target()->mutable_position()->set_x(
        std::numeric_limits<double>::quiet_NaN());
    assert(!validate_formation_state(nan_target, &validation_error));

    FormationState no_target = target_echo;
    no_target.set_formation_target_valid(false);
    no_target.clear_formation_target();
    assert(validate_formation_state(no_target, &validation_error));
    return 0;
}
