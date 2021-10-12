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

#include "mainwindow.h"

#include <DApplication>
#include <DWidgetUtil>

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDBusError>
#include <QDBusPendingReply>
#include <QLoggingCategory>

#include "helper.hpp"
#include "valuehandler.h"


DWIDGET_USE_NAMESPACE

#include <iostream>
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

int main(int argc, char *argv[])
{
    DApplication a(argc, argv);

    QCommandLineParser parser;
    parser.addHelpOption();

    QCommandLineOption localPrefixOption("p", QCoreApplication::translate("main", "working prefix directory."), "prefix", QString());
    parser.addOption(localPrefixOption);

    QCommandLineOption listOption("list", QCoreApplication::translate("main", "list configure information with appid, resource or subpath."));
    parser.addOption(listOption);

    QCommandLineOption appidOption("a", QCoreApplication::translate("main", "appid for the configure owner application."), "appid", QString());
    parser.addOption(appidOption);

    QCommandLineOption resourceOption("r", QCoreApplication::translate("main", "resource id for configure name."), "resource", QString());
    parser.addOption(resourceOption);

    QCommandLineOption subpathOption("s", QCoreApplication::translate("main", "subpath for configure."), "subpath", QString());
    parser.addOption(subpathOption);

    QCommandLineOption queryOption("query", QCoreApplication::translate("main", "query content for configure."));
    parser.addOption(queryOption);

    QCommandLineOption keyOption("k", QCoreApplication::translate("main", "configure item's key."), "key", QString());
    parser.addOption(keyOption);

    QCommandLineOption methodOption("m", QCoreApplication::translate("main", "method for the configure item."), "method", QString("value"));
    parser.addOption(methodOption);

    QCommandLineOption languageOption("l", QCoreApplication::translate("main", "language for the configuration item."), "language", QString());
    parser.addOption(languageOption);

    QCommandLineOption setOption("set", QCoreApplication::translate("main", "set configure item 's value."));
    parser.addOption(setOption);

    QCommandLineOption valueOption("v", QCoreApplication::translate("main", "new value to set configure item."), "value", QString());
    parser.addOption(valueOption);

    QCommandLineOption watchOption("watch", QCoreApplication::translate("main", "watch value changed for some configure item."));
    parser.addOption(watchOption);

    parser.process(a);
    // 没设置list、query、set、watch，则为命令行模式
    if (!(parser.isSet(listOption) ||
            parser.isSet(queryOption) ||
            parser.isSet(setOption) ||
            parser.isSet(watchOption))) {
        MainWindow window;
        window.show();

        Dtk::Widget::moveToCenter(&window);

        return a.exec();
    } else {
        const auto &appid = parser.value(appidOption);
        const auto &resourceid = parser.value(resourceOption);
        const auto &subpathid = parser.value(subpathOption);
        const auto &key = parser.value(keyOption);

        const auto &method = parser.value(methodOption);

        QLoggingCategory::setFilterRules("dtk.dsg.config=false");

        if (parser.isSet(listOption)) {
            // list命令，查看app、resource、subpath
            if (parser.isSet(appidOption)) {
                if (!existAppid(appid)) {
                    outpuSTDError(QString("not exist appid:%1").arg(appid));
                    return 1;
                }
                if (parser.isSet(resourceOption)) {
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
            } else if(parser.isSet(resourceOption)) {
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

        } else if (parser.isSet(queryOption)) {
            // query命令，查看指定配置的详细信息，操作方法和配置项信息
            if (!parser.isSet(appidOption) || !parser.isSet(resourceOption)) {
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
                if (!parser.isSet(keyOption) && !parser.isSet(methodOption)) {

                    QStringList result = manager->keyList();
                    for (auto item : result) {
                        outpuSTD(item);
                    }
                    return 0;
                }
                if (parser.isSet(keyOption)) {
                    const auto &language = parser.value(languageOption);

                    if (method == "value") {
                        QVariant result = manager->value(key);
                        if (result.type() == QVariant::Bool) {
                            outpuSTD(result.toBool() ? "true" : "false");
                        } else if (result.type() == QVariant::Double) {
                            outpuSTD(QString::number(result.toDouble()));
                        } else {
                            outpuSTD(QString("\"%1\"").arg(result.toString()));
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
        } else if (parser.isSet(setOption)) {
            // set命令，设置指定配置项
            if (!parser.isSet(appidOption) || !parser.isSet(resourceOption) || !parser.isSet(keyOption)
                    ||!parser.isSet(valueOption)) {
                outpuSTDError("not set appid, resource, key or value.");
                return 1;
            }

            if (!existResource(appid, resourceid)) {
                outpuSTDError(QString("not exist resouce:[%1] for the appid:[%2]").arg(resourceid).arg(appid));
                return 1;
            }

            const auto &value = parser.value(valueOption);
            ValueHandler handler(appid, resourceid, subpathid);
            QScopedPointer<ConfigGetter> manager(handler.createManager());
            if (manager) {
                QVariant result = manager->value(key);
                if (result.type() == QVariant::Bool) {
                    manager->setValue(key, QVariant(value).toBool());
                } else if (result.type() == QVariant::Double) {
                    manager->setValue(key, value.toDouble());
                } else {
                    manager->setValue(key, value);
                }
            } else {
                outpuSTDError(QString("not create value handler for appid=%1, resource=%2, subpath=%3.").arg(appid, resourceid, subpathid));
                return 1;
            }
        } else if (parser.isSet(watchOption)) {
            // watch命令，监控一些配置项改变信号
            if (!parser.isSet(appidOption) || !parser.isSet(resourceOption)) {
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
        return 0;
    }
}
