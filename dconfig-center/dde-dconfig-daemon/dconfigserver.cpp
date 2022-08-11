// SPDX-FileCopyrightText: 2021 - 2022 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "dconfigserver.h"
#include "dconfigresource.h"
#include "dconfigconn.h"
#include "dconfigrefmanager.h"
#include <QDBusMessage>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QCoreApplication>
#include <QDebug>
#include <QLoggingCategory>

#include "configmanager_adaptor.h"

#define DSG_CONFIG "org.desktopspec.ConfigManager"

#ifndef QT_DEBUG
Q_LOGGING_CATEGORY(cfLog, "dsg.config", QtInfoMsg);
#else
Q_LOGGING_CATEGORY(cfLog, "dsg.config");
#endif

__attribute__((constructor)) // 在库被加载时就执行此函数
static void registerMetaType ()
{
    qRegisterMetaType<ConnServiceName>("ConnServiceName");
    qRegisterMetaType<ConnKey>("ConnKey");
}

DSGConfigServer::DSGConfigServer(QObject *parent)
    :QObject (parent),
      m_watcher(nullptr),
      m_refManager(new RefManager(this))
{
    connect(this, &DSGConfigServer::releaseResource, this, &DSGConfigServer::onReleaseResource);
    connect(m_refManager, &RefManager::releaseResource, this, &DSGConfigServer::releaseResource);
    connect(this, &DSGConfigServer::tryExit, this, &DSGConfigServer::onTryExit);
}

DSGConfigServer::~DSGConfigServer()
{
    exit();
}

void DSGConfigServer::exit()
{
    qInfo() << "dconfig server exit and release resources.";
    qDeleteAll(m_resources);
    m_resources.clear();
    m_refManager->destroy();
}

/*
    \breaf 注册服务到dbus上
    \return 是否注册成功
*/
bool DSGConfigServer::registerService()
{
    (void) new DSGConfigAdaptor(this);

    QDBusConnection bus = QDBusConnection::systemBus();
    if (!bus.registerService(DSG_CONFIG)) {
        QString errorMsg = bus.lastError().message();
        if (errorMsg.isEmpty())
            errorMsg = "maybe it's running";

        qWarning() << QString("Can't register the %1 service, %2.").arg(DSG_CONFIG).arg(errorMsg);
        return false;
    }
    if (!bus.registerObject("/", this)) {
        qWarning() << QString("Can't register to the D-Bus object.");
        return false;
    }
    return true;
}

/*!
 \brief 获得指定连接key值的连接对象
 \a key 连接对象的唯一ID
 \return
 */
DSGConfigResource *DSGConfigServer::resourceObject(const ConnKey &key)
{
    return m_resources.value(key, nullptr);
}

/*!
 \brief 设置延迟释放的时间
 \a ms 延迟释放时间,单位为毫秒
 */
void DSGConfigServer::setDelayReleaseTime(const int ms)
{
    m_refManager->setDelayReleaseTime(ms);
}

int DSGConfigServer::delayReleaseTime() const
{
    return m_refManager->delayReleaseTime();
}

void DSGConfigServer::setLocalPrefix(const QString &localPrefix)
{
    m_localPrefix = localPrefix;
}

void DSGConfigServer::setEnableExit(const bool enable)
{
    m_enableExit = enable;
}

int DSGConfigServer::resourceSize() const
{
    return m_resources.size();
}

/*!
 \brief 响应请求配置文件管理连接
 \a 应用程序的唯一ID
 \a 配置文件名
 \a 配置文件子目录
 \return
 */
