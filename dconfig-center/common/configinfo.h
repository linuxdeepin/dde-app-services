// SPDX-FileCopyrightText: 2026 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QDBusArgument>
#include <QDBusMetaType>
#include <QList>
#include <QMetaType>
#include <QString>

class ConfigInfo
{
public:
    static void registerMetaType();

    QString appId;
    QString resourceId;
    QString subpath;

    bool operator==(const ConfigInfo &other) const
    {
        return appId == other.appId && resourceId == other.resourceId && subpath == other.subpath;
    }
};

using ConfigInfoList = QList<ConfigInfo>;

Q_DECLARE_METATYPE(ConfigInfo)
Q_DECLARE_METATYPE(ConfigInfoList)

inline void ConfigInfo::registerMetaType()
{
    static bool registered = false;
    if (registered)
        return;

    registered = true;
    qRegisterMetaType<ConfigInfo>("ConfigInfo");
    qDBusRegisterMetaType<ConfigInfo>();
    qRegisterMetaType<ConfigInfoList>("ConfigInfoList");
    qDBusRegisterMetaType<ConfigInfoList>();
}

inline QDBusArgument &operator<<(QDBusArgument &argument, const ConfigInfo &info)
{
    argument.beginStructure();
    argument << info.appId << info.resourceId << info.subpath;
    argument.endStructure();
    return argument;
}

inline const QDBusArgument &operator>>(const QDBusArgument &argument, ConfigInfo &info)
{
    argument.beginStructure();
    argument >> info.appId >> info.resourceId >> info.subpath;
    argument.endStructure();
    return argument;
}
