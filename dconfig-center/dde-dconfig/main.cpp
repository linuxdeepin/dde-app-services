// SPDX-FileCopyrightText: 2021 - 2023 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <QCoreApplication>
#include <QRegularExpression>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDBusError>
#include <QDBusPendingReply>
#include <QLoggingCategory>
#include <QProcess>
#include <iostream>

#include "helper.hpp"
#include "valuehandler.h"

struct Options {
    QCommandLineOption appidOption;
    QCommandLineOption resourceOption;
    QCommandLineOption subpathOption;
    QCommandLineOption keyOption;
    QCommandLineOption methodOption;
    QCommandLineOption languageOption;
    QCommandLineOption valueOption;
};

// output for standard ostream(dev 1)
inline void outpuSTD(const QString &value)
{
    std::cout << qPrintable(value) << std::endl;
}
// output for standard ostream(dev 2)
inline void outpuSTDError(const QString &value)
{
    std::cerr << qPrintable(value) << std::endl;
}

int onListOption(const QCommandLineParser &parser, const Options &options);
int onGetOption(const QCommandLineParser &parser, const Options &options);
int onSetOption(const QCommandLineParser &parser, const Options &options);
int onResetOption(const QCommandLineParser &parser, const Options &options);
int onWatchOption(const QCoreApplication &a, const QCommandLineParser &parser, const Options &options);

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    a.setApplicationVersion(VERSION);

    QCommandLineParser parser;
    parser.addHelpOption();

    QCommandLineOption localPrefixOption("p", QCoreApplication::translate("main", "working prefix directory."), "prefix", QString());
    parser.addOption(localPrefixOption);

    QCommandLineOption listOption("list", QCoreApplication::translate("main", "list configure information with appid, resource or subpath."));
    parser.addOption(listOption);

    QCommandLineOption appidOption("a", QCoreApplication::translate("main", "appid for a specific application.\n"
                                                                            "it is empty string if we need to manage application independent configuration."), "appid", QString());
    parser.addOption(appidOption);

    QCommandLineOption resourceOption("r", QCoreApplication::translate("main", "resource id for configure name."), "resource", QString());
    parser.addOption(resourceOption);

    QCommandLineOption subpathOption("s", QCoreApplication::translate("main", "subpath for configure."), "subpath", QString());
    parser.addOption(subpathOption);

    QCommandLineOption getOption("get", QCoreApplication::translate("main", "query content for configure."));
    parser.addOption(getOption);

    QCommandLineOption keyOption("k", QCoreApplication::translate("main", "configure item's key."), "key", QString());
    parser.addOption(keyOption);

    QCommandLineOption methodOption("m", QCoreApplication::translate("main", "method for the configure item."), "method", QString("value"));
    parser.addOption(methodOption);

    QCommandLineOption languageOption("l", QCoreApplication::translate("main", "language for the configuration item."), "language", QString());
    parser.addOption(languageOption);

    QCommandLineOption setOption("set", QCoreApplication::translate("main", "set configure item 's value."));
    parser.addOption(setOption);

    QCommandLineOption resetOption("reset", QCoreApplication::translate("main", "reset configure item 's value."));
    parser.addOption(resetOption);

    QCommandLineOption valueOption("v", QCoreApplication::translate("main", "new value to set configure item."), "value", QString());
    parser.addOption(valueOption);

    QCommandLineOption watchOption("watch", QCoreApplication::translate("main", "watch value changed for some configure item."));
    parser.addOption(watchOption);

    QCommandLineOption guiOption("gui", QCoreApplication::translate("main", "start dde-dconfig-editor as a gui configure tool."));
    parser.addOption(guiOption);

    parser.process(a);

    if (argc == 1) {
        parser.showHelp(0);
    }

    Options options { appidOption, resourceOption, subpathOption, keyOption, methodOption, languageOption, valueOption };
    if (parser.isSet(guiOption)) {
        QProcess::startDetached("/bin/dde-dconfig-editor");
        return 0;
    } else {
        if (parser.isSet(listOption)) {
            return onListOption(parser, options);
        } else if (parser.isSet(getOption)) {
            return onGetOption(parser, options);
        } else if (parser.isSet(setOption)) {
            return onSetOption(parser, options);
        } else if (parser.isSet(resetOption)) {
            return onResetOption(parser, options);
        } else if (parser.isSet(watchOption)) {
            return onWatchOption(a, parser, options);
        }
        parser.showHelp(0);
    }
}

