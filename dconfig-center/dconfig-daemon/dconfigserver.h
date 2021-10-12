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
#include <QObject>
#include <QDBusObjectPath>
#include <QDBusContext>
#include <QDBusServiceWatcher>

class DSGConfigResource;
class RefManager;
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

    void setDelayReleaseTime(const int ms);
    int delayReleaseTime() const;

    void setLocalPrefix(const QString &localPrefix);

    int resourceSize() const;

    static QString validDBusObjectPath(const QString &path);

Q_SIGNALS:
    void releaseResource(const ConnKey& resource);

public Q_SLOTS:
    QDBusObjectPath acquireManager(const QString &appid, const QString &name, const QString &subpath);

    void update(const QString &path);

    void sync(const QString &path);

private Q_SLOTS:
    void onReleaseChanged(const ConnServiceName &service, const ConnKey &connKey);

    void addConnWatchedService(const ConnServiceName &service);

    void onReleaseResource(const ConnKey &connKey);

private:

    ConfigureId getConfigureIdByPath(const QString &path);

    bool filterRequestPath(DSGConfigResource *resource, const ConfigureId &configureInfo) const;

private:

    // 所有链接，一个资源对应一个链接
    QMap<ResourceKey, DSGConfigResource*> m_resources;

    QDBusServiceWatcher* m_watcher;

    RefManager* m_refManager;

    QString m_localPrefix;
};
