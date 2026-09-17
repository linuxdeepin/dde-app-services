// SPDX-FileCopyrightText: 2026 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "resourcecatalogclient.h"
#include "helper.hpp"

#include "configmanager_internal_interface.h"

#include <QDBusConnection>
#include <QDBusPendingReply>
#include <QLoggingCategory>
#include <QSet>

namespace {
const char kServiceName[] = "org.desktopspec.ConfigManager";
const char kObjectPath[] = "/Internal";
}

ResourceCatalogClient &ResourceCatalogClient::instance()
{
    // Keep the QObject alive until process exit. A static value would be
    // destroyed after QCoreApplication and may access an invalid DBus connection.
    static ResourceCatalogClient *client = new ResourceCatalogClient();
    return *client;
}

ResourceCatalogClient::ResourceCatalogClient(QObject *parent)
    : QObject(parent)
    , m_interface(new DSGConfigInternal(kServiceName, kObjectPath,
                                        QDBusConnection::systemBus(), this))
{
    ConfigInfo::registerMetaType();

    connect(m_interface, &DSGConfigInternal::configurationsChanged,
            this, &ResourceCatalogClient::refresh);
}

ConfigInfoList ResourceCatalogClient::localConfigurations() const
{
    ConfigInfoList result;
    for (const QString &app : ::applications()) {
        const QStringList resources = ::resourcesForApp(app);
        for (const QString &resource : resources) {
            result.append({app, resource, QString()});
            const QStringList subpaths = ::subpathsForResource(app, resource);
            for (const QString &subpath : subpaths)
                result.append({app, resource, subpath});
        }
    }
    return result;
}

void ResourceCatalogClient::refresh()
{
    m_configurations.clear();
    m_configurationsValid = false;
}

QStringList ResourceCatalogClient::applications()
{
    QStringList result;
    result << NoAppId;

    QSet<QString> uniqueApps;
    const auto configs = configurations();
    for (const auto &config : configs) {
        if (!config.appId.isEmpty())
            uniqueApps.insert(config.appId);
    }
    result.append(uniqueApps.values());
    return result;
}

ConfigInfoList ResourceCatalogClient::configurations()
{
    if (m_configurationsValid)
        return m_configurations;

    auto reply = m_interface->configurations();
    reply.waitForFinished();
    if (reply.isError()) {
        qWarning() << "Failed to query configurations from dde-dconfig-daemon:"
                   << reply.error().message() << ", fallback to local directories.";
        return localConfigurations();
    }

    m_configurations = reply.value();
    m_configurationsValid = true;
    return m_configurations;
}

QStringList ResourceCatalogClient::resourcesForApp(const QString &appid)
{
    QStringList result;
    const auto configs = configurations();
    for (const auto &config : configs) {
        if (config.appId == appid && !result.contains(config.resourceId))
            result.append(config.resourceId);
    }
    return result;
}

QStringList ResourceCatalogClient::resourcesForAllApp()
{
    QStringList result;
    const auto configs = configurations();
    for (const auto &config : configs) {
        if (config.appId.isEmpty() && !result.contains(config.resourceId))
            result.append(config.resourceId);
    }
    return result;
}

QStringList ResourceCatalogClient::subpathsForResource(const QString &appid, const QString &resourceId)
{
    QStringList result;
    const auto configs = configurations();
    for (const auto &config : configs) {
        // The root configuration is displayed as the resource item itself.
        // Only non-empty subpath directories are displayed as child items.
        if (config.appId == appid && config.resourceId == resourceId
                && !config.subpath.isEmpty() && !result.contains(config.subpath)) {
            result.append(config.subpath);
        }
    }
    return result;
}

QStringList ResourceCatalogClient::availableResourcesForApp(const QString &appid)
{
    QStringList result;
    if (!appid.isEmpty())
        result.append(resourcesForApp(appid));
    result.append(resourcesForAllApp());

    QSet<QString> unique;
    for (const QString &resource : result)
        unique.insert(resource);
    return unique.values();
}

bool ResourceCatalogClient::existAppid(const QString &appid)
{
    return !resourcesForApp(appid).isEmpty();
}

bool ResourceCatalogClient::existResource(const QString &appid, const QString &resourceId)
{
    return resourcesForApp(appid).contains(resourceId)
            || resourcesForAllApp().contains(resourceId);
}