int onListOption(const QCommandLineParser &parser, const Options &options)
{
    const auto &appid = parser.value(options.appidOption);
    const auto &resourceid = parser.value(options.resourceOption);
    // list命令，查看app、resource、subpath
    if (parser.isSet(options.appidOption)) {
        if (!existAppid(appid)) {
            outpuSTDError(QString("not exist appid:%1").arg(appid));
            return 1;
        }
        if (parser.isSet(options.resourceOption)) {
            if (!existResource(appid, resourceid)) {
                outpuSTDError(QString("not exist resouce:[%1] for the appid:[%2]").arg(resourceid).arg(appid));
                return 1;
            }
            auto subpaths = subpathsForResource(appid, resourceid);
            for (auto item : subpaths) {
                outpuSTD(item);
            }
        } else {
            auto resources = resourcesForApp(appid);
            for (auto item : resources) {
                outpuSTD(item);
            }
        }
    } else if(parser.isSet(options.resourceOption)) {
        const auto &commons = resourcesForAllApp();
        QRegularExpression re(resourceid);
        for (auto item : commons) {
            auto match = re.match(item);
            if (match.hasMatch()) {
                outpuSTD(item);
            }
        }
    } else {
        auto apps = applications();
        for (auto item : apps) {
            outpuSTD(item);
        }
    }
    return 0;
}

int onGetOption(const QCommandLineParser &parser, const Options &options)
{
    const auto &appid = parser.value(options.appidOption);
    const auto &resourceid = parser.value(options.resourceOption);
    const auto &subpathid = parser.value(options.subpathOption);
    const auto &key = parser.value(options.keyOption);
    const auto &method = parser.value(options.methodOption);

    // query命令，查看指定配置的详细信息，操作方法和配置项信息
    if (!parser.isSet(options.appidOption) || !parser.isSet(options.resourceOption)) {
        const QStringList methods{"value",
                            "name",
                            "description",
                            "visibility",
                            "permissions",
                            "version"};
        for (auto item : methods) {
            outpuSTD(item);
        }
        return 0;
    }

    if (!existResource(appid, resourceid)) {
        outpuSTDError(QString("not exist resouce:[%1] for the appid:[%2]").arg(resourceid).arg(appid));
        return 1;
    }

    ValueHandler handler(appid, resourceid, subpathid);
    if (auto manager = handler.createManager()) {
        if (!parser.isSet(options.keyOption) && !parser.isSet(options.methodOption)) {

            QStringList result = manager->keyList();
            for (auto item : result) {
                outpuSTD(item);
            }
            return 0;
        }
        if (parser.isSet(options.keyOption)) {
            const auto &language = parser.value(options.languageOption);

            if (method == "value") {
                QVariant result = manager->value(key);
                if (result.type() == QVariant::Bool) {
                    outpuSTD(result.toBool() ? "true" : "false");
                } else if (result.type() == QVariant::Double) {
                    outpuSTD(QString::number(result.toDouble()));
                } else {
                    outpuSTD(QString("\"%1\"").arg(qvariantToString(result)));
                }
            } else if (method == "name") {
                QString result = manager->displayName(key, language);
                outpuSTD(result);
            } else if (method == "description") {
                QString result = manager->description(key, language);
                outpuSTD(result);
            } else if (method == "visibility") {
                QString result = manager->visibility(key);
                outpuSTD(result);
            } else if (method == "permissions") {
                QString result = manager->permissions(key);
                outpuSTD(result);
            } else if (method == "version") {
                QString result = manager->version();
                outpuSTD(result);
            } else {
                outpuSTDError(QString("not exit the method:[%1] for `query` command.").arg(method));
                return 1;
            }
        } else {
            outpuSTDError("not set key for `query` command.");
            return 1;
        }
    } else {
        outpuSTDError(QString("not create value handler for appid=%1, resource=%2, subpath=%3.").arg(appid, resourceid, subpathid));
        return 1;
    }
    return 0;
}

