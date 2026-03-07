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
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <iostream>

#include "helper.hpp"
#include "valuehandler.h"

class CommandManager {
public:
    CommandManager(const QCommandLineParser &parser,
            const QCommandLineOption &uidOption,
            const QCommandLineOption &appidOption,
            const QCommandLineOption &resourceOption,
            const QCommandLineOption &subpathOption,
            const QCommandLineOption &keyOption,
            const QCommandLineOption &methodOption,
            const QCommandLineOption &languageOption,
            const QCommandLineOption &valueOption,
            const QCommandLineOption &outputOption,
            const QCommandLineOption &fileOption)
        : parser(parser)
        , uidOption(uidOption)
        , appidOption(appidOption)
        , resourceOption(resourceOption)
        , subpathOption(subpathOption)
        , keyOption(keyOption)
        , methodOption(methodOption)
        , languageOption(languageOption)
        , valueOption(valueOption)
        , outputOption(outputOption)
        , fileOption(fileOption)
    {
        updateValues();
    }

    int listCommand();
    int getCommand();
    int setCommand();
    int resetCommand();
    int watchCommand();
    int exportCommand();   // T09-3: export snapshot
    int importCommand();   // T09-3: import snapshot

    QString appid;
    QString resourceid;
    QString subpathid;
    QString key;
    QString outputFormat;  // T09-1: "text" | "json"
    QString snapshotFile;  // T09-3: file path for export/import

    void updateValues()
    {
        if (parser.isSet(uidOption))
            uid = parser.value(uidOption).toInt();
        appid = fetchAppid();
        resourceid = fetchResourceid();
        subpathid = parser.value(subpathOption);
        key = fetchKey();
        outputFormat = parser.isSet(outputOption) ? parser.value(outputOption) : "text";
        snapshotFile = parser.isSet(fileOption) ? parser.value(fileOption) : QString();
    }
    // fallback to positionalArgument as key
    inline QString fetchKey() const
    {
        if (parser.isSet(keyOption))
            return parser.value(keyOption);
        if (parser.positionalArguments().size() > 2) {
            return parser.positionalArguments().at(2);
        }
        return QString();
    }

    inline bool isSetKey() const
    {
        return parser.isSet(keyOption) || parser.positionalArguments().size() > 2;
    }

    // fallback to positionalArgument as appid
    inline QString fetchAppid() const
    {
        if (parser.isSet(appidOption))
            return parser.value(appidOption);
        if (parser.positionalArguments().size() > 1) {
            return parser.positionalArguments().at(1);
        }
        return QString();
    }

    inline bool isSetAppid() const
    {
        return parser.isSet(appidOption) || parser.positionalArguments().size() >= 2;
    }

    // fallback to the same resource as appidOption
    inline QString fetchResourceid() const
    {
        if (parser.isSet(resourceOption))
            return parser.value(resourceOption);

        return fetchAppid();
    }

    inline bool isSetResourceid() const
    {
        return parser.isSet(resourceOption) || isSetAppid();
    }

private:
    const QCommandLineParser &parser;

    int uid = -1;
    QCommandLineOption uidOption;
    QCommandLineOption appidOption;
    QCommandLineOption resourceOption;
    QCommandLineOption subpathOption;
    QCommandLineOption keyOption;
    QCommandLineOption methodOption;
    QCommandLineOption languageOption;
    QCommandLineOption valueOption;
    QCommandLineOption outputOption;   // T09-1
    QCommandLineOption fileOption;     // T09-3
};

// output for standard ostream(dev 1)
inline void outpuSTD(const QString &value)
{
    std::cout << qPrintable(value) << std::endl;
}
// output for standard ostream(dev 1)
inline void outpuSTD(const bool value)
{
    std::cout << (value ? "true" : "false") << std::endl;
}
// output for standard ostream(dev 2)
inline void outpuSTDError(const QString &value)
{
    std::cerr << qPrintable(value) << std::endl;
}

