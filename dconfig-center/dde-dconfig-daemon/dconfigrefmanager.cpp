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

#include "dconfigrefmanager.h"
#include <QDebug>

// 管理服务
class ServiceRef {
public:
    // 清除此资源的引用
    bool reset(const ConnKey &resource);

    // 服务是否还在占用资源
    bool release();
    // 服务key
    ConnServiceName service;
    // 同一服务下的所有资源
    QMap<ConnKey, ResourceRef*> resources;
};

// 管理资源， 资源及引用的个数
class ResourceRef {
public:
    // 资源是否还被使用
    bool release()
    {
        for (auto iter = services.begin(); iter != services.end(); iter++) {
            if (iter.value() > 0) {
                return false;
            }
        }
        return true;
    }
    // 增加服务引用计数
    void ref(ServiceRef* serviceRef)
    {
        services[serviceRef]++;
    }
    // 减少服务引用计数
    bool deref(ServiceRef* serviceRef)
    {
        if (--services[serviceRef] <= 0) {
            return reset(serviceRef);
        }
        return false;
    }

    // 清除此服务的引用情况
    bool reset(ServiceRef* serviceRef)
    {
        if (services.contains(serviceRef)) {
            services.remove(serviceRef);
            serviceRef->reset(resource);
        }
        return release();
    }

    int count(ServiceRef* serviceRef) {
        auto iter = services.find(serviceRef);
        if (iter != services.end()) {
            return iter.value();
        }
        return 0;
    }

    int count() {
        return std::accumulate(services.begin(), services.end(), 0);
    }

    // 资源key
    ConnKey resource;

    // 资源下所有服务对其的引用
    QMap<ServiceRef*, ConnRefCount> services;
};

bool ServiceRef::reset(const ConnKey &resource)
{
    if (resources.contains(resource)) {
        if (auto resourceRef = resources.take(resource)) {
            resourceRef->reset(this);
        }
    }
    return release();
}

bool ServiceRef::release()
{
    return resources.empty();
}

/*!
  class RefManager
 \brief 管理所有服务的资源引用情况
  同一个服务，同一个资源，多个引用
  同一个资源，不同服务，多个引用
  多个服务，多个资源，交叉引用
  服务断开，所有资源的同一资源的所有引用清除，
 */
RefManager::RefManager(QObject *parent)
    : QObject(parent),
      m_delayReleaseTime(30000) // 30s
{
    m_timerPool.setInitFunc([](QTimer* timer){
        timer->setSingleShot(true);
    });
}
RefManager::~RefManager()
{
    destroy();
}

/*!
 \brief 释放所有管理资源
 */
void RefManager::destroy()
{
    qDeleteAll(services);
    services.clear();
    qDeleteAll(resources);
    resources.clear();
}

/*!
 \brief 增加资源的引用计数
 \a service 服务标识，对应连接进来的服务名称
 \a resource 资源标识，对应连接ID
 */
void RefManager::refResource(const ConnServiceName &service, const ConnKey &resource)
{
    auto serviceRef = getOrCreateService(service);
    auto resourceRef = getOrCreateResource(resource);

    serviceRef->resources[resource] = resourceRef;
    resourceRef->ref(serviceRef);
}

/*!
 \brief 减少资源的引用计数
 \a service 服务标识，对应连接进来的服务名称
 \a resource 资源标识，对应连接ID
 */
void RefManager::derefResource(const ConnServiceName &service, const ConnKey &resource)
{
    if (!services.contains(service)) {
        return;
    }
    auto serviceRef = services.value(service);

    if (!resources.contains(resource)) {
        return;
    }
    auto resourceRef = resources.value(resource);

    if (resourceRef->deref(serviceRef)) {
        deleteResource({resourceRef});
    }
}

/*!
 \brief 服务释放导致的资源释放，清空此服务的所有资源引用
 \a service 服务标识，对应连接进来的服务名称
 */
void RefManager::releaseService(const ConnServiceName &service)
{
    if (!services.contains(service)) {
        return;
    }
    auto serviceRef = services.take(service);
    QList<ResourceRef*> deleteResources;
    // 清除此服务下的所有资源引用情况，若资源无服务占用，则删除资源
    auto serviceResourceRef = serviceRef->resources;
    for (auto resourceRef : serviceResourceRef) {

        if (resourceRef->reset(serviceRef)) {
            deleteResources.push_back(resourceRef);
        }
    }
    if (!deleteResources.empty()) {
        deleteResource(deleteResources);
    }

    delete serviceRef;
}

/*!
 \brief 设置资源延迟释放时间
 \a ms
 */
void RefManager::setDelayReleaseTime(const int ms)
{
    m_delayReleaseTime = ms;

    const int TimeOut = 1; // min
    if (ms > (TimeOut * 1000 * 60)) {
        qCWarning(cfLog()) << "It maybe consume resources too much when delayReleaseTime too long"
                           <<", recommand less " << TimeOut  << " min.";
    }

    for (auto timer : m_delayReleaseingConns.values()) {
        // Recalculate remainingTime, to stop the timer when remainingTime less 0.
        int newRemainingTime = ms - timer->remainingTime();
        if (newRemainingTime > 0) {
            qCDebug(cfLog()) << "Reduce remaining time " << newRemainingTime << " ms";
            timer->start(newRemainingTime);
        } else {
            qCDebug(cfLog()) << "Stop Early " << std::abs(newRemainingTime) << " ms";
            timer->stop();
        }
    }
}

