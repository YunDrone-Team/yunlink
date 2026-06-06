#include "ui/main_window.hpp"

void MainWindow::stage_takeoff() {
    if (backend_ == nullptr) {
        return;
    }
    yunlink::TakeoffCommand cmd;
    cmd.relative_height_m = static_cast<float>(takeoff_height_spin_->value());
    cmd.max_velocity_mps = static_cast<float>(takeoff_velocity_spin_->value());
    backend_->send_takeoff(cmd);
}

void MainWindow::stage_land() {
    if (backend_ == nullptr) {
        return;
    }
    yunlink::LandCommand cmd;
    cmd.max_velocity_mps = static_cast<float>(land_velocity_spin_->value());
    backend_->send_land(cmd);
}

void MainWindow::stage_return() {
    if (backend_ == nullptr) {
        return;
    }
    yunlink::ReturnCommand cmd;
    cmd.loiter_before_return_s = static_cast<float>(return_loiter_spin_->value());
    backend_->send_return(cmd);
}

void MainWindow::stage_move_point() {
    if (backend_ == nullptr) {
        return;
    }
    yunlink::GotoCommand cmd;
    cmd.x_m = static_cast<float>(point_x_spin_->value());
    cmd.y_m = static_cast<float>(point_y_spin_->value());
    cmd.z_m = static_cast<float>(point_z_spin_->value());
    cmd.yaw_rad = static_cast<float>(point_yaw_spin_->value());
    backend_->send_goto(cmd);
}

void MainWindow::stage_move_velocity() {
    if (backend_ == nullptr) {
        return;
    }

    yunlink::VelocitySetpointCommand cmd;
    cmd.body_frame = false;
    cmd.vx_mps = static_cast<float>(vel_x_spin_->value());
    cmd.vy_mps = static_cast<float>(vel_y_spin_->value());
    cmd.vz_mps = static_cast<float>(vel_z_spin_->value());
    cmd.yaw_rate_radps = static_cast<float>(vel_yaw_rate_spin_->value());
    backend_->send_velocity_setpoint(cmd);
}
