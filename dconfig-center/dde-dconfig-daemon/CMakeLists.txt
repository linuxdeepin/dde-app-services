# SPDX-FileCopyrightText: 2022 Uniontech Software Technology Co.,Ltd.
#
# SPDX-License-Identifier: LGPL-3.0-only

find_package(Qt${QT_VERSION_MAJOR} ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS Core DBus)
find_package(Dtk${DTK_VERSION_MAJOR}Core REQUIRED)

# generate moc_predefs.h
set(CMAKE_AUTOMOC ON)
set(AUTOMOC_COMPILER_PREDEFINES ON)

include(src.cmake)

ADD_EXECUTABLE(dde-dconfig-daemon main.cpp ${HEADERS} ${SOURCES} ${DCONFIG_DBUS_XML})

set(COMMON_LIBS
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::DBus
    Dtk${DTK_VERSION_MAJOR}::Core
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
install (FILES ${DCONFIG_XMLS_FILES} DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/interfaces)

## dbus config file
install (FILES services/org.desktopspec.ConfigManager.conf DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/system.d)

## services files
install(FILES services/org.desktopspec.ConfigManager.service DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/system-services)
install(FILES services/dde-dconfig-daemon.service DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/systemd/system)

## bin
install(TARGETS dde-dconfig-daemon DESTINATION ${CMAKE_INSTALL_BINDIR})

# debug config file
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/misc/deepin-debug-config/dde-dconfig-daemon.json
    DESTINATION ${CMAKE_INSTALL_DATADIR}/deepin-debug-config/deepin-debug-config.d)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/misc/deepin-log-config/dde-dconfig-daemon.json
    DESTINATION ${CMAKE_INSTALL_DATADIR}/deepin-log-viewer/deepin-log.conf.d)
