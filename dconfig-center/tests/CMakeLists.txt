# SPDX-FileCopyrightText: 2022 - 2026 Uniontech Software Technology Co.,Ltd.
#
# SPDX-License-Identifier: LGPL-3.0-only

find_package(Qt${QT_VERSION_MAJOR} ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS Core DBus Test)
find_package(Dtk${DTK_VERSION_MAJOR}Core REQUIRED)

# generate moc_predefs.h
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(AUTOMOC_COMPILER_PREDEFINES ON)

option(ENABLE_COVERAGE "Enable coverage instrumentation" OFF)
# for coverage
if(ENABLE_COVERAGE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lgcov --coverage")
endif()

include(../dde-dconfig-daemon/src.cmake)

list(APPEND SOURCES
    ut_dconfigconn.cpp
    ut_dconfigrefmanager.cpp
    ut_dconfigserver.cpp
    ut_dconfig_global.cpp
    ut_helper.cpp
)

ADD_EXECUTABLE(dconfigtest main.cpp ${HEADERS} ${SOURCES} ${DCONFIG_DBUS_XML} data.qrc)

# Expose private/protected members for testing (equivalent to protected=public private=public
# but without breaking standard library headers)
target_compile_options(dconfigtest PRIVATE -fno-access-control)

target_include_directories(dconfigtest PUBLIC
    ../dde-dconfig-daemon
    ${CMAKE_CURRENT_BINARY_DIR}
)

set(COMMON_LIBS
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::DBus
    Qt${QT_VERSION_MAJOR}::Test
    Dtk${DTK_VERSION_MAJOR}::Core
)

target_link_libraries(dconfigtest PUBLIC ${COMMON_LIBS}
    -lgtest
    -lpthread
)

find_program(LCOV_PATH lcov)
find_program(GENHTML_PATH genhtml)
if(LCOV_PATH AND GENHTML_PATH)
    add_custom_target(coverage
        COMMAND ${LCOV_PATH} --capture --directory ${CMAKE_BINARY_DIR}
                --rc lcov_branch_coverage=1
                --output-file ${CMAKE_BINARY_DIR}/coverage.info
        COMMAND ${LCOV_PATH} --remove ${CMAKE_BINARY_DIR}/coverage.info
                --rc lcov_branch_coverage=1
                '*/moc_*.cpp' '*/moc_*.h' '*/qrc_*.cpp' '*/ui_*.h' '*/qml_*.h'
                '*/qmlcache_*.cpp' '*/_autogen/*' '*_json.h' '/usr/*'
                '*/ut_*' '*/main.cpp' '*/test_helper.hpp' '*_adaptor.*'
                --output-file ${CMAKE_BINARY_DIR}/coverage_filtered.info
        COMMAND ${GENHTML_PATH} ${CMAKE_BINARY_DIR}/coverage_filtered.info
                --branch-coverage
                --output-directory ${CMAKE_BINARY_DIR}/coverage_report
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating coverage report"
    )
endif()



