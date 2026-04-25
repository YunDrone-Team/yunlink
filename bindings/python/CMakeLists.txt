cmake_minimum_required(VERSION 3.20)
project(yunlink_python LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Python REQUIRED COMPONENTS Interpreter Development.Module)

execute_process(
  COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
  OUTPUT_VARIABLE nanobind_DIR
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

find_package(nanobind CONFIG REQUIRED)

set(YUNLINK_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(YUNLINK_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../.." "${CMAKE_CURRENT_BINARY_DIR}/yunlink-core")

nanobind_add_module(_yunlink_native src/ffi_module.cpp)
target_link_libraries(_yunlink_native PRIVATE yunlink_ffi)
target_include_directories(_yunlink_native PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../include")
set_target_properties(_yunlink_native PROPERTIES
  INSTALL_RPATH "@loader_path;$ORIGIN"
  BUILD_WITH_INSTALL_RPATH ON
)

install(TARGETS _yunlink_native LIBRARY DESTINATION yunlink)
install(TARGETS yunlink_ffi LIBRARY DESTINATION yunlink RUNTIME DESTINATION yunlink)
