#pragma once

#include <memory>
#include <string>
#include <vector>

#include <nanobind/nanobind.h>

#include "yunlink/c/yunlink_c.h"

class ConfigurationBridge {
  public:
    ConfigurationBridge();
    ~ConfigurationBridge();
    void attach(yunlink_runtime_t* runtime);
    void detach(yunlink_runtime_t* runtime) noexcept;
    nanobind::object poll();

  private:
    struct Impl;
    static void onList(void* user_data,
                       const yunlink_config_resource_list_response_view_t* response) noexcept;
    static void
    onDescribe(void* user_data,
               const yunlink_config_resource_describe_response_view_t* response) noexcept;
    static void onGet(void* user_data,
                      const yunlink_config_resource_get_response_view_t* response) noexcept;
    static void onPatch(void* user_data,
                        const yunlink_config_resource_patch_response_view_t* response) noexcept;
    static void onApply(void* user_data,
                        const yunlink_config_resource_apply_response_view_t* response) noexcept;
    std::unique_ptr<Impl> impl_;
};

nanobind::dict configurationList(yunlink_runtime_t* runtime,
                                 const std::string& peer_id,
                                 uint64_t session_id,
                                 const nanobind::dict& target);
nanobind::dict configurationDescribe(yunlink_runtime_t* runtime,
                                     const std::string& peer_id,
                                     uint64_t session_id,
                                     const nanobind::dict& target,
                                     const std::string& resource_id);
nanobind::dict configurationGet(yunlink_runtime_t* runtime,
                                const std::string& peer_id,
                                uint64_t session_id,
                                const nanobind::dict& target,
                                const std::string& resource_id);
nanobind::dict configurationPatch(yunlink_runtime_t* runtime,
                                  const std::string& peer_id,
                                  uint64_t session_id,
                                  const nanobind::dict& target,
                                  const std::string& resource_id,
                                  const std::string& expected_revision,
                                  const nanobind::list& updates,
                                  bool validate_only);
nanobind::dict configurationApply(yunlink_runtime_t* runtime,
                                  const std::string& peer_id,
                                  uint64_t session_id,
                                  const nanobind::dict& target,
                                  const std::string& resource_id,
                                  const std::string& expected_revision);
