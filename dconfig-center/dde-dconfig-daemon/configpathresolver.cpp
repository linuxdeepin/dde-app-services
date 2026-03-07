// SPDX-FileCopyrightText: 2026 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "configpathresolver.h"

#include <algorithm>
#include <QDir>

ConfigPathResolver &ConfigPathResolver::instance()
{
    static ConfigPathResolver s;
    return s;
}

void ConfigPathResolver::setLocalPrefix(const QString &prefix)
{
    m_localPrefix = prefix;
}

QString ConfigPathResolver::localPrefix() const
{
    return m_localPrefix;
}

void ConfigPathResolver::addSearchPath(const QString &path, int priority)
{
    const QString abs = m_localPrefix + path;
    // 避免重复
    for (const auto &p : m_paths) {
        if (p.second == abs) return;
    }
    m_paths.append({priority, abs});
    // 按 priority 降序排列
    std::sort(m_paths.begin(), m_paths.end(), [](const auto &a, const auto &b) {
        return a.first > b.first;
    });
}

void ConfigPathResolver::clearSearchPaths()
{
    m_paths.clear();
}

QStringList ConfigPathResolver::searchPaths() const
{
    QStringList result;
    for (const auto &p : m_paths)
        result << p.second;
    return result;
}

QStringList ConfigPathResolver::metaPaths(const QString &appid, const QString &resource) const
{
    QStringList result;
    for (const auto &p : m_paths) {
        // meta 文件路径：<base>/<appid>/<resource>.json 或 <base>/<resource>.json
        result << QString("%1/%2/%3.json").arg(p.second, appid, resource);
        result << QString("%1/%2.json").arg(p.second, resource);
    }
    return result;
}

QStringList ConfigPathResolver::overridePaths(const QString &appid, const QString &resource) const
{
    QStringList result;
    for (const auto &p : m_paths) {
        result << QString("%1/overrides/%2/%3.json").arg(p.second, appid, resource);
        result << QString("%1/overrides/%2.json").arg(p.second, resource);
    }
    // 系统管理员覆盖目录
    if (!m_localPrefix.isEmpty() || true) {
        result << QString("%1/etc/dsg/configs/overrides/%2/%3.json")
                      .arg(m_localPrefix, appid, resource);
    }
    return result;
}

QStringList ConfigPathResolver::allWatchedDirs() const
{
    QStringList dirs;
    for (const auto &p : m_paths) {
        if (QDir(p.second).exists())
            dirs << p.second;
    }
    return dirs;
}
