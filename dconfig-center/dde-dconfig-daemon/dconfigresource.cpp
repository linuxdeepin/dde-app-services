// SPDX-FileCopyrightText: 2021 - 2022 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "dconfigresource.h"
#include "dconfigconn.h"
#include "dconfigfile.h"
#include <QDBusMessage>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QFile>
#include <QDebug>

#include "manager_adaptor.h"

DCORE_USE_NAMESPACE

DSGConfigResource::DSGConfigResource(const ResourceKey &path, const ResourceKey &localPrefix, QObject *parent)
    : QObject (parent),
      m_path(path),
      m_localPrefix(localPrefix),
      m_config(nullptr)
{
}

DSGConfigResource::~DSGConfigResource()
{
    for (auto item : m_conns) {
        item->cache()->save(m_localPrefix);
    }
    qDeleteAll(m_conns);
    if (m_config) {
        m_config->save(m_localPrefix);
        m_config.reset();
    }
}

bool DSGConfigResource::load(const QString &appid, const QString &name, const QString &subpath)
{
    if (Q_UNLIKELY(!m_config)) {
        m_config.reset(new DTK_CORE_NAMESPACE::DConfigFile(appid, name, subpath));
        m_appid = appid;
        m_fileName = name;
        m_subpath = subpath;
    }
    return m_config->load(m_localPrefix);
}

DSGConfigConn *DSGConfigResource::connObject(const uint uid)
{
    return m_conns.value(getConnKey(uid), nullptr);
}

DSGConfigConn *DSGConfigResource::createConn(const uint uid)
{
    QString key = getConnKey(uid);
    QScopedPointer<DSGConfigConn> connPointer(new DSGConfigConn(key));
    QScopedPointer<DConfigCache> cache(m_config->createUserCache(uid));
    if (!cache) {
        qWarning() << QString("Create cache service error for [%1]'s [%2].").arg(uid).arg(m_path);
        return nullptr;
    }
    if (!cache->load()) {
        qWarning() << QString("Load cache error for [%1]'s [%2].").arg(uid).arg(m_path);
        return nullptr;
    }
    if (qgetenv("DSG_CONFIG_CONNECTION_DISABLE_DBUS").isEmpty()) {
        (void) new DSGConfigManagerAdaptor(connPointer.get());
        QDBusConnection bus = QDBusConnection::systemBus();
        bus.unregisterObject(key);
        if (!bus.registerObject(key, connPointer.get())) {
            qWarning() << QString("Can't register the object %1.").arg(key);
            return nullptr;
            //error.
        }
    }
    connPointer->setConfigCache(cache.take());
    connPointer->setConfigFile(m_config.get());

    auto conn = connPointer.take();
    m_conns.insert(key, conn);
    QObject::connect(conn, &DSGConfigConn::releaseChanged, this, &DSGConfigResource::onReleaseChanged);
    QObject::connect(this, &DSGConfigResource::globalValueChanged, this, &DSGConfigResource::onGlobalValueChanged);
    QObject::connect(conn, &DSGConfigConn::globalValueChanged, this, &DSGConfigResource::onGlobalValueChanged);
    QObject::connect(this, &DSGConfigResource::updateValueChanged, conn, &DSGConfigConn::valueChanged);
    return conn;
}

/*!
 \brief 重新解析文件
 \return 返回重新解析状态
 */
