#include "backend/advanced_monitor_backend.hpp"

#include <utility>

#include "mapping/value_map.hpp"

void AdvancedMonitorBackend::bind_yunlink_subscribers() {
    state_sub_tokens_.push_back(runtime_.state_subscriber().subscribe_local_odom(
        [this](const yunlink::TypedMessage<yunlink::LocalOdomSnapshot>& message) {
            std::unordered_map<std::string, std::string> values;
            fill_local_odom_from_yunlink(message.payload, values);
            update_yunlink("local_odom",
                           std::move(values),
                           "session=" + std::to_string(message.envelope.session_id) +
                               " msg_id=" + std::to_string(message.envelope.message_id),
                           message.payload.header.stamp_ns,
                           message.envelope.message_id,
                           message.envelope.created_at_ms,
                           message.envelope.session_id);
        }));

    state_sub_tokens_.push_back(runtime_.state_subscriber().subscribe_odom_state(
        [this](const yunlink::TypedMessage<yunlink::OdomStateSnapshot>& message) {
            std::unordered_map<std::string, std::string> values;
            fill_odom_state_from_yunlink(message.payload, values);
            update_yunlink("odom_state",
                           std::move(values),
                           "session=" + std::to_string(message.envelope.session_id) +
                               " msg_id=" + std::to_string(message.envelope.message_id),
                           message.payload.header.stamp_ns,
                           message.envelope.message_id,
                           message.envelope.created_at_ms,
                           message.envelope.session_id);
        }));

    state_sub_tokens_.push_back(runtime_.state_subscriber().subscribe_uav_control_state(
        [this](const yunlink::TypedMessage<yunlink::UavControlStateSnapshot>& message) {
            std::unordered_map<std::string, std::string> values;
            fill_control_state_from_yunlink(message.payload, values);
            update_yunlink("uav_control_state",
                           std::move(values),
                           "session=" + std::to_string(message.envelope.session_id) +
                               " msg_id=" + std::to_string(message.envelope.message_id),
                           message.payload.header.stamp_ns,
                           message.envelope.message_id,
                           message.envelope.created_at_ms,
                           message.envelope.session_id);
            apply_uav_control_state_history(message.payload);
        }));

    state_sub_tokens_.push_back(runtime_.state_subscriber().subscribe_mavros_state(
        [this](const yunlink::TypedMessage<yunlink::MavrosStateSnapshot>& message) {
            std::unordered_map<std::string, std::string> values;
            fill_mavros_state_from_yunlink(message.payload, values);
            update_yunlink("mavros_state",
                           std::move(values),
                           "session=" + std::to_string(message.envelope.session_id) +
                               " msg_id=" + std::to_string(message.envelope.message_id),
                           message.payload.header.stamp_ns,
                           message.envelope.message_id,
                           message.envelope.created_at_ms,
                           message.envelope.session_id);
        }));

    state_sub_tokens_.push_back(runtime_.state_subscriber().subscribe_px4_state(
        [this](const yunlink::TypedMessage<yunlink::Px4StateSnapshot>& message) {
            std::unordered_map<std::string, std::string> values;
            fill_px4_state_from_yunlink(message.payload, values);
            update_yunlink("px4_state",
                           std::move(values),
                           "session=" + std::to_string(message.envelope.session_id) +
                               " msg_id=" + std::to_string(message.envelope.message_id),
                           message.payload.header.stamp_ns,
                           message.envelope.message_id,
                           message.envelope.created_at_ms,
                           message.envelope.session_id);
        }));

    log(MonitorLogLevel::kInfo, MonitorLogSource::kRuntime, "状态快照订阅器已就绪");
}

void AdvancedMonitorBackend::bind_ros_subscribers() {
    ros::TransportHints latest_hints;
    latest_hints.tcpNoDelay();
    local_odom_sub_ = nh_.subscribe(
        topics_["local_odom"].ros_topic, 1, &AdvancedMonitorBackend::on_local_odom, this, latest_hints);
    odom_state_sub_ =
        nh_.subscribe(topics_["odom_state"].ros_topic, 1, &AdvancedMonitorBackend::on_odom_state, this, latest_hints);
    control_state_sub_ = nh_.subscribe(
        topics_["uav_control_state"].ros_topic, 1, &AdvancedMonitorBackend::on_control_state, this, latest_hints);
    mavros_state_sub_ =
        nh_.subscribe(topics_["mavros_state"].ros_topic, 1, &AdvancedMonitorBackend::on_mavros_state, this, latest_hints);
    px4_state_sub_ =
        nh_.subscribe(topics_["px4_state"].ros_topic, 1, &AdvancedMonitorBackend::on_px4_state, this, latest_hints);
    log(MonitorLogLevel::kInfo, MonitorLogSource::kRuntime, "ROS 原始话题订阅器已就绪");
}

void AdvancedMonitorBackend::bind_command_feedback() {
    authority_status_token_ = runtime_.event_subscriber().subscribe_authority_status(
        [this](const yunlink::TypedMessage<yunlink::AuthorityStatus>& message) {
            on_authority_status(message);
        });
    command_result_token_ = runtime_.event_subscriber().subscribe_command_results(
        [this](const yunlink::CommandResultView& message) { on_command_result(message); });
    log(MonitorLogLevel::kInfo,
        MonitorLogSource::kRuntime,
        "authority / command result 订阅器已就绪");
}
