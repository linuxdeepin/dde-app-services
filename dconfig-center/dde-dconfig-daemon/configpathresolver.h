// SPDX-FileCopyrightText: 2026 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QStringList>
#include <QList>
#include <QPair>
#include <QString>

/**
 * @brief ConfigPathResolver — 配置路径管理中心
 *
 * 统一管理配置文件搜索路径，替代原有各处硬编码路径字符串。
 *
 * 优先级数字越大表示越高优先（覆盖层排前面）：
 *   - /etc/dsg/configs            → 200（系统管理员覆盖）
 *   - /usr/share/dsg/configs      → 100（发行包 meta）
 *   - /var/lib/linglong/.../dsg   →  50（linglong 容器）
 *
 * 使用：
 *   auto &r = ConfigPathResolver::instance();
 *   r.setLocalPrefix("/some/prefix");
 *   r.addSearchPath("/usr/share/dsg/configs", 100);
 *   QStringList paths = r.metaPaths("org.deepin.demo", "example");
 */
class ConfigPathResolver
{
public:
    static ConfigPathResolver &instance();

    void setLocalPrefix(const QString &prefix);
    QString localPrefix() const;

    /// 注册一个搜索目录，priority 越大越靠前
    void addSearchPath(const QString &path, int priority = 0);
    void clearSearchPaths();

    /// 返回按优先级排序（高→低）的基础搜索路径列表
    QStringList searchPaths() const;

    /// meta 配置文件路径列表（priority 高→低）
    QStringList metaPaths(const QString &appid, const QString &resource) const;

    /// 覆盖配置文件路径列表（priority 高→低）
    QStringList overridePaths(const QString &appid, const QString &resource) const;

    /// 所有配置目录（用于 inotify 监听）
    QStringList allWatchedDirs() const;

private:
    ConfigPathResolver() = default;

    QString m_localPrefix;
    // (priority, absolutePath)
    QList<QPair<int, QString>> m_paths;
};
