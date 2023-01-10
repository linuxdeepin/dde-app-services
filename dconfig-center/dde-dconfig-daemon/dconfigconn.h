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
class DConfigCache;
DCORE_END_NAMESPACE

/**
 * @brief The DSGConfigConn class
 * 管理单个链接
 * 配置文件的解析及方法调用
 */
class DSGConfigConn : public QObject, protected QDBusContext
{
    Q_OBJECT
public:
    DSGConfigConn(const ConnKey &key, QObject *parent = nullptr);

    virtual ~DSGConfigConn() override;

    ConnKey key() const;

    uint uid() const;

    QSharedPointer<DTK_CORE_NAMESPACE::DConfigCache> cache() const;

    void setConfigFile(DTK_CORE_NAMESPACE::DConfigFile *configFile);

    void setConfigCache(QSharedPointer<DTK_CORE_NAMESPACE::DConfigCache> cache);

    void setGeneralConfigFile(DTK_CORE_NAMESPACE::DConfigFile *configFile);
    void setGeneralConfigCache(DTK_CORE_NAMESPACE::DConfigCache *cache);

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
    QString visibility(const QString &key) ;
    QString permissions(const QString &key) ;
    int flags(const QString &key);
Q_SIGNALS: // SIGNALS
    void valueChanged(const QString &key);
    void globalValueChanged(const QString &key);

private:
    uint getUid();
    QString getAppid();
    bool contains(const QString &key);

private:
    DTK_CORE_NAMESPACE::DConfigFile *m_config = nullptr;
    QSharedPointer<DTK_CORE_NAMESPACE::DConfigCache> m_cache = nullptr;
    DTK_CORE_NAMESPACE::DConfigFile *m_generalConfig = nullptr;
    DTK_CORE_NAMESPACE::DConfigCache *m_generalCache = nullptr;
    ConnKey m_key;
    QSet<QString> m_keys;
};

