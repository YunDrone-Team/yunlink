#include "ui/main_window.hpp"

#include "common/monitor_ui_style.hpp"
#include "packets/format/packet_trace_format.hpp"
#include "packets/model/packet_trace_table_model.hpp"

void MainWindow::update_packet_status_label() {
    if (packet_status_label_ == nullptr || backend_ == nullptr || packet_trace_model_ == nullptr) {
        return;
    }
    const auto traces = backend_->snapshot_packet_traces();
    size_t total_errors = 0;
    for (const auto& record : traces) {
        if (packet_trace_is_error(record)) {
            ++total_errors;
        }
    }
    size_t visible_errors = 0;
    for (const auto& record : packet_trace_model_->records()) {
        if (packet_trace_is_error(record)) {
            ++visible_errors;
        }
    }
    const QString text = QString("显示 %1 / 共 %2，错误 %3 / %4")
                             .arg(packet_trace_model_->rowCount())
                             .arg(static_cast<qulonglong>(traces.size()))
                             .arg(static_cast<qulonglong>(visible_errors))
                             .arg(static_cast<qulonglong>(total_errors));
    monitor_ui::set_tag(packet_status_label_,
                        total_errors > 0 ? monitor_ui::Level::kWarn : monitor_ui::Level::kOk,
                        text);
}
