// SPDX-FileCopyrightText: 2023 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "dconfig_global.h"
#include <dtkcore_global.h>
#include <DConfigFile>
#include <QObject>

DCORE_BEGIN_NAMESPACE
class DConfigFile;
class DConfigCache;
DCORE_END_NAMESPACE

DCORE_USE_NAMESPACE
/**
 * @brief The GeneralConfig class
 * 管理单个资源的公共配置信息
 * 与DSGConfigResource类似，含有一个DConfigFile及多个DConfigCache.
 */
class GeneralConfig
{
public:
    ~GeneralConfig();
    QSharedPointer<DConfigCache> cache(const uint uid) const;
    bool contains(const uint id) const;
    void removeCache(const uint uid);
    void addCache(const uint uid, QSharedPointer<DConfigCache> cache);
    void setConfig(QSharedPointer<DConfigFile> config);
    QSharedPointer<DConfigFile> config() const;
private:
    QSharedPointer<DConfigFile> m_config;
    QMap<uint, QSharedPointer<DConfigCache>> m_caches;
};

/**
 * @brief The DSGGeneralConfigManager class
 * 管理公共资源
 */
class DSGGeneralConfigManager : public QObject
{
    Q_OBJECT
public:
    DSGGeneralConfigManager(QObject *parent = nullptr);
    virtual ~DSGGeneralConfigManager() override;

    GeneralConfig *config(const GeneralConfigFileKey &key) const;
    bool contains(const GeneralConfigFileKey &key) const;
    GeneralConfig *createConfig(const GeneralConfigFileKey &key);
    void removeConfig(const GeneralConfigFileKey &key);

Q_SIGNALS:
    void valueChanged(const QString &key, const ConnKey &connKey);

private:
    QMap<GeneralConfigFileKey, GeneralConfig *> m_generalConfigs;
};
