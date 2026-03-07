// SPDX-FileCopyrightText: 2026 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QString>
#include <QHash>

/**
 * @brief ConnKey 类型安全的连接标识
 *
 * 替代原有 QString 拼接方案，避免手动解析 "/appid/resource/subpath/uid" 格式。
 *
 * 格式：/$appid/$resource$subpath/$uid
 * 示例：/dconfig-example/example//1000
 */
struct ConnKey {
    QString appid;
    QString resource;   ///< 配置描述文件名（不含 .json）
    QString subpath;    ///< 子路径（可为空）
    uint    uid = 0;

    /// 转为 D-Bus path 兼容字符串，与旧版 ConnKey(QString) 格式一致
    QString toString() const
    {
        return QString("/%1/%2%3/%4")
            .arg(appid, resource, subpath.isEmpty() ? QString() : "/" + subpath)
            .arg(uid);
    }

    /// 从旧格式字符串还原（"/appid/resource[/subpath]/uid"）
    static ConnKey fromString(const QString &s)
    {
        ConnKey k;
        QStringList parts = s.split('/', Qt::SkipEmptyParts);
        if (parts.size() < 3)
            return k;

        k.appid    = parts.at(0);
        k.resource = parts.at(1);

        // 最后一段是 uid
        bool ok = false;
        uint uid = parts.last().toUInt(&ok);
        if (ok) {
            k.uid = uid;
            // 中间的是 subpath（可为空）
            if (parts.size() > 3) {
                QStringList sub = parts.mid(2, parts.size() - 3);
                k.subpath = sub.join('/');
            }
        }
        return k;
    }

    bool isValid() const { return !appid.isEmpty() && !resource.isEmpty(); }

    bool operator==(const ConnKey &o) const
    {
        return appid == o.appid && resource == o.resource
            && subpath == o.subpath && uid == o.uid;
    }

    bool operator!=(const ConnKey &o) const { return !(*this == o); }
};

inline uint qHash(const ConnKey &key, uint seed = 0)
{
    return qHash(key.toString(), seed);
}

Q_DECLARE_METATYPE(ConnKey)
