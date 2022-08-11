// SPDX-FileCopyrightText: 2021 - 2022 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "dconfigconn.h"
#include "dconfigresource.h"
#include "dconfigfile.h"
#include "helper.hpp"

#include <QDBusMessage>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QFile>
#include <QDebug>

DCORE_USE_NAMESPACE

DSGConfigConn::DSGConfigConn(const ConnKey &key, QObject *parent)
    : QObject (parent),
      m_config(nullptr),
      m_cache(nullptr),
      m_key(key)
{
}

DSGConfigConn::~DSGConfigConn()
{
    if (m_cache) {
        delete m_cache;
        m_cache = nullptr;
    }
}

QString DSGConfigConn::key() const
{
    return m_key;
}

uint DSGConfigConn::uid() const
{
    return getConnectionKey(m_key);
}

DConfigCache *DSGConfigConn::cache() const
{
    return m_cache;
}

void DSGConfigConn::setConfigFile(DConfigFile *configFile)
{
    m_config = configFile;
    m_keys = m_config->meta()->keyList().toSet();
}

void DSGConfigConn::setConfigCache(DConfigCache *cache)
{
    m_cache = cache;
}

/*!
 \brief 返回配置内容的所有配置项
 \return
 */
QStringList DSGConfigConn::keyList() const
{
    return m_config->meta()->keyList();
}

/*!
 \brief 返回配置版本信息
 \return
 */
QString DSGConfigConn::version() const
{
    const auto ver = m_config->meta()->version();
    return QString("%1.%2").arg(ver.major).arg(ver.minor);
}

/*!
 \brief 返回指定配置项的描述信息
 \a key 配置项名称
 \a locale 语言版本,为空时返回默认语言的描述信息
 \return
 */
QString DSGConfigConn::description(const QString &key, const QString &locale)
{
    if (!contains(key))
        return QString();

    return m_config->meta()->description(key, locale.isEmpty() ? QLocale::AnyLanguage :  QLocale(locale));
}

/*!
 \brief 返回指定配置项的显示名称
 \a key 配置项名称
 \a locale 语言版本,为空时返回默认语言的显示信息
 \return
 */
QString DSGConfigConn::name(const QString &key, const QString &locale)
{
    if (!contains(key))
        return QString();

    return m_config->meta()->displayName(key, locale.isEmpty() ? QLocale::AnyLanguage :  QLocale(locale));
}

/*!
 \brief 释放资源引用
 当服务不使用此资源时,减少引用计数
 */
void DSGConfigConn::release()
{
    const QString &service = calledFromDBus() ? message().service() : "test.service";
    qCDebug(cfLog, "received release request, service:%s, path:%s.", qPrintable(service), qPrintable(m_key));

    emit releaseChanged(service);
}

/*!
 \brief 设置指定配置项的值
 \a key 配置项名称
 \a value 需要设置的值
 */
void DSGConfigConn::setValue(const QString &key, const QDBusVariant &value)
{
    if (!contains(key))
        return;

    const auto &v = decodeQDBusArgument(value.variant());
    qCDebug(cfLog) << "set value key:" << key << ", now value:" << v << ", old value:" << m_config->value(key, m_cache);
    if(!m_config->setValue(key, v, getAppid(), m_cache))
        return;

    if (m_config->meta()->flags(key).testFlag(DConfigFile::Global)) {
        emit globalValueChanged(key);
    } else {
        emit valueChanged(key);
    }
}

void DSGConfigConn::reset(const QString &key)
{
    if (!contains(key))
        return;

    const auto &v = m_config->meta()->value(key);
    qCDebug(cfLog) << "reset key:" << key << ", meta value:" << v << ", old value:" << m_config->value(key, m_cache);
    if(!m_config->setValue(key, v, getAppid(), m_cache))
        return;

    if (m_config->meta()->flags(key).testFlag(DConfigFile::Global)) {
        emit globalValueChanged(key);
    } else {
        emit valueChanged(key);
    }
}

/*!
 \brief 返回指定配置项的值
 \a key 配置项名称
 \return
 */
QDBusVariant DSGConfigConn::value(const QString &key)
{
    if (!contains(key))
        return QDBusVariant();

    const auto &value = m_config->value(key, m_cache);
    if (value.isNull()) {
        QString errorMsg = QString("[%1] requires the value in [%2].").arg(key).arg(getAppid());
        qWarning() << errorMsg;
        if (calledFromDBus()) {
            sendErrorReply(QDBusError::Failed, errorMsg);
        }
        return QDBusVariant();
    }

    qCDebug(cfLog) << "get value key:" << key << ", value:" << value;
    return QDBusVariant{value};
}

/*!
 \brief 返回指定配置项的可见性
 \a key 配置项名称
 \return
 */
QString DSGConfigConn::visibility(const QString &key)
{
    if (!contains(key))
        return QString();

    return m_config->meta()->visibility(key) == DTK_CORE_NAMESPACE::DConfigFile::Private ? QString("private") : QString("public");
}

QString DSGConfigConn::permissions(const QString &key)
{
    if (!contains(key))
        return QString();

    return m_config->meta()->permissions(key) == DTK_CORE_NAMESPACE::DConfigFile::ReadWrite ? QString("readwrite") : QString("readonly");
}

int DSGConfigConn::flags(const QString &key)
{
    return static_cast<int>(m_config->meta()->flags(key));
}

uint DSGConfigConn::getUid()
{
    if (calledFromDBus()) {
        const QString &service = message().service();
        return connection().interface()->serviceUid(service);
    }
    return 0;
}

QString DSGConfigConn::getAppid()
{
    if (calledFromDBus()) {
        const QString &service = message().service();
        return getProcessNameByPid(connection().interface()->servicePid(service));
    }
    return QString("testappid");
}

bool DSGConfigConn::contains(const QString &key)
{
    if (m_keys.contains(key))
        return true;

    QString errorMsg = QString("[%1] requires Non-existent configure item [%2] in [%3].").arg(getAppid()).arg(key).arg(m_key);
    if (calledFromDBus())
        sendErrorReply(QDBusError::Failed, errorMsg);
    qWarning() << errorMsg;

    return false;
}
