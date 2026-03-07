// SPDX-FileCopyrightText: 2026 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "inotifywatcher.h"

#include <QDir>
#include <QDebug>

#ifdef Q_OS_LINUX
#include <sys/inotify.h>
#include <unistd.h>
#endif

static constexpr int INOTIFY_BUF_SIZE = 4096;

InotifyWatcher::InotifyWatcher(QObject *parent)
    : QObject(parent)
{
#ifdef Q_OS_LINUX
    m_fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
    if (m_fd < 0) {
        qWarning() << "[InotifyWatcher] inotify_init1 failed:" << strerror(errno);
        return;
    }
    m_notifier = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, &InotifyWatcher::onInotifyEvent);
#else
    qWarning() << "[InotifyWatcher] inotify is only available on Linux.";
#endif
}

InotifyWatcher::~InotifyWatcher()
{
#ifdef Q_OS_LINUX
    for (int wd : m_wdToPath.keys()) {
        inotify_rm_watch(m_fd, wd);
    }
    if (m_fd >= 0)
        ::close(m_fd);
#endif
}

void InotifyWatcher::addPath(const QString &path)
{
    addWatchRecursive(path);
}

void InotifyWatcher::removePath(const QString &path)
{
#ifdef Q_OS_LINUX
    if (!m_pathToWd.contains(path))
        return;
    int wd = m_pathToWd.take(path);
    m_wdToPath.remove(wd);
    inotify_rm_watch(m_fd, wd);
#endif
}

void InotifyWatcher::addWatchRecursive(const QString &path)
{
#ifdef Q_OS_LINUX
    if (m_fd < 0 || m_pathToWd.contains(path))
        return;

    uint32_t mask = IN_CLOSE_WRITE | IN_MOVED_TO | IN_CREATE | IN_DELETE | IN_MOVED_FROM;
    int wd = inotify_add_watch(m_fd, path.toLocal8Bit().constData(), mask);
    if (wd < 0) {
        qWarning() << "[InotifyWatcher] inotify_add_watch failed for" << path << ":" << strerror(errno);
        return;
    }
    m_wdToPath[wd] = path;
    m_pathToWd[path] = wd;

    // 递归监听子目录
    QDir dir(path);
    for (const QFileInfo &fi : dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        addWatchRecursive(fi.absoluteFilePath());
    }
#endif
}

void InotifyWatcher::onInotifyEvent()
{
#ifdef Q_OS_LINUX
    char buf[INOTIFY_BUF_SIZE] __attribute__((aligned(__alignof__(inotify_event))));
    ssize_t len;

    while ((len = read(m_fd, buf, sizeof(buf))) > 0) {
        char *ptr = buf;
        while (ptr < buf + len) {
            auto *event = reinterpret_cast<inotify_event *>(ptr);

            const QString &dirPath = m_wdToPath.value(event->wd);
            if (dirPath.isEmpty()) {
                ptr += sizeof(inotify_event) + event->len;
                continue;
            }

            if (event->len > 0) {
                // 有文件名：具体文件变化
                const QString name = QString::fromLocal8Bit(event->name);
                const QString fullPath = dirPath + '/' + name;

                if (event->mask & (IN_CREATE | IN_ISDIR)) {
                    // 新创建的子目录，动态加入监听
                    if (event->mask & IN_ISDIR)
                        addWatchRecursive(fullPath);
                }

                if (!(event->mask & IN_ISDIR)) {
                    emit fileChanged(fullPath);
                } else {
                    emit directoryChanged(fullPath);
                }
            } else {
                // 无文件名：目录本身变化
                emit directoryChanged(dirPath);
            }

            ptr += sizeof(inotify_event) + event->len;
        }
    }
#endif
}
