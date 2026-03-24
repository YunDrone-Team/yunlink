cmake_minimum_required(VERSION 3.16)
project(SunrayComLib VERSION 0.1.0 LANGUAGES CXX)

option(SUNRAYCOM_BUILD_EXAMPLES "Build examples" ON)
option(SUNRAYCOM_BUILD_TESTS "Build tests" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(sunraycom STATIC
  src/core/event_bus.cpp
  src/core/protocol_codec.cpp
  src/core/frame_stream_parser.cpp
  src/core/legacy_adapter.cpp
  src/transport/udp_transport.cpp
  src/transport/tcp_client_pool.cpp
  src/transport/tcp_server.cpp
  src/runtime/runtime.cpp
  src/c/sunraycom_c.cpp
  compat/legacy_sunray/src/LegacyCodec.cpp
)

target_include_directories(sunraycom
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/compat/legacy_sunray/include
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/transport
)

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
  target_link_libraries(test_udp_source_isolation PRIVATE sunraycom)
  add_test(NAME test_udp_source_isolation COMMAND test_udp_source_isolation)

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
