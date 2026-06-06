#include <QApplication>
#include <QTimer>

#include <string>

#include "backend/advanced_monitor_backend.hpp"
#include "ui/main_window.hpp"

namespace {

std::string cli_arg_value(const std::string& arg) {
    const std::size_t pos = arg.find('=');
    if (pos == std::string::npos || pos + 1 >= arg.size()) {
        return std::string();
    }
    return arg.substr(pos + 1);
}

}  // namespace

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    AdvancedMonitorBackend::Config config;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg.rfind("--remote-ip=", 0) == 0) {
            config.remote_ip = cli_arg_value(arg);
        } else if (arg.rfind("--remote-tcp-port=", 0) == 0) {
            config.remote_tcp_port = std::stoi(cli_arg_value(arg));
        } else if (arg.rfind("--udp-bind-port=", 0) == 0) {
            config.udp_bind_port = std::stoi(cli_arg_value(arg));
        } else if (arg.rfind("--udp-target-port=", 0) == 0) {
            config.udp_target_port = std::stoi(cli_arg_value(arg));
        } else if (arg.rfind("--tcp-listen-port=", 0) == 0) {
            config.tcp_listen_port = std::stoi(cli_arg_value(arg));
        } else if (arg.rfind("--agent-id=", 0) == 0) {
            config.agent_id = std::stoi(cli_arg_value(arg));
        } else if (arg.rfind("--agent-name=", 0) == 0) {
            config.agent_name = cli_arg_value(arg);
        } else if (arg.rfind("--shared-secret=", 0) == 0) {
            config.shared_secret = cli_arg_value(arg);
        } else if (arg.rfind("--node-name=", 0) == 0) {
            config.node_name = cli_arg_value(arg);
        } else if (arg.rfind("--log-limit=", 0) == 0) {
            config.log_limit = std::stoi(cli_arg_value(arg));
        } else if (arg.rfind("--authority-ttl-ms=", 0) == 0) {
            config.authority_ttl_ms = std::stoi(cli_arg_value(arg));
        } else if (arg.rfind("--command-history-limit=", 0) == 0) {
            config.command_history_limit = std::stoi(cli_arg_value(arg));
        } else if (arg.rfind("--command-timeout-ms=", 0) == 0) {
            config.command_timeout_ms = std::stoi(cli_arg_value(arg));
        }
    }

    AdvancedMonitorBackend backend(config);
    MainWindow window(&backend);
    window.show();

    QTimer poll_timer;
    QObject::connect(&poll_timer, &QTimer::timeout, [&backend]() { backend.poll_runtime(); });
    poll_timer.start(1000);
    backend.poll_runtime();

    return app.exec();
}
