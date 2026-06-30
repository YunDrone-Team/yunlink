#ifndef YUNLINK_ADVANCED_MONITOR_UI_MAIN_WINDOW_HPP
#define YUNLINK_ADVANCED_MONITOR_UI_MAIN_WINDOW_HPP

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QDoubleSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTableView>
#include <QTabWidget>
#include <QCheckBox>
#include <QComboBox>

#include "backend/advanced_monitor_backend.hpp"
#include "common/monitor_format.hpp"
#include "model/command_model.hpp"
#include "dashboard/developer_status_model.hpp"
#include "model/monitor_state.hpp"
#include "model/monitor_topics.hpp"
#include "model/system_service_model.hpp"

class PacketTraceTableModel;
class PacketFlowCanvas;

class MainWindow : public QMainWindow {
  public:
    explicit MainWindow(AdvancedMonitorBackend* backend, QWidget* parent = nullptr);

  private:
    void build_ui();
    QWidget* build_dashboard_page(QWidget* parent);
    QWidget* build_description_page(QWidget* parent);
    QWidget* build_dashboard_card(QWidget* parent, const QString& title, QLabel** summary, QLabel** body);
    QWidget* build_commands_page(QWidget* parent);
    QWidget* build_devices_page(QWidget* parent);
    QWidget* build_status_panel(QWidget* parent);
    QWidget* build_command_panel(QWidget* parent);
    QWidget* build_command_history_panel(QWidget* parent);
    QWidget* build_system_service_panel(QWidget* parent);
    QWidget* build_topics_panel(QWidget* parent);
    QWidget* build_recent_issues_panel(QWidget* parent);
    QWidget* build_log_page_body(QWidget* parent);
    QWidget* build_log_panel(QWidget* parent);
    QWidget* build_packets_page(QWidget* parent);
    QWidget* build_packet_flow_tab(QWidget* parent);
    void set_current_page(int index);
    void refresh_view();
    void stage_refresh_discovery_devices();
    void refresh_dashboard();
    void refresh_status();
    void refresh_discovery_devices(bool force = false);
    void refresh_recent_issues();
    void refresh_command_controls();
    void refresh_command_history();
    void refresh_system_services();
    void refresh_topics();
    void refresh_topic(const std::string& key, const MonitorTopicState& topic);
    void refresh_logs();
    void refresh_packets();
    void refresh_packet_detail();
    void refresh_packet_flow();
    void clear_packet_texts(const QString& text);
    void remember_packet_splitter_sizes();
    void restore_packet_splitter_sizes();
    bool log_entry_visible(const MonitorLogEntry& entry) const;
    bool log_should_autofollow() const;
    bool packet_trace_visible(const yunlink::PacketTraceRecord& record) const;
    void update_packet_status_label();
    void stage_toggle_log_autofollow(bool checked);
    void stage_clear_packet_traces();
    static QTableWidget* create_topic_table(QWidget* parent);
    static QTableWidgetItem*
    set_item(QTableWidget* table, int row, int col, const std::string& text);
    static QString format_timestamp(uint64_t timestamp_ms);
    static QDoubleSpinBox*
    make_spin(double value, double min_value, double max_value, double step, int decimals = 2);
    void stage_takeoff();
    void stage_land();
    void stage_return();
    void stage_move_point();
    void stage_move_velocity();
    void stage_refresh_feature_list();
    void stage_refresh_feature_detail();
    void stage_start_feature();
    void stage_stop_feature();
    bool confirm_command_send(const QString& command, const QString& payload) const;

