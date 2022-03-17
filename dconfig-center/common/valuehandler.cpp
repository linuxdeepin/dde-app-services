/*
* Copyright (C) 2021 ~ 2021 Uniontech Software Technology Co.,Ltd.
*
* Author:     Ye ShanShan <yeshanshan@uniontech.com>
*
* Maintainer: Ye ShanShan <yeshanshan@uniontech.com>>
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

#include "valuehandler.h"

#include "configmanager_interface.h"
#include "manager_interface.h"
#include "helper.hpp"

#include <DConfigFile>

static constexpr char const *DSG_CONFIG = "org.desktopspec.ConfigManager";
static constexpr char const *DSG_CONFIG_MANAGER = "org.desktopspec.ConfigManager";

class DBusHandler : public ConfigGetter {

public:
    explicit DBusHandler(ValueHandler *aowner)
        : owner(aowner)
    {
    }

    ~DBusHandler() override;

    QStringList keyList() const override
    {
        return manager->keyList();
    }

    void setValue(const QString &key, const QVariant &value) override
    {
        auto reply = manager->setValue(key, QDBusVariant(value));
        reply.waitForFinished();
        if (reply.isError()) {
            qWarning() << "setValue error key:" << key << ", value:" << value << ", error message:" << reply.error().message();
        }
    }

    QVariant value(const QString &key) const override
    {
        const auto &v = manager->value(key).value().variant();
        return decodeQDBusArgument(v);
    }

    void reset(const QString &key) override
    {
        auto reply = manager->reset(key);
        reply.waitForFinished();
        if (reply.isError()) {
            qWarning() << "reset error key:" << key << ", error message:" << reply.error().message();
        }
    }

    QString permissions(const QString &key) const override
    {
        return manager->permissions(key);
    }

    QString visibility(const QString &key) const override
    {
        return manager->visibility(key);
    }

    QString displayName(const QString &key, const QString &locale) override
    {
        return manager->name(key, locale);
    }

    QString description(const QString &key, const QString &locale) override
    {
        return manager->description(key, locale);
    }

    QString version() const override
    {
        return manager->version();
    }

    void release() override
    {
        manager->release();
    }

    DSGConfigManager *createManager(const QString &appid, const QString &fileName, const QString &subpath)
    {
        DSGConfig dsg_config(DSG_CONFIG, "/", QDBusConnection::systemBus());
        QDBusPendingReply<QDBusObjectPath> dbus_reply = dsg_config.acquireManager(appid, fileName, subpath);
        const QDBusObjectPath dbus_path = dbus_reply.value();
        if (dbus_reply.isError() || dbus_path.path().isEmpty()) {
            qWarning() << QString("can't not get dbus path for appid=%1, resource=%2, subpath=%3.").arg(appid, fileName, subpath);
            return nullptr;
        }

        QScopedPointer<DSGConfigManager> config(new DSGConfigManager(DSG_CONFIG_MANAGER, dbus_path.path(),
                                                                     QDBusConnection::systemBus()));
        if (!config->isValid()) {
            qWarning() << QString("can't not get dbus handler for appid=%1, resource=%2, subpath=%3.").arg(appid, fileName, subpath);
            return nullptr;
        }
        manager.reset(config.take());
        QObject::connect(manager.get(), &DSGConfigManager::valueChanged, owner, &ValueHandler::valueChanged);

        return manager.get();
    }

    static bool isServiceRegistered()
    {
        return QDBusConnection::systemBus().interface()->isServiceRegistered(DSG_CONFIG);
    }

    static bool isServiceActivatable()
    {
         const QDBusReply<void> reply = QDBusConnection::systemBus().interface()->
                 call(QLatin1String("StartServiceByName"),
                 DSG_CONFIG,
                 uint(0));

         if (!reply.isValid())
             qDebug() << "Can't start dbus service" << reply.error().message();

         return reply.isValid();
    }
private:

    QScopedPointer<DSGConfigManager> manager;
    ValueHandler *owner;
};

DCORE_USE_NAMESPACE;

#include <unistd.h>
class FileHandler : public ConfigGetter {

public:
    explicit FileHandler(ValueHandler *aowner)
        : owner(aowner)
    {
    }

    ~FileHandler() override;


    QStringList keyList() const override
    {
        return manager->meta()->keyList();
    }

    void setValue(const QString &key, const QVariant &value) override
    {
        if (manager->setValue(key, value, qAppName(), userCache.get())) {
            emit owner->valueChanged(key);
        }
    }

    QVariant value(const QString &key) const override
    {
        return manager->value(key, userCache.get());
    }

    void reset(const QString &key) override
    {
        const auto &v = manager->meta()->value(key);
        setValue(key, v);
    }

    QString permissions(const QString &key) const override
    {
        return manager->meta()->permissions(key) == DTK_CORE_NAMESPACE::DConfigFile::ReadWrite ? QString("readwrite") : QString("readonly");
    }

    QString visibility(const QString &key) const override
    {
        return manager->meta()->visibility(key) == DTK_CORE_NAMESPACE::DConfigFile::Private ? QString("private") : QString("public");
    }

    QString displayName(const QString &key, const QString &locale) override
    {
        return manager->meta()->displayName(key, locale.isEmpty() ? QLocale::AnyLanguage :  QLocale(locale));
    }

    QString description(const QString &key, const QString &locale) override
    {
        return manager->meta()->description(key, locale.isEmpty() ? QLocale::AnyLanguage :  QLocale(locale));
    }

    QString version() const override
    {
        const auto ver = manager->meta()->version();
        return QString("%1.%2").arg(ver.major).arg(ver.minor);
    }

    void release() override
    {
        manager.reset();
    }


    DConfigFile *createManager(const QString &appid, const QString &fileName, const QString &subpath)
    {
        if (manager)
            return manager.get();

        QScopedPointer<DConfigFile> config(new DConfigFile(appid, fileName, subpath));
        QScopedPointer<DConfigCache> cache(config->createUserCache(getuid()));
        if (!config->load() || !cache->load()) {
            return nullptr;
        }

        manager.reset(config.take());
        userCache.reset(cache.take());
        return manager.get();
    }

private:

    QScopedPointer<DConfigFile> manager;
    QScopedPointer<DConfigCache> userCache;
    ValueHandler *owner;
};

FileHandler::~FileHandler()
{
    if (manager) {
        manager->save();
        userCache->save();
        manager.reset();
    }
}

DBusHandler::~DBusHandler()
{
    if (manager) {
        manager->release();
        manager.reset();
    }
}

ValueHandler::ValueHandler(const QString &appid, const QString &fileName, const QString &subpath)
    : appid(appid),
      fileName(fileName),
      subpath(subpath)
{
}

ValueHandler::~ValueHandler()
{
}

ConfigGetter* ValueHandler::createManager()
{
    if (DBusHandler::isServiceRegistered() || DBusHandler::isServiceActivatable()) {
        auto tmp = new DBusHandler(this);
        if (tmp->createManager(appid, fileName, subpath)) {
            return tmp;
        }
    } else {
        auto tmp = new FileHandler(this);
        if (tmp->createManager(appid, fileName, subpath)) {
            qDebug() << QString("using FileHandler to get value for appid=%1, resource=%2, subpath=%3.").arg(appid, fileName, subpath);
            return tmp;
        }
    }
    qWarning() << QString("get value handler error for appid=%1, resource=%2, subpath=%3.").arg(appid, fileName, subpath);
    return nullptr;
}

ConfigGetter::~ConfigGetter(){}
