#ifndef SUNRAY_YUNLINK_COMPARE_UI_UI_MAIN_WINDOW_HPP
#define SUNRAY_YUNLINK_COMPARE_UI_UI_MAIN_WINDOW_HPP

#include <string>
#include <unordered_map>

#include <QLabel>
#include <QMainWindow>
#include <QTableWidget>
#include <QTextEdit>

#include "backend/compare_backend.hpp"
#include "model/topic_state.hpp"

class MainWindow : public QMainWindow {
  public:
    explicit MainWindow(CompareBackend* backend, QWidget* parent = nullptr);

  private:
    void build_ui();
    void refresh_view();
    void refresh_topic(const std::string& key, const TopicState& topic);

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
    QTextEdit* logs_{nullptr};
    std::unordered_map<std::string, QLabel*> topic_info_;
    std::unordered_map<std::string, QLabel*> uncovered_labels_;
    std::unordered_map<std::string, QTableWidget*> topic_latest_tables_;
    std::unordered_map<std::string, QTableWidget*> topic_aligned_tables_;
};

#endif  // SUNRAY_YUNLINK_COMPARE_UI_UI_MAIN_WINDOW_HPP
