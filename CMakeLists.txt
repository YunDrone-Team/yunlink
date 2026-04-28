cmake_minimum_required(VERSION 3.16)
project(yunlink VERSION 0.1.0 LANGUAGES CXX)

option(YUNLINK_BUILD_EXAMPLES "Build examples" ON)
option(YUNLINK_BUILD_TESTS "Build tests" ON)
option(YUNLINK_ENABLE_CLANG_TIDY "Run clang-tidy during C++ compilation" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (YUNLINK_ENABLE_CLANG_TIDY)
  find_program(
    YUNLINK_CLANG_TIDY_BIN
    NAMES clang-tidy clang-tidy-18 clang-tidy-17 clang-tidy-16
    HINTS /opt/homebrew/opt/llvm/bin /opt/homebrew/bin
  )
  if (NOT YUNLINK_CLANG_TIDY_BIN)
    message(FATAL_ERROR "clang-tidy not found but YUNLINK_ENABLE_CLANG_TIDY=ON")
  endif()
  set(CMAKE_CXX_CLANG_TIDY "${YUNLINK_CLANG_TIDY_BIN}")
endif()

set(ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_dev_tooling_config.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/test_dev_tooling_config.cmake
  @ONLY
)

add_custom_target(clang-format
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/run_clang_format.sh
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  USES_TERMINAL
  COMMENT "Formatting C++ sources with clang-format"
)

add_custom_target(clang-format-check
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_clang_format.sh
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  USES_TERMINAL
  COMMENT "Checking C++ formatting with clang-format"
)

add_custom_target(clang-tidy
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/run_clang_tidy.sh --build-dir ${CMAKE_CURRENT_BINARY_DIR}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  USES_TERMINAL
  COMMENT "Running clang-tidy over project translation units"
)

add_custom_target(check-core-maxline
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_core_maxline.sh
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  USES_TERMINAL
  COMMENT "Checking maxline limits for core code"
)

add_custom_target(lint
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_codestyle.sh --build-dir ${CMAKE_CURRENT_BINARY_DIR}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  USES_TERMINAL
  COMMENT "Running format, clang-tidy and maxline checks"
)

set(YUNLINK_ASIO_INCLUDE_DIR
  ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/asio/asio/include
)
if (NOT EXISTS ${YUNLINK_ASIO_INCLUDE_DIR}/asio.hpp)
  message(
    FATAL_ERROR
      "standalone Asio not found: ${YUNLINK_ASIO_INCLUDE_DIR}/asio.hpp. "
      "Run: git submodule update --init --recursive"
  )
endif()

set(YUNLINK_LIBRARY_SOURCES
  src/core/event_bus.cpp
  src/core/protocol_codec.cpp
  src/core/envelope_stream_parser.cpp
  src/core/semantic_messages_session_authority.cpp
  src/core/semantic_messages_command.cpp
  src/core/semantic_messages_state_bulk.cpp
  src/transport/udp_transport.cpp
  src/transport/tcp_client_pool.cpp
  src/transport/tcp_server.cpp
  src/runtime/runtime_core.cpp
  src/runtime/runtime_session.cpp
  src/runtime/runtime_authority.cpp
  src/runtime/runtime_command.cpp
  src/runtime/runtime_state.cpp
  src/runtime/runtime_state_publish.cpp
  src/c/yunlink_c.cpp
)

add_library(yunlink STATIC ${YUNLINK_LIBRARY_SOURCES})
add_library(yunlink_ffi SHARED ${YUNLINK_LIBRARY_SOURCES})

add_library(yunlink_asio INTERFACE)
target_include_directories(yunlink_asio INTERFACE ${YUNLINK_ASIO_INCLUDE_DIR})
target_compile_definitions(yunlink_asio INTERFACE ASIO_STANDALONE)

target_include_directories(yunlink
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/transport
)
target_include_directories(yunlink_ffi
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/transport
)
target_link_libraries(yunlink PRIVATE yunlink_asio)
target_link_libraries(yunlink_ffi PRIVATE yunlink_asio)
target_compile_definitions(yunlink_ffi PRIVATE YUNLINK_FFI_SHARED_BUILD=1)
set_target_properties(yunlink_ffi PROPERTIES
  OUTPUT_NAME yunlink_ffi
  VERSION ${PROJECT_VERSION}
  SOVERSION 1
)

if (WIN32)
  target_link_libraries(yunlink PRIVATE ws2_32)
  target_link_libraries(yunlink_ffi PRIVATE ws2_32)
else()
  find_package(Threads REQUIRED)
  target_link_libraries(yunlink PRIVATE Threads::Threads)
  target_link_libraries(yunlink_ffi PRIVATE Threads::Threads)
endif()

if (YUNLINK_BUILD_EXAMPLES)
  add_executable(example_discovery_udp examples/discovery_udp/main.cpp)
  target_link_libraries(example_discovery_udp PRIVATE yunlink)

  add_executable(example_telemetry_receiver examples/telemetry_receiver/main.cpp)
  target_link_libraries(example_telemetry_receiver PRIVATE yunlink)

  add_executable(example_tcp_command_client examples/tcp_command_client/main.cpp)
  target_link_libraries(example_tcp_command_client PRIVATE yunlink)

  add_executable(example_udp_tcp_bridge examples/udp_tcp_bridge/main.cpp)
  target_link_libraries(example_udp_tcp_bridge PRIVATE yunlink)

  add_executable(example_cxx_air_roundtrip examples/cxx_air_roundtrip/main.cpp)
  target_link_libraries(example_cxx_air_roundtrip PRIVATE yunlink)
endif()

if (YUNLINK_BUILD_TESTS)
  enable_testing()
  find_package(Python3 COMPONENTS Interpreter REQUIRED)

  add_executable(test_protocol tests/test_protocol.cpp)
  target_link_libraries(test_protocol PRIVATE yunlink)
  add_test(NAME test_protocol COMMAND test_protocol)

  add_executable(test_parser tests/test_parser.cpp)
  target_link_libraries(test_parser PRIVATE yunlink)
  add_test(NAME test_parser COMMAND test_parser)

  add_executable(test_parser_resilience tests/core/test_parser_resilience.cpp)
  target_link_libraries(test_parser_resilience PRIVATE yunlink)
  add_test(NAME test_parser_resilience COMMAND test_parser_resilience)

  add_executable(test_target_selector tests/core/test_target_selector.cpp)
  target_link_libraries(test_target_selector PRIVATE yunlink)
  add_test(NAME test_target_selector COMMAND test_target_selector)

  add_executable(test_protocol_corruption tests/core/test_protocol_corruption.cpp)
  target_link_libraries(test_protocol_corruption PRIVATE yunlink)
  add_test(NAME test_protocol_corruption COMMAND test_protocol_corruption)

  add_executable(test_compat_roundtrip tests/test_compat_roundtrip.cpp)
  target_link_libraries(test_compat_roundtrip PRIVATE yunlink)
  add_test(NAME test_compat_roundtrip COMMAND test_compat_roundtrip)

  add_executable(test_transport_loop tests/test_transport_loop.cpp)
  target_link_libraries(test_transport_loop PRIVATE yunlink)
  add_test(NAME test_transport_loop COMMAND test_transport_loop)

  add_executable(test_udp_source_isolation tests/test_udp_source_isolation.cpp)
  target_link_libraries(test_udp_source_isolation PRIVATE yunlink yunlink_asio)
  add_test(NAME test_udp_source_isolation COMMAND test_udp_source_isolation)

  add_executable(test_runtime_control_paths tests/test_runtime_control_paths.cpp)
  target_link_libraries(test_runtime_control_paths PRIVATE yunlink)
  add_test(NAME test_runtime_control_paths COMMAND test_runtime_control_paths)

  add_executable(test_uav_state_snapshots tests/test_uav_state_snapshots.cpp)
  target_link_libraries(test_uav_state_snapshots PRIVATE yunlink)
  add_test(NAME test_uav_state_snapshots COMMAND test_uav_state_snapshots)

  add_executable(test_authority_edges tests/runtime/test_authority_edges.cpp)
  target_link_libraries(test_authority_edges PRIVATE yunlink)
  add_test(NAME test_authority_edges COMMAND test_authority_edges)

  add_executable(test_authority_target_partition tests/runtime/test_authority_target_partition.cpp)
  target_link_libraries(test_authority_target_partition PRIVATE yunlink)
  add_test(NAME test_authority_target_partition COMMAND test_authority_target_partition)

  add_executable(test_authority_status tests/runtime/test_authority_status.cpp)
  target_link_libraries(test_authority_status PRIVATE yunlink)
  add_test(NAME test_authority_status COMMAND test_authority_status)

  add_executable(test_runtime_restart tests/runtime/test_runtime_restart.cpp)
  target_link_libraries(test_runtime_restart PRIVATE yunlink)
  add_test(NAME test_runtime_restart COMMAND test_runtime_restart)

  add_executable(test_session_lifecycle tests/runtime/test_session_lifecycle.cpp)
  target_link_libraries(test_session_lifecycle PRIVATE yunlink)
  add_test(NAME test_session_lifecycle COMMAND test_session_lifecycle)

  add_executable(test_state_plane_semantics tests/runtime/test_state_plane_semantics.cpp)
  target_link_libraries(test_state_plane_semantics PRIVATE yunlink)
  add_test(NAME test_state_plane_semantics COMMAND test_state_plane_semantics)

  add_executable(test_capability_negotiation tests/runtime/test_capability_negotiation.cpp)
  target_link_libraries(test_capability_negotiation PRIVATE yunlink)
  add_test(NAME test_capability_negotiation COMMAND test_capability_negotiation)

  add_executable(test_broadcast_policy tests/runtime/test_broadcast_policy.cpp)
  target_link_libraries(test_broadcast_policy PRIVATE yunlink)
  add_test(NAME test_broadcast_policy COMMAND test_broadcast_policy)

  add_executable(test_remote_reconnect tests/runtime/test_remote_reconnect.cpp)
  target_link_libraries(test_remote_reconnect PRIVATE yunlink)
  add_test(NAME test_remote_reconnect COMMAND test_remote_reconnect)

  add_executable(test_external_command_handling tests/runtime/test_external_command_handling.cpp)
  target_link_libraries(test_external_command_handling PRIVATE yunlink)
  add_test(NAME test_external_command_handling COMMAND test_external_command_handling)

  add_executable(test_command_result_edges tests/runtime/test_command_result_edges.cpp)
  target_link_libraries(test_command_result_edges PRIVATE yunlink)
  add_test(NAME test_command_result_edges COMMAND test_command_result_edges)

  add_executable(test_runtime_ttl_freshness tests/runtime/test_runtime_ttl_freshness.cpp)
  target_link_libraries(test_runtime_ttl_freshness PRIVATE yunlink)
  add_test(NAME test_runtime_ttl_freshness COMMAND test_runtime_ttl_freshness)

  add_executable(test_trajectory_chunk_runtime tests/runtime/test_trajectory_chunk_runtime.cpp)
  target_link_libraries(test_trajectory_chunk_runtime PRIVATE yunlink)
  add_test(NAME test_trajectory_chunk_runtime COMMAND test_trajectory_chunk_runtime)

  add_executable(test_formation_task_runtime tests/runtime/test_formation_task_runtime.cpp)
  target_link_libraries(test_formation_task_runtime PRIVATE yunlink)
  add_test(NAME test_formation_task_runtime COMMAND test_formation_task_runtime)

  add_executable(test_bulk_channel_runtime tests/runtime/test_bulk_channel_runtime.cpp)
  target_link_libraries(test_bulk_channel_runtime PRIVATE yunlink)
  add_test(NAME test_bulk_channel_runtime COMMAND test_bulk_channel_runtime)

  add_executable(test_qos_runtime tests/runtime/test_qos_runtime.cpp)
  target_link_libraries(test_qos_runtime PRIVATE yunlink)
  add_test(NAME test_qos_runtime COMMAND test_qos_runtime)

  add_executable(test_runtime_version_rejection tests/runtime/test_runtime_version_rejection.cpp)
  target_link_libraries(test_runtime_version_rejection PRIVATE yunlink)
  add_test(NAME test_runtime_version_rejection COMMAND test_runtime_version_rejection)

  add_executable(test_semantic_payload_policy tests/runtime/test_semantic_payload_policy.cpp)
  target_link_libraries(test_semantic_payload_policy PRIVATE yunlink)
  add_test(NAME test_semantic_payload_policy COMMAND test_semantic_payload_policy)

  add_executable(test_runtime_stopped_errors tests/runtime/test_runtime_stopped_errors.cpp)
  target_link_libraries(test_runtime_stopped_errors PRIVATE yunlink)
  add_test(NAME test_runtime_stopped_errors COMMAND test_runtime_stopped_errors)

  add_executable(test_session_security tests/security/test_session_security.cpp)
  target_link_libraries(test_session_security PRIVATE yunlink)
  add_test(NAME test_session_security COMMAND test_session_security)

  add_executable(test_security_tags tests/security/test_security_tags.cpp)
  target_link_libraries(test_security_tags PRIVATE yunlink)
  add_test(NAME test_security_tags COMMAND test_security_tags)

  add_executable(
    test_routing_and_source_validation
    tests/security/test_routing_and_source_validation.cpp
  )
  target_link_libraries(test_routing_and_source_validation PRIVATE yunlink)
  add_test(
    NAME test_routing_and_source_validation
    COMMAND test_routing_and_source_validation
  )

  add_executable(test_tcp_resilience tests/transport/test_tcp_resilience.cpp)
  target_link_libraries(test_tcp_resilience PRIVATE yunlink)
  add_test(NAME test_tcp_resilience COMMAND test_tcp_resilience)

  add_executable(test_c_ffi_v1 tests/bindings/test_c_ffi_v1.cpp)
  target_link_libraries(test_c_ffi_v1 PRIVATE yunlink)
  add_test(NAME test_c_ffi_v1 COMMAND test_c_ffi_v1)

  add_executable(test_c_ffi_contract tests/bindings/test_c_ffi_contract.cpp)
  target_link_libraries(test_c_ffi_contract PRIVATE yunlink)
  add_test(NAME test_c_ffi_contract COMMAND test_c_ffi_contract)

  add_test(
    NAME test_c_ffi_loader
    COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/ffi/test_c_ffi_loader.py
            ${CMAKE_CURRENT_BINARY_DIR}
  )

  add_test(
    NAME test_dev_tooling_config
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/test_dev_tooling_config.cmake
  )

  add_test(
    NAME test_ci_regressions
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_ci_regressions.py
  )

  add_test(
    NAME test_testing_tooling
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_testing_tooling.py
  )

  add_test(
    NAME test_docs_consistency
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_docs_consistency.py
  )

  if (YUNLINK_BUILD_EXAMPLES)
    add_test(
      NAME smoke_examples_local
      COMMAND ${Python3_EXECUTABLE}
              ${CMAKE_CURRENT_SOURCE_DIR}/examples/smoke_local/run_smoke_local.py
              --bin-dir ${CMAKE_CURRENT_BINARY_DIR}
    )
    set_tests_properties(smoke_examples_local PROPERTIES TIMEOUT 30 LABELS "smoke")
  endif()
endif()

add_executable(yunlink_codec_fuzz_harness tests/core/fuzz_protocol_codec.cpp)
target_link_libraries(yunlink_codec_fuzz_harness PRIVATE yunlink)
