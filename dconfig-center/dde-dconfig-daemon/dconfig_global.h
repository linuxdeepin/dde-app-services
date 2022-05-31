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

#pragma once

#include <QFile>
#include <QQueue>
#include <QString>
#include <functional>
#include <QRegularExpression>
#include <DStandardPaths>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(cfLog);

using ConnKey = QString;
using ResourceKey = QString;
using ConnServiceName = QString;
using ConnRefCount = int;

inline QString getResourceKey(const ConnKey &connKey)
{
    return connKey.left(connKey.lastIndexOf('/'));
}
inline uint getConnectionKey(const ConnKey &connKey)
{
    return connKey.mid(connKey.lastIndexOf('/') + 1).toUInt();
}

struct ConfigureId {
    QString appid;
    QString subpath;
    QString resource;
    bool isInValid() const {
        return resource.isEmpty();
    }
};

// remove slash if back is slash.
static QString removeBackSlash(const QString &target)
{
    if (target.isEmpty() || target.back() != '/')
        return target;

    return target.left(target.count() - 1);
}

inline ConfigureId getMetaConfigureId(const QString &path)
{
    ConfigureId info;
    // /usr/share/dsg/configs/[$appid]/[$subpath]/$resource.json
    static QRegularExpression usrReg(R"(^/usr/share/dsg/configs/(?<appid>([a-z0-9\s\-_\@\-\^!#$%&]+\/)?)(?<subpath>([a-z0-9\s\-_\@\-\^!#$%&]+\/)*)(?<resource>[a-z0-9\s\-_\@\-\^!#$%&]+).json$)");

    QRegularExpressionMatch match;
    match = usrReg.match(path);
    if (!match.hasMatch()) {
        return info;
    }
    info.appid = removeBackSlash(match.captured("appid"));
    info.subpath = removeBackSlash(match.captured("subpath"));
    info.resource = match.captured("resource");

    return info;
}

inline ConfigureId getOverrideConfigureId(const QString &path)
{
    ConfigureId info;
    // /usr/share/dsg/configs/overrides/[$appid]/$resource/[$subpath]/$override_id.json
    static QRegularExpression usrReg(R"(^/usr/share/dsg/configs/overrides/(?<appid>([a-z0-9\s\-_\@\-\^!#$%&]+\/)?)(?<resource>[a-z0-9\s\-_\@\-\^!#$%&]+)/(?<subpath>([a-z0-9\s\-_\@\-\^!#$%&]+\/)*)(?<configurationid>[a-z0-9\s\-_\@\-\^!#$%&]+).json$)");

    // /etc/dsg/configs/overrides/[$appid]/$resource/[$subpath]/$override_id.json
    static QRegularExpression etcReg(R"(^/etc/dsg/configs/overrides/(?<appid>([a-z0-9\s\-_\@\-\^!#$%&]+\/)?)(?<resource>[a-z0-9\s\-_\@\-\^!#$%&]+)/(?<subpath>([a-z0-9\s\-_\@\-\^!#$%&]+\/)*)(?<configurationid>[a-z0-9\s\-_\@\-\^!#$%&]+).json$)");

    QRegularExpressionMatch match;
    match = usrReg.match(path);
    if (!match.hasMatch()) {
        match = etcReg.match(path);
        if (!match.hasMatch())
            return info;
    }
    info.appid = removeBackSlash(match.captured("appid"));
    info.subpath = removeBackSlash(match.captured("subpath"));
    info.resource = match.captured("resource");

    return info;
}

template<class T>
class ObjectPool
{
public:
    typedef T* DataType;
    ~ObjectPool()
    {
        qDeleteAll(m_pool);
        m_pool.clear();
    }
    using InitFunc = std::function<void(DataType)>;
    void setInitFunc(InitFunc func) { m_initFunc = func;}

    DataType pull()
    {
        if (m_pool.isEmpty()) {
            auto item = new T();
            if (m_initFunc)
                m_initFunc(item);

            return item;
        }
        return m_pool.dequeue();
    }
    void push(DataType item) { m_pool.enqueue(item);}

private:
    QQueue<DataType> m_pool;
    InitFunc m_initFunc;
};

inline QString getProcessNameByPid(const uint pid)
{
#ifdef Q_OS_LINUX
    const QString desc = QString("/proc/%1/status").arg(pid);
    QFile file(desc);
    if(file.open(QIODevice::ReadOnly)) {
        const QString &name = file.readLine();
        return name.mid(name.indexOf(':') + 1).trimmed();
    }
#endif // Q_OS_LINUX
    return QString::number(pid);
}

#ifdef Q_OS_LINUX
#include <pwd.h>
#endif // Q_OS_LINUX
inline QString getUserNameByUid(const uint uid)
{
#ifdef Q_OS_LINUX
    passwd *passwd = getpwuid(uid);
    return QString::fromLocal8Bit(passwd->pw_name);
#else // Q_OS_LINUX
    return QString::number(uid);
#endif
}
