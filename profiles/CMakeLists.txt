find_package(Protobuf 3.6 REQUIRED)

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
  YUNLINK_SUNRAY_PROTO_SRCS
  YUNLINK_SUNRAY_PROTO_HDRS
  ${YUNLINK_PROFILE_PROTO_ROOT}/com.yundrone.sunray/v1/sunray.proto
)
add_library(yunlink_profile_sunray STATIC
  ${YUNLINK_SUNRAY_PROTO_SRCS}
  ${YUNLINK_SUNRAY_PROTO_HDRS}
)
target_include_directories(yunlink_profile_sunray PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(yunlink_profile_sunray PUBLIC yunlink_profile_mobility protobuf::libprotobuf)
add_library(yunlink::profile_sunray ALIAS yunlink_profile_sunray)
