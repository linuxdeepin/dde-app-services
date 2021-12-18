/*
 * Copyright (C) 2021 Uniontech Technology Co., Ltd.
 *
 * Author:     yeshanshan <yeshanshan@uniontech.com>
 *
 * Maintainer: yeshanshan <yeshanshan@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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

    QString key() const;

    uint uid() const;

    DTK_CORE_NAMESPACE::DConfigCache *cache() const;

    void setConfigFile(DTK_CORE_NAMESPACE::DConfigFile *configFile);

    void setConfigCache(DTK_CORE_NAMESPACE::DConfigCache *cache);

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
    DTK_CORE_NAMESPACE::DConfigFile *m_config;
    DTK_CORE_NAMESPACE::DConfigCache *m_cache;
    QString m_key;
    QSet<QString> m_keys;
};

