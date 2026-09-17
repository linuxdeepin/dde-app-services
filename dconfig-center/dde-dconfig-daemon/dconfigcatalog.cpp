// SPDX-FileCopyrightText: 2026 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "dconfigcatalog.h"

#include <DConfigFile>
#include <QDirIterator>
#include <QFileInfo>
#include <QSet>

namespace {
QString subpathOf(const QString &baseDir, const QString &dir)
{
    const QString relativePath = QDir(baseDir).relativeFilePath(dir);
    return relativePath == QLatin1String(".") || relativePath.isEmpty()
            ? QString() : QLatin1Char('/') + relativePath;
}

QStringList resourcesForSubpath(const QString &baseDir, const QString &subpath)
{
    QStringList result;
    QDir dir(baseDir);
    if (!subpath.isEmpty())
        dir.cd(subpath.mid(1));

    do {
        const auto entries = dir.entryInfoList({QLatin1String("*.json")},
                                                QDir::Files | QDir::Readable);
        for (const QFileInfo &entry : entries) {
            const QString resource = entry.fileName().chopped(QStringLiteral(".json").size());
            if (!result.contains(resource))
                result.append(resource);
        }
    } while (dir != QDir(baseDir) && dir.cdUp());

    return result;
}

void appendConfigurations(ConfigInfoList &result, QSet<QString> &uniqueKeys,
                          const QString &appId, const QString &baseDir,
                          const QString &localPrefix)
{
    QStringList dirs;
    dirs.append(baseDir);

    if (appId != NoAppId) {
        QDirIterator dirIterator(baseDir, QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable,
                                 QDirIterator::Subdirectories);
        while (dirIterator.hasNext())
            dirs.append(dirIterator.next());
    }

    for (const QString &dir : dirs) {
        const QString subpath = subpathOf(baseDir, dir);
        const QStringList resources = resourcesForSubpath(baseDir, subpath);
        for (const QString &resource : resources) {
            Dtk::Core::DConfigFile file(appId, resource, subpath);
            if (!file.load(localPrefix))
                continue;

            const QString key = QStringLiteral("%1/%2/%3").arg(appId, resource, subpath);
            if (!uniqueKeys.contains(key)) {
                uniqueKeys.insert(key);
                result.append({appId, resource, subpath});
            }
        }
    }
}
}

DSGConfigCatalog::DSGConfigCatalog(QObject *parent)
    : QObject(parent)
{
    ConfigInfo::registerMetaType();
}

void DSGConfigCatalog::setLocalPrefix(const QString &localPrefix)
{
    if (m_localPrefix == localPrefix)
        return;

    m_localPrefix = localPrefix;
    Q_EMIT configurationsChanged();
}

ConfigInfoList DSGConfigCatalog::configurations() const
{
    ConfigInfoList result;
    QSet<QString> uniqueKeys;

    for (const QString &dir : Dtk::Core::DConfigMeta::genericMetaDirs(m_localPrefix)) {
        appendConfigurations(result, uniqueKeys, NoAppId, dir, m_localPrefix);

        const auto appDirs = QDir(dir).entryList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable);
        for (const QString &appDir : appDirs) {
            if (appDir == QLatin1String("overrides"))
                continue;

            appendConfigurations(result, uniqueKeys, appDir, QDir(dir).filePath(appDir),
                                  m_localPrefix);
        }
    }

    return result;
}
