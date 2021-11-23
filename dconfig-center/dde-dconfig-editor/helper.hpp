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
#include <QString>
#include <QSet>
#include <QList>
#include <QDir>
#include <QDirIterator>

#include <DStandardPaths>

using ResourceId = QString;
using AppId = QString;
using SubpathKey = QString;
using ResourceList = QList<ResourceId>;
using AppList = QSet<AppId>;
using SubpathList = QList<SubpathKey>;

static const QString &SUFFIX = QString(".json");
constexpr int ConfigUserRole = Qt::UserRole + 10;
enum ConfigType {
    InvalidType = 0x00,
    AppType = 0x10,
    ResourceType = 0x20,
    AppResourceType = ResourceType | 0x01,
    CommonResourceType = ResourceType | 0x02,
    SubpathType = 0x30,
    KeyType = 0x40,
};

static QString resourcePath(const QString &appid, const QString &localPrefix = QString())
{
    const auto &usrDir = QString("%1/usr/share/dsg/apps/%2/configs").arg(localPrefix, appid);
    if (QDir(usrDir).exists())
        return usrDir;

    const auto &optDir = QString("%1/opt/apps/%2/files/schemas/configs").arg(localPrefix, appid);
    if (QDir(optDir).exists())
        return optDir;

    return QString();
}

static AppList applications(const QString &localPrefix = QString())
{
    AppList result;
    result.reserve(50);

    QStringList appDirs = {QString("%1/opt/apps").arg(localPrefix),
                           QString("%1/usr/share/dsg/apps").arg(localPrefix)
                          };
    for (auto item : appDirs)
    {
        QDir appsDir(item);

        for (auto appid : appsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            result.insert(appid);
        }
    }
    return result;
}

static ResourceList resourcesForApp(const QString &appid, const QString &localPrefix = QString())
{
    QSet<ResourceId> result;
    const auto &resPath = resourcePath(appid, localPrefix);
    if (resPath.isEmpty()) {
        return result.toList();
    }
    QDir resourceDir(resPath);
    QDirIterator iterator(resourceDir);
    result.reserve(50);
    while(iterator.hasNext()) {
        iterator.next();
        const QFileInfo &file(iterator.fileInfo());
        if (file.isDir()) {
            continue;
        }

        if (!file.fileName().endsWith(SUFFIX))
            continue;

        ResourceId resourceName = file.fileName().chopped(SUFFIX.size());
        result.insert(resourceName);
    }
    return result.toList();
}

static ResourceList resourcesForAllApp(const QString &localPrefix = QString())
{
    DCORE_USE_NAMESPACE;
    QDir resourceDir(QString("%1/%2/configs").arg(localPrefix, DStandardPaths::path(DStandardPaths::DSG::DataDir)));
    QDirIterator iterator(resourceDir);
    QSet<ResourceId> result;
    result.reserve(50);
    while(iterator.hasNext()) {
        iterator.next();
        const QFileInfo &file(iterator.fileInfo());
        if (file.isDir()) {
            continue;
        }

        if (!file.fileName().endsWith(SUFFIX))
            continue;

        ResourceId resourceName = file.fileName().chopped(SUFFIX.size());
        result.insert(resourceName);
    }
    return result.toList();
}


static ResourceList subpathsForResource(const AppId &appid, const ResourceId &resourceId, const QString &localPrefix = QString())
{
    SubpathList result;
    const auto &resPath = resourcePath(appid, localPrefix);
    if (resPath.isEmpty()) {
        return result;
    }
    QDir resourceDir(resPath);
    auto filters = QDir::Dirs | QDir::NoDotAndDotDot;
    resourceDir.setFilter(filters);
    QDirIterator iterator(resourceDir, QDirIterator::Subdirectories);

    while(iterator.hasNext()) {
        iterator.next();
        const QFileInfo &file(iterator.fileInfo());
        if (QDir(file.absoluteFilePath()).exists(resourceId + SUFFIX)) {
            auto subpath = file.absoluteFilePath().replace(resourceDir.absolutePath(), "");
            result.append(subpath);
        }
    }
    return result;
}

static bool existAppid(const QString &appid, const QString &localPrefix = QString())
{
    return !resourcePath(appid, localPrefix).isEmpty();
}

static bool existResource(const AppId &appid, const ResourceId &resourceId, const QString &localPrefix = QString())
{
    if (!existAppid(appid, localPrefix))
        return false;

    const ResourceList &apps = resourcesForApp(appid, localPrefix);
    if (apps.contains(resourceId))
        return true;

    const ResourceList &commons = resourcesForAllApp(localPrefix);
    if (commons.contains(resourceId)) {
        return true;
    }

    return false;
}
