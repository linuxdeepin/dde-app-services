// SPDX-FileCopyrightText: 2026 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "configinfo.h"

#include <QObject>
#include <QStringList>

class DSGConfigInternal;

class ResourceCatalogClient : public QObject
{
    Q_OBJECT
public:
    static ResourceCatalogClient &instance();

    QStringList applications();
    ConfigInfoList configurations();
    void refresh();
    QStringList resourcesForApp(const QString &appid);
    QStringList resourcesForAllApp();
    QStringList subpathsForResource(const QString &appid, const QString &resourceId);

    QStringList availableResourcesForApp(const QString &appid);
    bool existAppid(const QString &appid);
    bool existResource(const QString &appid, const QString &resourceId);

private:
    explicit ResourceCatalogClient(QObject *parent = nullptr);
    ConfigInfoList localConfigurations() const;

    DSGConfigInternal *m_interface = nullptr;
    ConfigInfoList m_configurations;
    bool m_configurationsValid = false;
};
