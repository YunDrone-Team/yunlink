/**
 * @file include/yunlink/runtime/session.hpp
 * @brief Runtime session facade declarations.
 */

#ifndef YUNLINK_RUNTIME_SESSION_HPP
#define YUNLINK_RUNTIME_SESSION_HPP

#include <cstdint>
#include <string>

#include "yunlink/core/semantic_messages.hpp"

namespace yunlink {

class Runtime;

class SessionClient {
  public:
    explicit SessionClient(Runtime* runtime = nullptr);

    uint64_t open_active_session(const std::string& peer_id, const std::string& node_name);
    void bind(Runtime* runtime);

  private:
    Runtime* runtime_ = nullptr;
};

class SessionServer {
  public:
    explicit SessionServer(Runtime* runtime = nullptr);

    bool has_active_session(uint64_t session_id) const;
    bool has_authenticated_active_session(const std::string& peer_id,
                                          uint64_t session_id) const;
    bool describe_session(uint64_t session_id, SessionDescriptor* out) const;
    bool
    describe_session(const std::string& peer_id, uint64_t session_id, SessionDescriptor* out) const;
    bool find_active_session(SessionDescriptor* out) const;
    void bind(Runtime* runtime);

  private:
    Runtime* runtime_ = nullptr;
};

}  // namespace yunlink

#endif  // YUNLINK_RUNTIME_SESSION_HPP
