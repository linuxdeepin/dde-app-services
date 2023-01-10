// SPDX-FileCopyrightText: 2023 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "dgeneralconfigmanager.h"
#include "dconfigfile.h"
#include <QDebug>

GeneralConfig::~GeneralConfig()
{
    m_caches.clear();
}

QSharedPointer<DConfigCache> GeneralConfig::cache(const uint uid) const
{
    return m_caches.value(uid);
}

bool GeneralConfig::contains(const uint id) const
{
    return m_caches.contains(id);
}

void GeneralConfig::removeCache(const uint uid)
{
    const auto iter = m_caches.find(uid);
    if (iter != m_caches.end()) {
        m_caches.erase(iter);
    }
}

void GeneralConfig::addCache(const uint uid, QSharedPointer<DConfigCache> cache)
{
    Q_ASSERT(cache);
    m_caches[uid] = cache;
}

void GeneralConfig::setConfig(QSharedPointer<DConfigFile> config)
{
    m_config = config;
}

QSharedPointer<DConfigFile> GeneralConfig::config() const
{
    return m_config;
}

DSGGeneralConfigManager::DSGGeneralConfigManager(QObject *parent)
    : QObject (parent)
{

}

DSGGeneralConfigManager::~DSGGeneralConfigManager()
{
    for (auto item : m_generalConfigs) {
        delete item;
    }
    m_generalConfigs.clear();
}

GeneralConfig *DSGGeneralConfigManager::config(const GeneralConfigFileKey &key) const
{
    return m_generalConfigs.value(key);
}

bool DSGGeneralConfigManager::contains(const GeneralConfigFileKey &key) const
{
    return m_generalConfigs.contains(key);
}

GeneralConfig *DSGGeneralConfigManager::createConfig(const GeneralConfigFileKey &key)
{
    auto iter = m_generalConfigs.find(key);
    if (iter != m_generalConfigs.end())
        return *iter;

    auto config = new GeneralConfig();
    m_generalConfigs[key] = config;
    return config;
}

void DSGGeneralConfigManager::removeConfig(const GeneralConfigFileKey &key)
{
    const auto iter = m_generalConfigs.find(key);
    if (iter != m_generalConfigs.end()) {
        delete iter.value();
        m_generalConfigs.erase(iter);
    }
}
