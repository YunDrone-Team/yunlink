cmake_minimum_required(VERSION 3.16)
project(yunlink VERSION 2.0.1 LANGUAGES CXX)

option(YUNLINK_BUILD_TESTS "Build tests" ON)
option(YUNLINK_ENABLE_CLANG_TIDY "Run clang-tidy during C++ compilation" OFF)
option(YUNLINK_BUILD_PROFILES "Build optional Protobuf profiles" OFF)

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
  COMMENT "Checking C++ formatting"
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
  COMMENT "Checking MaxLine and source directory fanout limits"
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
  src/core/wire_v2.cpp
  src/core/wire_v2_codec.cpp
  src/core/core_messages_v2.cpp
  src/core/core_messages_v2_configuration.cpp
  src/runtime/v2/runtime.cpp
  src/runtime/v2/runtime_api.cpp
  src/runtime/v2/runtime_common.cpp
  src/runtime/v2/runtime_network.cpp
  src/runtime/v2/session.cpp
  src/runtime/v2/authority.cpp
  src/c/v2/runtime.cpp
  src/c/v2/discovery.cpp
  src/discovery/discovery_v2.cpp
  src/discovery/discovery_v2_advertiser.cpp
  src/core/configuration_service/codec.cpp
)

add_library(yunlink STATIC ${YUNLINK_LIBRARY_SOURCES})
add_library(yunlink_ffi SHARED ${YUNLINK_LIBRARY_SOURCES})

add_library(yunlink_asio INTERFACE)
target_include_directories(yunlink_asio INTERFACE ${YUNLINK_ASIO_INCLUDE_DIR})
target_compile_definitions(yunlink_asio INTERFACE ASIO_STANDALONE)

target_include_directories(yunlink PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(yunlink_ffi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(yunlink PRIVATE yunlink_asio)
target_link_libraries(yunlink_ffi PRIVATE yunlink_asio)
target_compile_definitions(yunlink_ffi PRIVATE YUNLINK_FFI_SHARED_BUILD=1)
set_target_properties(yunlink_ffi PROPERTIES
  OUTPUT_NAME yunlink_ffi
  VERSION ${PROJECT_VERSION}
  SOVERSION 2
)

if (YUNLINK_BUILD_PROFILES)
  add_subdirectory(profiles)
endif()

if (WIN32)
  target_link_libraries(yunlink PRIVATE ws2_32)
  target_link_libraries(yunlink_ffi PRIVATE ws2_32)
else()
  find_package(Threads REQUIRED)
  target_link_libraries(yunlink PRIVATE Threads::Threads)
  target_link_libraries(yunlink_ffi PRIVATE Threads::Threads)
endif()

if (YUNLINK_BUILD_TESTS)
  enable_testing()
  find_package(Python3 COMPONENTS Interpreter REQUIRED)

  # The C++ tests use assert() as their test primitive, including for setup calls.
  if (MSVC)
    add_compile_options(/UNDEBUG)
  else()
    add_compile_options(-UNDEBUG)
  endif()

  add_executable(test_wire_v2 tests/core/test_wire_v2.cpp)
  target_link_libraries(test_wire_v2 PRIVATE yunlink)
  add_test(NAME test_wire_v2 COMMAND test_wire_v2)

  add_executable(test_core_messages_v2 tests/core/test_core_messages_v2.cpp)
  target_link_libraries(test_core_messages_v2 PRIVATE yunlink)
  add_test(NAME test_core_messages_v2 COMMAND test_core_messages_v2)

  add_executable(test_discovery_v2 tests/core/test_discovery_v2.cpp)
  target_link_libraries(test_discovery_v2 PRIVATE yunlink yunlink_asio)
  add_test(NAME test_discovery_v2 COMMAND test_discovery_v2)

  add_executable(test_runtime_v2 tests/runtime/test_runtime_v2.cpp)
  target_link_libraries(test_runtime_v2 PRIVATE yunlink)
  add_test(NAME test_runtime_v2 COMMAND test_runtime_v2)

  add_executable(test_c_ffi_v2 tests/bindings/test_c_ffi_v2.cpp)
  target_link_libraries(test_c_ffi_v2 PRIVATE yunlink_ffi yunlink)
  add_test(NAME test_c_ffi_v2 COMMAND test_c_ffi_v2)

  add_test(
    NAME test_protocol_boundaries_v2
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_protocol_boundaries_v2.py
  )

  if (YUNLINK_BUILD_PROFILES)
    add_executable(test_profile_golden tests/profiles/test_profile_golden.cpp)
    target_link_libraries(test_profile_golden PRIVATE
      yunlink_profile_sunray
      yunlink_profile_telemetry
      yunlink_profile_media
      yunlink_profile_system
    )
    add_test(NAME test_profile_golden COMMAND test_profile_golden)

    add_executable(test_visual_contract tests/profiles/test_visual_contract.cpp)
    target_compile_definitions(test_visual_contract PRIVATE
      YUNLINK_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    )
    add_test(NAME test_visual_contract COMMAND test_visual_contract)
  endif()
endif()

add_executable(yunlink_codec_fuzz_harness tests/core/fuzz_wire_v2.cpp)
target_link_libraries(yunlink_codec_fuzz_harness PRIVATE yunlink)
