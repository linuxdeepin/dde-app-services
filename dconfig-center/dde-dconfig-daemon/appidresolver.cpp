// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "appidresolver.h"

#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QDirIterator>

#ifdef Q_OS_LINUX
#include <sys/syscall.h>
#include <unistd.h>
#include <dbus/dbus.h>
#include <cerrno>
#include <memory>
#endif

Q_LOGGING_CATEGORY(cfLog, "dsg.config", QtInfoMsg);

AppIdResolver::AppIdResolver(QObject *parent)
    : QObject(parent)
{
}

QString AppIdResolver::resolveAppId(const ConnServiceName &service, uint pid, uint uid)
{
    // 检查缓存
    if (m_cache.contains(service)) {
        return m_cache.value(service);
    }

    QString appId;

#ifdef Q_OS_LINUX
    // 首先尝试通过 AM Identify 获取
    appId = identifyByAM(pid, uid);
    
    // 如果 AM 不可用，fallback 到 /proc 解析
    if (appId.isEmpty()) {
        appId = fallbackFromProc(pid);
    }
#else
    // 非 Linux 平台直接 fallback
    appId = fallbackFromProc(pid);
#endif

    // 缓存结果
    if (!appId.isEmpty()) {
        m_cache.insert(service, appId);
    }

    return appId;
}

void AppIdResolver::clearCache(const ConnServiceName &service)
{
    m_cache.remove(service);
}

#ifdef Q_OS_LINUX
QString AppIdResolver::identifyByAM(uint pid, uint uid)
{
    // RAII helpers for libdbus-1 resources
    auto dbusErrorDeleter = [](DBusError *err) { if (err) { dbus_error_free(err); delete err; } };
    auto dbusConnDeleter = [](DBusConnection *c) { if (c) dbus_connection_unref(c); };
    auto dbusMsgDeleter = [](DBusMessage *m) { if (m) dbus_message_unref(m); };

    // 构造 session bus 地址（daemon 运行在 system bus，需跨总线连接用户 session bus）
    QByteArray address = QString("unix:path=/run/user/%1/bus").arg(uid).toUtf8();
    
    auto error = std::unique_ptr<DBusError, decltype(dbusErrorDeleter)>(new DBusError, dbusErrorDeleter);
    dbus_error_init(error.get());
    
    // 使用 dbus_connection_open_private 连接到用户的 session bus
    // dbus_bus_get(DBUS_BUS_SESSION) 不可用，因为 daemon 没有自己的 session bus
    DBusConnection *rawConn = dbus_connection_open_private(address.constData(), error.get());
    if (!rawConn) {
        qCDebug(cfLog) << "Failed to connect to session bus for uid" << uid << ":" << error->message;
        return QString();
    }
    auto connGuard = std::unique_ptr<DBusConnection, decltype(dbusConnDeleter)>(rawConn, dbusConnDeleter);

    // 注册连接（完成认证握手）
    if (!dbus_bus_register(rawConn, error.get())) {
        qCDebug(cfLog) << "Failed to register on session bus for uid" << uid << ":" << error->message;
        return QString();
    }
    
    // 获取 pidfd
    int pidfd = syscall(SYS_pidfd_open, pid, 0);
    if (pidfd < 0) {
        qCDebug(cfLog) << "Failed to get pidfd for pid" << pid << ":" << strerror(errno);
        return QString();
    }
    
    // 创建 Identify 方法调用
    DBusMessage *rawMsg = dbus_message_new_method_call(
        "org.desktopspec.ApplicationManager1",
        "/org/desktopspec/ApplicationManager1",
        "org.desktopspec.ApplicationManager1",
        "Identify");
    if (!rawMsg) {
        qCDebug(cfLog) << "Failed to create D-Bus message for Identify";
        close(pidfd);
        return QString();
    }
    auto msgGuard = std::unique_ptr<DBusMessage, decltype(dbusMsgDeleter)>(rawMsg, dbusMsgDeleter);
    
    // 附加 pidfd 参数
    if (!dbus_message_append_args(rawMsg, DBUS_TYPE_UNIX_FD, &pidfd, DBUS_TYPE_INVALID)) {
        qCDebug(cfLog) << "Failed to append pidfd to D-Bus message";
        close(pidfd);
        return QString();
    }
    
    // 发送调用并等待回复
    DBusMessage *rawReply = dbus_connection_send_with_reply_and_block(rawConn, rawMsg, 5000, error.get());
    msgGuard.reset(); // msg consumed by the call
    close(pidfd);
    
    if (dbus_error_is_set(error.get())) {
        qCDebug(cfLog) << "Identify call failed:" << error->message;
        return QString();
    }
    
    if (!rawReply) {
        qCDebug(cfLog) << "No reply received from Identify";
        return QString();
    }
    auto replyGuard = std::unique_ptr<DBusMessage, decltype(dbusMsgDeleter)>(rawReply, dbusMsgDeleter);
    
    // 解析回复
    const char *appIdStr = nullptr;
    if (!dbus_message_get_args(rawReply, error.get(), DBUS_TYPE_STRING, &appIdStr, DBUS_TYPE_INVALID)) {
        qCDebug(cfLog) << "Failed to parse Identify reply:" << error->message;
        return QString();
    }
    
    QString result = QString::fromUtf8(appIdStr);
    qCDebug(cfLog) << "Got appId from AM:" << result << "for pid" << pid;
    return result;
}
#endif

QString AppIdResolver::fallbackFromProc(uint pid)
{
    // 从 /proc/{pid}/exe 获取进程路径
    QString exePath = QFile::symLinkTarget(QString("/proc/%1/exe").arg(pid));
    
    if (exePath.isEmpty()) {
        // Fallback to cmdline
        QFile cmdlineFile(QString("/proc/%1/cmdline").arg(pid));
        if (cmdlineFile.open(QIODevice::ReadOnly)) {
            QByteArray cmdline = cmdlineFile.readLine();
            exePath = cmdline.split('\0').first();
        }
    }
    
    if (exePath.isEmpty()) {
        return QString();
    }
    
    // 格式化为 appId
    // 参考 DSGApplication::formatAppId 的实现
    QString appId = exePath;
    
    // 移除前导路径分隔符并替换为 .
    appId = appId.replace(QRegularExpression(QStringLiteral("^/+")), QString());
    appId = appId.replace(QDir::separator(), QStringLiteral("."));
    
    // 替换特殊字符为 -
    static const QRegularExpression regex(QStringLiteral("[^\\w\\-\\.]"));
    appId = appId.replace(regex, QStringLiteral("-"));
    
    // 移除开头的 .
    static const QString dotPrefix = QStringLiteral(".");
    while (appId.startsWith(dotPrefix)) {
        appId = appId.mid(dotPrefix.size());
    }
    
    qCDebug(cfLog) << "Fallback appId from /proc:" << appId << "for pid" << pid;
    
    return appId;
}
