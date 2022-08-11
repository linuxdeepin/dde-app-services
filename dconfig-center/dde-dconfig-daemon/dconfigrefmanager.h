// SPDX-FileCopyrightText: 2021 - 2022 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "dconfig_global.h"
#include <QObject>
#include <QHash>
#include <QMap>
#include <QTimer>

class ResourceRef;
class ServiceRef;

class RefManager : public QObject{
    Q_OBJECT
public:
    explicit RefManager(QObject* parent = nullptr);

    ~RefManager();

    void destroy();

    void refResource(const ConnServiceName &service, const ConnKey &resource);

    void derefResource(const ConnServiceName &service, const ConnKey &resource);

    void releaseService(const ConnServiceName &service);

    void setDelayReleaseTime(const int ms);
    int delayReleaseTime() const;

    int getServiceCount();
    int getResourceCount();

    int getServiceCountOnTheResource(const ConnKey &resource);
    int getResourceCountOnTheService(const ConnServiceName &service);

    int getRefResourceCountOnAllService(const ConnKey &resource);
    int getRefResourceCountOnTheService(const ConnServiceName &service);
    int getRefResourceCountOnTheSR(const ConnServiceName &service, const ConnKey &resource);

Q_SIGNALS:
    // 资源无服务使用时，释放资源
    void releaseResource(const ConnKey &resource);

private:
    ResourceRef* getOrCreateResource(const ConnKey &resource);

    ServiceRef* getOrCreateService(const ConnServiceName &service);

    void deleteResource(const QList<ResourceRef *>& deleteResources);

    void doDeleteResource(const QList<ResourceRef *>& deleteResources);

    void delayDeleteResource(const QList<ResourceRef *> &deleteResources);

private:
    // 所有服务，每一个进程对应一个服务，两级关联(用户、pid)
    QMap<ConnServiceName, ServiceRef*> services;

    // 所有资源，每一个配置文件对应一个资源(用户)
    QMap<ConnKey, ResourceRef*> resources;

    // 延迟释放
    int m_delayReleaseTime;
    QMap<ConnKey, QTimer*> m_delayReleaseingConns;
    ObjectPool<QTimer> m_timerPool;
};
