// SPDX-FileCopyrightText: 2026 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QObject>
#include <QHash>
#include <QSocketNotifier>
#include <QString>

/**
 * @brief InotifyWatcher
 *
 * 封装 Linux inotify，监听配置目录下文件变化（IN_CLOSE_WRITE | IN_MOVED_TO | IN_CREATE | IN_DELETE）。
 * 替代原有 allConfigureFileSignatures() 轮询扫描方案，降低 CPU 开销。
 */
class InotifyWatcher : public QObject
{
    Q_OBJECT
public:
    explicit InotifyWatcher(QObject *parent = nullptr);
    ~InotifyWatcher() override;

    /// 递归监听指定目录（包括其直接子目录）
    void addPath(const QString &path);
    void removePath(const QString &path);

Q_SIGNALS:
    void fileChanged(const QString &path);
    void directoryChanged(const QString &path);

private Q_SLOTS:
    void onInotifyEvent();

private:
    void addWatchRecursive(const QString &path);
    int m_fd = -1;
    QSocketNotifier *m_notifier = nullptr;
    QHash<int, QString> m_wdToPath;    ///< watch descriptor → absolute path
    QHash<QString, int> m_pathToWd;   ///< absolute path → watch descriptor
};
