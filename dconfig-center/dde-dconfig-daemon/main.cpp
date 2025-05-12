// SPDX-FileCopyrightText: 2021 - 2022 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <DLog>

#include "dconfigserver.h"

#include <csignal>

static void exitApp(int signal)
{
    qInfo() << "App exited due to receiving signal" << signal;
    QCoreApplication::exit(1);
}
int main(int argc, char *argv[])
{
    // 异常处理，调用QCoreApplication::exit，使DSGConfigServer正常析构。
    struct sigaction sa;
    sa.sa_handler = exitApp;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESETHAND;

    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGILL, &sa, nullptr);
    sigaction(SIGABRT, &sa, nullptr);
    sigaction(SIGFPE, &sa, nullptr);

    QCoreApplication a(argc, argv);
    a.setOrganizationName("deepin");
    a.setApplicationName("dde-dconfig-daemon");

    QCommandLineParser parser;
    parser.addHelpOption();

    QCommandLineOption delayTimeOption("t", QCoreApplication::translate("main", "delay time when need to release resource."), "time", QString::number(0));
    parser.addOption(delayTimeOption);

    QCommandLineOption localPrefixOption("p", QCoreApplication::translate("main", "working prefix directory."), "prefix", QString());
    parser.addOption(localPrefixOption);

    QCommandLineOption exitOption("e", QCoreApplication::translate("main", "exit application when all resource released."), "exit", QString::number(true));
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
        qInfo() << "Starting dconfig daemon succeeded.";
    } else {
        qInfo() << "Starting dconfig daemon failed.";
        return 1;
    }

    // Initialization of DtkCore needs to be later than `registerService` avoid earlier request itself.
    Dtk::Core::DLogManager::registerConsoleAppender();
    const char *logsDirectory("LOGS_DIRECTORY");
    if (!qEnvironmentVariableIsEmpty(logsDirectory)) {
        const QString path(qEnvironmentVariable(logsDirectory));
        const auto logPath(QString("%1/%2/%2.log").arg(path).arg(a.applicationName()));
        const QFileInfo file(logPath);
        if (!file.exists()) {
            if (!QDir().mkpath(file.absoluteDir().path())) {
                qWarning() << "Failed to create log path." << file.absoluteFilePath();
            }
        }
        Dtk::Core::DLogManager::setlogFilePath(logPath);
    }
    Dtk::Core::DLogManager::registerFileAppender();
    qInfo() << "Log path is:" << Dtk::Core::DLogManager::getlogFilePath();
    QObject::connect(qApp, &QCoreApplication::aboutToQuit, [&dsgConfig]() {
        qInfo() << "Exit dconfig daemon and release resources.";
        dsgConfig.exit();
    });

    return a.exec();
}
