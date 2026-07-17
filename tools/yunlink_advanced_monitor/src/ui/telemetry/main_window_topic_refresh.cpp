#include "ui/main_window.hpp"

#include <cctype>

#include <QDateTime>

#include "common/monitor_ui_style.hpp"
#include "common/sunray_status_format.hpp"

namespace {

bool is_blank_topic_value(const std::string& value) {
    if (value.empty()) {
        return true;
    }
    for (unsigned char ch : value) {
        if (!std::isspace(ch)) {
            return false;
        }
    }
    return true;
}

std::string normalize_topic_value(const std::string& value) {
    return is_blank_topic_value(value) ? std::string("<empty>") : value;
}

std::string topic_value_or_default(const MonitorTopicState& topic, const std::string& key) {
    const auto it = topic.latest.values.find(key);
    if (it != topic.latest.values.end()) {
        return normalize_topic_value(it->second);
    }
    return monitor_has_snapshot(topic.latest) ? std::string("--") : std::string("WAIT");
}

std::string topic_label_for_display(const std::string& label, const std::string& key) {
    std::string out = label;
    const bool is_yaw_rate =
        key.find("yaw_rate_radps") != std::string::npos ||
        key.find("body_rate_radps") != std::string::npos ||
        key.find("angular_radps") != std::string::npos;
    const bool is_yaw_angle =
        !is_yaw_rate &&
        (key.find("yaw_rad") != std::string::npos || key.find("desired_yaw_rad") != std::string::npos);
    if (is_yaw_rate) {
        for (std::string::size_type pos = 0;
             (pos = out.find("radps", pos)) != std::string::npos;
             pos += 5) {
            out.replace(pos, 5, "degps");
        }
        return out;
    }
    if (is_yaw_angle) {
        for (std::string::size_type pos = 0;
             (pos = out.find("rad", pos)) != std::string::npos;
             pos += 3) {
            out.replace(pos, 3, "deg");
        }
    }
    return out;
}

std::string topic_value_for_display(const MonitorTopicState& topic, const std::string& key) {
    const std::string value = topic_value_or_default(topic, key);
    if (value == "WAIT" || value == "--" || value == "<empty>") {
        return value;
    }

    double parsed = 0.0;
    const bool is_yaw_rate =
        key.find("yaw_rate_radps") != std::string::npos ||
        key.find("body_rate_radps") != std::string::npos ||
        key.find("angular_radps") != std::string::npos;
    const bool is_yaw_angle =
        !is_yaw_rate &&
        (key.find("yaw_rad") != std::string::npos || key.find("desired_yaw_rad") != std::string::npos);

    if ((is_yaw_angle || is_yaw_rate) && monitor_parse_double(value, &parsed)) {
        return is_yaw_rate ? monitor_fmt_degrees_per_sec(parsed) : monitor_fmt_degrees(parsed);
    }
    return value;
}

}  // namespace

void MainWindow::refresh_topics() {
    if (backend_ == nullptr) {
        return;
    }

    const auto topics = backend_->snapshot_topics();
    for (const auto& key : monitor_topic_display_order()) {
        const auto it = topics.find(key);
        if (it == topics.end()) {
            continue;
        }
        refresh_topic(key, it->second);
    }
}

void MainWindow::refresh_topic(const std::string& key, const MonitorTopicState& topic) {
    auto table_it = topic_tables_.find(key);
    if (table_it == topic_tables_.end()) {
        return;
    }
    const uint64_t now_ms = static_cast<uint64_t>(QDateTime::currentMSecsSinceEpoch());
    const auto summary_it = topic_summary_labels_.find(key);
    if (summary_it != topic_summary_labels_.end() && summary_it->second != nullptr) {
        summary_it->second->setText(monitor_ui::topic_summary_text(topic, now_ms));
    }

    auto* table = table_it->second;
    table->setRowCount(static_cast<int>(topic.rows.size()));
    for (int row = 0; row < static_cast<int>(topic.rows.size()); ++row) {
        const auto& field = topic.rows[static_cast<size_t>(row)];
        set_item(table, row, 0, topic_label_for_display(field.label, field.key));
        set_item(table, row, 1, field.key);
        auto* value_item = set_item(table, row, 2, topic_value_for_display(topic, field.key));
        const std::string value = value_item->text().toStdString();
        if (value == "WAIT" || value == "STALE" || value == "TIMEOUT") {
            monitor_ui::set_status_item(value_item, monitor_ui::Level::kWarn);
        } else if (value == "--" || value == "<empty>") {
            monitor_ui::set_status_item(value_item, monitor_ui::Level::kNeutral);
        }
    }
}
