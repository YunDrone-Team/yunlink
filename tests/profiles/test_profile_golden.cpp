#include <cassert>
#include <limits>
#include <iomanip>
#include <sstream>
#include <string>

#include "com.yundrone.sunray/v2/sunray.pb.h"
#include "com.yundrone.sunray/v2/control_validation.hpp"
#include "org.yunlink.mobility/v1/mobility.pb.h"
#include "org.yunlink.media/v1/media.pb.h"
#include "org.yunlink.media/v1/media_validation.hpp"
#include "org.yunlink.telemetry/v1/summary_validation.hpp"
#include "org.yunlink.system/v1/system.pb.h"
#include "org.yunlink.system/v1/system_validation.hpp"

namespace {

std::string hex(const std::string& value) {
    std::ostringstream stream;
    stream << std::hex << std::setfill('0');
    for (unsigned char byte : value) {
        stream << std::setw(2) << static_cast<unsigned>(byte);
    }
    return stream.str();
}

void valid_yaw(com::yundrone::sunray::v2::UavDirectControlGoal* goal) {
    goal->mutable_yaw()->set_mode(com::yundrone::sunray::v2::UAV_YAW_KEEP);
    goal->set_controller(com::yundrone::sunray::v2::UAV_CONTROLLER_DEFAULT);
}

template <typename Message> void assert_round_trip(const Message& source) {
    Message decoded;
    assert(decoded.ParseFromString(source.SerializeAsString()));
    assert(decoded.SerializeAsString() == source.SerializeAsString());
}

}  // namespace

