cmake_minimum_required(VERSION 3.16)
project(sunray_yunlink_compare_ui)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(catkin REQUIRED COMPONENTS
  roscpp
  nav_msgs
  mavros_msgs
  sunray_msgs
)
find_package(Qt5 REQUIRED COMPONENTS Widgets)
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

catkin_package()

include_directories(
  ${catkin_INCLUDE_DIRS}
  ${YUNLINK_INCLUDE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

add_executable(sunray_yunlink_compare_ui
  src/backend/compare_backend.cpp
  src/backend/compare_backend_runtime.cpp
  src/main.cpp
  src/mapping/control_state.cpp
  src/mapping/local_odom.cpp
  src/mapping/mavros_state.cpp
  src/mapping/odom_state.cpp
  src/mapping/px4_state.cpp
  src/mapping/value_setters.cpp
  src/model/comparison.cpp
  src/model/format.cpp
  src/model/topic_defs.cpp
  src/ui/main_window.cpp
  src/ui/main_window_logs.cpp
  src/ui/main_window_tables.cpp
)

target_link_libraries(sunray_yunlink_compare_ui
  ${catkin_LIBRARIES}
  Qt5::Widgets
  ${YUNLINK_LIBRARY}
  Threads::Threads
)

install(TARGETS sunray_yunlink_compare_ui
  RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
)

install(DIRECTORY launch/
  DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}/launch
  FILES_MATCHING PATTERN "*.launch"
)
