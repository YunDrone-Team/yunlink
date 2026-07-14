#include "ui/main_window.hpp"

#include "packets/model/packet_trace_table_model.hpp"
#include "packets/packet_trace_detail.hpp"
#include "packets/packet_trace_semantic.hpp"

void MainWindow::clear_packet_texts(const QString& text) {
    auto set_text = [&text](QPlainTextEdit* view) {
        if (view != nullptr) {
            view->setPlainText(text);
        }
    };
    set_text(packet_summary_text_);
    set_text(packet_header_text_);
    set_text(packet_target_text_);
    set_text(packet_security_text_);
    set_text(packet_payload_text_);
    set_text(packet_raw_text_);
    set_text(packet_semantic_text_);
    set_text(packet_errors_text_);
}

void MainWindow::remember_packet_splitter_sizes() {
    if (packet_splitter_ != nullptr) {
        packet_splitter_sizes_ = packet_splitter_->sizes();
    }
}

void MainWindow::restore_packet_splitter_sizes() {
    if (!packet_splitter_sizes_.empty() && packet_splitter_ != nullptr) {
        packet_splitter_->setSizes(packet_splitter_sizes_);
    }
}

void MainWindow::refresh_packet_detail() {
    if (packet_trace_table_ == nullptr || packet_trace_model_ == nullptr) {
        return;
    }
    const int row = packet_trace_table_->selectionModel() != nullptr
                        ? packet_trace_table_->selectionModel()->currentIndex().row()
                        : -1;
    const auto* record = packet_trace_model_->record_at(row);
    const QString empty = "未选择数据包。";
    auto set_text = [](QPlainTextEdit* view, const QString& text) {
        if (view != nullptr) {
            view->setPlainText(text);
        }
    };
    if (record == nullptr) {
        packet_detail_rendered_trace_id_ = 0;
        clear_packet_texts(empty);
        return;
    }
    if (record->trace_id == packet_detail_rendered_trace_id_) {
        return;
    }
    packet_detail_rendered_trace_id_ = record->trace_id;

    set_text(packet_summary_text_, QString::fromStdString(packet_summary_detail(*record)));
    set_text(packet_header_text_, QString::fromStdString(packet_header_detail(*record)));
    set_text(packet_target_text_, QString::fromStdString(packet_source_target_detail(*record)));
    set_text(packet_security_text_, QString::fromStdString(packet_security_detail(*record)));
    set_text(packet_payload_text_, QString::fromStdString(packet_payload_detail(*record)));
    set_text(packet_raw_text_, QString::fromStdString(packet_raw_detail(*record)));
    set_text(packet_semantic_text_, QString::fromStdString(packet_semantic_detail(*record)));
    set_text(packet_errors_text_, QString::fromStdString(packet_errors_detail(*record)));
}

void MainWindow::stage_clear_packet_traces() {
    if (backend_ != nullptr) {
        backend_->clear_packet_traces();
    }
    if (packet_trace_model_ != nullptr) {
        packet_trace_model_->set_records({});
    }
    packet_last_seen_trace_id_ = 0;
    packet_selected_trace_id_ = 0;
    packet_detail_rendered_trace_id_ = 0;
    packet_flow_rendered_trace_id_ = 0;
    packet_filter_signature_.clear();
    refresh_packet_detail();
    refresh_packet_flow();
    update_packet_status_label();
}
