#include "ui/main_window.hpp"

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>

namespace {

constexpr int kNavContainerWidth = 144;
constexpr int kNavInactiveWidth = 118;
constexpr int kNavActiveWidth = 136;

QPushButton* make_nav_button(const QString& text, QWidget* parent) {
    auto* button = new QPushButton(text, parent);
    button->setCheckable(true);
    button->setCursor(Qt::PointingHandCursor);
    button->setMinimumHeight(42);
    button->setMaximumHeight(42);
    button->setFixedWidth(kNavInactiveWidth);
    button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    button->setStyleSheet("QPushButton {"
                          " text-align:left;"
                          " padding:0 16px;"
                          " border:1px solid #c2cad3;"
                          " border-right:none;"
                          " border-top-left-radius:8px;"
                          " border-bottom-left-radius:8px;"
                          " background:#e9edf2;"
                          " color:#26323b;"
                          "}"
                          "QPushButton:hover { background:#f1f4f8; }"
                          "QPushButton:checked {"
                          " background:#fff7d6;"
                          " border-color:#d6b95d;"
                          " color:#1d2329;"
                          " font-weight:600;"
                          "}");
    return button;
}

}  // namespace

void MainWindow::build_ui() {
    auto* central = new QWidget(this);
    auto* root = new QHBoxLayout(central);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(0);

    auto* nav_host = new QWidget(central);
    nav_host->setFixedWidth(kNavContainerWidth);
    auto* nav_layout = new QVBoxLayout(nav_host);
    nav_layout->setContentsMargins(0, 8, 0, 8);
    nav_layout->setSpacing(10);

    auto* nav_group = new QButtonGroup(nav_host);
    nav_group->setExclusive(true);
    const QStringList page_labels = {"Commands", "System", "State", "Logs"};
    for (int index = 0; index < page_labels.size(); ++index) {
        auto* button = make_nav_button(page_labels[index], nav_host);
        page_nav_buttons_.push_back(button);
        nav_group->addButton(button, index);
        nav_layout->addWidget(button, 0, Qt::AlignLeft);
    }
    nav_layout->addStretch(1);

    auto* right_shell = new QWidget(central);
    auto* right_layout = new QVBoxLayout(right_shell);
    right_layout->setContentsMargins(0, 0, 0, 0);
    right_layout->setSpacing(0);

    page_stack_ = new QStackedWidget(right_shell);
    page_stack_->addWidget(build_commands_page(page_stack_));
    page_stack_->addWidget(build_system_service_panel(page_stack_));
    page_stack_->addWidget(build_topics_panel(page_stack_));
    page_stack_->addWidget(build_log_panel(page_stack_));
    right_layout->addWidget(page_stack_, 1);

    root->addWidget(nav_host, 0, Qt::AlignTop);
    root->addSpacing(8);
    root->addWidget(right_shell, 1);

    connect(nav_group,
            QOverload<int>::of(&QButtonGroup::buttonClicked),
            this,
            &MainWindow::set_current_page);

    setCentralWidget(central);
    set_current_page(0);
    refresh_view();
}

void MainWindow::set_current_page(int index) {
    if (page_stack_ == nullptr || index < 0 || index >= page_stack_->count()) {
        return;
    }

    page_stack_->setCurrentIndex(index);
    for (size_t i = 0; i < page_nav_buttons_.size(); ++i) {
        auto* button = page_nav_buttons_[i];
        if (button == nullptr) {
            continue;
        }
        const bool active = static_cast<int>(i) == index;
        button->setChecked(active);
        button->setFixedWidth(active ? kNavActiveWidth : kNavInactiveWidth);
    }
}
