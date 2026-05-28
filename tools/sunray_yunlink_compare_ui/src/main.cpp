#include <QApplication>
#include <QTimer>

#include <ros/ros.h>

#include "backend/compare_backend.hpp"
#include "ui/main_window.hpp"

int main(int argc, char** argv) {
    ros::init(argc, argv, "sunray_yunlink_compare_ui");
    QApplication app(argc, argv);

    CompareBackend backend;
    MainWindow window(&backend);
    window.show();

    QTimer ros_timer;
    QObject::connect(&ros_timer, &QTimer::timeout, []() { ros::spinOnce(); });
    ros_timer.start(20);

    return app.exec();
}
