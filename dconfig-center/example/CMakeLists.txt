# SPDX-FileCopyrightText: 2022 Uniontech Software Technology Co.,Ltd.
#
# SPDX-License-Identifier: LGPL-3.0-only

find_package(Qt${QT_VERSION_MAJOR} ${REQUIRED_QT_VERSION} REQUIRED COMPONENTS Core)
find_package(Dtk${DTK_VERSION_MAJOR}Core REQUIRED)

ADD_EXECUTABLE(dconfig-example ./main.cpp)

target_link_libraries(dconfig-example PUBLIC Qt${QT_VERSION_MAJOR}::Core Dtk${DTK_VERSION_MAJOR}::Core)

# 配置元文件
dtk_add_config_meta_files(APPID dconfig-example BASE ./configs FILES ./configs/a/example.json)
dtk_add_config_meta_files(COMMONID true FILES ./configs/example.json ./configs/a/example.json)
dtk_add_config_meta_files(APPID dconfig-example FILES ./configs/example.json)

# 配置覆盖文件
dtk_add_config_override_files(APPID dconfig-example BASE ./configs META_NAME example FILES ./configs/dconf-example.override.json ./configs/a/dconf-example.override.a.json)
dtk_add_config_override_files(APPID dconfig-example META_NAME example FILES ./configs/dconf-example.override.json ./configs/a/dconf-example.override.a.json)
