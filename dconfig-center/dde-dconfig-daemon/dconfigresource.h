// SPDX-FileCopyrightText: 2021 - 2022 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "dconfig_global.h"
#include <dtkcore_global.h>
#include <QObject>
#include <QDBusObjectPath>
#include <QDBusContext>

DCORE_BEGIN_NAMESPACE
class DConfigFile;
class DConfigMeta;
class DConfigCache;
DCORE_END_NAMESPACE

DCORE_USE_NAMESPACE
class DSGConfigConn;
class ConfigSyncRequestCache;
class DSGGeneralConfigManager;
/**
 * @brief The DSGConfigResource class
 * 管理单个链接
 * 配置文件的解析及方法调用
 */
class DSGConfigResource : public QObject
{
    Q_OBJECT
public:
    DSGConfigResource(const ResourceKey &path, const ResourceKey &localPrefix = QString(), QObject *parent = nullptr);

    virtual ~DSGConfigResource() override;

    bool load(const QString &appid, const QString &name, const QString &subpath);

    void setSyncRequestCache(ConfigSyncRequestCache *cache);

    DSGConfigConn* connObject(const uint uid) const;
    DSGConfigConn* connObject(const ConnKey key) const;

    DSGConfigConn* createConn(const uint uid);

    ResourceKey path() const;

    QString getName() const;

    QString getAppid() const;

    void removeConn(const ConnKey &connKey);

    bool isEmptyConn() const;

    void save();

    void doSyncConfigCache(const ConfigCacheKey &key);

    bool isGeneralResource() const;
    void setGeneralConfigManager(DSGGeneralConfigManager *ma);

Q_SIGNALS: // SIGNALS
    void updateValueChanged(const QString &key);

    void releaseResource(const ConnServiceName &service);

    void releaseConn(const ConnServiceName &service, const ConnKey &connKey);

    void globalValueChanged(const QString &key);

Q_SIGNALS:
    void generalConfigValueChanged(const uint uid, const QString &key);

public Q_SLOTS: // METHODS
    void onGlobalValueChanged(const QString &key);

    bool reparse();

    void onReleaseChanged(const ConnServiceName &service);

private Q_SLOTS:
    void onValueChanged(const QString &key);

private:
    QString getConnKey(const uint uid) const;

    void repareCache(DConfigCache* cache, DConfigMeta *oldMeta, DConfigMeta *newMeta);
    bool setGeneralConfigForConn(DSGConfigConn *conn);
    QSharedPointer<DConfigFile> getOrCreateConfig(bool *isCreate = nullptr) const;
    QSharedPointer<DConfigCache> getOrCreateCache(const uint uid, bool *isCreate = nullptr) const;

private:
    ResourceKey m_path;
    QString m_localPrefix;
    QSharedPointer<DConfigFile> m_config;
    QMap<ConnKey, DSGConfigConn*> m_conns;
    ResourceConfig *m_configs = nullptr;
    QString m_appid;
    QString m_fileName;
    QString m_subpath;
    ConfigSyncRequestCache *m_syncRequestCache = nullptr;
    DSGGeneralConfigManager *m_generalConfigManager = nullptr;
};

