// SPDX-FileCopyrightText: 2021 - 2022 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <QCoreApplication>
#include <QDebug>
#include <DLog>
#include <QCommandLineParser>
#include <QDir>

#include "dconfigserver.h"

#include <csignal>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    a.setOrganizationName("deepin");

    QCommandLineParser parser;
    parser.addHelpOption();

    QCommandLineOption delayTimeOption("t", QCoreApplication::translate("main", "delay time when need to release resource."), "time", QString::number(0));
    parser.addOption(delayTimeOption);

    QCommandLineOption localPrefixOption("p", QCoreApplication::translate("main", "working prefix directory."), "prefix", QString());
    parser.addOption(localPrefixOption);

    QCommandLineOption exitOption("e", QCoreApplication::translate("main", "exit application when all resource released."), "exit", QString(true));
    parser.addOption(exitOption);

    parser.process(a);

    DSGConfigServer dsgConfig;

    if (parser.isSet(delayTimeOption)) {
        dsgConfig.setDelayReleaseTime(parser.value(delayTimeOption).toInt());
    }

    if (parser.isSet(localPrefixOption)) {
        dsgConfig.setLocalPrefix(parser.value(localPrefixOption));
    }

    if (parser.isSet(exitOption)) {
        dsgConfig.setEnableExit(QVariant(parser.value(exitOption)).toBool());
    }

    if (dsgConfig.registerService()) {
        qInfo() << "start dconfig daemon successfully.";
    } else {
        qInfo() << "start dconfig daemon failed.";
        return 1;
    }

    // Initialization of DtkCore needs to be later than `registerService` avoid earlier request itself.
    Dtk::Core::DLogManager::registerConsoleAppender();
    Dtk::Core::DLogManager::setlogFilePath(QString("/var/log/%1/%2/%2.log").arg(a.organizationName(), a.applicationName()));
    const QDir &logDir = QFileInfo((Dtk::Core::DLogManager::getlogFilePath())).dir();
    if (!logDir.exists())
        QDir().mkpath(logDir.path());

    Dtk::Core::DLogManager::registerFileAppender();
    qInfo() << "Log path is:" << Dtk::Core::DLogManager::getlogFilePath();

    // 异常处理，调用QCoreApplication::exit，使DSGConfigServer正常析构。
    std::signal(SIGINT, &QCoreApplication::exit);
    std::signal(SIGABRT, &QCoreApplication::exit);
    std::signal(SIGTERM, &QCoreApplication::exit);
    std::signal(SIGKILL, &QCoreApplication::exit) ;

    return a.exec();
}
