#include "ui/main_window.hpp"

#include <algorithm>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QScrollBar>
#include <QSplitter>
#include <QTabWidget>
#include <QVBoxLayout>

#include "common/monitor_ui_style.hpp"
#include "packets/packet_trace_detail.hpp"
#include "packets/filters/packet_trace_filters.hpp"
#include "packets/format/packet_trace_format.hpp"
#include "packets/selection/packet_trace_selection.hpp"
#include "packets/search/packet_trace_search.hpp"
#include "packets/packet_trace_semantic.hpp"
#include "packets/model/packet_trace_table_header.hpp"
#include "packets/model/packet_trace_table_model.hpp"

namespace {

QPlainTextEdit* make_packet_text(QWidget* parent) {
    auto* text = new QPlainTextEdit(parent);
    text->setReadOnly(true);
    text->setLineWrapMode(QPlainTextEdit::NoWrap);
    monitor_ui::configure_copyable_log_view(text);
    return text;
}

bool combo_is_all(const QComboBox* combo) {
    return combo == nullptr || combo->currentIndex() <= 0;
}

}  // namespace

QWidget* MainWindow::build_packets_page(QWidget* parent) {
    auto* page = new QWidget(parent);
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(8);

    auto* tabs = new QTabWidget(page);
    auto* flow_tab = build_packet_flow_tab(tabs);

    auto* table_tab = new QWidget(tabs);
    auto* table_root = new QVBoxLayout(table_tab);
    table_root->setContentsMargins(0, 0, 0, 0);
    table_root->setSpacing(8);

    auto* toolbar = new QGroupBox("Packet filters", table_tab);
    auto* toolbar_layout = new QGridLayout(toolbar);
    toolbar_layout->setContentsMargins(10, 10, 10, 10);
    toolbar_layout->setHorizontalSpacing(8);
    toolbar_layout->setVerticalSpacing(6);

    packet_pause_checkbox_ = new QCheckBox("Pause", toolbar);
    packet_follow_checkbox_ = new QCheckBox("Follow latest", toolbar);
    packet_follow_checkbox_->setChecked(true);
    packet_errors_only_checkbox_ = new QCheckBox("Errors only", toolbar);
    packet_search_edit_ = new QLineEdit(toolbar);
    packet_search_edit_->setPlaceholderText("Search trace, peer, session, message, error");
    packet_search_edit_->setClearButtonEnabled(true);
    clear_packets_button_ = new QPushButton("Clear", toolbar);
    monitor_ui::style_button(clear_packets_button_, monitor_ui::ButtonRole::kSecondary);
    packet_status_label_ = new QLabel(toolbar);
    packet_status_label_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    packet_direction_combo_ = new QComboBox(toolbar);
    packet_direction_combo_->addItems({"All directions", "RX", "TX"});
    packet_transport_combo_ = new QComboBox(toolbar);
    packet_transport_combo_->addItems({"All transports",
                                       "TCP_SERVER",
                                       "TCP_CLIENT",
                                       "UDP_UNICAST",
                                       "UDP_BROADCAST",
                                       "UDP_MULTICAST"});
    packet_family_combo_ = new QComboBox(toolbar);
    packet_family_combo_->addItems({"All families",
                                    "Session",
                                    "Authority",
                                    "Command",
                                    "CommandResult",
                                    "StateSnapshot",
                                    "StateEvent",
                                    "BulkChannelDescriptor",
                                    "SystemService"});

    toolbar_layout->addWidget(packet_pause_checkbox_, 0, 0);
    toolbar_layout->addWidget(packet_follow_checkbox_, 0, 1);
    toolbar_layout->addWidget(packet_errors_only_checkbox_, 0, 2);
    toolbar_layout->addWidget(packet_direction_combo_, 0, 3);
    toolbar_layout->addWidget(packet_transport_combo_, 0, 4);
    toolbar_layout->addWidget(packet_family_combo_, 0, 5);
    toolbar_layout->addWidget(packet_search_edit_, 0, 6);
    toolbar_layout->addWidget(clear_packets_button_, 0, 7);
    toolbar_layout->addWidget(packet_status_label_, 0, 8);
    toolbar_layout->setColumnStretch(6, 1);

    packet_splitter_ = new QSplitter(Qt::Horizontal, table_tab);
    packet_trace_model_ = new PacketTraceTableModel(this);
    packet_trace_table_ = new QTableView(packet_splitter_);
    packet_trace_table_->setModel(packet_trace_model_);
    packet_trace_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    packet_trace_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    packet_trace_table_->setSelectionMode(QAbstractItemView::SingleSelection);
    packet_trace_table_->setAlternatingRowColors(true);
    packet_trace_table_->setSortingEnabled(false);
    packet_trace_table_->verticalHeader()->setVisible(false);
    packet_trace_table_->verticalHeader()->setDefaultSectionSize(26);
    packet_trace_table_->setWordWrap(false);
    configure_packet_table_header(packet_trace_table_);

    auto* details = new QTabWidget(packet_splitter_);
    packet_summary_text_ = make_packet_text(details);
    packet_header_text_ = make_packet_text(details);
    packet_target_text_ = make_packet_text(details);
    packet_security_text_ = make_packet_text(details);
    packet_payload_text_ = make_packet_text(details);
    packet_raw_text_ = make_packet_text(details);
    packet_semantic_text_ = make_packet_text(details);
    packet_errors_text_ = make_packet_text(details);
    details->addTab(packet_summary_text_, "Summary");
    details->addTab(packet_header_text_, "Header");
    details->addTab(packet_target_text_, "Source / Target");
    details->addTab(packet_security_text_, "Security / QoS");
    details->addTab(packet_payload_text_, "Payload");
    details->addTab(packet_raw_text_, "Raw Hex");
    details->addTab(packet_semantic_text_, "Semantic");
    details->addTab(packet_errors_text_, "Errors");

    packet_splitter_->addWidget(packet_trace_table_);
    packet_splitter_->addWidget(details);
    packet_splitter_->setStretchFactor(0, 3);
    packet_splitter_->setStretchFactor(1, 2);

    table_root->addWidget(toolbar, 0);
    table_root->addWidget(packet_splitter_, 1);
    tabs->addTab(flow_tab, "Flow");
    tabs->addTab(table_tab, "Table");
    root->addWidget(tabs, 1);

    connect(
        clear_packets_button_, &QPushButton::clicked, this, &MainWindow::stage_clear_packet_traces);
    const auto refresh_filter = [this]() {
        refresh_packets();
        refresh_packet_detail();
    };
    connect(packet_direction_combo_,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            refresh_filter);
    connect(packet_transport_combo_,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            refresh_filter);
    connect(packet_family_combo_,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            refresh_filter);
    connect(packet_errors_only_checkbox_, &QCheckBox::toggled, this, refresh_filter);
    connect(packet_search_edit_, &QLineEdit::textChanged, this, refresh_filter);
    connect(packet_splitter_, &QSplitter::splitterMoved, this, [this]() {
        remember_packet_splitter_sizes();
    });
    connect(packet_trace_table_->selectionModel(),
            &QItemSelectionModel::selectionChanged,
            this,
            [this]() {
                if (packet_trace_table_ != nullptr && packet_trace_model_ != nullptr) {
                    const int row =
                        packet_trace_table_->selectionModel() != nullptr
                            ? packet_trace_table_->selectionModel()->currentIndex().row()
                            : -1;
                    const auto* record = packet_trace_model_->record_at(row);
                    packet_selected_trace_id_ = record != nullptr ? record->trace_id : 0;
                }
                refresh_packet_detail();
                if (packet_flow_mode_combo_ != nullptr &&
                    packet_flow_mode_combo_->currentIndex() == 1) {
                    packet_flow_rendered_trace_id_ = 0;
                    refresh_packet_flow();
                }
            });
    refresh_packet_flow();
    return page;
}

