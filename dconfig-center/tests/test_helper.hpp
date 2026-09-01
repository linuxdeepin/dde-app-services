// SPDX-FileCopyrightText: 2021 - 2026 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QDir>
#include <QFile>

class EnvGuard {
public:
    ~EnvGuard()
    {
        if (m_name)
            restore();
        if (!m_createdDir.isEmpty())
            QDir().rmpath(m_createdDir);
    }
    void set(const char *name, const QByteArray &value)
    {
        m_name = name;
        m_wasSet = qEnvironmentVariableIsSet(m_name);
        m_originValue = qgetenv(m_name);
        qputenv(m_name, value);

        if (!QDir(value).exists()) {
            QDir().mkpath(value);
            m_createdDir = QString::fromUtf8(value);
        }
    }
    void restore()
    {
        if (!m_name)
            return;
        if (m_wasSet)
            qputenv(m_name, m_originValue);
        else
            qunsetenv(m_name);
    }
    QString value()
    {
        return qgetenv(m_name);
    }
private:
    QByteArray m_originValue;
    const char* m_name = nullptr;
    bool m_wasSet = false;
    QString m_createdDir;
};


class FileCopyGuard {
public:
    FileCopyGuard(const QString &source, const QString &target)
        : m_target(target)
    {
        if (!QFile::exists(QFileInfo(target).path()))
            QDir().mkpath(QFileInfo(target).path());
        QFile::copy(source, target);
    }
    ~FileCopyGuard(){ QFile::remove(m_target); }
private:
    QString m_target;
};

// RAII guard for meta files modified during tests.
// Backs up original content on construction, restores on destruction —
// even if ASSERT_* causes early return, the file is always restored.
class MetaFileGuard {
public:
    explicit MetaFileGuard(const QString &path) : m_path(path)
    {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            m_originalContent = file.readAll();
            file.close();
            m_hasOriginal = true;
        }
    }
    ~MetaFileGuard()
    {
        if (m_hasOriginal) {
            QFile file(m_path);
            if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                file.write(m_originalContent);
                file.close();
            }
        }
    }
private:
    QString m_path;
    QByteArray m_originalContent;
    bool m_hasOriginal = false;
};