int main() {
    org::yunlink::system::v1::ClockSyncRequest clock_request;
    clock_request.set_unix_time_ms(1767225600123ULL);
    clock_request.set_source("sunray-gcs");
    assert(org::yunlink::system::v1::validate_clock_sync_request(clock_request));
    assert(hex(clock_request.SerializeAsString()) ==
           "08fbd0eab6b733120a73756e7261792d676373");

    org::yunlink::system::v1::ClockSyncResponse clock_response;
    clock_response.set_error(org::yunlink::system::v1::CLOCK_SYNC_OK);
    clock_response.set_message("synchronized");
    clock_response.set_previous_unix_time_ms(1767225600000ULL);
    clock_response.set_applied_unix_time_ms(1767225600123ULL);
    clock_response.set_delta_ms(123);
    assert(org::yunlink::system::v1::validate_clock_sync_response(clock_response));
    clock_response.set_previous_unix_time_ms(31449600000ULL);
    clock_response.set_delta_ms(1735776000123LL);
    assert(org::yunlink::system::v1::validate_clock_sync_response(clock_response));
    clock_request.set_unix_time_ms(1000);
    assert(!org::yunlink::system::v1::validate_clock_sync_request(clock_request));
    clock_response.clear_previous_unix_time_ms();
    assert(!org::yunlink::system::v1::validate_clock_sync_response(clock_response));
    clock_response.set_delta_ms(1767225600123LL);
    assert(org::yunlink::system::v1::validate_clock_sync_response(clock_response));

    org::yunlink::mobility::v1::GotoGoal goal;
    goal.set_frame_id("map");
    goal.mutable_position()->set_x(1.0);
    goal.mutable_position()->set_y(-2.0);
    goal.mutable_position()->set_z(0.5);
    goal.set_yaw_rad(0.25);
    assert(hex(goal.SerializeAsString()) ==
           "0a036d6170121b09000000000000f03f1100000000000000c019000000000000e03f190000000000"
           "00d03f");

    com::yundrone::sunray::v2::FeatureStartRequest request;
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

    org::yunlink::media::v1::CameraTakePhotoRequest photo_request;
    photo_request.set_camera_uid("front");
    assert(hex(photo_request.SerializeAsString()) == "0a0566726f6e74");
    assert(org::yunlink::media::v1::validate_camera_request(photo_request.camera_uid()));

    org::yunlink::media::v1::MediaAssetListRequest list_request;
    list_request.set_camera_uid("front");
    list_request.add_kinds(org::yunlink::media::v1::MEDIA_PHOTO);
    list_request.add_kinds(org::yunlink::media::v1::MEDIA_VIDEO);
    list_request.set_created_after_ns(10);
    list_request.set_created_before_ns(20);
    list_request.set_page_size(25);
    list_request.set_page_token("Y3Vyc29y");
    assert(org::yunlink::media::v1::validate_media_asset_list_request(list_request));
    assert(hex(list_request.SerializeAsString()) ==
           "0a0566726f6e7412020103180a2014281932085933567963323979");

    org::yunlink::media::v1::MediaAssetListResponse list_response;
    list_response.set_error(org::yunlink::media::v1::MEDIA_OK);
    list_response.set_next_page_token("next");
    list_response.set_catalog_revision(7);
    auto* item = list_response.add_items();
    auto* listed_asset = item->mutable_asset();
    listed_asset->set_asset_id("photo-1");
    listed_asset->set_kind(org::yunlink::media::v1::MEDIA_PHOTO);
    listed_asset->set_mime_type("image/png");
    listed_asset->set_size_bytes(8);
    listed_asset->set_sha256(std::string(32, '\x01'));
    listed_asset->set_created_at_ns(42);
    listed_asset->set_camera_uid("front");
    listed_asset->set_display_name("photo.png");
    auto* listed_thumbnail = item->mutable_thumbnail();
    listed_thumbnail->set_asset_id("thumb-1");
    listed_thumbnail->set_kind(org::yunlink::media::v1::MEDIA_THUMBNAIL);
    listed_thumbnail->set_mime_type("image/png");
    listed_thumbnail->set_size_bytes(4);
    listed_thumbnail->set_sha256(std::string(32, '\x02'));
    listed_thumbnail->set_created_at_ns(42);
    listed_thumbnail->set_camera_uid("front");
    listed_thumbnail->set_display_name("thumb.png");
    item->set_width(1920);
    item->set_height(1080);
    assert(org::yunlink::media::v1::validate_media_asset_list_response(list_response));
    list_request.set_page_size(101);
    assert(!org::yunlink::media::v1::validate_media_asset_list_request(list_request));

    org::yunlink::media::v1::CameraStartRtspResponse rtsp_response;
    rtsp_response.set_error(org::yunlink::media::v1::MEDIA_OK);
    rtsp_response.set_message("ready");
    rtsp_response.set_rtsp_url(
        "rtsp://viewer:secret@192.168.10.60:8554/front/main?profile=high&token=a%2Fb");
    assert(org::yunlink::media::v1::validate_camera_start_rtsp_response(rtsp_response));
    assert(hex(rtsp_response.SerializeAsString()) ==
           "0801120572656164791a4b727473703a2f2f7669657765723a736563726574403139322e"
           "3136382e31302e36303a383535342f66726f6e742f6d61696e3f70726f66696c653d6869"
           "676826746f6b656e3d6125324662");
    rtsp_response.clear_rtsp_url();
    assert(!org::yunlink::media::v1::validate_camera_start_rtsp_response(rtsp_response));
    rtsp_response.set_error(org::yunlink::media::v1::MEDIA_OPERATION_FAILED);
    assert(org::yunlink::media::v1::validate_camera_start_rtsp_response(rtsp_response));

    org::yunlink::media::v1::CameraCatalogSnapshot camera_catalog;
    camera_catalog.set_generated_at_ns(42);
    camera_catalog.set_camera_manager_available(true);
    auto* camera = camera_catalog.add_cameras();
    camera->set_camera_uid("front");
    camera->set_camera_id(1);
    camera->set_name("Front camera");
    camera->set_online(true);
    camera->set_frame_rate_hz(30.0);
    camera->set_live_view_supported(true);
    camera->set_live_view_active(true);
    camera->set_live_view_autostart(true);
    camera->set_rtsp_url("rtsp://192.168.10.38:8554/front");
    assert(org::yunlink::media::v1::validate_camera_catalog_snapshot(camera_catalog));
    assert_round_trip(camera_catalog);
    camera->clear_rtsp_url();
    assert(!org::yunlink::media::v1::validate_camera_catalog_snapshot(camera_catalog));
    camera->set_rtsp_url("rtsp://192.168.10.38:8554/front");

    org::yunlink::media::v1::MediaAssetRef photo_asset;
    photo_asset.set_asset_id("asset-01");
    photo_asset.set_kind(org::yunlink::media::v1::MEDIA_PHOTO);
    photo_asset.set_mime_type("image/png");
    photo_asset.set_size_bytes(8);
    photo_asset.set_sha256(std::string(32, '\x01'));
    photo_asset.set_camera_uid("front");
    assert(org::yunlink::media::v1::validate_media_asset_ref(photo_asset));
    assert_round_trip(photo_asset);

    org::yunlink::media::v1::MediaAssetChunkResponse rejected_chunk;
    rejected_chunk.set_error(org::yunlink::media::v1::MEDIA_BUSY);
    rejected_chunk.set_message("queue is full");
    assert(org::yunlink::media::v1::validate_media_asset_chunk(rejected_chunk));
    rejected_chunk.set_transfer_id("not valid!");
    assert(!org::yunlink::media::v1::validate_media_asset_chunk(rejected_chunk));

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

    using namespace com::yundrone::sunray::v2;
    FlightControlState flight_control;
    flight_control.set_source_stamp_ns(42);
    flight_control.set_armed(true);
    flight_control.set_control_mode(1);
    flight_control.set_control_state(3);
    flight_control.set_battery_voltage_v(15.2F);
    flight_control.set_battery_percent(88);
    assert(validate_flight_control_state(flight_control, &validation_error));
    assert_round_trip(flight_control);
    flight_control.set_battery_percent(101);
    assert(!validate_flight_control_state(flight_control, &validation_error));

    EmergencyKillGoal emergency_kill;
    assert(!validate_emergency_kill_goal(emergency_kill, &validation_error));
    assert(validation_error == "emergency kill requires explicit confirmation");
    emergency_kill.set_confirmed(true);
    assert(validate_emergency_kill_goal(emergency_kill, &validation_error));
    assert_round_trip(emergency_kill);
    assert(hex(emergency_kill.SerializeAsString()) == "0801");

    TakeoffGoal takeoff;
    takeoff.set_takeoff_relative_height_m(1.2);
    takeoff.set_takeoff_max_velocity_mps(0.5);
    assert(validate_takeoff_goal(takeoff, &validation_error));
    assert_round_trip(takeoff);
    takeoff.set_takeoff_relative_height_m(-1.0);
    assert(!validate_takeoff_goal(takeoff, &validation_error));

    LandGoal land;
    land.set_land_max_velocity_mps(0.4);
    assert(validate_land_goal(land, &validation_error));
    assert_round_trip(land);
    land.set_land_max_velocity_mps(std::numeric_limits<double>::infinity());
    assert(!validate_land_goal(land, &validation_error));

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

    UgvMovePointGoal ugv_move;
    ugv_move.set_frame(UGV_MOVE_LOCAL);
    ugv_move.mutable_point_m()->set_x(1.0);
    ugv_move.mutable_point_m()->set_y(-2.0);
    ugv_move.set_yaw_mode(UGV_YAW_SET);
    ugv_move.set_desired_yaw_rad(0.25);
    ugv_move.set_local_frame_id("map");
    assert(validate_ugv_move_point_goal(ugv_move, &validation_error));
    assert(hex(ugv_move.SerializeAsString()) ==
           "121209000000000000f03f1100000000000000c0180121000000000000d03f2a036d6170");
    ugv_move.mutable_point_m()->set_z(0.1);
    assert(!validate_ugv_move_point_goal(ugv_move, &validation_error));

    UgvVelocityGoal ugv_velocity;
    ugv_velocity.mutable_body()->mutable_linear_mps()->set_x(0.5);
    ugv_velocity.mutable_body()->mutable_linear_mps()->set_y(0.1);
    ugv_velocity.mutable_body()->set_yaw_rate_radps(-0.2);
    ugv_velocity.set_lease_ms(750);
    assert(validate_ugv_velocity_goal(ugv_velocity, &validation_error));
    assert(hex(ugv_velocity.SerializeAsString()) ==
           "121d0a1209000000000000e03f119a9999999999b93f119a9999999999c9bf18ee05");
    ugv_velocity.set_lease_ms(kMinDirectControlLeaseMs - 1);
    assert(!validate_ugv_velocity_goal(ugv_velocity, &validation_error));

    UgvControlState ugv_state;
    ugv_state.set_source_stamp_ns(42);
    ugv_state.set_drive_type(1);
    ugv_state.set_diagnostic_message("hold");
    ugv_state.set_fsm_state(1);
    ugv_state.set_odom_ready(true);
    assert(hex(ugv_state.SerializeAsString()) == "082a28014a04686f6c6450017801");
    assert(UgvHoldGoal{}.SerializeAsString().empty());

    GimbalParams gimbal_state;
    gimbal_state.set_roll_rad(1.0);
    gimbal_state.set_pitch_rad(-0.5);
    gimbal_state.set_yaw_rad(0.25);
    gimbal_state.set_zoom(3.5);
    gimbal_state.set_connected(true);
    gimbal_state.set_roll_rate_rad_s(0.1);
    gimbal_state.set_pitch_rate_rad_s(-0.2);
    gimbal_state.set_yaw_rate_rad_s(0.3);
    assert(hex(gimbal_state.SerializeAsString()) ==
           "11000000000000f03f19000000000000e0bf21000000000000d03f290000000000000c40"
           "3801419a9999999999b93f499a9999999999c9bf51333333333333d33f");

    GimbalAngleGoal gimbal_angle;
    gimbal_angle.set_yaw_rad(1.5707963267948966);
    gimbal_angle.set_pitch_rad(-0.7853981633974483);
    assert(validate_gimbal_angle_goal(gimbal_angle, &validation_error));
    assert(hex(gimbal_angle.SerializeAsString()) ==
           "09182d4454fb21f93f11182d4454fb21e9bf");

    GimbalRateGoal gimbal_rate;
    gimbal_rate.set_yaw_control(45);
    gimbal_rate.set_pitch_control(-30);
    assert(validate_gimbal_rate_goal(gimbal_rate, &validation_error));
    assert(hex(gimbal_rate.SerializeAsString()) == "082d10e2ffffffffffffffff01");
    gimbal_rate.set_yaw_control(101);
    assert(!validate_gimbal_rate_goal(gimbal_rate, &validation_error));

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
    mission.set_task_name("yunlink-task-42");
    mission.set_completion_action(UAV_MISSION_FINISH_HOVER);
    first->set_arrival_action(UAV_WAYPOINT_HOLD_SET_YAW);
    second->set_arrival_action(UAV_WAYPOINT_NEXT);
    assert(validate_uav_waypoint_mission_goal(mission, &validation_error));
    assert(hex(mission.SerializeAsString()) ==
           "0a036d617012310a1b09000000000000f03f11000000000000004019000000000000084011000000000000e03f19000000000000f83f200212260a1b09000000000000f0bf1100000000000000c019000000000000104011000000000000d0bf1a0f79756e6c696e6b2d7461736b2d3432");
    assert_round_trip(mission);

    UavWaypointMissionGoal empty_mission;
    empty_mission.set_frame_id("map");
    empty_mission.set_task_name("yunlink-empty");
    assert(!validate_uav_waypoint_mission_goal(empty_mission, &validation_error));
    for (int index = 0; index <= kMaxWaypointCount; ++index) {
        empty_mission.add_waypoints()->mutable_position_m();
    }
    assert(!validate_uav_waypoint_mission_goal(empty_mission, &validation_error));
    return 0;
}
