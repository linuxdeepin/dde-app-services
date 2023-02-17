// SPDX-FileCopyrightText: 2021 - 2022 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QFile>
#include <QQueue>
#include <QString>
#include <functional>
#include <QRegularExpression>
#include <DStandardPaths>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(cfLog);

// /appid/filename/subpath/userid
using ConnKey = QString;
// /appid/filename/subpath
using ResourceKey = QString;
// /filename/subpath
using GenericResourceKey = QString;
using ConnServiceName = QString;
using ConnRefCount = int;
// user: u-${ConnKey}, global: g-${ResourceKey}
using ConfigCacheKey = QString;
static constexpr int TestUid = 0U;
inline QString formatDBusObjectPath(QString path)
{
    return path.replace('.', '_').replace('-', '_');
}
inline QString outerAppidToInner(const QString &appid)
{
    return appid;
}
inline QString innerAppidToOuter(const QString &appid)
{
    return appid;
}
inline ResourceKey getResourceKey(const QString &appid, const GenericResourceKey &key)
{
    return QString("/%1%2").arg(appid).arg(key);
}
inline ResourceKey getResourceKey(const ConnKey &connKey)
{
    return connKey.left(connKey.lastIndexOf('/'));
}
inline GenericResourceKey getGenericResourceKey(const QString &name, const QString &subpath)
{
    return QString("/%1%2").arg(name, subpath);
}
inline GenericResourceKey getGenericResourceKey(const ConnKey &connKey)
{
    return getResourceKey(connKey).mid(connKey.indexOf('/', 1));
}
inline uint getConnectionKey(const ConnKey &connKey)
{
    return connKey.mid(connKey.lastIndexOf('/') + 1).toUInt();
}
inline ConnKey getConnectionKey(const ResourceKey &key, const uint uid)
{
    return QString("%1/%2").arg(key).arg(uid);
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
    static QRegularExpression usrReg(R"(/configs/(?<appid>([a-z0-9\s\-_\@\-\^!#$%&.]+\/)?)(?<subpath>([a-z0-9\s\-_\@\-\^!#$%&.]+\/)*)(?<resource>[a-z0-9\s\-_\@\-\^!#$%&.]+).json$)");

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
    static QRegularExpression usrReg(R"(/configs/overrides/(?<appid>([a-z0-9\s\-_\@\-\^!#$%&.]+\/)?)(?<resource>[a-z0-9\s\-_\@\-\^!#$%&.]+)/(?<subpath>([a-z0-9\s\-_\@\-\^!#$%&.]+\/)*)(?<configurationid>[a-z0-9\s\-_\@\-\^!#$%&.]+).json$)");

    // /etc/dsg/configs/overrides/[$appid]/$resource/[$subpath]/$override_id.json
    static QRegularExpression etcReg(R"(^/etc/dsg/configs/overrides/(?<appid>([a-z0-9\s\-_\@\-\^!#$%&.]+\/)?)(?<resource>[a-z0-9\s\-_\@\-\^!#$%&.]+)/(?<subpath>([a-z0-9\s\-_\@\-\^!#$%&.]+\/)*)(?<configurationid>[a-z0-9\s\-_\@\-\^!#$%&.]+).json$)");

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
