#include "model/monitor_topics.hpp"

#include <string>
#include <utility>
#include <vector>

namespace {

MonitorFieldDef field_runtime(std::string key, std::string label) {
    MonitorFieldDef view;
    view.key = std::move(key);
    view.label = std::move(label);
    return view;
}

void append_topic_diagnostic_rows(std::vector<MonitorFieldDef>& rows,
                                  const std::string& prefix,
                                  const std::string& label) {
    rows.push_back(field_runtime(prefix + "key", label + " key | " + prefix + "key"));
    rows.push_back(field_runtime(prefix + "topic", label + " topic | " + prefix + "topic"));
    rows.push_back(field_runtime(prefix + "configured", label + " configured | " + prefix + "configured"));
    rows.push_back(field_runtime(prefix + "has_message", label + " has_message | " + prefix + "has_message"));
    rows.push_back(field_runtime(prefix + "publisher_count",
                                 label + " publisher_count | " + prefix + "publisher_count"));
    rows.push_back(
        field_runtime(prefix + "message_count", label + " message_count | " + prefix + "message_count"));
    rows.push_back(field_runtime(prefix + "hz", label + " hz | " + prefix + "hz"));
    rows.push_back(field_runtime(prefix + "age_ms", label + " age_ms | " + prefix + "age_ms"));
    rows.push_back(field_runtime(prefix + "stale", label + " stale | " + prefix + "stale"));
    rows.push_back(field_runtime(prefix + "status", label + " status | " + prefix + "status"));
    rows.push_back(field_runtime(prefix + "detail", label + " detail | " + prefix + "detail"));
    rows.push_back(field_runtime(prefix + "last_transition",
                                 label + " last_transition | " + prefix + "last_transition"));
    rows.push_back(field_runtime(prefix + "last_transition_age_ms",
                                 label + " last_transition_age_ms | " +
                                     prefix + "last_transition_age_ms"));
    rows.push_back(field_runtime(prefix + "publish_fail_count",
                                 label + " publish_fail_count | " + prefix + "publish_fail_count"));
    rows.push_back(field_runtime(prefix + "expected_min_hz",
                                 label + " expected_min_hz | " + prefix + "expected_min_hz"));
    rows.push_back(field_runtime(prefix + "sparse", label + " sparse | " + prefix + "sparse"));
}

}  // namespace

MonitorTopicState make_sunray_runtime_diagnostic_topic() {
    MonitorTopicState topic;
    topic.key = "sunray_runtime_diagnostic";
    topic.title = "sunray_runtime_diagnostic";
    topic.yunlink_name = "SunrayRuntimeDiagnosticSnapshot";
    topic.rows = {
        field_runtime("header.frame_id", "Header frame | header.frame_id"),
        field_runtime("header.stamp_ns", "Header stamp_ns | header.stamp_ns"),
        field_runtime("agent_key", "agent key | agent_key"),
        field_runtime("stale_timeout_ms", "stale timeout ms | stale_timeout_ms"),
        field_runtime("runtime_started", "runtime_started | runtime_started"),
        field_runtime("peer_ready", "peer_ready | peer_ready"),
        field_runtime("session_state", "session_state | session_state"),
        field_runtime("last_connect_error", "last_connect_error | last_connect_error"),
        field_runtime("last_session_error", "last_session_error | last_session_error"),
        field_runtime("last_publish_error", "last_publish_error | last_publish_error"),
        field_runtime("last_error_age_ms", "last_error_age_ms | last_error_age_ms"),
        field_runtime("connect_attempt_count", "connect_attempt_count | connect_attempt_count"),
        field_runtime("session_lost_count", "session_lost_count | session_lost_count"),
        field_runtime("ros_to_yunlink_publish_count",
                      "ros_to_yunlink_publish_count | ros_to_yunlink_publish_count"),
        field_runtime("ros_to_yunlink_fail_count",
                      "ros_to_yunlink_fail_count | ros_to_yunlink_fail_count"),
        field_runtime("yunlink_to_ros_command_count",
                      "yunlink_to_ros_command_count | yunlink_to_ros_command_count"),
        field_runtime("yunlink_to_ros_publish_count",
                      "yunlink_to_ros_publish_count | yunlink_to_ros_publish_count"),
        field_runtime("yunlink_to_ros_fail_count",
                      "yunlink_to_ros_fail_count | yunlink_to_ros_fail_count"),
        field_runtime("last_fail_direction", "last_fail_direction | last_fail_direction"),
        field_runtime("last_fail_key", "last_fail_key | last_fail_key"),
        field_runtime("last_fail_error_code", "last_fail_error_code | last_fail_error_code"),
        field_runtime("last_fail_detail", "last_fail_detail | last_fail_detail"),
        field_runtime("worst_level", "worst_level | worst_level"),
        field_runtime("summary", "summary | summary"),
    };
    append_topic_diagnostic_rows(topic.rows, "external_odom.", "external_odom");
    append_topic_diagnostic_rows(topic.rows, "odom_state.", "odom_state");
    append_topic_diagnostic_rows(topic.rows, "local_odom.", "local_odom");
    append_topic_diagnostic_rows(topic.rows, "global_odom.", "global_odom");
    append_topic_diagnostic_rows(topic.rows, "uav_control_cmd.", "uav_control_cmd");
    append_topic_diagnostic_rows(topic.rows, "uav_control_state.", "uav_control_state");
    append_topic_diagnostic_rows(topic.rows, "px4_state.", "px4_state");
    return topic;
}