    AdvancedMonitorBackend* backend_{nullptr};
    QLabel* status_value_{nullptr};
    QLabel* command_status_value_{nullptr};
    QLabel* devices_summary_value_{nullptr};
    QLabel* peer_value_{nullptr};
    QLabel* command_peer_value_{nullptr};
    QLabel* session_id_value_{nullptr};
    QLabel* command_session_id_value_{nullptr};
    QLabel* remote_value_{nullptr};
    QLabel* command_remote_value_{nullptr};
    QLabel* tcp_listen_value_{nullptr};
    QLabel* command_tcp_listen_value_{nullptr};
    QLabel* authority_value_{nullptr};
    QLabel* command_authority_value_{nullptr};
    QLabel* note_value_{nullptr};
    QLabel* command_note_value_{nullptr};
    QLabel* error_value_{nullptr};
    QLabel* command_error_value_{nullptr};
    QLabel* recent_issues_value_{nullptr};
    QLabel* dashboard_yunlink_summary_{nullptr};
    QLabel* dashboard_px4_summary_{nullptr};
    QLabel* dashboard_localization_summary_{nullptr};
    QLabel* dashboard_control_summary_{nullptr};
    QLabel* dashboard_command_summary_{nullptr};
    QLabel* dashboard_yunlink_body_{nullptr};
    QLabel* dashboard_px4_body_{nullptr};
    QLabel* dashboard_localization_body_{nullptr};
    QLabel* dashboard_control_body_{nullptr};
    QLabel* dashboard_command_body_{nullptr};
    QLabel* dashboard_localization_panel_{nullptr};
    QLabel* dashboard_control_panel_{nullptr};
    QLabel* dashboard_issues_value_{nullptr};
    QLabel* command_hint_label_{nullptr};
    QLabel* current_command_value_{nullptr};
    QLabel* current_execution_state_value_{nullptr};
    QLabel* current_battery_value_{nullptr};
    QLabel* current_execution_reason_value_{nullptr};
    QLabel* current_ready_takeoff_value_{nullptr};
    QLabel* current_ready_land_value_{nullptr};
    QTableWidget* command_history_table_{nullptr};
    QTableWidget* discovery_table_{nullptr};
    QTableWidget* system_service_history_table_{nullptr};
    QStackedWidget* page_stack_{nullptr};
    std::vector<QPushButton*> page_nav_buttons_;
    QListWidget* feature_list_widget_{nullptr};
    QPlainTextEdit* feature_detail_text_{nullptr};
    QLabel* feature_request_preview_{nullptr};
    QLineEdit* feature_override_args_edit_{nullptr};
    QLineEdit* feature_name_edit_{nullptr};
    QCheckBox* feature_restart_checkbox_{nullptr};
    QCheckBox* feature_terminal_checkbox_{nullptr};
    QCheckBox* feature_force_stop_checkbox_{nullptr};
    std::unordered_map<std::string, QTableWidget*> topic_tables_;
    std::unordered_map<std::string, QLabel*> topic_summary_labels_;
    QPlainTextEdit* logs_{nullptr};
    QSplitter* packet_splitter_{nullptr};
    PacketFlowCanvas* packet_flow_canvas_{nullptr};
    QPlainTextEdit* packet_flow_detail_text_{nullptr};
    QTableView* packet_trace_table_{nullptr};
    PacketTraceTableModel* packet_trace_model_{nullptr};
    QPlainTextEdit* packet_summary_text_{nullptr};
    QPlainTextEdit* packet_header_text_{nullptr};
    QPlainTextEdit* packet_target_text_{nullptr};
    QPlainTextEdit* packet_security_text_{nullptr};
    QPlainTextEdit* packet_payload_text_{nullptr};
    QPlainTextEdit* packet_raw_text_{nullptr};
    QPlainTextEdit* packet_semantic_text_{nullptr};
    QPlainTextEdit* packet_errors_text_{nullptr};
    QLabel* packet_status_label_{nullptr};
    QLineEdit* packet_search_edit_{nullptr};
    QComboBox* log_filter_combo_{nullptr};
    QComboBox* packet_direction_combo_{nullptr};
    QComboBox* packet_transport_combo_{nullptr};
    QComboBox* packet_family_combo_{nullptr};
    QComboBox* packet_flow_mode_combo_{nullptr};
    QComboBox* packet_flow_window_combo_{nullptr};
    QCheckBox* log_autofollow_checkbox_{nullptr};
    QCheckBox* packet_pause_checkbox_{nullptr};
    QCheckBox* packet_follow_checkbox_{nullptr};
    QCheckBox* packet_errors_only_checkbox_{nullptr};
    QCheckBox* packet_flow_pause_checkbox_{nullptr};
    QPushButton* reconnect_button_{nullptr};
    QPushButton* clear_logs_button_{nullptr};
    QPushButton* clear_packets_button_{nullptr};
    QPushButton* takeoff_button_{nullptr};
    QPushButton* land_button_{nullptr};
    QPushButton* return_button_{nullptr};
    QPushButton* point_button_{nullptr};
    QPushButton* velocity_button_{nullptr};
    QPushButton* refresh_discovery_button_{nullptr};
    QPushButton* refresh_feature_list_button_{nullptr};
    QPushButton* refresh_feature_detail_button_{nullptr};
    QPushButton* start_feature_button_{nullptr};
    QPushButton* stop_feature_button_{nullptr};
    QDoubleSpinBox* takeoff_height_spin_{nullptr};
    QDoubleSpinBox* takeoff_velocity_spin_{nullptr};
    QDoubleSpinBox* land_velocity_spin_{nullptr};
    QDoubleSpinBox* return_loiter_spin_{nullptr};
    QDoubleSpinBox* point_x_spin_{nullptr};
    QDoubleSpinBox* point_y_spin_{nullptr};
    QDoubleSpinBox* point_z_spin_{nullptr};
    QDoubleSpinBox* point_yaw_spin_{nullptr};
    QDoubleSpinBox* vel_x_spin_{nullptr};
    QDoubleSpinBox* vel_y_spin_{nullptr};
    QDoubleSpinBox* vel_z_spin_{nullptr};
    QDoubleSpinBox* vel_yaw_rate_spin_{nullptr};
    uint64_t rendered_last_sequence_{0};
    size_t rendered_log_count_{0};
    int rendered_visible_log_count_{-1};
    bool log_autofollow_{true};
    uint64_t packet_last_seen_trace_id_{0};
    uint64_t packet_selected_trace_id_{0};
    uint64_t packet_detail_rendered_trace_id_{0};
    uint64_t packet_flow_rendered_trace_id_{0};
    size_t packet_flow_demo_step_{0};
    QString packet_filter_signature_;
    QList<int> packet_splitter_sizes_;
    uint64_t last_discovery_refresh_ms_{0};
    static constexpr uint64_t kDiscoveryRefreshIntervalMs = 2000;
};

#endif  // YUNLINK_ADVANCED_MONITOR_UI_MAIN_WINDOW_HPP
