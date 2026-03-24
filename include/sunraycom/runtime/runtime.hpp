/**
 * @file include/sunraycom/runtime/runtime.hpp
 * @brief Runtime 聚合组件定义。
 */

#ifndef SUNRAYCOM_RUNTIME_RUNTIME_HPP
#define SUNRAYCOM_RUNTIME_RUNTIME_HPP

#include "sunraycom/core/event_bus.hpp"
#include "sunraycom/core/runtime_config.hpp"
#include "sunraycom/transport/tcp_client_pool.hpp"
#include "sunraycom/transport/tcp_server.hpp"
#include "sunraycom/transport/udp_transport.hpp"

namespace sunraycom {

/**
 * @brief SunrayComLib 统一运行时。
 */
class Runtime {
  public:
    Runtime();
    ~Runtime();

    /**
     * @brief 启动运行时。
     * @param config 运行时配置。
     * @return 错误码。
     */
    ErrorCode start(const RuntimeConfig& config);
    /**
     * @brief 停止运行时并释放资源。
     */
    void stop();

    /**
     * @brief 访问事件总线。
     * @return 事件总线引用。
     */
    EventBus& event_bus() {
        return bus_;
    }
    /**
     * @brief 访问 UDP 传输组件。
     * @return UDP 传输组件引用。
     */
    UdpTransport& udp() {
        return udp_;
    }
    /**
     * @brief 访问 TCP 客户端池组件。
     * @return TCP 客户端池引用。
     */
    TcpClientPool& tcp_clients() {
        return tcp_clients_;
    }
    /**
     * @brief 访问 TCP 服务端组件。
     * @return TCP 服务端引用。
     */
    TcpServer& tcp_server() {
        return tcp_server_;
    }

  private:
    RuntimeConfig config_;
    EventBus bus_;
    UdpTransport udp_;
    TcpClientPool tcp_clients_;
    TcpServer tcp_server_;
    bool is_started_ = false;
};

}  // namespace sunraycom

#endif  // SUNRAYCOM_RUNTIME_RUNTIME_HPP
