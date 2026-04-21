cmake_minimum_required(VERSION 3.16)
project(sunray_communication_lib VERSION 0.1.0 LANGUAGES CXX)

option(SUNRAYCOM_BUILD_EXAMPLES "Build examples" ON)
option(SUNRAYCOM_BUILD_TESTS "Build tests" ON)
option(SUNRAYCOM_ENABLE_CLANG_TIDY "Run clang-tidy during C++ compilation" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (SUNRAYCOM_ENABLE_CLANG_TIDY)
  find_program(
    SUNRAYCOM_CLANG_TIDY_BIN
    NAMES clang-tidy clang-tidy-18 clang-tidy-17 clang-tidy-16
    HINTS /opt/homebrew/opt/llvm/bin /opt/homebrew/bin
  )
  if (NOT SUNRAYCOM_CLANG_TIDY_BIN)
    message(FATAL_ERROR "clang-tidy not found but SUNRAYCOM_ENABLE_CLANG_TIDY=ON")
  endif()
  set(CMAKE_CXX_CLANG_TIDY "${SUNRAYCOM_CLANG_TIDY_BIN}")
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

set(SUNRAYCOM_ASIO_INCLUDE_DIR
  ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/asio/asio/include
)
if (NOT EXISTS ${SUNRAYCOM_ASIO_INCLUDE_DIR}/asio.hpp)
  message(
    FATAL_ERROR
      "standalone Asio not found: ${SUNRAYCOM_ASIO_INCLUDE_DIR}/asio.hpp. "
      "Run: git submodule update --init --recursive"
  )
endif()

add_library(sunraycom STATIC
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
  src/c/sunraycom_c.cpp
)

add_library(sunraycom_asio INTERFACE)
target_include_directories(sunraycom_asio INTERFACE ${SUNRAYCOM_ASIO_INCLUDE_DIR})
target_compile_definitions(sunraycom_asio INTERFACE ASIO_STANDALONE)

target_include_directories(sunraycom
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/transport
)
target_link_libraries(sunraycom PRIVATE sunraycom_asio)

if (WIN32)
  target_link_libraries(sunraycom PRIVATE ws2_32)
else()
  find_package(Threads REQUIRED)
  target_link_libraries(sunraycom PRIVATE Threads::Threads)
endif()

if (SUNRAYCOM_BUILD_EXAMPLES)
  add_executable(example_discovery_udp examples/discovery_udp/main.cpp)
  target_link_libraries(example_discovery_udp PRIVATE sunraycom)

  add_executable(example_telemetry_receiver examples/telemetry_receiver/main.cpp)
  target_link_libraries(example_telemetry_receiver PRIVATE sunraycom)

  add_executable(example_tcp_command_client examples/tcp_command_client/main.cpp)
  target_link_libraries(example_tcp_command_client PRIVATE sunraycom)

  add_executable(example_udp_tcp_bridge examples/udp_tcp_bridge/main.cpp)
  target_link_libraries(example_udp_tcp_bridge PRIVATE sunraycom)

  add_executable(example_embedded_min_loop examples/embedded_min_loop/main.cpp)
  target_link_libraries(example_embedded_min_loop PRIVATE sunraycom)
endif()

if (SUNRAYCOM_BUILD_TESTS)
  enable_testing()

  add_executable(test_protocol tests/test_protocol.cpp)
  target_link_libraries(test_protocol PRIVATE sunraycom)
  add_test(NAME test_protocol COMMAND test_protocol)

  add_executable(test_parser tests/test_parser.cpp)
  target_link_libraries(test_parser PRIVATE sunraycom)
  add_test(NAME test_parser COMMAND test_parser)

  add_executable(test_compat_roundtrip tests/test_compat_roundtrip.cpp)
  target_link_libraries(test_compat_roundtrip PRIVATE sunraycom)
  add_test(NAME test_compat_roundtrip COMMAND test_compat_roundtrip)

  add_executable(test_transport_loop tests/test_transport_loop.cpp)
  target_link_libraries(test_transport_loop PRIVATE sunraycom)
  add_test(NAME test_transport_loop COMMAND test_transport_loop)

  add_executable(test_udp_source_isolation tests/test_udp_source_isolation.cpp)
  target_link_libraries(test_udp_source_isolation PRIVATE sunraycom sunraycom_asio)
  add_test(NAME test_udp_source_isolation COMMAND test_udp_source_isolation)

  add_executable(test_runtime_control_paths tests/test_runtime_control_paths.cpp)
  target_link_libraries(test_runtime_control_paths PRIVATE sunraycom)
  add_test(NAME test_runtime_control_paths COMMAND test_runtime_control_paths)

  add_test(
    NAME test_dev_tooling_config
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/test_dev_tooling_config.cmake
  )

  if (SUNRAYCOM_BUILD_EXAMPLES)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    add_test(
      NAME smoke_examples_local
      COMMAND ${Python3_EXECUTABLE}
              ${CMAKE_CURRENT_SOURCE_DIR}/examples/smoke_local/run_smoke_local.py
              --bin-dir ${CMAKE_CURRENT_BINARY_DIR}
    )
    set_tests_properties(smoke_examples_local PROPERTIES TIMEOUT 30 LABELS "smoke")
  endif()
endif()
