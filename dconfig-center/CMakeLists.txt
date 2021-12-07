CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

project(dconfig-center)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory("dde-dconfig-daemon")
add_subdirectory("dde-dconfig-editor")
add_subdirectory("dde-dconfig")
add_subdirectory("example")
add_subdirectory("tests")
