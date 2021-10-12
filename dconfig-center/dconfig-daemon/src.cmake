find_package(Qt5DBus REQUIRED)

qt5_add_dbus_adaptor(DCONFIG_DBUS_XML ../dconfig-daemon/services/org.desktopspec.ConfigManager.xml
    dconfigserver.h DSGConfigServer
    configmanager_adaptor DSGConfig)

qt5_add_dbus_adaptor(DCONFIG_DBUS_XML ../dconfig-daemon/services/org.desktopspec.ConfigManager.Manager.xml
    dconfigconn.h DSGConfigConn
    manager_adaptor DSGConfigManager)

set(HEADERS
    ${CMAKE_CURRENT_LIST_DIR}/dconfig_global.h
    ${CMAKE_CURRENT_LIST_DIR}/dconfigserver.h
    ${CMAKE_CURRENT_LIST_DIR}/dconfigresource.h
    ${CMAKE_CURRENT_LIST_DIR}/dconfigconn.h
    ${CMAKE_CURRENT_LIST_DIR}/dconfigrefmanager.h
)
set(SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/dconfigserver.cpp
    ${CMAKE_CURRENT_LIST_DIR}/dconfigresource.cpp
    ${CMAKE_CURRENT_LIST_DIR}/dconfigconn.cpp
    ${CMAKE_CURRENT_LIST_DIR}/dconfigrefmanager.cpp
)