/*!
 \brief 资源延迟释放时间
 \return
 */
int RefManager::delayReleaseTime() const
{
    return m_delayReleaseTime;
}

/*!
  \internal
 \brief 获得所有服务对指定资源的引用数量之和
 */
int RefManager::getRefResourceCountOnAllService(const ConnKey &resource)
{
    auto iter = resources.find(resource);
    if (iter == resources.end()) {
        return 0;
    }
    return iter.value()->count();
}

/*!
  \internal
 \brief 获得指定服务对所有资源的引用数量之和
 */
int RefManager::getRefResourceCountOnTheService(const ConnServiceName &service)
{
    auto iter = services.find(service);
    if (iter == services.end()) {
        return 0;
    }
    const auto &serviceResourceRef = iter.value()->resources;
    return std::accumulate(serviceResourceRef.begin(), serviceResourceRef.end(), 0,
                    [iter](int init, ResourceRef *item){
        return init + item->count(iter.value());
    });
}

/*!
  \internal
 \brief 获得指定服务对所有资源的引用数量之和
 */
int RefManager::getServiceCountOnTheResource(const ConnKey &resource)
{
    auto iter = resources.find(resource);
    if (iter == resources.end()) {
        return 0;
    }
    return iter.value()->services.count();
}

/*!
  \internal
 \brief 获得指定服务对资源的引用数量
 */
int RefManager::getResourceCountOnTheService(const ConnServiceName &service)
{
    auto iter = services.find(service);
    if (iter == services.end()) {
        return 0;
    }
    return iter.value()->resources.count();
}

/*!
  \internal
 \brief 获得指定服务对指定资源的引用之和
 */
int RefManager::getRefResourceCountOnTheSR(const ConnServiceName &service, const ConnKey &resource)
{
    if (!services.contains(service)) {
        return 0;
    }
    auto serviceRef = services.value(service);

    if (!resources.contains(resource)) {
        return 0;
    }

    return resources.value(resource)->count(serviceRef);
}

/*!
  \internal
 \brief 获得服务数量
 */
int RefManager::getServiceCount()
{
    return services.count();
}

/*!
  \internal
 \brief 获得资源数量
 */
int RefManager::getResourceCount()
{
    return resources.count();
}

/*!
 \brief 获得资源,当资源不存在时,创建并初始化资源
 \a resource
 \return
 */
ResourceRef* RefManager::getOrCreateResource(const ConnKey &resource)
{
    auto iter = resources.find(resource);
    if (iter != resources.end()) {
        return *iter;
    }
    auto ref = new ResourceRef();
    ref->resource = resource;
    resources.insert(resource, ref);
    return ref;
}

/*!
 \brief 获得服务,当资源不存在时,创建并初始化服务
 \a service
 \return
 */
ServiceRef* RefManager::getOrCreateService(const ConnServiceName &service)
{
    auto iter = services.find(service);
    if (iter != services.end()) {
        return *iter;
    }
    auto ref = new ServiceRef();
    ref->service = service;
    services.insert(service, ref);
    return ref;
}

/*!
 \brief 删除资源
 \a deleteResources 需要删除的资源
 */
void RefManager::deleteResource(const QList<ResourceRef *> &deleteResources)
{
    if (m_delayReleaseTime <= 0) {
        doDeleteResource(deleteResources);
    } else {
        delayDeleteResource(deleteResources);
    }
}

/*!
 \brief 执行删除资源，真正删除资源
 \a deleteResources 需要删除的资源
 */
void RefManager::doDeleteResource(const QList<ResourceRef *> &deleteResources)
{
    QList<ServiceRef*> deleteServiceRefs;
    for (auto resourceRef : deleteResources) {
        // 清除此资源下的所有服务对引用情况，若服务没有引用任一资源，则删除服务
        for (auto serviceRef : resourceRef->services.keys()) {

            if (serviceRef->reset(resourceRef->resource)) {
                deleteServiceRefs.push_back(services.take(serviceRef->service));
            }
        }

        resources.remove(resourceRef->resource);
        emit releaseResource(resourceRef->resource);
    }
    qDeleteAll(deleteResources);
    qDeleteAll(deleteServiceRefs);
}

/*!
 \brief 延迟执行删除资源
 \a deleteResources 需要删除的资源
 */
void RefManager::delayDeleteResource(const QList<ResourceRef *> &deleteResources)
{
    for (auto resourceRef : deleteResources) {
        const ConnKey &resource = resourceRef->resource;

        QTimer *timer = nullptr;
        // 没有引用时，延迟删除连接
        if (m_delayReleaseingConns.contains(resource)) {
            timer = m_delayReleaseingConns.value(resource);
        } else {
            timer = m_timerPool.pull();
            QObject::disconnect(timer, &QTimer::timeout, nullptr, nullptr);
            QObject::connect(timer, &QTimer::timeout, this, [this, resourceRef, timer](){
                m_timerPool.push(timer);
                m_delayReleaseingConns.remove(resourceRef->resource);
                if (resourceRef->release()) {
                    qDebug() << QString("resource[%1] removing.").arg(resourceRef->resource);
                    doDeleteResource({resourceRef});
                }
            });
            m_delayReleaseingConns.insert(resource, timer);
        }
        timer->start(m_delayReleaseTime);
    }
}
