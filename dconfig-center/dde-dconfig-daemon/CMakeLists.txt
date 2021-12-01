PROJECT(dde-dconfig-daemon)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

set(REQUIRED_QT_VERSION 5.11.3)
find_package(Qt5 ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS Core DBus)
find_package(DtkCore REQUIRED)

# generate moc_predefs.h
set(CMAKE_AUTOMOC ON)
set(AUTOMOC_COMPILER_PREDEFINES ON)

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX /usr)
endif ()

include(src.cmake)

ADD_EXECUTABLE(dde-dconfig-daemon main.cpp ${HEADERS} ${SOURCES} ${DCONFIG_DBUS_XML})

set(COMMON_LIBS
    Qt5::Core
    Qt5::DBus
    ${DtkCore_LIBRARIES}
)

target_link_libraries(dde-dconfig-daemon PUBLIC ${COMMON_LIBS})

target_include_directories(dde-dconfig-daemon PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}
)

## dbus xml files
set(DCONFIG_XMLS_FILES
    services/org.desktopspec.ConfigManager.xml
    services/org.desktopspec.ConfigManager.Manager.xml
)
install (FILES ${DCONFIG_XMLS_FILES} DESTINATION share/dbus-1/interfaces)

## dbus config file
install (FILES services/org.desktopspec.ConfigManager.conf DESTINATION /usr/share/dbus-1/system.d)

## services files
install(FILES services/org.desktopspec.ConfigManager.service DESTINATION /usr/share/dbus-1/system-services)
install(FILES services/dconfig-center-daemon.service DESTINATION /usr/lib/systemd/system)

## bin
install(TARGETS dde-dconfig-daemon DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)