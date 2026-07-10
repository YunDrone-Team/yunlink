/**
 * @file src/c/abi/event_queue.cpp
 * @brief C ABI runtime event queue wiring.
 */

#include "../internal.hpp"

#include <cstring>
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
                safe_copy(out.data.feature_get.name, sizeof(out.data.feature_get.name), msg.payload.name);
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
}

}  // namespace yunlink_c_abi
