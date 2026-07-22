#pragma once

#include <deque>
#include <mutex>
#include <string>
#include <vector>

#include <nanobind/nanobind.h>

#include "yunlink/c/yunlink_c.h"

class ManagedEntityBridge {
  public:
    void attach(yunlink_runtime_t* runtime);
    void detach(yunlink_runtime_t* runtime) noexcept;
    nanobind::object poll();
    nanobind::dict request(yunlink_runtime_t* runtime,
                           const std::string& peer_id,
                           uint64_t session_id,
                           const nanobind::dict& target);
    nanobind::dict request_attachment(yunlink_runtime_t* runtime,
                                      const std::string& peer_id,
                                      uint64_t session_id,
                                      const nanobind::dict& target,
                                      const std::string& endpoint_uid,
                                      const std::string& directory_revision,
                                      const std::string& action,
                                      const nanobind::list& entity_uids);

  private:
    struct Identity {
        uint8_t agent_type = 0;
        uint32_t agent_id = 0;
        uint8_t role = 0;
        std::vector<uint32_t> group_ids;
    };
    struct Entity {
        std::string entity_uid;
        Identity identity;
        std::string display_name;
        std::string hardware_id;
        std::vector<std::string> capabilities;
        uint8_t availability = 0;
    };
    struct AttachmentResult {
        std::string entity_uid;
        bool accepted = false;
        std::string message;
    };
    enum class EventKind : uint8_t {
        kDirectory,
        kDirectoryChanged,
        kAttachment,
    };
    struct Event {
        EventKind kind = EventKind::kDirectory;
        uint64_t session_id = 0;
        uint64_t message_id = 0;
        uint64_t correlation_id = 0;
        bool success = false;
        std::string message;
        std::string endpoint_uid;
        std::string revision;
        Identity primary_identity;
        std::vector<Entity> entities;
        std::vector<AttachmentResult> attachment_results;
        std::vector<std::string> attached_entity_uids;
    };

    static void onDirectory(void* user_data,
                            const yunlink_managed_entity_list_response_view_t* response) noexcept;
    static void onChanged(void* user_data,
                          const yunlink_managed_entity_directory_changed_view_t* event) noexcept;
    static void
    onAttachment(void* user_data,
                 const yunlink_managed_entity_attachment_response_view_t* response) noexcept;
    void push(Event event) noexcept;

    std::mutex mutex_;
    std::deque<Event> events_;
    std::vector<size_t> tokens_;
};
