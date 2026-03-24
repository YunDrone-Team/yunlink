/**
 * @file include/sunraycom/core/event_bus.hpp
 * @brief 事件总线定义。
 */

#ifndef SUNRAYCOM_CORE_EVENT_BUS_HPP
#define SUNRAYCOM_CORE_EVENT_BUS_HPP

#include <cstddef>
#include <functional>
#include <mutex>
#include <unordered_map>

#include "sunraycom/core/types.hpp"

namespace sunraycom {

/**
 * @brief 帧事件。
 */
struct FrameEvent {
    TransportType transport = TransportType::kUdpUnicast;
    PeerInfo peer;
    Frame frame;
};

/**
 * @brief 错误事件。
 */
struct ErrorEvent {
    ErrorCode code = ErrorCode::kOk;
    TransportType transport = TransportType::kUdpUnicast;
    PeerInfo peer;
    std::string message;
};

/**
 * @brief 链路状态事件。
 */
struct LinkEvent {
    TransportType transport = TransportType::kTcpClient;
    PeerInfo peer;
    bool is_up = false;
};

/**
 * @brief 线程安全事件总线。
 */
class EventBus {
  public:
    using FrameHandler = std::function<void(const FrameEvent&)>;
    using ErrorHandler = std::function<void(const ErrorEvent&)>;
    using LinkHandler = std::function<void(const LinkEvent&)>;

    /**
     * @brief 订阅帧事件。
     * @param cb 回调函数。
     * @return 订阅 token。
     */
    size_t subscribe_frame(FrameHandler cb);
    /**
     * @brief 订阅错误事件。
     * @param cb 回调函数。
     * @return 订阅 token。
     */
    size_t subscribe_error(ErrorHandler cb);
    /**
     * @brief 订阅链路事件。
     * @param cb 回调函数。
     * @return 订阅 token。
     */
    size_t subscribe_link(LinkHandler cb);
    /**
     * @brief 取消订阅。
     * @param token 订阅 token。
     */
    void unsubscribe(size_t token);

    /**
     * @brief 发布帧事件。
     * @param ev 帧事件。
     */
    void publish_frame(const FrameEvent& ev) const;
    /**
     * @brief 发布错误事件。
     * @param ev 错误事件。
     */
    void publish_error(const ErrorEvent& ev) const;
    /**
     * @brief 发布链路事件。
     * @param ev 链路事件。
     */
    void publish_link(const LinkEvent& ev) const;

  private:
    mutable std::mutex mu_;
    size_t next_token_ = 1;
    std::unordered_map<size_t, FrameHandler> frame_handlers_;
    std::unordered_map<size_t, ErrorHandler> error_handlers_;
    std::unordered_map<size_t, LinkHandler> link_handlers_;
};

}  // namespace sunraycom

#endif  // SUNRAYCOM_CORE_EVENT_BUS_HPP