QDBusObjectPath DSGConfigServer::acquireManager(const QString &appid, const QString &name, const QString &subpath)
{
    const auto &service = calledFromDBus() ? message().service() : "test.service";
    const uint &uid = calledFromDBus() ? connection().interface()->serviceUid(service).value() : 0U;
    qCDebug(cfLog, "acquireManager service:%s, uid:%d", qPrintable(service), uid);
    QString path = validDBusObjectPath(QString("/%1/%2%3").arg(appid, name, subpath));
    auto resource = resourceObject(path);
    if (!resource) {
        resource = new DSGConfigResource(path, m_localPrefix);
        bool loadStatus = resource->load(appid, name, subpath);
        if (loadStatus) {
            m_resources.insert(path, resource);
            QObject::connect(resource, &DSGConfigResource::releaseConn, this, &DSGConfigServer::onReleaseChanged);
            qInfo() << "created resource:" << path;
        } else {
            //error
            QString errorMsg = QString("Can't load resource. expecting path:%1.").arg(path);
            if (calledFromDBus())
                sendErrorReply(QDBusError::Failed, errorMsg);

            qInfo() << errorMsg;
            resource->deleteLater();
            return QDBusObjectPath();
        }
    }

    auto conn = resource->connObject(uid);
    if (!conn) {
        conn = resource->createConn(uid);
        if (!conn) {
            QString errorMsg = QString("Can't register Connection object for the acquire. expecting path:%1.").arg(path);
            if (calledFromDBus())
                sendErrorReply(QDBusError::Failed, errorMsg);

            qInfo() << errorMsg;
            return QDBusObjectPath();
        }
        qCInfo(cfLog, "created connection:%s", qPrintable(conn->key()));
    } else {
        qCInfo(cfLog, "reuse connection:%s", qPrintable(conn->key()));
    }
    addConnWatchedService(service);
    m_refManager->refResource(service, conn->key());

    return QDBusObjectPath(conn->key());
}

/*!
 \brief 释放此连接服务使用的指定资源引用
 当一个服务引用了多个资源时,此方法只会释放指定资源的引用,不会影响此服务的其它资源的引用情况.
 \a service 服务名称,关联特定进程
 */
void DSGConfigServer::onReleaseChanged(const ConnServiceName &service, const ConnKey &connKey)
{
    m_refManager->derefResource(service, connKey);

    const auto remainingCount = m_refManager->getRefResourceCountOnTheSR(service, connKey);
    qCInfo(cfLog, "reduced connection refrence service. service:%s, path:%s, remaining refrence %d", qPrintable(service), qPrintable(connKey), remainingCount);
}

/*!
 \brief 释放此连接的所有资源
 \a resource 连接名称,一个配置资源的ID
 */
void DSGConfigServer::onReleaseResource(const ConnKey &connKey)
{
    const ResourceKey resourceKey = getResourceKey(connKey);
    if (m_resources.contains(resourceKey)) {
        auto resource = m_resources.value(resourceKey);
        qCInfo(cfLog, "remove connection:%s", qPrintable(connKey));
        resource->removeConn(connKey);
        if (resource->isEmptyConn()) {
            qCInfo(cfLog, "remove resource:%s", qPrintable(resourceKey));
            m_resources.remove(resourceKey);
            resource->save();
            resource->deleteLater();

            if (m_enableExit) {
                Q_EMIT tryExit();
            }
        }
    }
}

void DSGConfigServer::onTryExit()
{
    const int count = resourceSize();
    qCDebug(cfLog()) << "try exit application, resource size:" << count;

    if (count <= 0) {
        qCInfo(cfLog()) << "exit application because of not exist resource.";
        exit();
        qApp->quit();
    }
}

ConfigureId DSGConfigServer::getConfigureIdByPath(const QString &path)
{
    QFileInfo fileInfo(path);
    if (!fileInfo.exists()) {
        return ConfigureId();
    }

    const auto &absolutePath = fileInfo.absoluteFilePath();

    auto res = getMetaConfigureId(absolutePath);
    if (res.isInValid()) {
        if (res.isInValid()) {
            res = getOverrideConfigureId(absolutePath);
        }
    }
    return res;
}

