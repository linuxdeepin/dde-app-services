PROJECT(tests)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

set(REQUIRED_QT_VERSION 5.11.3)
find_package(Qt5 ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS Core DBus)
find_package(DtkCore REQUIRED)
find_package(Qt5 ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS Test)

# generate moc_predefs.h
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(AUTOMOC_COMPILER_PREDEFINES ON)

set(DTK_HAS_UNIT_TEST ON)
# for coverage
if (DTK_HAS_UNIT_TEST)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lgcov --coverage")
endif()

include(../dde-dconfig-daemon/src.cmake)

list(APPEND SOURCES
    ut_dconfigconn.cpp
    ut_dconfigrefmanager.cpp
    ut_dconfigserver.cpp
)

ADD_EXECUTABLE(dconfigtest main.cpp ${HEADERS} ${SOURCES} ${DCONFIG_DBUS_XML} data.qrc)

target_include_directories(dconfigtest PUBLIC
    ../dde-dconfig-daemon
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}
)

set(COMMON_LIBS
    Qt5::Core
    Qt5::DBus
    Qt5::Test
    ${DtkCore_LIBRARIES}
)

target_link_libraries(dconfigtest PUBLIC ${COMMON_LIBS}
    -lgtest
    -lpthread
)
