PROJECT(dconfig-example)
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

set(REQUIRED_QT_VERSION 5.11.3)
find_package(Qt5 ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS Core)
find_package(Dtk REQUIRED Core)

ADD_EXECUTABLE(dconfig-example ./main.cpp)

target_link_libraries(${PROJECT_NAME} PUBLIC Qt5::Core ${DtkCore${DTK_VERSION_SUFFIX}_LIBRARIES})

dconfig_meta_files(APPID dconfig-example BASE ./configs FILES ./configs/example.json ./configs/a/example.json)

dconfig_meta_files(COMMONID true FILES ./configs/example.json ./configs/a/example.json)

dconfig_meta_files(APPID dconfig-example FILES ./configs/example.json ./configs/a/example.json)

dconfig_override_files(APPID dconfig-example BASE ./configs META_NAME example  FILES ./configs/dconf-example.override.json ./configs/a/dconf-example.override.a.json)

dconfig_override_files(APPID dconfig-example  META_NAME example  FILES ./configs/dconf-example.override.json ./configs/a/dconf-example.override.a.json)