bool DSGConfigServer::filterRequestPath(DSGConfigResource *resource, const ConfigureId &configureInfo) const
{
    const QString fileName = validDBusObjectPath(configureInfo.resource);
    const QString &rfileName = validDBusObjectPath(resource->getName());
    if (rfileName != fileName)
        return true;

    if (!configureInfo.appid.isEmpty()) {
        const QString &appid = validDBusObjectPath(configureInfo.appid);
        const QString &rappid = validDBusObjectPath(resource->getAppid());
        if (appid != rappid)
            return true;
    }

    return false;
}

QString DSGConfigServer::validDBusObjectPath(const QString &path)
{
    QString tmp = path;
    return tmp.replace('.', '_').replace('-', '_');
}

/*!
 \brief 文件刷新，
 当描述文件被修改或override目录新增、移除、修改文件时，需要重新解析对应的文件内容，
 提供刷新服务，由配置工具调用来运行时刷新提供的文件访问信息。
 */
void DSGConfigServer::update(const QString &path)
{
    qInfo() << "update resource:" << path;

    const auto &configureInfo = getConfigureIdByPath(path);
    if (configureInfo.isInValid()) {
        QString errorMsg = QString("it's illegal resource [%1].").arg(path);
        if (calledFromDBus()) {
            sendErrorReply(QDBusError::Failed, errorMsg);
        }
        qWarning() << errorMsg;
        return;
    }

    qInfo(cfLog()) << QString("update the configuration: appid:[%1], subpath:[%2], configurationid:[%3].").arg(configureInfo.appid).arg(configureInfo.subpath).arg(configureInfo.resource);
    for (auto resource : m_resources) {
        if (filterRequestPath(resource, configureInfo))
            continue;

        const QString &resourceKey = resource->path();
        qInfo() << QString("updated the object path[%1]").arg(resourceKey);
        if (!resource->reparse()) {
            QString errorMsg = QString("reparse the object path[%1] error.").arg(resourceKey);
            if (calledFromDBus()) {
                sendErrorReply(QDBusError::Failed, errorMsg);
            }
            qWarning() << errorMsg;
        }
    }
}

void DSGConfigServer::sync(const QString &path)
{
    qInfo() << "sync resource:" << path;

    const auto &configureInfo = getConfigureIdByPath(path);
    if (configureInfo.isInValid()) {
        QString errorMsg = QString("it's illegal resource [%1].").arg(path);
        if (calledFromDBus()) {
            sendErrorReply(QDBusError::Failed, errorMsg);
        }
        qWarning() << errorMsg;
        return;
    }

    qInfo(cfLog()) << QString("sync the configuration: appid:[%1], subpath:[%2], configurationid:[%3].").arg(configureInfo.appid).arg(configureInfo.subpath).arg(configureInfo.resource);
    for (auto resource : m_resources) {
        if (filterRequestPath(resource, configureInfo))
            continue;

        qInfo(cfLog()) << QString("sync the object path[%1]").arg(resource->path());
        resource->save();
    }
}

/*!
 \brief 添加连接服务监控
 * 当服务退出时,会清空此服务的所有引用资源,即使服务异常退出,DBus也可以检测到.
 \a service 服务名称
 */
void DSGConfigServer::addConnWatchedService(const ConnServiceName & service)
{
    if (!calledFromDBus()) {
        return;
    }
    if (!m_watcher) {
        m_watcher = new QDBusServiceWatcher(this);
        m_watcher->setConnection(connection());
        m_watcher->setWatchMode(QDBusServiceWatcher::WatchForUnregistration);
        connect(m_watcher, &QDBusServiceWatcher::serviceUnregistered, [this](const QString &service){

            qCInfo(cfLog, "remove watchered service:%s", qPrintable(service));
            m_watcher->removeWatchedService(service);
            m_refManager->releaseService(service);
        });
    }
    if (!m_watcher->watchedServices().contains(service)) {
        qCInfo(cfLog, "add watchered service:%s, appid:%s, user:%s.",
                qPrintable(service),
                qPrintable(getProcessNameByPid(connection().interface()->servicePid(service).value())),
                qPrintable(getUserNameByUid(connection().interface()->serviceUid(service).value())));
        m_watcher->addWatchedService(service);
    }
}
