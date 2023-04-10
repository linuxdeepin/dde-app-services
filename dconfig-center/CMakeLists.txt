# SPDX-FileCopyrightText: 2021 - 2022 Uniontech Software Technology Co.,Ltd.
#
# SPDX-License-Identifier: LGPL-3.0-only

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

project(dconfig-center)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
include(GNUInstallDirs)

set(DVERSION "1.0.0" CACHE STRING "define project version")
add_definitions(-DVERSION="${DVERSION}")

add_subdirectory("dde-dconfig-daemon")
add_subdirectory("dde-dconfig-editor")
add_subdirectory("dde-dconfig")
add_subdirectory("example")
add_subdirectory("tests")
