#ifndef SUNRAY_YUNLINK_COMPARE_UI_UI_MAIN_WINDOW_HPP
#define SUNRAY_YUNLINK_COMPARE_UI_UI_MAIN_WINDOW_HPP

#include <string>
#include <unordered_map>
#include <vector>

#include <QCheckBox>
#include <QLabel>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>

#include "backend/compare_backend.hpp"
#include "model/topic_state.hpp"

class MainWindow : public QMainWindow {
  public:
    explicit MainWindow(CompareBackend* backend, QWidget* parent = nullptr);

  private:
    struct RenderedLogGroup {
        LogEntry entry;
        int repeat_count{1};
    };

    void build_ui();
    QWidget* build_log_panel(QWidget* parent);
    void refresh_view();
    void refresh_topic(const std::string& key, const TopicState& topic);
    void refresh_logs(const std::vector<LogEntry>& logs);
    void append_log_entry(const LogEntry& entry);
    void append_log_group_block(const RenderedLogGroup& group);
    void update_last_log_group_block();
    void rebuild_log_view_from_entries(const std::vector<LogEntry>& logs);
    void reset_log_view();
    void sync_log_status();
    void scroll_logs_to_bottom();
    bool is_log_view_at_bottom() const;
    void copy_selected_logs();
    void export_logs();
    QString format_log_line(const RenderedLogGroup& group) const;
    static QString format_log_timestamp(uint64_t timestamp_ms);
    static QString level_label(LogLevel level);
    static QString source_label(LogSource source);

    static QTableWidget* create_compare_table(QWidget* parent, const QStringList& headers);
    static void populate_compare_table(QTableWidget* table,
                                       const TopicState& topic,
                                       const ComparisonSelection& selection);
    static bool is_numeric(const std::string& value);
    static QTableWidgetItem* set_item(QTableWidget* table,
                                      int row,
                                      int col,
                                      const std::string& text);

    CompareBackend* backend_;
    double align_window_ms_{kDefaultAlignWindowMs};
    QLabel* summary_label_{nullptr};
    QPlainTextEdit* logs_{nullptr};
    QLabel* log_status_label_{nullptr};
    QPushButton* jump_to_latest_button_{nullptr};
    QCheckBox* follow_latest_checkbox_{nullptr};
    QCheckBox* pause_logs_checkbox_{nullptr};
    QPushButton* clear_logs_button_{nullptr};
    QPushButton* copy_selected_button_{nullptr};
    QPushButton* export_logs_button_{nullptr};
    std::unordered_map<std::string, QLabel*> topic_info_;
    std::unordered_map<std::string, QLabel*> uncovered_labels_;
    std::unordered_map<std::string, QTableWidget*> topic_latest_tables_;
    std::unordered_map<std::string, QTableWidget*> topic_aligned_tables_;
    std::vector<RenderedLogGroup> rendered_log_groups_;
    uint64_t rendered_last_sequence_{0};
    int pending_new_log_count_{0};
    bool log_overflow_since_render_{false};
    bool suppress_log_scroll_events_{false};
};

#endif  // SUNRAY_YUNLINK_COMPARE_UI_UI_MAIN_WINDOW_HPP
