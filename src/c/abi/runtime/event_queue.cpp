/**
 * @file src/c/abi/event_queue.cpp
 * @brief C ABI runtime event queue wiring.
 */

#include "../internal.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <sstream>

namespace yunlink_c_abi {

namespace {

std::string join_csv(const std::vector<std::string>& values) {
    std::ostringstream out;
    for (size_t index = 0; index < values.size(); ++index) {
        if (index != 0) {
            out << ",";
        }
        out << values[index];
    }
    return out.str();
}

std::string join_lines(const std::vector<std::string>& values) {
    std::ostringstream out;
    for (size_t index = 0; index < values.size(); ++index) {
        if (index != 0) {
            out << '\n';
        }
        out << values[index];
    }
    return out.str();
}

std::string escape_feature_field(const std::string& value) {
    std::ostringstream out;
    out << std::uppercase << std::hex;
    for (const unsigned char ch : value) {
        if (ch == '%' || ch == '\x1E' || ch == '\x1F' || ch == '\r' || ch == '\n') {
            out << '%' << std::setw(2) << std::setfill('0') << static_cast<unsigned>(ch);
            out << std::setfill(' ');
        } else {
            out << static_cast<char>(ch);
        }
    }
    return out.str();
}

std::string encode_feature_descriptors(const std::vector<yunlink::FeatureDescriptor>& features) {
    std::ostringstream out;
    for (size_t index = 0; index < features.size(); ++index) {
        if (index != 0) {
            out << '\x1E';
        }
        const auto& feature = features[index];
        const std::array<std::string, 15> fields = {
            feature.name,
            feature.display_name,
            feature.group_name,
            feature.group_display_name,
            feature.description,
            feature.core_feature ? "1" : "0",
            feature.example_feature ? "1" : "0",
            feature.basic_feature ? "1" : "0",
            feature.auto_start ? "1" : "0",
            feature.check_feature_state ? "1" : "0",
            std::to_string(feature.runtime_state),
            feature.runtime_error,
            join_lines(feature.depends_on),
            join_lines(feature.start_preview_units),
            join_lines(feature.start_preview_commands),
        };
        for (size_t field = 0; field < fields.size(); ++field) {
            if (field != 0) {
                out << '\x1F';
            }
            out << escape_feature_field(fields[field]);
        }
    }
    return out.str();
}

std::string encode_topic_list(const std::vector<yunlink::TopicDescriptor>& topics) {
    std::ostringstream out;
    for (const auto& topic : topics) {
        out << topic.name << '\t' << topic.type_name << '\t' << topic.publisher_count << '\n';
    }
    return out.str();
}

float yaw_from_quaternion(const yunlink::Quaternionf& orientation) {
    const float sin_yaw = 2.0F * (orientation.w * orientation.z + orientation.x * orientation.y);
    const float cos_yaw =
        1.0F - 2.0F * (orientation.y * orientation.y + orientation.z * orientation.z);
    return std::atan2(sin_yaw, cos_yaw);
}

}  // namespace

void push_event(yunlink_runtime_t* runtime, const yunlink_runtime_event_t& event) {
    std::lock_guard<std::mutex> lock(runtime->mu);
    runtime->queue.push_back(event);
}

void clear_queue(yunlink_runtime_t* runtime) {
    std::lock_guard<std::mutex> lock(runtime->mu);
    runtime->queue.clear();
}

void subscribe_runtime_events(yunlink_runtime_t* runtime) {
    auto& bus = runtime->runtime.event_bus();
    runtime->tok_error = bus.subscribe_error([runtime](const yunlink::ErrorEvent& ev) {
        yunlink_runtime_event_t out{};
        out.type = YUNLINK_RUNTIME_EVENT_ERROR;
        out.data.error.code = static_cast<uint16_t>(to_result(ev.code));
        out.data.error.transport = static_cast<uint8_t>(ev.transport);
        out.data.error.peer_port = ev.peer.port;
        safe_copy(out.data.error.peer_id, sizeof(out.data.error.peer_id), ev.peer.id);
        safe_copy(out.data.error.peer_ip, sizeof(out.data.error.peer_ip), ev.peer.ip);
        safe_copy(out.data.error.message, sizeof(out.data.error.message), ev.message);
        push_event(runtime, out);
    });

    runtime->tok_link = bus.subscribe_link([runtime](const yunlink::LinkEvent& ev) {
        yunlink_runtime_event_t out{};
        out.type = YUNLINK_RUNTIME_EVENT_LINK;
        out.data.link.transport = static_cast<uint8_t>(ev.transport);
        out.data.link.is_up = ev.is_up ? 1 : 0;
        out.data.link.peer_port = ev.peer.port;
        safe_copy(out.data.link.peer_id, sizeof(out.data.link.peer_id), ev.peer.id);
        safe_copy(out.data.link.peer_ip, sizeof(out.data.link.peer_ip), ev.peer.ip);
        push_event(runtime, out);
    });

    runtime->tok_vehicle_core = runtime->runtime.state_subscriber().subscribe_vehicle_core(
        [runtime](const yunlink::TypedMessage<yunlink::VehicleCoreState>& msg) {
            yunlink_runtime_event_t out{};
            out.type = YUNLINK_RUNTIME_EVENT_VEHICLE_CORE_STATE;
            out.data.vehicle_core_state.session_id = msg.envelope.session_id;
            out.data.vehicle_core_state.message_id = msg.envelope.message_id;
            out.data.vehicle_core_state.correlation_id = msg.envelope.correlation_id;
            out.data.vehicle_core_state.source_type =
                static_cast<uint8_t>(msg.envelope.source.agent_type);
            out.data.vehicle_core_state.source_id = msg.envelope.source.agent_id;
            out.data.vehicle_core_state.source_role =
                static_cast<uint8_t>(msg.envelope.source.role);
            out.data.vehicle_core_state.armed = msg.payload.armed ? 1 : 0;
            out.data.vehicle_core_state.nav_mode = msg.payload.nav_mode;
            out.data.vehicle_core_state.x_m = msg.payload.x_m;
            out.data.vehicle_core_state.y_m = msg.payload.y_m;
            out.data.vehicle_core_state.z_m = msg.payload.z_m;
            out.data.vehicle_core_state.vx_mps = msg.payload.vx_mps;
            out.data.vehicle_core_state.vy_mps = msg.payload.vy_mps;
            out.data.vehicle_core_state.vz_mps = msg.payload.vz_mps;
            out.data.vehicle_core_state.battery_percent = msg.payload.battery_percent;
            push_event(runtime, out);
        });

    runtime->tok_px4_state = runtime->runtime.state_subscriber().subscribe_px4_state(
        [runtime](const yunlink::TypedMessage<yunlink::Px4StateSnapshot>& msg) {
            yunlink_runtime_event_t out{};
            out.type = YUNLINK_RUNTIME_EVENT_PX4_STATE;
            out.data.px4_state.session_id = msg.envelope.session_id;
            out.data.px4_state.message_id = msg.envelope.message_id;
            out.data.px4_state.correlation_id = msg.envelope.correlation_id;
            out.data.px4_state.source_type = static_cast<uint8_t>(msg.envelope.source.agent_type);
            out.data.px4_state.source_id = msg.envelope.source.agent_id;
            out.data.px4_state.source_role = static_cast<uint8_t>(msg.envelope.source.role);
            out.data.px4_state.connected = msg.payload.connected ? 1 : 0;
            out.data.px4_state.armed = msg.payload.armed ? 1 : 0;
            safe_copy(out.data.px4_state.flight_mode,
                      sizeof(out.data.px4_state.flight_mode),
                      msg.payload.flight_mode);
            out.data.px4_state.system_status = msg.payload.system_status;
            out.data.px4_state.landed_state = msg.payload.landed_state;
            out.data.px4_state.battery_voltage_v = msg.payload.battery_voltage_v;
            out.data.px4_state.battery_current_a = msg.payload.battery_current_a;
            out.data.px4_state.battery_percentage = msg.payload.battery_percentage;
            out.data.px4_state.local_x_m = msg.payload.local_pose.position_m.x;
            out.data.px4_state.local_y_m = msg.payload.local_pose.position_m.y;
            out.data.px4_state.local_z_m = msg.payload.local_pose.position_m.z;
            out.data.px4_state.local_vx_mps = msg.payload.local_velocity.linear_mps.x;
            out.data.px4_state.local_vy_mps = msg.payload.local_velocity.linear_mps.y;
            out.data.px4_state.local_vz_mps = msg.payload.local_velocity.linear_mps.z;
            out.data.px4_state.local_yaw_rad =
                yaw_from_quaternion(msg.payload.local_pose.orientation);
            out.data.px4_state.local_orientation_x = msg.payload.local_pose.orientation.x;
            out.data.px4_state.local_orientation_y = msg.payload.local_pose.orientation.y;
            out.data.px4_state.local_orientation_z = msg.payload.local_pose.orientation.z;
            out.data.px4_state.local_orientation_w = msg.payload.local_pose.orientation.w;
            out.data.px4_state.target_x_m = msg.payload.pos_setpoint_m.x;
            out.data.px4_state.target_y_m = msg.payload.pos_setpoint_m.y;
            out.data.px4_state.target_z_m = msg.payload.pos_setpoint_m.z;
            out.data.px4_state.target_yaw_rad = msg.payload.yaw_setpoint_rad;
            out.data.px4_state.target_valid = msg.payload.setpoint_coordinate_frame != 0 ? 1 : 0;
            push_event(runtime, out);
        });

    runtime->tok_local_odom = runtime->runtime.state_subscriber().subscribe_local_odom(
        [runtime](const yunlink::TypedMessage<yunlink::LocalOdomSnapshot>& msg) {
            yunlink_runtime_event_t out{};
            out.type = YUNLINK_RUNTIME_EVENT_LOCAL_ODOM;
            out.data.local_odom.session_id = msg.envelope.session_id;
            out.data.local_odom.message_id = msg.envelope.message_id;
            out.data.local_odom.correlation_id = msg.envelope.correlation_id;
            out.data.local_odom.source_type = static_cast<uint8_t>(msg.envelope.source.agent_type);
            out.data.local_odom.source_id = msg.envelope.source.agent_id;
            out.data.local_odom.source_role = static_cast<uint8_t>(msg.envelope.source.role);
            out.data.local_odom.source_stamp_ns = msg.payload.header.stamp_ns;
            safe_copy(out.data.local_odom.frame_id,
                      sizeof(out.data.local_odom.frame_id),
                      msg.payload.header.frame_id);
            safe_copy(out.data.local_odom.child_frame_id,
                      sizeof(out.data.local_odom.child_frame_id),
                      msg.payload.child_frame_id);
            out.data.local_odom.x_m = msg.payload.pose.position_m.x;
            out.data.local_odom.y_m = msg.payload.pose.position_m.y;
            out.data.local_odom.z_m = msg.payload.pose.position_m.z;
            out.data.local_odom.orientation_x = msg.payload.pose.orientation.x;
            out.data.local_odom.orientation_y = msg.payload.pose.orientation.y;
            out.data.local_odom.orientation_z = msg.payload.pose.orientation.z;
            out.data.local_odom.orientation_w = msg.payload.pose.orientation.w;
            out.data.local_odom.vx_mps = msg.payload.twist.linear_mps.x;
            out.data.local_odom.vy_mps = msg.payload.twist.linear_mps.y;
            out.data.local_odom.vz_mps = msg.payload.twist.linear_mps.z;
            out.data.local_odom.angular_x_radps = msg.payload.twist.angular_radps.x;
            out.data.local_odom.angular_y_radps = msg.payload.twist.angular_radps.y;
            out.data.local_odom.angular_z_radps = msg.payload.twist.angular_radps.z;
            push_event(runtime, out);
        });

    runtime->tok_ugv_control_cmd = runtime->runtime.state_subscriber().subscribe_ugv_control_cmd(
        [runtime](const yunlink::TypedMessage<yunlink::UgvControlCmdSnapshot>& msg) {
            yunlink_runtime_event_t out{};
            out.type = YUNLINK_RUNTIME_EVENT_UGV_CONTROL_CMD;
            auto& data = out.data.ugv_control_cmd;
            data.session_id = msg.envelope.session_id;
            data.message_id = msg.envelope.message_id;
            data.correlation_id = msg.envelope.correlation_id;
            data.source_type = static_cast<uint8_t>(msg.envelope.source.agent_type);
            data.source_id = msg.envelope.source.agent_id;
            data.source_role = static_cast<uint8_t>(msg.envelope.source.role);
            data.source_stamp_ns = msg.payload.header.stamp_ns;
            safe_copy(data.frame_id, sizeof(data.frame_id), msg.payload.header.frame_id);
            data.cmd_source = msg.payload.cmd_source;
            data.control_cmd = msg.payload.control_cmd;
            data.desired_position_x_m = msg.payload.desired_position_m.x;
            data.desired_position_y_m = msg.payload.desired_position_m.y;
            data.desired_position_z_m = msg.payload.desired_position_m.z;
            data.desired_velocity_x_mps = msg.payload.desired_velocity_mps.x;
            data.desired_velocity_y_mps = msg.payload.desired_velocity_mps.y;
            data.desired_velocity_z_mps = msg.payload.desired_velocity_mps.z;
            data.body_linear_velocity_x_mps = msg.payload.body_linear_velocity_mps.x;
            data.body_linear_velocity_y_mps = msg.payload.body_linear_velocity_mps.y;
            data.body_linear_velocity_z_mps = msg.payload.body_linear_velocity_mps.z;
            data.body_angular_velocity_x_radps = msg.payload.body_angular_velocity_radps.x;
            data.body_angular_velocity_y_radps = msg.payload.body_angular_velocity_radps.y;
            data.body_angular_velocity_z_radps = msg.payload.body_angular_velocity_radps.z;
            data.desired_yaw_rad = msg.payload.desired_yaw_rad;
            data.desired_wgs84_latitude_deg = msg.payload.desired_wgs84_position.latitude_deg;
            data.desired_wgs84_longitude_deg = msg.payload.desired_wgs84_position.longitude_deg;
            data.desired_wgs84_altitude_m = msg.payload.desired_wgs84_position.altitude_m;
            push_event(runtime, out);
        });

    runtime->tok_ugv_control_state =
        runtime->runtime.state_subscriber().subscribe_ugv_control_state(
            [runtime](const yunlink::TypedMessage<yunlink::UgvControlStateSnapshot>& msg) {
                yunlink_runtime_event_t out{};
                out.type = YUNLINK_RUNTIME_EVENT_UGV_CONTROL_STATE;
                auto& data = out.data.ugv_control_state;
                data.session_id = msg.envelope.session_id;
                data.message_id = msg.envelope.message_id;
                data.correlation_id = msg.envelope.correlation_id;
                data.source_type = static_cast<uint8_t>(msg.envelope.source.agent_type);
                data.source_id = msg.envelope.source.agent_id;
                data.source_role = static_cast<uint8_t>(msg.envelope.source.role);
                data.source_stamp_ns = msg.payload.header.stamp_ns;
                safe_copy(data.frame_id, sizeof(data.frame_id), msg.payload.header.frame_id);
                safe_copy(data.agent_name, sizeof(data.agent_name), msg.payload.agent_name);
                data.agent_id = msg.payload.agent_id;
                data.drive_type = msg.payload.drive_type;
                data.control_cmd_valid = msg.payload.control_cmd_valid ? 1 : 0;
                data.inside_geo_fence = msg.payload.inside_geo_fence ? 1 : 0;
                data.diagnostic_level = msg.payload.diagnostic_level;
                safe_copy(data.diagnostic_message,
                          sizeof(data.diagnostic_message),
                          msg.payload.diagnostic_message);
                data.fsm_state = msg.payload.fsm_state;
                data.active_control_cmd = msg.payload.active_command.control_cmd;
                data.odom_valid = msg.payload.odom_valid ? 1 : 0;
                data.odom_x_m = msg.payload.self_odom.pose.position_m.x;
                data.odom_y_m = msg.payload.self_odom.pose.position_m.y;
                data.odom_z_m = msg.payload.self_odom.pose.position_m.z;
                data.odom_vx_mps = msg.payload.self_odom.twist.linear_mps.x;
                data.odom_vy_mps = msg.payload.self_odom.twist.linear_mps.y;
                data.odom_vz_mps = msg.payload.self_odom.twist.linear_mps.z;
                data.target_valid = msg.payload.target_valid ? 1 : 0;
                data.target_x_m = msg.payload.target_position_m.x;
                data.target_y_m = msg.payload.target_position_m.y;
                data.target_z_m = msg.payload.target_position_m.z;
                data.target_yaw_rad = msg.payload.target_yaw_rad;
                data.controller_linear_x_mps = msg.payload.controller_linear_velocity_mps.x;
                data.controller_linear_y_mps = msg.payload.controller_linear_velocity_mps.y;
                data.controller_linear_z_mps = msg.payload.controller_linear_velocity_mps.z;
                data.controller_angular_x_radps = msg.payload.controller_angular_velocity_radps.x;
                data.controller_angular_y_radps = msg.payload.controller_angular_velocity_radps.y;
                data.controller_angular_z_radps = msg.payload.controller_angular_velocity_radps.z;
                data.geo_fence_min_x_m = msg.payload.geo_fence_min_m.x;
                data.geo_fence_min_y_m = msg.payload.geo_fence_min_m.y;
                data.geo_fence_min_z_m = msg.payload.geo_fence_min_m.z;
                data.geo_fence_max_x_m = msg.payload.geo_fence_max_m.x;
                data.geo_fence_max_y_m = msg.payload.geo_fence_max_m.y;
                data.geo_fence_max_z_m = msg.payload.geo_fence_max_m.z;
                push_event(runtime, out);
            });

    runtime->tok_host_system = runtime->runtime.state_subscriber().subscribe_host_system(
        [runtime](const yunlink::TypedMessage<yunlink::HostSystemSnapshot>& msg) {
            yunlink_runtime_event_t out{};
            out.type = YUNLINK_RUNTIME_EVENT_HOST_SYSTEM;
            out.data.host_system.session_id = msg.envelope.session_id;
            out.data.host_system.message_id = msg.envelope.message_id;
            out.data.host_system.correlation_id = msg.envelope.correlation_id;
            out.data.host_system.source_id = msg.envelope.source.agent_id;
            out.data.host_system.source_stamp_ns = msg.payload.header.stamp_ns;
            out.data.host_system.cpu_percent = msg.payload.cpu_percent;
            out.data.host_system.memory_percent = msg.payload.memory_percent;
            out.data.host_system.sample_period_ms = msg.payload.sample_period_ms;
            safe_copy(out.data.host_system.component_kind,
                      sizeof(out.data.host_system.component_kind),
                      msg.payload.component_kind);
            safe_copy(out.data.host_system.active_components,
                      sizeof(out.data.host_system.active_components),
                      join_lines(msg.payload.active_components));
            push_event(runtime, out);
        });

    runtime->tok_authority_status = runtime->runtime.event_subscriber().subscribe_authority_status(
        [runtime](const yunlink::TypedMessage<yunlink::AuthorityStatus>& msg) {
            yunlink_runtime_event_t out{};
            out.type = YUNLINK_RUNTIME_EVENT_AUTHORITY_STATUS;
            out.data.authority_status.state = static_cast<uint8_t>(msg.payload.state);
            out.data.authority_status.session_id = msg.payload.session_id;
            out.data.authority_status.source_type =
                static_cast<uint8_t>(msg.envelope.source.agent_type);
            out.data.authority_status.source_id = msg.envelope.source.agent_id;
            out.data.authority_status.source_role = static_cast<uint8_t>(msg.envelope.source.role);
            out.data.authority_status.lease_ttl_ms = msg.payload.lease_ttl_ms;
            out.data.authority_status.reason_code = msg.payload.reason_code;
            safe_copy(out.data.authority_status.detail,
                      sizeof(out.data.authority_status.detail),
                      msg.payload.detail);
            push_event(runtime, out);
        });

    runtime->tok_vehicle_event = runtime->runtime.event_subscriber().subscribe_vehicle_event(
        [runtime](const yunlink::TypedMessage<yunlink::VehicleEvent>& msg) {
            yunlink_runtime_event_t out{};
            out.type = YUNLINK_RUNTIME_EVENT_VEHICLE_EVENT;
            out.data.vehicle_event.session_id = msg.envelope.session_id;
            out.data.vehicle_event.message_id = msg.envelope.message_id;
            out.data.vehicle_event.correlation_id = msg.envelope.correlation_id;
            out.data.vehicle_event.kind = static_cast<uint8_t>(msg.payload.kind);
            out.data.vehicle_event.severity = msg.payload.severity;
            safe_copy(out.data.vehicle_event.detail,
                      sizeof(out.data.vehicle_event.detail),
                      msg.payload.detail);
            push_event(runtime, out);
        });

    runtime->tok_command_result = runtime->runtime.event_subscriber().subscribe_command_results(
        [runtime](const yunlink::CommandResultView& view) {
            yunlink_runtime_event_t out{};
            out.type = YUNLINK_RUNTIME_EVENT_COMMAND_RESULT;
            out.data.command_result.session_id = view.envelope.session_id;
            out.data.command_result.message_id = view.envelope.message_id;
            out.data.command_result.correlation_id = view.envelope.correlation_id;
            out.data.command_result.source_type =
                static_cast<uint8_t>(view.envelope.source.agent_type);
            out.data.command_result.source_id = view.envelope.source.agent_id;
            out.data.command_result.source_role = static_cast<uint8_t>(view.envelope.source.role);
            out.data.command_result.command_kind = static_cast<uint16_t>(view.payload.command_kind);
            out.data.command_result.phase = static_cast<uint8_t>(view.payload.phase);
            out.data.command_result.result_code = view.payload.result_code;
            out.data.command_result.progress_percent = view.payload.progress_percent;
            safe_copy(out.data.command_result.detail,
                      sizeof(out.data.command_result.detail),
                      view.payload.detail);
            push_event(runtime, out);
        });

    runtime->tok_feature_list =
        runtime->runtime.system_service_subscriber().subscribe_feature_list_responses(
            [runtime](const yunlink::TypedMessage<yunlink::FeatureListResponse>& msg) {
                yunlink_runtime_event_t out{};
                out.type = YUNLINK_RUNTIME_EVENT_FEATURE_LIST;
                out.data.feature_list.session_id = msg.envelope.session_id;
                out.data.feature_list.message_id = msg.envelope.message_id;
                out.data.feature_list.correlation_id = msg.envelope.correlation_id;
                out.data.feature_list.success = msg.payload.success ? 1 : 0;
                safe_copy(out.data.feature_list.message,
                          sizeof(out.data.feature_list.message),
                          msg.payload.message);
                safe_copy(out.data.feature_list.feature_names,
                          sizeof(out.data.feature_list.feature_names),
                          join_csv(msg.payload.feature_names));
                safe_copy(out.data.feature_list.feature_descriptors,
                          sizeof(out.data.feature_list.feature_descriptors),
                          encode_feature_descriptors(msg.payload.features));
                push_event(runtime, out);
            });

    runtime->tok_feature_get =
        runtime->runtime.system_service_subscriber().subscribe_feature_get_responses(
            [runtime](const yunlink::TypedMessage<yunlink::FeatureGetResponse>& msg) {
                yunlink_runtime_event_t out{};
                out.type = YUNLINK_RUNTIME_EVENT_FEATURE_GET;
                out.data.feature_get.session_id = msg.envelope.session_id;
                out.data.feature_get.message_id = msg.envelope.message_id;
                out.data.feature_get.correlation_id = msg.envelope.correlation_id;
                out.data.feature_get.success = msg.payload.success ? 1 : 0;
                out.data.feature_get.running = msg.payload.running ? 1 : 0;
                out.data.feature_get.auto_start = msg.payload.auto_start ? 1 : 0;
                safe_copy(out.data.feature_get.message,
                          sizeof(out.data.feature_get.message),
                          msg.payload.message);
                safe_copy(
                    out.data.feature_get.name, sizeof(out.data.feature_get.name), msg.payload.name);
                safe_copy(out.data.feature_get.title,
                          sizeof(out.data.feature_get.title),
                          msg.payload.title);
                safe_copy(out.data.feature_get.group,
                          sizeof(out.data.feature_get.group),
                          msg.payload.group);
                safe_copy(out.data.feature_get.description,
                          sizeof(out.data.feature_get.description),
                          msg.payload.description);
                safe_copy(out.data.feature_get.depends_on,
                          sizeof(out.data.feature_get.depends_on),
                          join_csv(msg.payload.depends_on));
                safe_copy(out.data.feature_get.start_preview_units,
                          sizeof(out.data.feature_get.start_preview_units),
                          join_csv(msg.payload.start_preview_units));
                safe_copy(out.data.feature_get.start_preview_commands,
                          sizeof(out.data.feature_get.start_preview_commands),
                          join_csv(msg.payload.start_preview_commands));
                push_event(runtime, out);
            });

    runtime->tok_feature_start =
        runtime->runtime.system_service_subscriber().subscribe_feature_start_responses(
            [runtime](const yunlink::TypedMessage<yunlink::FeatureStartResponse>& msg) {
                yunlink_runtime_event_t out{};
                out.type = YUNLINK_RUNTIME_EVENT_FEATURE_START;
                out.data.feature_start.session_id = msg.envelope.session_id;
                out.data.feature_start.message_id = msg.envelope.message_id;
                out.data.feature_start.correlation_id = msg.envelope.correlation_id;
                out.data.feature_start.success = msg.payload.success ? 1 : 0;
                safe_copy(out.data.feature_start.message,
                          sizeof(out.data.feature_start.message),
                          msg.payload.message);
                safe_copy(out.data.feature_start.feature_name,
                          sizeof(out.data.feature_start.feature_name),
                          msg.payload.feature_name);
                push_event(runtime, out);
            });

    runtime->tok_topic_list =
        runtime->runtime.system_service_subscriber().subscribe_topic_list_responses(
            [runtime](const yunlink::TypedMessage<yunlink::TopicListResponse>& msg) {
                yunlink_runtime_event_t out{};
                out.type = YUNLINK_RUNTIME_EVENT_TOPIC_LIST;
                out.data.topic_list.session_id = msg.envelope.session_id;
                out.data.topic_list.message_id = msg.envelope.message_id;
                out.data.topic_list.correlation_id = msg.envelope.correlation_id;
                out.data.topic_list.success = msg.payload.success ? 1 : 0;
                safe_copy(out.data.topic_list.message,
                          sizeof(out.data.topic_list.message),
                          msg.payload.message);
                safe_copy(out.data.topic_list.revision,
                          sizeof(out.data.topic_list.revision),
                          msg.payload.revision);
                safe_copy(out.data.topic_list.topics,
                          sizeof(out.data.topic_list.topics),
                          encode_topic_list(msg.payload.topics));
                push_event(runtime, out);
            });

    runtime->tok_topic_subscription =
        runtime->runtime.system_service_subscriber().subscribe_topic_subscription_responses(
            [runtime](const yunlink::TypedMessage<yunlink::TopicSubscriptionResponse>& msg) {
                yunlink_runtime_event_t out{};
                out.type = YUNLINK_RUNTIME_EVENT_TOPIC_SUBSCRIPTION;
                out.data.topic_subscription.session_id = msg.envelope.session_id;
                out.data.topic_subscription.message_id = msg.envelope.message_id;
                out.data.topic_subscription.correlation_id = msg.envelope.correlation_id;
                out.data.topic_subscription.success = msg.payload.success ? 1 : 0;
                out.data.topic_subscription.subscribed = msg.payload.subscribed ? 1 : 0;
                out.data.topic_subscription.max_rate_hz = msg.payload.max_rate_hz;
                out.data.topic_subscription.max_payload_bytes = msg.payload.max_payload_bytes;
                safe_copy(out.data.topic_subscription.message,
                          sizeof(out.data.topic_subscription.message),
                          msg.payload.message);
                safe_copy(out.data.topic_subscription.topic_name,
                          sizeof(out.data.topic_subscription.topic_name),
                          msg.payload.topic_name);
                safe_copy(out.data.topic_subscription.type_name,
                          sizeof(out.data.topic_subscription.type_name),
                          msg.payload.type_name);
                push_event(runtime, out);
            });

    runtime->tok_topic_sample = runtime->runtime.state_subscriber().subscribe_topic_samples(
        [runtime](const yunlink::TypedMessage<yunlink::TopicSample>& msg) {
            yunlink_runtime_event_t out{};
            out.type = YUNLINK_RUNTIME_EVENT_TOPIC_SAMPLE;
            out.data.topic_sample.session_id = msg.envelope.session_id;
            out.data.topic_sample.message_id = msg.envelope.message_id;
            out.data.topic_sample.correlation_id = msg.envelope.correlation_id;
            out.data.topic_sample.source_type =
                static_cast<uint8_t>(msg.envelope.source.agent_type);
            out.data.topic_sample.source_id = msg.envelope.source.agent_id;
            out.data.topic_sample.source_role = static_cast<uint8_t>(msg.envelope.source.role);
            out.data.topic_sample.receive_time_ns = msg.payload.receive_time_ns;
            out.data.topic_sample.sequence = msg.payload.sequence;
            out.data.topic_sample.metadata_included = msg.payload.metadata_included ? 1 : 0;
            safe_copy(out.data.topic_sample.topic_name,
                      sizeof(out.data.topic_sample.topic_name),
                      msg.payload.topic_name);
            safe_copy(out.data.topic_sample.type_name,
                      sizeof(out.data.topic_sample.type_name),
                      msg.payload.type_name);
            safe_copy(out.data.topic_sample.type_hash,
                      sizeof(out.data.topic_sample.type_hash),
                      msg.payload.type_hash);
            safe_copy(out.data.topic_sample.encoding,
                      sizeof(out.data.topic_sample.encoding),
                      msg.payload.encoding);
            safe_copy(out.data.topic_sample.message_definition,
                      sizeof(out.data.topic_sample.message_definition),
                      msg.payload.message_definition);
            const size_t copied = std::min(msg.payload.data.size(),
                                           static_cast<size_t>(YUNLINK_TOPIC_SAMPLE_DATA_CAPACITY));
            out.data.topic_sample.data_size = static_cast<uint32_t>(copied);
            out.data.topic_sample.data_truncated = copied != msg.payload.data.size() ? 1 : 0;
            if (copied != 0) {
                std::memcpy(out.data.topic_sample.data, msg.payload.data.data(), copied);
            }
            push_event(runtime, out);
        });
}

void unsubscribe_runtime_events(yunlink_runtime_t* runtime) {
    if (runtime->tok_error != 0) {
        runtime->runtime.event_bus().unsubscribe(runtime->tok_error);
        runtime->tok_error = 0;
    }
    if (runtime->tok_link != 0) {
        runtime->runtime.event_bus().unsubscribe(runtime->tok_link);
        runtime->tok_link = 0;
    }
    if (runtime->tok_vehicle_core != 0) {
        runtime->runtime.state_subscriber().unsubscribe(runtime->tok_vehicle_core);
        runtime->tok_vehicle_core = 0;
    }
    if (runtime->tok_px4_state != 0) {
        runtime->runtime.state_subscriber().unsubscribe(runtime->tok_px4_state);
        runtime->tok_px4_state = 0;
    }
    if (runtime->tok_local_odom != 0) {
        runtime->runtime.state_subscriber().unsubscribe(runtime->tok_local_odom);
        runtime->tok_local_odom = 0;
    }
    if (runtime->tok_ugv_control_cmd != 0) {
        runtime->runtime.state_subscriber().unsubscribe(runtime->tok_ugv_control_cmd);
        runtime->tok_ugv_control_cmd = 0;
    }
    if (runtime->tok_ugv_control_state != 0) {
        runtime->runtime.state_subscriber().unsubscribe(runtime->tok_ugv_control_state);
        runtime->tok_ugv_control_state = 0;
    }
    if (runtime->tok_host_system != 0) {
        runtime->runtime.state_subscriber().unsubscribe(runtime->tok_host_system);
        runtime->tok_host_system = 0;
    }
    if (runtime->tok_authority_status != 0) {
        runtime->runtime.event_subscriber().unsubscribe(runtime->tok_authority_status);
        runtime->tok_authority_status = 0;
    }
    if (runtime->tok_vehicle_event != 0) {
        runtime->runtime.event_subscriber().unsubscribe(runtime->tok_vehicle_event);
        runtime->tok_vehicle_event = 0;
    }
    if (runtime->tok_command_result != 0) {
        runtime->runtime.event_subscriber().unsubscribe(runtime->tok_command_result);
        runtime->tok_command_result = 0;
    }
    if (runtime->tok_feature_list != 0) {
        runtime->runtime.system_service_subscriber().unsubscribe(runtime->tok_feature_list);
        runtime->tok_feature_list = 0;
    }
    if (runtime->tok_feature_get != 0) {
        runtime->runtime.system_service_subscriber().unsubscribe(runtime->tok_feature_get);
        runtime->tok_feature_get = 0;
    }
    if (runtime->tok_feature_start != 0) {
        runtime->runtime.system_service_subscriber().unsubscribe(runtime->tok_feature_start);
        runtime->tok_feature_start = 0;
    }
    if (runtime->tok_topic_list != 0) {
        runtime->runtime.system_service_subscriber().unsubscribe(runtime->tok_topic_list);
        runtime->tok_topic_list = 0;
    }
    if (runtime->tok_topic_subscription != 0) {
        runtime->runtime.system_service_subscriber().unsubscribe(runtime->tok_topic_subscription);
        runtime->tok_topic_subscription = 0;
    }
    if (runtime->tok_topic_sample != 0) {
        runtime->runtime.state_subscriber().unsubscribe(runtime->tok_topic_sample);
        runtime->tok_topic_sample = 0;
    }
}

}  // namespace yunlink_c_abi
