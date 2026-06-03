#ifndef SUNRAY_ADVANCED_MONITOR_UI_MAIN_WINDOW_HPP
#define SUNRAY_ADVANCED_MONITOR_UI_MAIN_WINDOW_HPP

#include <cstdint>

#include <QLabel>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QTableWidget>

#include "backend/advanced_monitor_backend.hpp"
#include "model/command_model.hpp"
#include "common/monitor_format.hpp"
#include "model/monitor_topics.hpp"

class MainWindow : public QMainWindow {
  public:
    explicit MainWindow(AdvancedMonitorBackend* backend, QWidget* parent = nullptr);

  private:
    void build_ui();
    QWidget* build_status_panel(QWidget* parent);
    QWidget* build_command_panel(QWidget* parent);
    QWidget* build_command_history_panel(QWidget* parent);
    QWidget* build_topics_panel(QWidget* parent);
    QWidget* build_log_panel(QWidget* parent);
    void refresh_view();
    void refresh_status();
    void refresh_command_controls();
    void refresh_command_history();
    void refresh_topics();
    void refresh_topic(const std::string& key, const MonitorTopicState& topic);
    void refresh_logs();
    static QTableWidget* create_topic_table(QWidget* parent);
    static QTableWidgetItem* set_item(QTableWidget* table, int row, int col, const std::string& text);
    static QString format_timestamp(uint64_t timestamp_ms);
    static QDoubleSpinBox* make_spin(double value,
                                     double min_value,
                                     double max_value,
                                     double step,
                                     int decimals = 2);
    void stage_takeoff();
    void stage_land();
    void stage_return();
    void stage_move_point();
    void stage_move_point_body();
    void stage_move_velocity(bool body_frame);

    AdvancedMonitorBackend* backend_{nullptr};
    QLabel* status_value_{nullptr};
    QLabel* peer_value_{nullptr};
    QLabel* session_id_value_{nullptr};
    QLabel* remote_value_{nullptr};
    QLabel* tcp_listen_value_{nullptr};
    QLabel* note_value_{nullptr};
    QLabel* error_value_{nullptr};
    QLabel* command_hint_label_{nullptr};
    QTableWidget* command_history_table_{nullptr};
    std::unordered_map<std::string, QTableWidget*> topic_tables_;
    QPlainTextEdit* logs_{nullptr};
    QPushButton* reconnect_button_{nullptr};
    QPushButton* clear_logs_button_{nullptr};
    QPushButton* takeoff_button_{nullptr};
    QPushButton* land_button_{nullptr};
    QPushButton* return_button_{nullptr};
    QPushButton* point_button_{nullptr};
    QPushButton* body_point_button_{nullptr};
    QPushButton* velocity_button_{nullptr};
    QPushButton* body_velocity_button_{nullptr};
    QDoubleSpinBox* takeoff_height_spin_{nullptr};
    QDoubleSpinBox* takeoff_velocity_spin_{nullptr};
    QDoubleSpinBox* land_velocity_spin_{nullptr};
    QDoubleSpinBox* return_loiter_spin_{nullptr};
    QDoubleSpinBox* point_x_spin_{nullptr};
    QDoubleSpinBox* point_y_spin_{nullptr};
    QDoubleSpinBox* point_z_spin_{nullptr};
    QDoubleSpinBox* point_yaw_spin_{nullptr};
    QDoubleSpinBox* body_point_x_spin_{nullptr};
    QDoubleSpinBox* body_point_y_spin_{nullptr};
    QDoubleSpinBox* body_point_z_spin_{nullptr};
    QDoubleSpinBox* body_point_yaw_spin_{nullptr};
    QDoubleSpinBox* vel_x_spin_{nullptr};
    QDoubleSpinBox* vel_y_spin_{nullptr};
    QDoubleSpinBox* vel_z_spin_{nullptr};
    QDoubleSpinBox* vel_yaw_rate_spin_{nullptr};
    QDoubleSpinBox* body_vx_spin_{nullptr};
    QDoubleSpinBox* body_vy_spin_{nullptr};
    QDoubleSpinBox* body_height_spin_{nullptr};
    QDoubleSpinBox* body_yaw_rate_spin_{nullptr};
    uint64_t rendered_last_sequence_{0};
    size_t rendered_log_count_{0};
};

#endif  // SUNRAY_ADVANCED_MONITOR_UI_MAIN_WINDOW_HPP
