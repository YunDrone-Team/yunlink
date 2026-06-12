#include "ui/main_window.hpp"

#include <QButtonGroup>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QScrollArea>
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

QWidget* build_description_card(QWidget* parent, const QString& title, const QString& html) {
    auto* group = new QGroupBox(title, parent);
    group->setStyleSheet("QGroupBox { font-weight:600; }");
    auto* layout = new QVBoxLayout(group);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(0);

    auto* label = new QLabel(group);
    label->setWordWrap(true);
    label->setTextFormat(Qt::RichText);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    label->setStyleSheet("color:#334155; line-height:1.2;");
    label->setText(html);
    layout->addWidget(label);
    return group;
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
    const QStringList page_labels = {"Dashboard", "Description", "Devices", "Commands", "System", "State", "Logs"};
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
    page_stack_->addWidget(build_dashboard_page(page_stack_));
    page_stack_->addWidget(build_description_page(page_stack_));
    page_stack_->addWidget(build_devices_page(page_stack_));
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

QWidget* MainWindow::build_description_page(QWidget* parent) {
    auto* page = new QWidget(parent);
    auto* root = new QGridLayout(page);
    root->setContentsMargins(0, 0, 0, 0);
    root->setHorizontalSpacing(10);
    root->setVerticalSpacing(10);

    root->addWidget(build_description_card(
                        page,
                        "Dashboard",
                        "<b>YunLink</b>: runtime, session, authority, peer, remote 总览<br>"
                        "<b>PX4</b>: 飞控连接、飞行状态、电池、位置、setpoint<br>"
                        "<b>Localization</b>: 外部里程计、odom_state、诊断 summary<br>"
                        "<b>Control</b>: 控制器 FSM、里程计有效性、起降参数<br>"
                        "<b>Command</b>: 最近命令、原始输入、已接纳命令<br>"
                        "<b>Issues</b>: 当前关键 WARN / ERROR 汇总"),
                    0,
                    0);
    root->addWidget(build_description_card(
                        page,
                        "Commands",
                        "<b>状态</b>: runtime=进程状态，session=会话状态，link=链路状态<br>"
                        "<b>Authority</b>: 当前控制权状态；是否可控看它，不只看按钮是否可点<br>"
                        "<b>TAKEOFF</b>: relative_height_m=相对起飞高度，max_velocity_mps=最大起飞速度<br>"
                        "<b>LAND</b>: max_velocity_mps=最大降落速度<br>"
                        "<b>RETURN</b>: loiter_before_return_s=返航前盘旋时间<br>"
                        "<b>MOVE_POINT</b>: x_m/y_m/z_m=目标点，yaw_deg=目标偏航角<br>"
                        "<b>MOVE_VELOCITY</b>: vx_mps/vy_mps/vz_mps=惯性系速度，yaw_rate_degps=偏航角速度<br>"
                        "<b>命令状态</b>: SENT=已发出，ACTIVE=已收到/执行中，SUCCEEDED=成功，FAILED=失败，CANCELLED=取消，TIMEOUT=超时/过期"),
                    0,
                    1);
    root->addWidget(build_description_card(
                        page,
                        "System",
                        "<b>FeatureList</b>: 当前可见 feature 列表 + 最近列表请求状态<br>"
                        "<b>FeatureGet</b>: success=查询是否成功，running=当前是否运行，auto_start=是否自启动<br>"
                        "<b>详情</b>: message、description、depends_on、start_preview_*、last_action_*<br>"
                        "<b>feature_name</b>: 目标 feature 名称<br>"
                        "<b>override_args</b>: 启动参数覆盖，逗号分隔<br>"
                        "<b>restart_if_running/start_with_terminal/force_stop</b>: 重启、带终端启动、强制停止开关<br>"
                        "<b>请求状态</b>: PENDING=等待结果，SUCCEEDED=成功，FAILED=失败，TIMEOUT=超时"),
                    0,
                    2);
    root->addWidget(build_description_card(
                        page,
                        "State",
                        "<b>WAIT</b>: 还没收到这个 topic 的任何消息<br>"
                        "<b>--</b>: 收到过消息，但当前字段没有值<br>"
                        "<b>&lt;empty&gt;</b>: 收到的是空字符串<br>"
                        "<b>frame_id</b>: 消息坐标系，<b>stamp_ns</b>: 发送侧原始时间戳，<b>xxx_name</b>: 文字名称<br>"
                        "<b>local_odom</b>: 原始局部里程计；<b>odom_state</b>: 定位融合状态<br>"
                        "<b>uav_control_cmd</b>: 原始命令；<b>uav_control_state</b>: 控制器接纳后的状态<br>"
                        "<b>command_execution_status</b>: 控制侧真实命令执行状态与 ready/busy 门禁<br>"
                        "<b>px4_state</b>: 飞控、电池、GPS、setpoint；<b>sunray_runtime_diagnostic</b>: 聚合诊断"),
                    1,
                    0);
    root->addWidget(build_description_card(
                        page,
                        "Logs",
                        "<b>格式</b>: [时间][级别][来源] 内容<br>"
                        "<b>级别</b>: INFO=信息，WARN=需要注意，ERROR=明确异常<br>"
                        "<b>来源</b>: Connection=连接，Authority=控制权，Command=命令，System=系统服务<br>"
                        "<b>最近异常</b>: 最近几条非 INFO 日志摘要"),
                    1,
                    1);
    root->addWidget(build_description_card(
                        page,
                        "Status",
                        "<b>INIT</b>=未开始，<b>RUNNING</b>=runtime 已启动，<b>FAILED</b>=启动或处理失败<br>"
                        "<b>WAITING_SELECTION</b>=等待用户选择设备，<b>WAITING_CONNECT</b>=等连接，<b>CONNECT_RETRYING</b>=连接失败后重试，<b>ACTIVE</b>=当前会话活跃，<b>RECONNECTING</b>=旧会话断开后重连<br>"
                        "<b>PENDING_GRANT</b>=等待授予控制权，<b>CONTROLLER</b>=已拿到控制权，<b>OBSERVER</b>=仅观察，<b>REJECTED</b>=被拒绝"),
                    1,
                    2);

    root->setColumnStretch(0, 1);
    root->setColumnStretch(1, 1);
    root->setColumnStretch(2, 1);
    root->setRowStretch(0, 1);
    root->setRowStretch(1, 1);
    return page;
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
