/**
 * @file src/runtime/runtime.cpp
 * @brief SunrayComLib source file.
 */

#include "sunraycom/runtime/runtime.hpp"

namespace sunraycom {

Runtime::Runtime() = default;

Runtime::~Runtime() {
    stop();
}

ErrorCode Runtime::start(const RuntimeConfig& config) {
    if (is_started_)
        return ErrorCode::kOk;
    config_ = config;

    const ErrorCode ec_udp = udp_.start(config_, &bus_);
    if (ec_udp != ErrorCode::kOk)
        return ec_udp;

    const ErrorCode ec_clients = tcp_clients_.start(config_, &bus_);
    if (ec_clients != ErrorCode::kOk) {
        udp_.stop();
        return ec_clients;
    }

    const ErrorCode ec_server = tcp_server_.start(config_, &bus_);
    if (ec_server != ErrorCode::kOk) {
        tcp_clients_.stop();
        udp_.stop();
        return ec_server;
    }

    is_started_ = true;
    return ErrorCode::kOk;
}

void Runtime::stop() {
    if (!is_started_)
        return;
    tcp_server_.stop();
    tcp_clients_.stop();
    udp_.stop();
    is_started_ = false;
}

}  // namespace sunraycom
