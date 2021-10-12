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
class DConfigMeta;
class DConfigCache;
DCORE_END_NAMESPACE

DCORE_USE_NAMESPACE
class DSGConfigConn;
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

    DSGConfigConn* connObject(const uint uid);

    DSGConfigConn* createConn(const uint uid);

    QString path() const;

    QString getName() const;

    QString getAppid() const;

    void removeConn(const ConnKey &connKey);

    bool isEmptyConn() const;

    void save();

Q_SIGNALS: // SIGNALS
    void updateValueChanged(const QString &key);

    void releaseResource(const ConnServiceName &service);

    void releaseConn(const ConnServiceName &service, const ConnKey &connKey);

    void globalValueChanged(const QString &key);

public Q_SLOTS: // METHODS
    void onGlobalValueChanged(const QString &key);

    bool reparse();

    void onReleaseChanged(const ConnServiceName &service);

private:
    QString getConnKey(const uint uid) const;

    void repareCache(DConfigCache* cache, DConfigMeta *oldMeta, DConfigMeta *newMeta);
private:
    QString m_path;
    QString m_localPrefix;
    QScopedPointer<DConfigFile> m_config;
    QMap<ConnKey, DSGConfigConn*> m_conns;
    QString m_appid;
    QString m_fileName;
    QString m_subpath;
};

