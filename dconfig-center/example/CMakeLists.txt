# SPDX-FileCopyrightText: 2022 Uniontech Software Technology Co.,Ltd.
#
# SPDX-License-Identifier: LGPL-3.0-only

PROJECT(dconfig-example)
CMAKE_MINIMUM_REQUIRED(VERSION 3.7)

set(REQUIRED_QT_VERSION 5.11.3)
find_package(Qt${QT_VERSION_MAJOR} ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS Core)
find_package(Dtk${DTK_VERSION_MAJOR}Core REQUIRED)

ADD_EXECUTABLE(dconfig-example ./main.cpp)

target_link_libraries(${PROJECT_NAME} PUBLIC Qt${QT_VERSION_MAJOR}::Core Dtk${DTK_VERSION_MAJOR}::Core)

dtk_add_config_meta_files(APPID dconfig-example BASE ./configs FILES ./configs/a/example.json)

dtk_add_config_meta_files(COMMONID true FILES ./configs/example.json ./configs/a/example.json)

dtk_add_config_meta_files(APPID dconfig-example FILES ./configs/example.json)

dtk_add_config_override_files(APPID dconfig-example BASE ./configs META_NAME example  FILES ./configs/dconf-example.override.json ./configs/a/dconf-example.override.a.json)

dtk_add_config_override_files(APPID dconfig-example  META_NAME example  FILES ./configs/dconf-example.override.json ./configs/a/dconf-example.override.a.json)