bool MainWindow::packet_trace_visible(const yunlink::PacketTraceRecord& record) const {
    if (packet_errors_only_checkbox_ != nullptr && packet_errors_only_checkbox_->isChecked() &&
        !packet_trace_is_error(record)) {
        return false;
    }
    if (!combo_is_all(packet_direction_combo_)) {
        const QString selected = packet_direction_combo_->currentText();
        if (selected == "RX" && record.direction != yunlink::PacketTraceDirection::kRx) {
            return false;
        }
        if (selected == "TX" && record.direction != yunlink::PacketTraceDirection::kTx) {
            return false;
        }
    }
    if (!combo_is_all(packet_transport_combo_) &&
        packet_transport_combo_->currentText().toStdString() !=
            packet_transport_trace_label(record.transport)) {
        return false;
    }
    if (!combo_is_all(packet_family_combo_)) {
        if (!record.has_envelope || packet_family_combo_->currentText().toStdString() !=
                                        packet_family_label(record.envelope.message_family)) {
            return false;
        }
    }
    if (packet_search_edit_ != nullptr) {
        const QString needle = packet_search_edit_->text().trimmed();
        if (!needle.isEmpty() && !packet_trace_matches_search(record, needle)) {
            return false;
        }
    }
    return true;
}

void MainWindow::refresh_packets() {
    if (backend_ == nullptr || packet_trace_model_ == nullptr ||
        (packet_pause_checkbox_ != nullptr && packet_pause_checkbox_->isChecked())) {
        update_packet_status_label();
        return;
    }

    const QString signature = packet_filter_signature(packet_direction_combo_,
                                                      packet_transport_combo_,
                                                      packet_family_combo_,
                                                      packet_errors_only_checkbox_,
                                                      packet_search_edit_);
    bool full_rebuild = signature != packet_filter_signature_;
    if (full_rebuild) {
        packet_filter_signature_ = signature;
    }

    bool reset_required = false;
    std::vector<yunlink::PacketTraceRecord> filtered;
    const int horizontal_scroll =
        packet_trace_table_ != nullptr && packet_trace_table_->horizontalScrollBar() != nullptr
            ? packet_trace_table_->horizontalScrollBar()->value()
            : 0;
    const auto traces =
        full_rebuild ? backend_->snapshot_packet_traces()
                     : backend_->snapshot_packet_traces_since(packet_last_seen_trace_id_,
                                                              packet_trace_model_->first_trace_id(),
                                                              &reset_required);
    full_rebuild = full_rebuild || reset_required;
    filtered.reserve(traces.size());
    uint64_t newest_trace_id = packet_last_seen_trace_id_;
    for (const auto& record : traces) {
        newest_trace_id = std::max(newest_trace_id, record.trace_id);
        if (packet_trace_visible(record) &&
            (full_rebuild || record.trace_id > packet_last_seen_trace_id_)) {
            filtered.push_back(record);
        }
    }

    if (full_rebuild) {
        packet_trace_model_->replace_records(std::move(filtered));
    } else {
        packet_trace_model_->append_records(filtered);
    }
    restore_table_horizontal_scroll(packet_trace_table_, horizontal_scroll);
    restore_packet_splitter_sizes();
    packet_last_seen_trace_id_ = newest_trace_id;
    update_packet_status_label();

    if (packet_trace_table_ == nullptr || packet_trace_model_->rowCount() <= 0) {
        refresh_packet_detail();
        return;
    }

    const bool should_refresh_detail = update_packet_trace_selection(
        packet_trace_table_,
        packet_trace_model_,
        packet_follow_checkbox_ != nullptr && packet_follow_checkbox_->isChecked(),
        full_rebuild,
        &packet_selected_trace_id_);
    if (!should_refresh_detail) {
        return;
    }
    refresh_packet_detail();
}
