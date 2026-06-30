#include "ui/main_window.hpp"

#include <unordered_set>

#include <QDateTime>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QVBoxLayout>

#include "common/monitor_ui_style.hpp"
#include "packets/flow/packet_flow_canvas.hpp"
#include "packets/flow/packet_flow_format.hpp"
#include "packets/flow/packet_flow_model.hpp"
#include "packets/flow/packet_flow_samples.hpp"
#include "packets/model/packet_trace_table_model.hpp"

namespace {

QPlainTextEdit* make_flow_text(QWidget* parent) {
    auto* text = new QPlainTextEdit(parent);
    text->setReadOnly(true);
    text->setLineWrapMode(QPlainTextEdit::NoWrap);
    monitor_ui::configure_copyable_log_view(text);
    return text;
}

uint64_t flow_window_ms(const QComboBox* combo) {
    if (combo == nullptr) {
        return 10000;
    }
    switch (combo->currentIndex()) {
    case 1:
        return 30000;
    case 2:
        return 120000;
    default:
        return 10000;
    }
}

PacketFlowMode flow_mode(const QComboBox* combo) {
    if (combo == nullptr) {
        return PacketFlowMode::kDemo;
    }
    switch (combo->currentIndex()) {
    case 1:
        return PacketFlowMode::kSelected;
    case 2:
        return PacketFlowMode::kDemo;
    default:
        return PacketFlowMode::kLive;
    }
}

void append_unique_records(std::vector<yunlink::PacketTraceRecord>* out,
                           std::unordered_set<uint64_t>* seen,
                           const std::vector<yunlink::PacketTraceRecord>& records) {
    if (out == nullptr || seen == nullptr) {
        return;
    }
    for (const auto& record : records) {
        if (record.trace_id != 0 && !seen->insert(record.trace_id).second) {
            continue;
        }
        out->push_back(record);
    }
}

}  // namespace

QWidget* MainWindow::build_packet_flow_tab(QWidget* parent) {
    auto* tab = new QWidget(parent);
    auto* root = new QVBoxLayout(tab);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(8);

    auto* toolbar = new QGroupBox("Protocol Flow", tab);
    auto* toolbar_layout = new QGridLayout(toolbar);
    toolbar_layout->setContentsMargins(10, 10, 10, 10);
    toolbar_layout->setHorizontalSpacing(8);
    toolbar_layout->setVerticalSpacing(6);

    packet_flow_mode_combo_ = new QComboBox(toolbar);
    packet_flow_mode_combo_->addItems({"Live", "Selected", "Demo"});
    packet_flow_mode_combo_->setCurrentIndex(0);
    packet_flow_window_combo_ = new QComboBox(toolbar);
    packet_flow_window_combo_->addItems({"10s", "30s", "2min"});
    packet_flow_pause_checkbox_ = new QCheckBox("Pause", toolbar);

    toolbar_layout->addWidget(new QLabel("mode", toolbar), 0, 0);
    toolbar_layout->addWidget(packet_flow_mode_combo_, 0, 1);
    toolbar_layout->addWidget(new QLabel("window", toolbar), 0, 2);
    toolbar_layout->addWidget(packet_flow_window_combo_, 0, 3);
    toolbar_layout->addWidget(packet_flow_pause_checkbox_, 0, 4);
    toolbar_layout->setColumnStretch(5, 1);

    packet_flow_canvas_ = new PacketFlowCanvas(tab);
    packet_flow_detail_text_ = make_flow_text(tab);
    packet_flow_detail_text_->setMaximumHeight(150);
    root->addWidget(toolbar, 0);
    root->addWidget(packet_flow_canvas_, 1);
    root->addWidget(packet_flow_detail_text_, 0);

    const auto refresh_flow = [this]() {
        packet_flow_rendered_trace_id_ = 0;
        refresh_packet_flow();
    };
    connect(packet_flow_mode_combo_,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            refresh_flow);
    connect(packet_flow_window_combo_,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            refresh_flow);
    connect(packet_flow_pause_checkbox_, &QCheckBox::toggled, this, refresh_flow);
    return tab;
}

void MainWindow::refresh_packet_flow() {
    if (packet_flow_canvas_ == nullptr || packet_flow_detail_text_ == nullptr) {
        return;
    }
    if (packet_flow_pause_checkbox_ != nullptr && packet_flow_pause_checkbox_->isChecked()) {
        return;
    }

    const uint64_t now_ms = static_cast<uint64_t>(QDateTime::currentMSecsSinceEpoch());
    const PacketFlowMode mode = flow_mode(packet_flow_mode_combo_);
    PacketFlowSnapshot snapshot;
    if (mode == PacketFlowMode::kDemo) {
        packet_flow_demo_step_ = (packet_flow_demo_step_ + 1) % 8;
        snapshot = packet_flow_takeoff_demo_snapshot(packet_flow_demo_step_, now_ms);
    } else {
        if (mode == PacketFlowMode::kSelected) {
            std::vector<yunlink::PacketTraceRecord> selected_source;
            std::unordered_set<uint64_t> seen_trace_ids;
            if (packet_trace_model_ != nullptr) {
                append_unique_records(
                    &selected_source, &seen_trace_ids, packet_trace_model_->records());
            }
            if (backend_ != nullptr) {
                append_unique_records(
                    &selected_source, &seen_trace_ids, backend_->snapshot_packet_traces());
            }
            snapshot =
                packet_flow_selected_snapshot(selected_source, packet_selected_trace_id_, now_ms);
        } else {
            const auto traces = backend_ != nullptr ? backend_->snapshot_packet_traces()
                                                    : std::vector<yunlink::PacketTraceRecord>{};
            snapshot = packet_flow_live_snapshot(
                traces, now_ms, flow_window_ms(packet_flow_window_combo_), 8);
        }
        if (snapshot.newest_trace_id != 0 &&
            snapshot.newest_trace_id == packet_flow_rendered_trace_id_) {
            return;
        }
        packet_flow_rendered_trace_id_ = snapshot.newest_trace_id;
    }
    packet_flow_canvas_->set_snapshot(snapshot);
    packet_flow_detail_text_->setPlainText(
        QString::fromStdString(packet_flow_journey_detail(snapshot)));
}