bool DSGConfigResource::reparse()
{
    QScopedPointer<DConfigFile> config(new DConfigFile(*m_config));
    auto newMeta = config->meta();
    if (!newMeta->load()) {
        qWarning() << QString("Reparse resource error for [%1].").arg(m_path);
        return false;
    }

    QMap<DConfigCache*, QList<QString>> cacheChangedValues;
    DConfigMeta *oldMeta = m_config->meta();
    QList<DConfigCache*> caches;
    for (auto iter = m_conns.begin(); iter != m_conns.end(); iter++) {
        caches.push_back(iter.value()->cache());
    }
    caches.push_back(config->globalCache());

    // cache and valuechanged.
    for (auto cache : caches) {
        QList<QString> changedValues;
        for (auto key : oldMeta->keyList()) {
            if (oldMeta->flags(key).testFlag(DConfigFile::Global) ^ cache->isGlobal())
                continue;

            if (m_config->value(key, cache) == config->value(key, cache)) {
                continue;
            }

            changedValues.push_back(key);
        }
        if (!changedValues.isEmpty()) {
            cacheChangedValues[cache] = changedValues;
        }
        repareCache(cache, oldMeta, newMeta);
    }

    // config refresh.
    m_config.reset(config.take());
    for (auto conn : m_conns) {
        conn->setConfigFile(m_config.get());
    }

    // emit valuechanged.
    for (auto iter = cacheChangedValues.begin(); iter != cacheChangedValues.end(); ++iter) {
        if (iter.key()->isGlobal()) {
            for (const QString &key : iter.value()) {
                emit globalValueChanged(key);
            }
        } else {
            if (auto conn = m_conns.value(getConnKey(iter.key()->uid()))) {
                for (const QString &key : iter.value()) {
                    emit conn->valueChanged(key);
                }
            } else {
                qWarning() << "Invalid connection:" << getConnKey(iter.key()->uid());
            }
        }
    }
    qDebug() << "Those key's value changed:" << cacheChangedValues;

    return true;
}

void DSGConfigResource::onReleaseChanged(const ConnServiceName &service)
{
    auto conn = qobject_cast<DSGConfigConn*>(sender());
    if (conn) {
        emit releaseConn(service, conn->key());
    }
}

QString DSGConfigResource::getConnKey(const uint uid) const
{
    return QString("%1/%2").arg(m_path).arg(uid);
}

/*
  \internal

    \breaf 重新解析缓存对象
*/
void DSGConfigResource::repareCache(DConfigCache *cache, DConfigMeta *oldMeta, DConfigMeta *newMeta)
{
    const auto &newKeyList = newMeta->keyList().toSet();
    const auto &oldKeyList = oldMeta->keyList().toSet();

    // 配置项已经被移除，oldMeta - newMeta，移除cache值
    const auto subtractKeys = oldKeyList - (newKeyList);
    for (const auto &key :subtractKeys) {
        cache->remove(key);
        qDebug() << QString("Cache removed because of meta item removed. "
                            "path:%1,uid:%2,key:%3.").arg(m_path).arg(cache->uid()).arg(key);
    }
    // 权限变化，ReadWrite -> ReadOnly，移除cache值
    auto intersectKeys = newKeyList;
    intersectKeys = intersectKeys.intersect(oldKeyList);
    for(const auto &key :intersectKeys) {
        if (newMeta->permissions(key) == DConfigFile::ReadOnly &&
                oldMeta->permissions(key) == DConfigFile::ReadWrite) {
            cache->remove(key);
            qDebug() << QString("Cache removed because of permissions changed from readwrite to readonly. "
                                "path:%1,uid:%2,key:%3.").arg(m_path).arg(cache->uid()).arg(key);
        }
    }
}

QString DSGConfigResource::path() const
{
    return m_path;
}

QString DSGConfigResource::getName() const
{
    const QStringList &sps = m_path.split('/');
    return sps[2];
}

QString DSGConfigResource::getAppid() const
{
    const QStringList &sps = m_path.split('/');
    return sps[1];
}

void DSGConfigResource::removeConn(const ConnKey &connKey)
{
    if (auto conn = m_conns.value(connKey, nullptr)) {
        conn->cache()->save(m_localPrefix);
        conn->deleteLater();
        m_conns.remove(connKey);
        qDebug() << QString("removed connection:%1, remaining %2 connection.").arg(connKey).arg(m_conns.count());
    }
}

bool DSGConfigResource::isEmptyConn() const
{
    return m_conns.count() <= 0;
}

void DSGConfigResource::save()
{
    if (m_config) {
        m_config->save(m_localPrefix);
        for (auto conn : m_conns) {
            conn->cache()->save(m_localPrefix);
        }
    }
}

void DSGConfigResource::onGlobalValueChanged(const QString &key)
{
    for (auto conn : m_conns) {
        emit conn->valueChanged(key);
    }
}
