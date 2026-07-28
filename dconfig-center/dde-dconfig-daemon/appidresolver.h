// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "dconfig_global.h"
#include <QObject>
#include <QHash>
#include <QDBusServiceWatcher>

class AppIdResolver : public QObject {
    Q_OBJECT
public:
    explicit AppIdResolver(QObject *parent = nullptr);
    ~AppIdResolver() override = default;

    // 主入口：通过 service/pid/uid 解析标准 appId
    // service: DBus service name，用于缓存 key
    // pid: 调用方进程 ID
    // uid: 调用方用户 ID
    QString resolveAppId(const ConnServiceName &service, uint pid, uint uid);

    // 清理指定 service 的缓存
    void clearCache(const ConnServiceName &service);

private:
    // 通过 AM Identify 获取 appId
    QString identifyByAM(uint pid, uint uid);
    
    // 从 /proc/exe 格式化 appId 作为 fallback
    QString fallbackFromProc(uint pid);

    // service -> appId 缓存
    QHash<ConnServiceName, QString> m_cache;
};
