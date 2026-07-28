// SPDX-FileCopyrightText: 2021 - 2022 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "dconfig_global.h"
#include "appidresolver.h"
#include <dtkcore_global.h>
#include <QObject>
#include <QDBusObjectPath>
#include <QDBusContext>

DCORE_BEGIN_NAMESPACE
class DConfigFile;
class DConfigCache;
class DConfigMeta;
DCORE_END_NAMESPACE

/**
 * @brief The DSGConfigConn class
 * 管理单个链接
 * 配置文件的解析及方法调用
 */
class DSGConfigResource;
class DSGConfigConn : public QObject, protected QDBusContext
{
    Q_OBJECT
public:
    DSGConfigConn(const ConnKey &key, QObject *parent = nullptr);

    virtual ~DSGConfigConn() override;

    ConnKey key() const;
    QString path() const;
    bool containsWithoutProp(const QString &key) const;

    void setResource(DSGConfigResource *resource);
    
    // 设置配置文件所属的 appId
    void setConfigAppId(const QString &appId);
    
    // 设置 appId 解析器
    void setAppIdResolver(AppIdResolver *resolver);

Q_SIGNALS:
    void releaseChanged(const ConnServiceName &service);

public: // PROPERTIES
    Q_PROPERTY(QStringList keyList READ keyList)
    QStringList keyList() const;

    Q_PROPERTY(QString version READ version)
    QString version() const;

public Q_SLOTS: // METHODS
    QString description(const QString &key, const QString &locale);
    QString name(const QString &key, const QString &locale);
    void release();
    void setValue(const QString &key, const QDBusVariant &value);
    void reset(const QString &key);
    QDBusVariant value(const QString &key);
    bool isDefaultValue(const QString &key);
    QString visibility(const QString &key) ;
    QString permissions(const QString &key) ;
    int flags(const QString &key);
Q_SIGNALS: // SIGNALS
    void valueChanged(const QString &key);
    void globalValueChanged(const QString &key);

private:
    QString getAppid() const;
    QString getCallerAppId() const;  // 获取调用方的标准 appId（用于权限检查）
    bool contains(const QString &key);
    DTK_CORE_NAMESPACE::DConfigMeta *meta() const;
    DTK_CORE_NAMESPACE::DConfigFile *file() const;
    DTK_CORE_NAMESPACE::DConfigCache *cache() const;
    bool hasPermissionByUid(const QString &key) const;
    bool hasPermissionByVisibility(const QString &key) const;  // 检查 private 权限

private:
    ConnKey m_key;
    DSGConfigResource *m_resource = nullptr;
    QString m_configAppId;       // 配置文件所属的 appId
    mutable QString m_callerAppId;        // 调用方的标准 appId（用于权限检查）
    mutable QString m_lastAppIdService;   // 上次解析 appId 时的 service（用于缓存）
    AppIdResolver *m_resolver = nullptr;  // appId 解析器
    mutable QString m_appName;
    mutable QString m_lastService;
};
