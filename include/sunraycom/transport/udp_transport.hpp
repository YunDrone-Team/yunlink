/**
 * @file include/sunraycom/transport/udp_transport.hpp
 * @brief UDP 传输组件定义。
 */

#ifndef SUNRAYCOM_TRANSPORT_UDP_TRANSPORT_HPP
#define SUNRAYCOM_TRANSPORT_UDP_TRANSPORT_HPP

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

#include "sunraycom/core/event_bus.hpp"
#include "sunraycom/core/frame_stream_parser.hpp"
#include "sunraycom/core/protocol_codec.hpp"
#include "sunraycom/core/runtime_config.hpp"

namespace sunraycom {

/**
 * @brief UDP 收发组件。
 */
class UdpTransport {
  public:
    UdpTransport();
    ~UdpTransport();

    /**
     * @brief 启动 UDP 组件。
     * @param config 运行时配置。
     * @param bus 事件总线。
     * @return 错误码。
     */
    ErrorCode start(const RuntimeConfig& config, EventBus* bus);
    /**
     * @brief 停止 UDP 组件。
     */
    void stop();

    /**
     * @brief 查询运行状态。
     * @return true 表示正在运行。
     */
    bool is_running() const {
        return is_running_.load();
    }
    /**
     * @brief 兼容旧接口的运行状态查询。
     * @return true 表示正在运行。
     */
    bool running() const {
        return is_running();
    }

    /**
     * @brief 发送 UDP 单播数据。
     */
    int send_unicast(const ByteBuffer& bytes, const std::string& ip, uint16_t port);
    /**
     * @brief 发送 UDP 广播数据。
     */
    int send_broadcast(const ByteBuffer& bytes, uint16_t port);
    /**
     * @brief 发送 UDP 组播数据。
     */
    int send_multicast(const ByteBuffer& bytes, uint16_t port);

    /**
     * @brief 编码并发送单播帧。
     */
    int send_frame_unicast(const Frame& frame, const std::string& ip, uint16_t port);
    /**
     * @brief 编码并发送广播帧。
     */
    int send_frame_broadcast(const Frame& frame, uint16_t port);
    /**
     * @brief 编码并发送组播帧。
     */
    int send_frame_multicast(const Frame& frame, uint16_t port);

  private:
    void recv_loop();

    RuntimeConfig config_;
    EventBus* bus_ = nullptr;
    ProtocolCodec codec_;

    std::atomic<bool> is_running_{false};
    int sock_ = -1;
    std::thread recv_thread_;

    std::mutex parser_mu_;
    std::unordered_map<std::string, FrameStreamParser> parsers_;
};

}  // namespace sunraycom

#endif  // SUNRAYCOM_TRANSPORT_UDP_TRANSPORT_HPP
