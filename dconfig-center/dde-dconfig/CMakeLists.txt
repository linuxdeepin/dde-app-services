# SPDX-FileCopyrightText: 2022 Uniontech Software Technology Co.,Ltd.
#
# SPDX-License-Identifier: LGPL-3.0-only

PROJECT(dde-dconfig)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

set(REQUIRED_QT_VERSION 5.11.3)
find_package(Qt5 ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS Core DBus)
find_package(DtkWidget REQUIRED)

# generate moc_predefs.h
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(AUTOMOC_COMPILER_PREDEFINES ON)

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX /usr)
endif ()

set(DCONFIG_DBUS_XML_CONFIGMANAGER ../dde-dconfig-daemon/services/org.desktopspec.ConfigManager.xml)
set_source_files_properties(${DCONFIG_DBUS_XML_CONFIGMANAGER} PROPERTIES CLASSNAME DSGConfig NO_NAMESPACE NO_NAMESPACE)
qt5_add_dbus_interface(DCONFIG_DBUS_XML ${DCONFIG_DBUS_XML_CONFIGMANAGER} manager_interface)

set(DCONFIG_DBUS_XML_MANAGER ../dde-dconfig-daemon/services/org.desktopspec.ConfigManager.Manager.xml)
set_source_files_properties(${DCONFIG_DBUS_XML_MANAGER} PROPERTIES CLASSNAME DSGConfigManager NO_NAMESPACE NO_NAMESPACE)
qt5_add_dbus_interface(DCONFIG_DBUS_XML ${DCONFIG_DBUS_XML_MANAGER} configmanager_interface)

set(HEADERS
    ../common/helper.hpp
    ../common/valuehandler.h
)
set(SOURCES
    main.cpp
    ../common/valuehandler.cpp
)

ADD_EXECUTABLE(dde-dconfig ${HEADERS} ${SOURCES} ${DCONFIG_DBUS_XML})

set(COMMON_LIBS
    Qt5::Core
    Qt5::DBus
    ${DtkWidget_LIBRARIES}
)

target_link_libraries(${PROJECT_NAME} PUBLIC ${COMMON_LIBS})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}
    ${DtkWidget_INCLUDE_DIRS}
    ../common
)

install(TARGETS dde-dconfig DESTINATION ${CMAKE_INSTALL_BINDIR})

set(BASH_COMPLETION_COMPLETIONSDIR ${CMAKE_INSTALL_DATADIR}/bash-completion/completions/)
install(DIRECTORY bash_completion.d/ DESTINATION ${BASH_COMPLETION_COMPLETIONSDIR})
