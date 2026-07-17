cmake_minimum_required(VERSION 3.16)
project(yunlink_advanced_monitor)

include(CTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt5 QUIET COMPONENTS Widgets)
if(Qt5_FOUND)
  set(YUNLINK_QT_WIDGETS_TARGET Qt5::Widgets)
else()
  find_package(Qt6 REQUIRED COMPONENTS Widgets)
  set(YUNLINK_QT_WIDGETS_TARGET Qt6::Widgets)
endif()
find_package(Threads REQUIRED)

get_filename_component(YUNLINK_ROOT_DEFAULT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
set(YUNLINK_ROOT "${YUNLINK_ROOT_DEFAULT}" CACHE PATH "Path to yunlink repository")
set(YUNLINK_BUILD_DIR "${YUNLINK_ROOT}/build/ninja-debug" CACHE PATH "Path to yunlink build dir")
set(YUNLINK_INCLUDE_DIR "${YUNLINK_ROOT}/include" CACHE PATH "Path to yunlink includes")
set(YUNLINK_LIBRARY "${YUNLINK_BUILD_DIR}/libyunlink.a" CACHE FILEPATH "Path to libyunlink.a")

if(NOT EXISTS "${YUNLINK_INCLUDE_DIR}/yunlink/yunlink.hpp")
  message(FATAL_ERROR "yunlink headers not found: ${YUNLINK_INCLUDE_DIR}/yunlink/yunlink.hpp")
endif()

if(NOT EXISTS "${YUNLINK_LIBRARY}")
  message(FATAL_ERROR "yunlink static library not found: ${YUNLINK_LIBRARY}")
endif()

include_directories(
  ${YUNLINK_INCLUDE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_executable(yunlink_advanced_monitor
  src/backend/advanced_monitor_backend.cpp
  src/backend/advanced_monitor_backend_bindings.cpp
  src/backend/advanced_monitor_backend_commands.cpp
  src/backend/discovery/advanced_monitor_backend_discovery.cpp
  src/backend/advanced_monitor_backend_setup.cpp
  src/backend/advanced_monitor_backend_state.cpp
  src/backend/logging/advanced_monitor_backend_log_policy.cpp
  src/backend/advanced_monitor_backend_utils.cpp
  src/backend/advanced_monitor_runtime.cpp
  src/backend/system_service/advanced_monitor_backend_system_service_feedback.cpp
  src/backend/system_service/advanced_monitor_backend_system_service_requests.cpp
  src/backend/configuration/advanced_monitor_backend_configuration_feedback.cpp
  src/backend/configuration/advanced_monitor_backend_configuration_requests.cpp
  src/common/monitor_format.cpp
  src/common/monitor_topics_command_execution.cpp
  src/common/monitor_ui_style.cpp
  src/common/ui/monitor_ui_dialogs.cpp
  src/common/ui/monitor_ui_log_view.cpp
  src/common/sunray_status_format.cpp
  src/dashboard/developer_status_command_cards.cpp
  src/dashboard/developer_status_model.cpp
  src/dashboard/developer_status_platform_cards.cpp
  src/dashboard/monitor_topics_sunray_runtime.cpp
  src/main.cpp
  src/mapping/value_map.cpp
  src/mapping/value_setters.cpp
  src/model/command_model.cpp
  src/model/discovery/discovery_device.hpp
  src/model/monitor_state.cpp
  src/model/monitor_topics.cpp
  src/model/system_service_model.cpp
  src/packets/flow/packet_flow_canvas.cpp
  src/packets/flow/packet_flow_format.cpp
  src/packets/flow/packet_flow_model.cpp
  src/packets/flow/packet_flow_samples.cpp
  src/packets/format/packet_trace_format.cpp
  src/packets/format/configuration_trace_format.cpp
  src/packets/format/packet_trace_format_detail.cpp
  src/packets/packet_trace_detail.cpp
  src/packets/filters/packet_trace_filters.cpp
  src/packets/selection/packet_trace_selection.cpp
  src/packets/search/packet_trace_search.cpp
  src/packets/status/main_window_packet_status.cpp
  src/packets/packet_trace_semantic.cpp
  src/ui/commands/main_window_command_history.cpp
  src/ui/commands/command_history_table.cpp
  src/ui/main_window_commands.cpp
  src/ui/main_window.cpp
  src/ui/discovery/main_window_devices.cpp
  src/ui/main_window_layout.cpp
  src/ui/main_window_logs.cpp
  src/ui/main_window_navigation.cpp
  src/packets/main_window_packet_flow.cpp
  src/packets/main_window_packet_detail.cpp
  src/packets/main_window_packets.cpp
  src/packets/model/packet_trace_table_header.cpp
  src/packets/model/packet_trace_table_model.cpp
  src/ui/main_window_refresh.cpp
  src/ui/system_service/system_service_ui_helpers.cpp
  src/ui/system_service/main_window_system_service_refresh.cpp
  src/ui/main_window_system_services.cpp
  src/ui/telemetry/main_window_topic_refresh.cpp
  src/ui/configuration/main_window_configuration.cpp
  src/ui/configuration/main_window_configuration_refresh.cpp
)

target_link_libraries(yunlink_advanced_monitor
  ${YUNLINK_QT_WIDGETS_TARGET}
  ${YUNLINK_LIBRARY}
  Threads::Threads
)

install(TARGETS yunlink_advanced_monitor
  RUNTIME DESTINATION bin
)

if(BUILD_TESTING)
  add_executable(test_command_history_table
    tests/test_command_history_table.cpp
    src/ui/commands/command_history_table.cpp
  )
  target_link_libraries(test_command_history_table
    ${YUNLINK_QT_WIDGETS_TARGET}
  )
  add_test(NAME command_history_table COMMAND test_command_history_table)
  set_tests_properties(command_history_table PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
  )
endif()