int CommandManager::listCommand()
{
    // list命令，查看app、resource、subpath
    const bool jsonOut = (outputFormat == "json");
    QStringList items;

    if (isSetAppid()) {
        if (!existAppid(appid)) {
            outpuSTDError(QString("not exist appid:%1").arg(appid));
            return 1;
        }
        if (parser.isSet(resourceOption)) {
            if (!existResource(appid, resourceid)) {
                outpuSTDError(QString("not exist resouce:[%1] for the appid:[%2]").arg(resourceid).arg(appid));
                return 1;
            }
            items = subpathsForResource(appid, resourceid);
        } else {
            items = resourcesForApp(appid);
        }
    } else if(parser.isSet(resourceOption)) {
        const auto &commons = resourcesForAllApp();
        QRegularExpression re(resourceid);
        for (auto item : commons) {
            if (re.match(item).hasMatch())
                items << item;
        }
    } else {
        items = applications();
    }

    if (jsonOut) {
        QJsonArray arr;
        for (const auto &item : items) arr.append(item);
        std::cout << QJsonDocument(arr).toJson(QJsonDocument::Compact).constData() << std::endl;
    } else {
        for (auto item : items) outpuSTD(item);
    }
    return 0;
}

int CommandManager::getCommand()
{
    const auto &method = parser.value(methodOption);

    // query命令，查看指定配置的详细信息，操作方法和配置项信息
    if (!isSetAppid() || !isSetResourceid()) {
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

    const bool jsonOut = (outputFormat == "json");
    ValueHandler handler(uid, appid, resourceid, subpathid);
    if (auto manager = handler.createManager()) {
        if (!isSetKey() && !parser.isSet(methodOption)) {
            QStringList result = manager->keyList();
            if (jsonOut) {
                QJsonArray arr;
                for (const auto &k : result) {
                    QJsonObject obj;
                    obj["key"] = k;
                    obj["value"] = manager->value(k).toString();
                    obj["appid"] = appid;
                    obj["resource"] = resourceid;
                    arr.append(obj);
                }
                std::cout << QJsonDocument(arr).toJson(QJsonDocument::Compact).constData() << std::endl;
            } else {
                for (auto item : result) outpuSTD(item);
            }
            return 0;
        }
        if (isSetKey()) {
            const auto &language = parser.value(languageOption);

            if (method == "value") {
                QVariant result = manager->value(key);
                if (jsonOut) {
                    QJsonObject obj;
                    obj["key"] = key;
                    obj["value"] = result.toString();
                    obj["appid"] = appid;
                    obj["resource"] = resourceid;
                    std::cout << QJsonDocument(obj).toJson(QJsonDocument::Compact).constData() << std::endl;
                } else {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                if (result.typeId() == QMetaType::Bool) {
                    outpuSTD(result.toBool());
                } else if (result.typeId() == QMetaType::Double) {
#else
                if (result.type() == QVariant::Bool) {
                    outpuSTD(result.toBool());
                } else if (result.type() == QVariant::Double) {
#endif
                    outpuSTD(QString::number(result.toDouble()));
                } else {
                    outpuSTD(QString("\"%1\"").arg(qvariantToString(result)));
                }
                } // end else (non-json)
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
            } else if (method == "isDefaultValue") {
                bool result = manager->isDefaultValue(key);
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

int CommandManager::setCommand()
{
    // set命令，设置指定配置项
    if (!isSetAppid() || !isSetResourceid() || !isSetKey()
            ||!parser.isSet(valueOption)) {
        outpuSTDError("not set appid, resource, key or value.");
        return 1;
    }

    if (!existResource(appid, resourceid)) {
        outpuSTDError(QString("not exist resouce:[%1] for the appid:[%2]").arg(resourceid).arg(appid));
        return 1;
    }

    const auto &value = parser.value(valueOption);
    ValueHandler handler(uid, appid, resourceid, subpathid);
    {
        QScopedPointer<ConfigGetter> manager(handler.createManager());
        if (manager) {
            QVariant result = manager->value(key);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            if (result.typeId() == QMetaType::Bool) {
                manager->setValue(key, QVariant(value).toBool());
            } else if (result.typeId() == QMetaType::Double) {
#else
            if (result.type() == QVariant::Bool) {
                manager->setValue(key, QVariant(value).toBool());
            } else if (result.type() == QVariant::Double) {
#endif
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

int CommandManager::resetCommand()
{
    // reset命令，设置指定配置项
    if (!isSetAppid() || !isSetResourceid()) {
        outpuSTDError("not set appid, resource");
        return 1;
    }

    if (!existResource(appid, resourceid)) {
        outpuSTDError(QString("not exist resouce:[%1] for the appid:[%2]").arg(resourceid).arg(appid));
        return 1;
    }

    ValueHandler handler(uid, appid, resourceid, subpathid);
    {
        QScopedPointer<ConfigGetter> manager(handler.createManager());
        if (manager) {
            if (isSetKey()) {
                manager->reset(key);
            } else {
                const auto keys = manager->keyList();
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                for (const auto &item : std::as_const(keys)) {
#else
                for (const auto &item : qAsConst(keys)) {
#endif
                    manager->reset(item);
                }
            }
        } else {
            outpuSTDError(QString("not create value handler for appid=%1, resource=%2, subpath=%3.").arg(appid, resourceid, subpathid));
            return 1;
        }
    }
    return 0;
}

int CommandManager::watchCommand()
{
    // watch命令，监控一些配置项改变信号
    if (!isSetAppid() || !isSetResourceid()) {
        outpuSTDError("not set appid or resource.");
        return 1;
    }

    if (!existResource(appid, resourceid)) {
        outpuSTDError(QString("not exist resouce:[%1] for the appid:[%2]").arg(resourceid).arg(appid));
        return 1;
    }

    ValueHandler handler(uid, appid, resourceid, subpathid);
    QScopedPointer<ConfigGetter> manager(handler.createManager());
    const bool jsonOut = (outputFormat == "json");
    if (manager) {
        const auto &matchKey = key;
        QObject::connect(&handler, &ValueHandler::valueChanged, [matchKey, jsonOut, this](const QString &changedKey){
            QRegularExpression re(matchKey);
            if (!matchKey.isEmpty() && !re.match(changedKey).hasMatch())
                return;
            if (jsonOut) {
                QJsonObject obj;
                obj["key"] = changedKey;
                obj["appid"] = appid;
                obj["resource"] = resourceid;
                std::cout << QJsonDocument(obj).toJson(QJsonDocument::Compact).constData() << std::endl;
            } else {
                outpuSTD(changedKey);
            }
        });
    } else {
        outpuSTDError(QString("not create value handler for appid=%1, resource=%2, subpath=%3.").arg(appid, resourceid, subpathid));
        return 1;
    }
    return qApp->exec();
}

// T09-3: export snapshot
int CommandManager::exportCommand()
{
    if (!isSetAppid() || !isSetResourceid()) {
        outpuSTDError("export requires -a <appid> -r <resource> -o <file>");
        return 1;
    }
    if (snapshotFile.isEmpty()) {
        outpuSTDError("export requires --file/-o <snapshot.json>");
        return 1;
    }
    if (!existResource(appid, resourceid)) {
        outpuSTDError(QString("not exist resouce:[%1] for the appid:[%2]").arg(resourceid).arg(appid));
        return 1;
    }

    ValueHandler handler(uid, appid, resourceid, subpathid);
    QScopedPointer<ConfigGetter> manager(handler.createManager());
    if (!manager) {
        outpuSTDError("failed to create manager");
        return 1;
    }

    QJsonObject snapshot;
    snapshot["appid"] = appid;
    snapshot["resource"] = resourceid;
    snapshot["subpath"] = subpathid;
    QJsonObject values;
    for (const auto &k : manager->keyList()) {
        values[k] = manager->value(k).toString();
    }
    snapshot["values"] = values;

    QFile f(snapshotFile);
    if (!f.open(QIODevice::WriteOnly)) {
        outpuSTDError(QString("cannot open file: %1").arg(snapshotFile));
        return 1;
    }
    f.write(QJsonDocument(snapshot).toJson());
    qInfo("Exported %d keys to %s", values.size(), qPrintable(snapshotFile));
    return 0;
}

// T09-3: import snapshot
int CommandManager::importCommand()
{
    if (snapshotFile.isEmpty()) {
        outpuSTDError("import requires --file/-o <snapshot.json>");
        return 1;
    }
    QFile f(snapshotFile);
    if (!f.open(QIODevice::ReadOnly)) {
        outpuSTDError(QString("cannot open file: %1").arg(snapshotFile));
        return 1;
    }
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError) {
        outpuSTDError(QString("JSON parse error: %1").arg(err.errorString()));
        return 1;
    }
    QJsonObject snapshot = doc.object();
    const QString importAppid    = appid.isEmpty()    ? snapshot["appid"].toString()    : appid;
    const QString importResource = resourceid.isEmpty()? snapshot["resource"].toString(): resourceid;
    const QString importSubpath  = subpathid.isEmpty() ? snapshot["subpath"].toString() : subpathid;

    if (!existResource(importAppid, importResource)) {
        outpuSTDError(QString("not exist resouce:[%1] for the appid:[%2]").arg(importResource).arg(importAppid));
        return 1;
    }

    ValueHandler handler(uid, importAppid, importResource, importSubpath);
    QScopedPointer<ConfigGetter> manager(handler.createManager());
    if (!manager) {
        outpuSTDError("failed to create manager");
        return 1;
    }
    const QJsonObject values = snapshot["values"].toObject();
    int count = 0;
    for (auto it = values.begin(); it != values.end(); ++it) {
        manager->setValue(it.key(), it.value().toVariant());
        count++;
    }
    qInfo("Imported %d keys from %s", count, qPrintable(snapshotFile));
    return 0;
}

int main(int argc, char *argv[])
{
    setenv("DSG_DATA_DIRS", "/usr/share/dsg:/var/lib/linglong/entries/share/dsg", 0);

    QCoreApplication a(argc, argv);
    a.setApplicationVersion(VERSION);

    loadTranslation("dde-dconfig");

    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::translate("main", "A console tool to get and set configuration items for DTK Config."));
    parser.addHelpOption();

    QCommandLineOption uidOption("u", QCoreApplication::translate("main", "operate configure items of the user uid."), "uid", QString());
    parser.addOption(uidOption);

    QCommandLineOption appidOption("a", QCoreApplication::translate("main", "appid for a specific application, \n"
                                                                            "it is empty string if we need to manage application independent configuration.\n"
                                                                            "second positional argument as appid if not the option"), "appid", QString());
    parser.addOption(appidOption);

    QCommandLineOption resourceOption("r", QCoreApplication::translate("main", "resource id for configure name, \n"
                                                                               "it's value is same as `a` option if not the option."), "resource", QString());
    parser.addOption(resourceOption);

    QCommandLineOption subpathOption("s", QCoreApplication::translate("main", "subpath for configure."), "subpath", QString());
    parser.addOption(subpathOption);

    QCommandLineOption keyOption("k", QCoreApplication::translate("main", "configure item's key.\n"
                                                                          "three positional argument as key if not the option"), "key", QString());
    parser.addOption(keyOption);

    QCommandLineOption valueOption("v", QCoreApplication::translate("main", "new value to set configure item."), "value", QString());
    parser.addOption(valueOption);

    QCommandLineOption localPrefixOption("p", QCoreApplication::translate("main", "working prefix directory."), "prefix", QString());
    parser.addOption(localPrefixOption);

    QCommandLineOption methodOption("m", QCoreApplication::translate("main", "method for the configure item."), "method", QString("value"));
    parser.addOption(methodOption);

    QCommandLineOption languageOption("l", QCoreApplication::translate("main", "language for the configuration item."), "language", QString());
    parser.addOption(languageOption);

    QCommandLineOption listOption("list", QCoreApplication::translate("main", "list configure information with appid, resource or subpath."));
    listOption.setFlags(listOption.flags() ^ QCommandLineOption::HiddenFromHelp);
    parser.addOption(listOption);

    QCommandLineOption getOption("get", QCoreApplication::translate("main", "query content for configure, including the configure item's all keys, value ..."));
    getOption.setFlags(getOption.flags() ^ QCommandLineOption::HiddenFromHelp);
    parser.addOption(getOption);

    QCommandLineOption setOption("set", QCoreApplication::translate("main", "set configure item 's value."));
    setOption.setFlags(setOption.flags() ^ QCommandLineOption::HiddenFromHelp);
    parser.addOption(setOption);

    QCommandLineOption resetOption("reset", QCoreApplication::translate("main", "reset configure item's value, reset all configure item's value if not `-k` option."));
    resetOption.setFlags(resetOption.flags() ^ QCommandLineOption::HiddenFromHelp);
    parser.addOption(resetOption);

    QCommandLineOption watchOption("watch", QCoreApplication::translate("main", "watch value changed for some configure item."));
    watchOption.setFlags(watchOption.flags() ^ QCommandLineOption::HiddenFromHelp);
    parser.addOption(watchOption);

    QCommandLineOption guiOption("gui", QCoreApplication::translate("main", "start dde-dconfig-editor as a gui configure tool."));
    guiOption.setFlags(guiOption.flags() ^ QCommandLineOption::HiddenFromHelp);
    parser.addOption(guiOption);

    // T09-1: --output json|text
    QCommandLineOption outputOption(QStringList{"output", "O"},
        QCoreApplication::translate("main", "output format: text (default) or json."), "format", "text");
    parser.addOption(outputOption);

    // T09-3: --file for export/import
    QCommandLineOption fileOption(QStringList{"file", "o"},
        QCoreApplication::translate("main", "snapshot file path for export/import."), "file", QString());
    parser.addOption(fileOption);

    QCommandLineOption exportOption("export", QCoreApplication::translate("main", "export all config values to a JSON snapshot file."));
    exportOption.setFlags(exportOption.flags() ^ QCommandLineOption::HiddenFromHelp);
    parser.addOption(exportOption);

    QCommandLineOption importOption("import", QCoreApplication::translate("main", "import config values from a JSON snapshot file."));
    importOption.setFlags(importOption.flags() ^ QCommandLineOption::HiddenFromHelp);
    parser.addOption(importOption);


    // support positional argument for subcommand.
    parser.addPositionalArgument(listOption.names().constFirst(), listOption.description(), "\n list: dde-dconfig list \n");
    parser.addPositionalArgument(getOption.names().constFirst(), getOption.description(), "get: dde-dconfig get -a dconfig-example -r example -k key1 \n");
    parser.addPositionalArgument(setOption.names().constFirst(), setOption.description(), "set: dde-dconfig set -a dconfig-example -r example -k key1 -v 1 \n");
    parser.addPositionalArgument(resetOption.names().constFirst(), resetOption.description(), "reset: dde-dconfig reset -a dconfig-example -r example -k key1 \n");
    parser.addPositionalArgument(watchOption.names().constFirst(), watchOption.description(), "watch: dde-dconfig watch -a dconfig-example -r example -k key1 \n");
    parser.addPositionalArgument(guiOption.names().constFirst(), guiOption.description(), "gui: dde-dconfig gui \n");

    parser.process(a);

    if (argc == 1) {
        parser.showHelp(0);
    }

    const auto positions = parser.positionalArguments();
    const auto subcommand = positions.isEmpty() ? QString() : positions.constFirst();

    CommandManager manager {parser, uidOption, appidOption, resourceOption, subpathOption, keyOption, methodOption, languageOption, valueOption, outputOption, fileOption};
    if (parser.isSet(guiOption) || guiOption.names().contains(subcommand)) {
        const QString guiTool("dde-dconfig-editor");
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
        return QProcess::startDetached(guiTool);
#else
        return QProcess::startDetached(guiTool, {});
#endif
    }
    if (parser.isSet(listOption) || listOption.names().contains(subcommand)) {
        return manager.listCommand();
    } else if (parser.isSet(getOption) || getOption.names().contains(subcommand)) {
        return manager.getCommand();
    } else if (parser.isSet(setOption) || setOption.names().contains(subcommand)) {
        return manager.setCommand();
    } else if (parser.isSet(resetOption) || resetOption.names().contains(subcommand)) {
        return manager.resetCommand();
    } else if (parser.isSet(watchOption) || watchOption.names().contains(subcommand)) {
        return manager.watchCommand();
    } else if (parser.isSet(exportOption) || exportOption.names().contains(subcommand)) {
        return manager.exportCommand();
    } else if (parser.isSet(importOption) || importOption.names().contains(subcommand)) {
        return manager.importCommand();
    }

    parser.showHelp(0);
}
