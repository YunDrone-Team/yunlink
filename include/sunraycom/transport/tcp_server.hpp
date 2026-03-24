/**
 * @file include/sunraycom/transport/tcp_server.hpp
 * @brief TCP 服务端组件定义。
 */

#ifndef SUNRAYCOM_TRANSPORT_TCP_SERVER_HPP
#define SUNRAYCOM_TRANSPORT_TCP_SERVER_HPP

#include <atomic>
#include <memory>
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
 * @brief TCP 服务端收发组件。
 */
class TcpServer {
  public:
    TcpServer();
    ~TcpServer();

    /**
     * @brief 启动 TCP 服务端。
     * @param config 运行时配置。
     * @param bus 事件总线。
     * @return 错误码。
     */
    ErrorCode start(const RuntimeConfig& config, EventBus* bus);
    /**
     * @brief 停止 TCP 服务端。
     */
    void stop();

    /**
     * @brief 向指定连接发送字节。
     * @param peer_id 对端 ID。
     * @param bytes 发送内容。
     * @return 发送字节数；失败返回负值。
     */
    int send(const std::string& peer_id, const ByteBuffer& bytes);
    /**
     * @brief 编码并发送协议帧。
     * @param peer_id 对端 ID。
     * @param frame 协议帧。
     * @return 发送字节数；失败返回负值。
     */
    int send_frame(const std::string& peer_id, const Frame& frame);
    /**
     * @brief 向全部连接广播字节。
     * @param bytes 广播内容。
     */
    void broadcast(const ByteBuffer& bytes);
    /**
     * @brief 向全部连接广播协议帧。
     * @param frame 广播帧。
     */
    void broadcast_frame(const Frame& frame);

  private:
    struct ClientConn {
        int sock = -1;
        PeerInfo peer;
        std::atomic<bool> is_running{false};
        std::thread rx_thread;
        FrameStreamParser parser;

        ClientConn() : parser(1U << 20) {}
    };

    void accept_loop();
    void client_loop(std::shared_ptr<ClientConn> conn);

    RuntimeConfig config_;
    EventBus* bus_ = nullptr;
    ProtocolCodec codec_;

    std::atomic<bool> is_running_{false};
    int listen_sock_ = -1;
    std::thread accept_thread_;

    std::mutex mu_;
    std::unordered_map<std::string, std::shared_ptr<ClientConn>> clients_;
};

}  // namespace sunraycom

#endif  // SUNRAYCOM_TRANSPORT_TCP_SERVER_HPP