int onSetOption(const QCommandLineParser &parser, const Options &options)
{
    const auto &appid = parser.value(options.appidOption);
    const auto &resourceid = parser.value(options.resourceOption);
    const auto &subpathid = parser.value(options.subpathOption);
    const auto &key = parser.value(options.keyOption);

    // set命令，设置指定配置项
    if (!parser.isSet(options.appidOption) || !parser.isSet(options.resourceOption) || !parser.isSet(options.keyOption)
            ||!parser.isSet(options.valueOption)) {
        outpuSTDError("not set appid, resource, key or value.");
        return 1;
    }

    if (!existResource(appid, resourceid)) {
        outpuSTDError(QString("not exist resouce:[%1] for the appid:[%2]").arg(resourceid).arg(appid));
        return 1;
    }

    const auto &value = parser.value(options.valueOption);
    ValueHandler handler(appid, resourceid, subpathid);
    {
        QScopedPointer<ConfigGetter> manager(handler.createManager());
        if (manager) {
            QVariant result = manager->value(key);
            if (result.type() == QVariant::Bool) {
                manager->setValue(key, QVariant(value).toBool());
            } else if (result.type() == QVariant::Double) {
                manager->setValue(key, value.toDouble());
            } else {
                manager->setValue(key, stringToQVariant(value));
            }
        } else {
            outpuSTDError(QString("not create value handler for appid=%1, resource=%2, subpath=%3.").arg(appid, resourceid, subpathid));
            return 1;
        }
    }
    return 0;
}

int onResetOption(const QCommandLineParser &parser, const Options &options)
{
    // reset命令，设置指定配置项
    if (!parser.isSet(options.appidOption) || !parser.isSet(options.resourceOption) || !parser.isSet(options.keyOption)) {
        outpuSTDError("not set appid, resource or key.");
        return 1;
    }

    const auto &appid = parser.value(options.appidOption);
    const auto &resourceid = parser.value(options.resourceOption);
    const auto &subpathid = parser.value(options.subpathOption);
    const auto &key = parser.value(options.keyOption);

    if (!existResource(appid, resourceid)) {
        outpuSTDError(QString("not exist resouce:[%1] for the appid:[%2]").arg(resourceid).arg(appid));
        return 1;
    }

    ValueHandler handler(appid, resourceid, subpathid);
    {
        QScopedPointer<ConfigGetter> manager(handler.createManager());
        if (manager) {
            manager->reset(key);
        } else {
            outpuSTDError(QString("not create value handler for appid=%1, resource=%2, subpath=%3.").arg(appid, resourceid, subpathid));
            return 1;
        }
    }
    return 0;
}

int onWatchOption(const QCoreApplication &a, const QCommandLineParser &parser, const Options &options)
{
    const auto &appid = parser.value(options.appidOption);
    const auto &resourceid = parser.value(options.resourceOption);
    const auto &subpathid = parser.value(options.subpathOption);
    const auto &key = parser.value(options.keyOption);

    // watch命令，监控一些配置项改变信号
    if (!parser.isSet(options.appidOption) || !parser.isSet(options.resourceOption)) {
        outpuSTDError("not set appid or resource.");
        return 1;
    }

    if (!existResource(appid, resourceid)) {
        outpuSTDError(QString("not exist resouce:[%1] for the appid:[%2]").arg(resourceid).arg(appid));
        return 1;
    }

    ValueHandler handler(appid, resourceid, subpathid);
    QScopedPointer<ConfigGetter> manager(handler.createManager());
    if (manager) {
        const auto &matchKey = key;
        QObject::connect(&handler, &ValueHandler::valueChanged, [matchKey](const QString &key){
            QRegularExpression re(matchKey);
            auto match = re.match(key);
            if (match.hasMatch()) {
                outpuSTD(key);
            }
        });
    } else {
        outpuSTDError(QString("not create value handler for appid=%1, resource=%2, subpath=%3.").arg(appid, resourceid, subpathid));
        return 1;
    }
    return a.exec();
}
