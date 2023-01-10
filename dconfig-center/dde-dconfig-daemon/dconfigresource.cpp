// SPDX-FileCopyrightText: 2021 - 2022 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "dconfigresource.h"
#include "dconfigconn.h"
#include "dconfigrefmanager.h"
#include "dconfigfile.h"
#include "dgeneralconfigmanager.h"
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
    m_appid = appid;
    m_fileName = name;
    m_subpath = subpath;

    bool isCreate = false;
    m_config = getOrCreateConfig(&isCreate);
    if (isCreate)
        return m_config->load(m_localPrefix);

    return true;
}

void DSGConfigResource::setSyncRequestCache(ConfigSyncRequestCache *cache)
{
    m_syncRequestCache = cache;
}

DSGConfigConn *DSGConfigResource::connObject(const uint uid) const
{
    return m_conns.value(getConnKey(uid), nullptr);
}

DSGConfigConn *DSGConfigResource::connObject(const ConnKey key) const
{
    return m_conns.value(key);
}

DSGConfigConn *DSGConfigResource::createConn(const uint uid)
{
    QString key = getConnKey(uid);
    QScopedPointer<DSGConfigConn> connPointer(new DSGConfigConn(key));
    bool isCreate = false;
    QSharedPointer<DConfigCache> cache(getOrCreateCache(uid, &isCreate));

    if (!cache) {
        qWarning() << QString("Create cache service error for [%1]'s [%2].").arg(uid).arg(m_path);
        return nullptr;
    }
    if (isCreate && !cache->load()) {
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
    connPointer->setConfigCache(cache);
    connPointer->setConfigFile(m_config.get());

    if (!setGeneralConfigForConn(connPointer.data()))
        return nullptr;

    auto conn = connPointer.take();
    m_conns.insert(key, conn);
    QObject::connect(conn, &DSGConfigConn::releaseChanged, this, &DSGConfigResource::onReleaseChanged);
    QObject::connect(this, &DSGConfigResource::globalValueChanged, this, &DSGConfigResource::onGlobalValueChanged);
    QObject::connect(conn, &DSGConfigConn::globalValueChanged, this, &DSGConfigResource::onGlobalValueChanged);
    QObject::connect(this, &DSGConfigResource::updateValueChanged, conn, &DSGConfigConn::valueChanged);
    QObject::connect(conn, &DSGConfigConn::valueChanged, this, &DSGConfigResource::onValueChanged);
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
        caches.push_back(iter.value()->cache().data());
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

void DSGConfigResource::doSyncConfigCache(const ConfigCacheKey &key)
{
    if (ConfigSyncRequestCache::isUserKey(key)) {
        if (auto conn = m_conns.value(ConfigSyncRequestCache::getUserKey(key))) {
            qCDebug(cfLog()) << "do sync conn cache for user cache, key:" << key;
            conn->cache()->save(m_localPrefix);
        }
    } else if (ConfigSyncRequestCache::isGlobalKey(key)) {
        if (ConfigSyncRequestCache::getGlobalKey(key) == m_path && m_config) {
            qCDebug(cfLog()) << "do sync conn cache for global cache, key:" << key;
            m_config->save(m_localPrefix);
        }
    } else {
        qCWarning(cfLog()) << "it's not exist config cache key" << key;
    }
}

bool DSGConfigResource::isGeneralResource() const
{
    return ::isGeneralResource(m_appid);
}

void DSGConfigResource::setGeneralConfigManager(DSGGeneralConfigManager *ma)
{
    m_generalConfigManager = ma;
}

bool DSGConfigResource::setGeneralConfigForConn(DSGConfigConn *conn)
{
    Q_ASSERT(conn);
    const uint uid = conn->uid();

    // 只有appResource才需要fallback到公共配置
    if (isGeneralResource()) {
        const auto generalKey = getGeneralConfigKey(m_path, true);
        // 当为generalResource时，需要记录公共配置到generalManager，供其他appResource使用
        auto config = m_generalConfigManager->config(generalKey);
        if (!config) {
            config = m_generalConfigManager->createConfig(generalKey);
            config->setConfig(m_config);
        }
        if (!config->contains(uid))
            config->addCache(uid, conn->cache());

        return true;
    }

    // 判断是否需要fallback到公共配置
    QSharedPointer<DConfigFile> file(new DConfigFile(EmptyAppId, m_fileName, m_subpath));
    const bool canFallbackToGeneral = !file->meta()->metaPath(m_localPrefix).isEmpty();
    if (!canFallbackToGeneral)
        return true;

    const auto generalKey = getGeneralConfigKey(m_path, false);
    auto config = m_generalConfigManager->config(generalKey);
    if (!config) {
        if (!file->load(m_localPrefix)) {
            qCWarning(cfLog()) << "Can't load general configuration:" << m_fileName;
            return false;
        }

        // 创建空appid配置
        config = m_generalConfigManager->createConfig(generalKey);
        config->setConfig(file);
    }

    auto cache = config->cache(uid);
    if (!cache) {
        cache = QSharedPointer<DConfigCache>(config->config()->createUserCache(uid));
        if (!cache->load(m_localPrefix)) {
            qCWarning(cfLog()) << "Can't load general configuration's cache for the resource:" << m_fileName;
            return false;
        }
        config->addCache(uid, cache);
    }
    // only appResource to set general config.
    conn->setGeneralConfigFile(config->config().data());
    conn->setGeneralConfigCache(cache.data());
    qCInfo(cfLog()) << "Set general configuration as value's fallback for the connection:" << conn->key();
    return true;
}

QSharedPointer<DConfigFile> DSGConfigResource::getOrCreateConfig(bool *isCreate) const
{
    if (isGeneralResource()) {
        // 当资源为公共资源时，若generalManager已经存在配置，则直接共用
        const auto generalKey = getGeneralConfigKey(m_path, true);
        if (auto config = m_generalConfigManager->config(generalKey)) {
            if (isCreate)
                *isCreate = false;
            return config->config();
        }
    }
    if (isCreate)
        *isCreate = true;
    return QSharedPointer<DConfigFile>(new DConfigFile(m_appid, m_fileName, m_subpath));
}

QSharedPointer<DConfigCache> DSGConfigResource::getOrCreateCache(const uint uid, bool *isCreate) const
{
    if (isGeneralResource()) {
        // 当资源为公共资源时，若generalManager已经存在缓存，则直接共用
        const auto generalKey = getGeneralConfigKey(m_path, true);
        if (auto config = m_generalConfigManager->config(generalKey)) {
            if (auto cache = config->cache(uid)) {
                if (isCreate)
                    *isCreate = false;
                return cache;
            }
        }
    }
    if (isCreate)
        *isCreate = true;
    return QSharedPointer<DConfigCache>(m_config->createUserCache(uid));
}

void DSGConfigResource::onValueChanged(const QString &key)
{
    if (auto conn = qobject_cast<DSGConfigConn*>(sender())) {
        do {
            // global field changed don't cause user field to save, bug `valueChanged` signal is emit.
            if (m_config && Q_UNLIKELY(m_config->meta()->flags(key).testFlag(DConfigFile::Global)))
                break;

            if (Q_UNLIKELY(!m_syncRequestCache))
                break;
            m_syncRequestCache->pushRequest(ConfigSyncRequestCache::userKey(conn->key()));
        } while (false);

        // to emit general's valueChanged if valueChanged is emited from general resource.
        if (isGeneralResource())
            Q_EMIT m_generalConfigManager->valueChanged(key, conn->key());
    }
}

QString DSGConfigResource::getConnKey(const uint uid) const
{
    return getConnectionKey(m_path, uid);
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

ResourceKey DSGConfigResource::path() const
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
        const auto generalKey = getGeneralConfigKey(m_path, isGeneralResource());
        if (auto config = m_generalConfigManager->config(generalKey)) {
            const auto uid = getConnectionKey(connKey);
            config->removeCache(uid);
        }
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
    if (m_syncRequestCache)
        m_syncRequestCache->pushRequest(ConfigSyncRequestCache::globalKey(m_path));
    // to emit general's valueChanged if valueChanged is emited from general resource.
    for (auto conn : m_conns) {
        emit conn->valueChanged(key);
    }
}
