#include <QApplication>

#include <ros/ros.h>

#include "backend/advanced_monitor_backend.hpp"
#include "ui/main_window.hpp"

int main(int argc, char** argv) {
    ros::init(argc, argv, "sunray_advanced_monitor");
    QApplication app(argc, argv);

    AdvancedMonitorBackend backend;
    MainWindow window(&backend);
    window.show();

    ros::AsyncSpinner spinner(1);
    spinner.start();

    const int exit_code = app.exec();
    spinner.stop();
    return exit_code;
}
