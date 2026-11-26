// SPDX-FileCopyrightText: 2021 - 2023 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDebug>
#include <QTimer>

#include <csignal>
#include <DConfig>
#include <DLog>
DCORE_USE_NAMESPACE

class DConfigExample {
public:
    explicit DConfigExample(const QString &fileName)
        :fileName(fileName)
    {
        baseAPI();
        watchValueChanged();
        subpath();
        mutilFetchConfig();
        generalConfig();
    }

    void watchValueChanged()
    {
        heapConfig.reset(new DConfig(fileName));
        if (!heapConfig->isValid()) {
            qWarning() << QString("DConfig is invalide, name:[%1], subpath[%2].").
                          arg(heapConfig->name(), heapConfig->subpath());
            return;
        }

        // 监听值改变的信号
        const auto &oldValue = heapConfig->value("canExit").toBool();
        QObject::connect(heapConfig.get(), &DConfig::valueChanged, [oldValue, this](const QString &key){
            qDebug() << "value changed, oldValue:" << oldValue << ", newValue:" << heapConfig->value(key).toBool();
        });

        // reset
        heapConfig->setValue("canExit", !oldValue);
        heapConfig->reset("canExit");
    }

    void baseAPI()
    {
        // 构造DConfig
        DConfig config(fileName);

        // 判断是否有效
        if (!config.isValid()) {
            qWarning() << QString("DConfig is invalide, name:[%1], subpath[%2].").
                          arg(config.name(), config.subpath());
            return;
        }

        // 获取所有配置项的key
        qDebug() << "All key items." << config.keyList();

        for (auto item :config.keyList()) {
            qDebug() << item << config.value(item);
        }

        // 获取指定配置项的值
        qDebug() << "key item's value:" << config.value("canExit").toBool();

        // 设置值指定配置项的值
        config.setValue("canExit", false);

        qDebug() << "key item's value is default:" << config.isDefaultValue("canExit");

        // 重置指定配置项的值
        config.reset("canExit");
    }

    void subpath()
    {
        // 构造DConfig
        DConfig config(fileName, "/a");

        // 判断是否有效
        if (!config.isValid()) {
            qWarning() << QString("DConfig is invalide, name:[%1], subpath[%2].").
                          arg(config.name(), config.subpath());
            return;
        }

        // 获取指定配置项的值
        qDebug() << "subpath's key item's value:" << config.value("key3").toString();

        qDebug() << "no subpath's key item's value:" << DConfig(fileName).value("key3").toString();
    }

    void mutilFetchConfig()
    {
        QVariantMap map;
        for (int i = 0; i < 1; i++) {
            QVariantMap nestItem;
            for (int j = 0; j < 1; j++) {
                nestItem[QString::number(j)] = QString::number(j);
            }
            map[QString::number(i)] = nestItem;
        }
        {
            DConfig config(fileName);
            config.setValue("map", map);
        }
        {
            DConfig config(fileName);
            qDebug() << config.value("map", map);
        }
        // reset
        {
            DConfig config(fileName);
            config.reset("map");
        }
    }
    void generalConfig()
    {
        // 空appid，直接获取公共配置
        {
            QScopedPointer<DConfig> config(DConfig::createGeneric(fileName));
            qDebug() << config->value("canExit");
        }
        // 空appid，设置公共配置
        {
            QScopedPointer<DConfig> config(DConfig::createGeneric(fileName));
            config->setValue("canExit", true);
            qDebug() << config->value("canExit");
        }
        // 默认使用本appid去获取，会fallback到公共配置
        {
            DConfig config(fileName);
            qDebug() << config.value("canExit");
        }
        // 指定使用appid去获取，会fallback到公共配置
        {
            QScopedPointer<DConfig> config(DConfig::create("noexist-appid", fileName));
            qDebug() << config->value("canExit");
        }
        // reset
        {
            QScopedPointer<DConfig> config(DConfig::createGeneric(fileName));
            config->reset("canExit");
        }
    }

private:
    QScopedPointer<DConfig> heapConfig;
    QString fileName;
};

// dconfig-example --watch-subpaths [连接的 subpath，例如 /a 或 /a/b]
class SubpathsExample : public QObject
{
    Q_OBJECT
public:
    int run(const QString &subpath)
    {
        const QString service = QStringLiteral("org.desktopspec.ConfigManager");
        auto bus = QDBusConnection::systemBus();
        QDBusInterface server(service, "/", service, bus);
        QDBusReply<QDBusObjectPath> acquired = server.call("acquireManager", "dconfig-example", "example", subpath);
        if (!acquired.isValid()) {
            qWarning() << acquired.error().message();
            return 1;
        }

        const QString path = acquired.value().path();
        const QString interface = service + ".Manager";
        QDBusInterface manager(service, path, interface, bus);
        // 先订阅，再读取初始值，以免错过两者之间的变化。
        const bool subscribed = bus.connect(service, path, "org.freedesktop.DBus.Properties", "PropertiesChanged",
                                            this, SLOT(propertiesChanged(QString,QVariantMap,QStringList)));
        QDBusInterface properties(service, path, "org.freedesktop.DBus.Properties", bus);
        QDBusReply<QDBusVariant> initial = properties.call("Get", interface, "subpaths");
        int status = 1;
        if (!subscribed) {
            qWarning() << "Failed to subscribe to PropertiesChanged:" << bus.lastError().message();
        } else if (!initial.isValid()) {
            qWarning() << initial.error().message();
        } else {
            qInfo() << "Initial subpaths:" << initial.value().variant().toStringList();
            status = QCoreApplication::exec();
        }
        manager.call("release");
        return status;
    }

private Q_SLOTS:
    void propertiesChanged(const QString &interface, const QVariantMap &changed, const QStringList &)
    {
        if (interface == QStringLiteral("org.desktopspec.ConfigManager.Manager") && changed.contains("subpaths"))
            qInfo() << "Changed subpaths:" << changed.value("subpaths").toStringList();
    }
};

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QCommandLineParser parser;
    parser.addHelpOption();
    QCommandLineOption watchSubpaths("watch-subpaths", "Print the initial subpaths and watch for changes.");
    parser.addOption(watchSubpaths);
    parser.addPositionalArgument("subpath", "Manager connection subpath (default: root).", "[subpath]");
    parser.process(a);

    // 异常处理，调用QCoreApplication::exit，使DConfig正常析构。
    std::signal(SIGINT, &QCoreApplication::exit);
    std::signal(SIGABRT, &QCoreApplication::exit);
    std::signal(SIGTERM, &QCoreApplication::exit);
    std::signal(SIGKILL, &QCoreApplication::exit) ;

    if (parser.isSet(watchSubpaths)) {
        SubpathsExample example;
        return example.run(parser.positionalArguments().value(0));
    }

    Dtk::Core::DLogManager::registerConsoleAppender();
    DConfigExample example("example");
    return a.exec();
}

#include "main.moc"
