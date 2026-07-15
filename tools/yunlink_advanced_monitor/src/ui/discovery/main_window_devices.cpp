#include "ui/main_window.hpp"

#include <QDateTime>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QPushButton>
#include <QStringList>
#include <QVBoxLayout>

#include "common/monitor_ui_style.hpp"

namespace {

QString capability_text(const std::vector<std::string>& capabilities) {
    QStringList items;
    for (const auto& item : capabilities) {
        items.append(QString::fromStdString(item));
    }
    return items.join(", ");
}

QString relative_seen_text(uint64_t now_ms, uint64_t last_seen_ms) {
    if (last_seen_ms == 0 || now_ms < last_seen_ms) {
        return "-";
    }
    return QString::number(static_cast<qulonglong>(now_ms - last_seen_ms)) + " ms";
}

}  // namespace

QWidget* MainWindow::build_devices_page(QWidget* parent) {
    auto* page = new QWidget(parent);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    auto* summary_group = new QGroupBox("设备发现与连接", page);
    auto* summary_layout = new QVBoxLayout(summary_group);
    summary_layout->setContentsMargins(12, 12, 12, 12);
    devices_summary_value_ = new QLabel(summary_group);
    devices_summary_value_->setWordWrap(true);
    devices_summary_value_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    summary_layout->addWidget(devices_summary_value_);
    auto* action_row = new QHBoxLayout();
    action_row->setContentsMargins(0, 0, 0, 0);
    refresh_discovery_button_ = new QPushButton("刷新设备", summary_group);
    monitor_ui::style_button(refresh_discovery_button_, monitor_ui::ButtonRole::kSecondary);
    action_row->addWidget(refresh_discovery_button_);
    action_row->addStretch(1);
    summary_layout->addLayout(action_row);
    connect(refresh_discovery_button_, &QPushButton::clicked, this, &MainWindow::stage_refresh_discovery_devices);
    layout->addWidget(summary_group);

    discovery_table_ = new QTableWidget(page);
    discovery_table_->setColumnCount(8);
    discovery_table_->setHorizontalHeaderLabels(
        {"设备名", "Agent", "IP", "TCP", "能力", "最近发现", "状态", "操作"});
    discovery_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    discovery_table_->setSelectionMode(QAbstractItemView::NoSelection);
    discovery_table_->verticalHeader()->setVisible(false);
    discovery_table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    discovery_table_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    discovery_table_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    discovery_table_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    discovery_table_->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
    discovery_table_->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    discovery_table_->horizontalHeader()->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    discovery_table_->horizontalHeader()->setSectionResizeMode(7, QHeaderView::ResizeToContents);
    monitor_ui::style_table(discovery_table_);
    layout->addWidget(discovery_table_, 1);
    return page;
}

void MainWindow::stage_refresh_discovery_devices() {
    if (backend_ == nullptr) {
        return;
    }
    backend_->request_discovery_scan();
    backend_->poll_runtime();
    refresh_discovery_devices(true);
}

void MainWindow::refresh_discovery_devices(bool force) {
    if (backend_ == nullptr || discovery_table_ == nullptr) {
        return;
    }

    const uint64_t now_ms = static_cast<uint64_t>(QDateTime::currentMSecsSinceEpoch());
    if (!force && last_discovery_refresh_ms_ != 0 &&
        now_ms < last_discovery_refresh_ms_ + kDiscoveryRefreshIntervalMs) {
        return;
    }
    last_discovery_refresh_ms_ = now_ms;

    const auto devices = backend_->snapshot_discovery_devices();
    const auto selected_key = backend_->selected_discovery_device_key();
    const auto connection = backend_->snapshot_connection();
    if (devices_summary_value_ != nullptr) {
        int active_count = 0;
        int stale_count = 0;
        for (const auto& device : devices) {
            if (device.stale) {
                ++stale_count;
            } else {
                ++active_count;
            }
        }
        devices_summary_value_->setText(
            QString("活跃=%1 | 过期=%2 | 已选设备=%3 | 当前 peer=%4")
                .arg(active_count)
                .arg(stale_count)
                .arg(selected_key.empty() ? "-" : QString::fromStdString(selected_key))
                .arg(connection.peer_id.empty() ? "-" : QString::fromStdString(connection.peer_id)));
    }

    discovery_table_->setRowCount(static_cast<int>(devices.size()));
    for (int row = 0; row < static_cast<int>(devices.size()); ++row) {
        const auto& device = devices[static_cast<std::size_t>(row)];
        set_item(discovery_table_, row, 0, device.display_name);
        set_item(discovery_table_, row, 1, device.agent_type + "/" + std::to_string(device.agent_id));
        set_item(discovery_table_, row, 2, device.source_ip);
        set_item(discovery_table_, row, 3, std::to_string(device.tcp_listen_port));
        set_item(discovery_table_, row, 4, capability_text(device.capabilities).toStdString());
        set_item(discovery_table_,
                 row,
                 5,
                 relative_seen_text(now_ms, device.last_seen_ms).toStdString());

        std::string status = device.stale ? "STALE" : "ACTIVE";
        if (device.dedupe_key == selected_key) {
            status += "/SELECTED";
        }
        auto* status_item = set_item(discovery_table_, row, 6, status);
        monitor_ui::set_status_item(status_item, monitor_ui::level_from_status(status));

        const bool is_selected = device.dedupe_key == selected_key;
        const bool is_connected_target =
            is_selected && connection.remote_endpoint == (device.source_ip + ":" + std::to_string(device.tcp_listen_port));
        auto* button = qobject_cast<QPushButton*>(discovery_table_->cellWidget(row, 7));
        if (button == nullptr) {
            button = new QPushButton("连接", discovery_table_);
            discovery_table_->setCellWidget(row, 7, button);
        }
        button->setText(is_connected_target ? "断开" : "连接");
        monitor_ui::style_button(button,
                                 is_connected_target ? monitor_ui::ButtonRole::kWarning
                                                     : monitor_ui::ButtonRole::kPrimary);
        button->setEnabled(is_connected_target || !device.stale);
        button->setProperty("dedupe_key", QString::fromStdString(device.dedupe_key));
        button->setProperty("disconnect_current", is_connected_target);
        button->disconnect();
        connect(button, &QPushButton::clicked, this, [this, button]() {
            if (backend_ == nullptr) {
                return;
            }
            const bool disconnect_current = button->property("disconnect_current").toBool();
            if (disconnect_current) {
                backend_->disconnect_current_device();
                return;
            }
            const QString key = button->property("dedupe_key").toString();
            backend_->connect_to_discovered_device(key.toStdString());
        });
    }
}
