// SPDX-FileCopyrightText: 2021 - 2022 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "dconfig_global.h"
#include <QObject>
#include <QDBusObjectPath>
#include <QDBusContext>
#include <QDBusServiceWatcher>

class DSGConfigResource;
class RefManager;
class ConfigSyncBatchRequest;
class ConfigSyncRequestCache;
class DSGGeneralConfigManager;
/**
 * @brief The DSGConfigServer class
 * 管理配置策略服务
 *
 */
class DSGConfigServer : public QObject, protected QDBusContext
{
    Q_OBJECT
public:
    explicit DSGConfigServer(QObject *parent = nullptr);

    ~DSGConfigServer();

    void exit();

    bool registerService();

    DSGConfigResource* resourceObject(const ResourceKey &key);

    void setLocalPrefix(const QString &localPrefix);

    void setEnableExit(const bool enable);

    int resourceSize() const;

    static QString validDBusObjectPath(const QString &path);

Q_SIGNALS:
    void releaseResource(const ConnKey& resource);

    void tryExit();

public Q_SLOTS:
    QDBusObjectPath acquireManager(const QString &appid, const QString &name, const QString &subpath);

    void update(const QString &path);

    void sync(const QString &path);

    void setDelayReleaseTime(const int ms);
    int delayReleaseTime() const;

private Q_SLOTS:
    void onReleaseChanged(const ConnServiceName &service, const ConnKey &connKey);

    void addConnWatchedService(const ConnServiceName &service);

    void onReleaseResource(const ConnKey &connKey);

    void onTryExit();

    void doSyncConfigCache(const ConfigSyncBatchRequest &request);

    void doUpdateGeneralConfigValueChanged(const QString &key, const ConnKey &connKey);

private:
    ResourceKey getResourceKeyByConfigCache(const ConfigCacheKey &key);

    ConfigureId getConfigureIdByPath(const QString &path);

    bool filterRequestPath(DSGConfigResource *resource, const ConfigureId &configureInfo) const;

private:

    // 所有链接，一个资源对应一个链接
    QMap<ResourceKey, DSGConfigResource*> m_resources;

    QDBusServiceWatcher* m_watcher;

    RefManager* m_refManager;

    QString m_localPrefix;
    bool m_enableExit = false;
    ConfigSyncRequestCache *m_syncRequestCache = nullptr;
    DSGGeneralConfigManager *m_generalConfigManager = nullptr;
};
