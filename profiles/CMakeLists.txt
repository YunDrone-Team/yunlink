set(protobuf_MODULE_COMPATIBLE TRUE)
find_program(YUNLINK_PROTOC_EXECUTABLE protoc)
set(YUNLINK_PROTOBUF_HINTS)
if (YUNLINK_PROTOC_EXECUTABLE)
  get_filename_component(YUNLINK_PROTOC_BIN_DIR "${YUNLINK_PROTOC_EXECUTABLE}" DIRECTORY)
  get_filename_component(YUNLINK_PROTOC_PREFIX "${YUNLINK_PROTOC_BIN_DIR}" DIRECTORY)
  list(APPEND YUNLINK_PROTOBUF_HINTS
    "${YUNLINK_PROTOC_PREFIX}/lib/cmake/protobuf"
    "${YUNLINK_PROTOC_PREFIX}/lib64/cmake/protobuf"
  )
endif()
find_package(Protobuf CONFIG QUIET NAMES protobuf HINTS ${YUNLINK_PROTOBUF_HINTS})
if (Protobuf_FOUND AND Protobuf_VERSION VERSION_LESS 3.6)
  message(FATAL_ERROR "Protobuf 3.6 or newer is required")
endif()
if (NOT Protobuf_FOUND)
  find_package(Protobuf 3.6 REQUIRED)
endif()

set(YUNLINK_PROFILE_PROTO_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(PROTOBUF_IMPORT_DIRS ${YUNLINK_PROFILE_PROTO_ROOT})
set(PROTOBUF_GENERATE_CPP_APPEND_PATH OFF)

protobuf_generate_cpp(
  YUNLINK_MOBILITY_PROTO_SRCS
  YUNLINK_MOBILITY_PROTO_HDRS
  ${YUNLINK_PROFILE_PROTO_ROOT}/org.yunlink.mobility/v1/mobility.proto
)
add_library(yunlink_profile_mobility STATIC
  ${YUNLINK_MOBILITY_PROTO_SRCS}
  ${YUNLINK_MOBILITY_PROTO_HDRS}
)
target_include_directories(yunlink_profile_mobility PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(yunlink_profile_mobility PUBLIC protobuf::libprotobuf)
add_library(yunlink::profile_mobility ALIAS yunlink_profile_mobility)

protobuf_generate_cpp(
  YUNLINK_TELEMETRY_PROTO_SRCS
  YUNLINK_TELEMETRY_PROTO_HDRS
  ${YUNLINK_PROFILE_PROTO_ROOT}/org.yunlink.telemetry/v1/telemetry.proto
)
add_library(yunlink_profile_telemetry STATIC
  ${YUNLINK_TELEMETRY_PROTO_SRCS}
  ${YUNLINK_TELEMETRY_PROTO_HDRS}
  ${YUNLINK_PROFILE_PROTO_ROOT}/org.yunlink.telemetry/v1/summary_validation.cpp
)
target_include_directories(yunlink_profile_telemetry PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}
  ${YUNLINK_PROFILE_PROTO_ROOT}
)
target_link_libraries(yunlink_profile_telemetry PUBLIC protobuf::libprotobuf)
add_library(yunlink::profile_telemetry ALIAS yunlink_profile_telemetry)

protobuf_generate_cpp(
  YUNLINK_MEDIA_PROTO_SRCS
  YUNLINK_MEDIA_PROTO_HDRS
  ${YUNLINK_PROFILE_PROTO_ROOT}/org.yunlink.media/v1/media.proto
)
add_library(yunlink_profile_media STATIC
  ${YUNLINK_MEDIA_PROTO_SRCS}
  ${YUNLINK_MEDIA_PROTO_HDRS}
  ${YUNLINK_PROFILE_PROTO_ROOT}/org.yunlink.media/v1/media_validation.cpp
)
target_include_directories(yunlink_profile_media PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}
  ${YUNLINK_PROFILE_PROTO_ROOT}
)
target_link_libraries(yunlink_profile_media PUBLIC protobuf::libprotobuf)
add_library(yunlink::profile_media ALIAS yunlink_profile_media)

protobuf_generate_cpp(
  YUNLINK_SUNRAY_PROTO_SRCS
  YUNLINK_SUNRAY_PROTO_HDRS
  ${YUNLINK_PROFILE_PROTO_ROOT}/com.yundrone.sunray/v2/sunray.proto
)
add_library(yunlink_profile_sunray STATIC
  ${YUNLINK_SUNRAY_PROTO_SRCS}
  ${YUNLINK_SUNRAY_PROTO_HDRS}
  ${YUNLINK_PROFILE_PROTO_ROOT}/com.yundrone.sunray/v2/control_validation.cpp
)
target_include_directories(yunlink_profile_sunray PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}
  ${YUNLINK_PROFILE_PROTO_ROOT}
)
target_link_libraries(yunlink_profile_sunray PUBLIC yunlink_profile_mobility protobuf::libprotobuf)
add_library(yunlink::profile_sunray ALIAS yunlink_profile_sunray)

protobuf_generate_cpp(
  YUNLINK_SYSTEM_PROTO_SRCS
  YUNLINK_SYSTEM_PROTO_HDRS
  ${YUNLINK_PROFILE_PROTO_ROOT}/org.yunlink.system/v1/system.proto
)
add_library(yunlink_profile_system STATIC
  ${YUNLINK_SYSTEM_PROTO_SRCS}
  ${YUNLINK_SYSTEM_PROTO_HDRS}
  ${YUNLINK_PROFILE_PROTO_ROOT}/org.yunlink.system/v1/system_validation.cpp
)
target_include_directories(yunlink_profile_system PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}
  ${YUNLINK_PROFILE_PROTO_ROOT}
)
target_link_libraries(yunlink_profile_system PUBLIC protobuf::libprotobuf)
add_library(yunlink::profile_system ALIAS yunlink_profile_system)
