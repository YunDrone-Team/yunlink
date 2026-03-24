/**
 * @file include/sunraycom/transport/tcp_client_pool.hpp
 * @brief TCP 客户端池组件定义。
 */

#ifndef SUNRAYCOM_TRANSPORT_TCP_CLIENT_POOL_HPP
#define SUNRAYCOM_TRANSPORT_TCP_CLIENT_POOL_HPP

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
 * @brief TCP 客户端连接池。
 */
class TcpClientPool {
  public:
    TcpClientPool();
    ~TcpClientPool();

    /**
     * @brief 启动客户端池。
     * @param config 运行时配置。
     * @param bus 事件总线。
     * @return 错误码。
     */
    ErrorCode start(const RuntimeConfig& config, EventBus* bus);
    /**
     * @brief 停止客户端池并断开全部连接。
     */
    void stop();

    /**
     * @brief 连接指定对端。
     * @param ip 目标 IP。
     * @param port 目标端口。
     * @param out_peer_id 输出对端 ID，可空。
     * @return 错误码。
     */
    ErrorCode
    connect_peer(const std::string& ip, uint16_t port, std::string* out_peer_id = nullptr);
    /**
     * @brief 关闭指定对端连接。
     * @param peer_id 对端 ID。
     */
    void close_peer(const std::string& peer_id);

    /**
     * @brief 向指定对端发送原始字节。
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

  private:
    struct PeerConn {
        int sock = -1;
        PeerInfo peer;
        std::atomic<bool> is_running{false};
        std::thread rx_thread;
        FrameStreamParser parser;

        PeerConn() : parser(1U << 20) {}
    };

    void peer_loop(std::shared_ptr<PeerConn> conn);

    RuntimeConfig config_;
    EventBus* bus_ = nullptr;
    ProtocolCodec codec_;

    std::atomic<bool> is_running_{false};
    std::mutex mu_;
    std::unordered_map<std::string, std::shared_ptr<PeerConn>> peers_;
};

}  // namespace sunraycom

#endif  // SUNRAYCOM_TRANSPORT_TCP_CLIENT_POOL_HPP
