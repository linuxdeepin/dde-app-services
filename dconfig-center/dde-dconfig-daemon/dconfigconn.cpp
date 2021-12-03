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

#include "dconfigconn.h"
#include "dconfigresource.h"
#include "dconfigfile.h"
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
    qCDebug(cfLog, "set value key:[%s], now value:[%s], old value:[%s]", qPrintable(key), qPrintable(value.variant().toString()), qPrintable(m_config->value(key, m_cache).toString()));
    if(!m_config->setValue(key, value.variant(), getAppid(), m_cache))
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
    qCDebug(cfLog, "get value key:[%s], value:[%s]", qPrintable(key), qPrintable(m_config->value(key, m_cache).toString()));
    return QDBusVariant{m_config->value(key, m_cache)};
}

/*!
 \brief 返回指定配置项的可见性
 \a key 配置项名称
 \return
 */
QString DSGConfigConn::visibility(const QString &key)
{
    return m_config->meta()->visibility(key) == DTK_CORE_NAMESPACE::DConfigFile::Private ? QString("private") : QString("public");
}

QString DSGConfigConn::permissions(const QString &key)
{
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
