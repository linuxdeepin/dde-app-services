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
    
    void initialize();

    DSGConfigResource* resourceObject(const GenericResourceKey &key) const;

    void setLocalPrefix(const QString &localPrefix);

    void setEnableExit(const bool enable);

    int resourceSize() const;

Q_SIGNALS:
    void releaseResource(const ConnKey& resource);

    void tryExit();

public Q_SLOTS:
    QDBusObjectPath acquireManager(const QString &appid, const QString &name, const QString &subpath);

    QDBusObjectPath acquireManagerV2(const uint &uid, const QString &appid, const QString &name, const QString &subpath);

    void update(const QString &path);

    void sync(const QString &path);

    void setDelayReleaseTime(const int ms);
    int delayReleaseTime() const;

    void enableVerboseLogging();
    void disableVerboseLogging();
    void setLogRules(const QString &rules);

    void removeUserData(const uint &uid);

    void reload();

private Q_SLOTS:
    void onReleaseChanged(const ConnServiceName &service, const ConnKey &connKey);

    void addConnWatchedService(const ConnServiceName &service);

    void onReleaseResource(const ConnKey &connKey);

    void onTryExit();

    void doSyncConfigCache(const ConfigSyncBatchRequest &request);

private:
    ResourceKey getResourceKeyByConfigCache(const ConfigCacheKey &key);

    ConfigureId getConfigureIdByPath(const QString &path);

    bool isConfigurePath(const QString &path, const QString& appId) const;
    // Reload interface related structures and methods
    struct FileSignature {
        qint64 size;
        QDateTime changeTime;
        QString filePath;
    };
    static QVector<FileSignature> allConfigureFileSignatures(const QString &localPrefix);

private:

    // 所有链接，一个资源对应一个链接
    QMap<GenericResourceKey, DSGConfigResource *> m_resources;

    QDBusServiceWatcher *m_watcher = nullptr;

    RefManager *m_refManager = nullptr;

    QString m_localPrefix;
    bool m_enableExit = false;
    ConfigSyncRequestCache *m_syncRequestCache = nullptr;

    // Last time of the configuration file signature
    QVector<FileSignature> m_fileSignatures;
};
