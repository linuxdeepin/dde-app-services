// SPDX-FileCopyrightText: 2021 - 2022 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QSocketNotifier>
#include <DLog>

#include "dconfigserver.h"
#include "servicelifecycle.h"

#include <csignal>
#include <sys/socket.h>
#include <unistd.h>

// ---- POSIX 信号 → Qt 事件桥 ----
static int g_signalFd[2] = {-1, -1};

static void posixSignalHandler(int sig)
{
    // async-signal-safe: 只写一个字节
    char byte = static_cast<char>(sig);
    ::write(g_signalFd[0], &byte, 1);
}

static bool setupSignalBridge()
{
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, g_signalFd) != 0) {
        qWarning("socketpair() failed for signal bridge");
        return false;
    }
    struct sigaction sa;
    sa.sa_handler = posixSignalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGINT,  &sa, nullptr);
    return true;
}

int main(int argc, char *argv[])
{
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

    // 生命周期状态机
    ServiceLifecycle lifecycle;

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

    // T01-2：POSIX 信号 → Qt 事件桥（有序关闭）
    if (setupSignalBridge()) {
        auto *signalNotifier = new QSocketNotifier(g_signalFd[1], QSocketNotifier::Read, &a);
        QObject::connect(signalNotifier, &QSocketNotifier::activated, &a, [&lifecycle, &dsgConfig](int) {
            char sig = 0;
            ::read(g_signalFd[1], &sig, 1);
            qInfo("[main] Received signal %d, starting ordered shutdown...", static_cast<int>(sig));
            lifecycle.transitionTo(ServiceState::Stopping);
            dsgConfig.exit();
            lifecycle.transitionTo(ServiceState::Stopped);
            QCoreApplication::quit();
        });
    } else {
        // 降级：直接用旧的 signal handler
        struct sigaction sa;
        sa.sa_handler = [](int sig) {
            qInfo("Received signal %d, exiting.", sig);
            QCoreApplication::exit(0);
        };
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESETHAND;
        sigaction(SIGTERM, &sa, nullptr);
        sigaction(SIGINT,  &sa, nullptr);
    }

    QObject::connect(qApp, &QCoreApplication::aboutToQuit, [&dsgConfig]() {
        qInfo() << "Exit dconfig daemon and release resources.";
        dsgConfig.exit();
    });

    // T01-3：统一日志格式（时间戳 + pid + category）
    if (qEnvironmentVariableIsEmpty("QT_MESSAGE_PATTERN")) {
        qputenv("QT_MESSAGE_PATTERN",
                "%{time yyyy-MM-dd hh:mm:ss.zzz} [%{pid}] %{category} %{type}: %{message}");
    }

    dsgConfig.initialize();
    lifecycle.transitionTo(ServiceState::Running);
    qInfo("[main] Service is now Running.");

    return a.exec();
}

