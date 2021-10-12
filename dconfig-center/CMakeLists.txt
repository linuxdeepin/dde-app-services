CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

project(dconfig-center)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory("dconfig-daemon")
add_subdirectory("dconfig-editor")
add_subdirectory("example")
add_subdirectory("tests")
