// SPDX-FileCopyrightText: 2021 - 2023 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QString>
#include <QSet>
#include <QList>
#include <QDir>
#include <QDirIterator>
#include <QDBusArgument>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonArray>
#include <DConfigFile>
#include <dstandardpaths.h>
#include <QCoreApplication>
#include <QTranslator>

using ResourceId = QString;
using ResourcePath = QString;
using AppId = QString;
using SubpathKey = QString;
using ResourceList = QList<ResourceId>;
using AppList = QList<AppId>;
using SubpathList = QList<SubpathKey>;
using UserInfo = QPair<QString, int>;

static const QString &SUFFIX = QString(".json");
constexpr int ConfigUserRole = Qt::UserRole + 10;
constexpr int AppidRole = Qt::UserRole + 11;
constexpr int ResourceRole = Qt::UserRole + 12;
constexpr int SubpathRole = Qt::UserRole + 13;
constexpr int KeyRole = Qt::UserRole + 14;
constexpr int ValueRole = Qt::UserRole + 15;
constexpr int DescriptionRole = Qt::UserRole + 16;
constexpr int FlagsRole = Qt::UserRole + 17;

static const QString NoAppId;
static const QString VirtualAppName("virtual-generic-applicaiton");

enum ConfigType {
    InvalidType = 0x00,
    AppType = 0x10,
    ResourceType = 0x20,
    AppResourceType = ResourceType | 0x01,
    CommonResourceType = ResourceType | 0x02,
    SubpathType = 0x30,
    KeyType = 0x40,
};

static AppList applications(const QString &localPrefix = QString())
{
    AppList result;
    result << NoAppId;

    result.reserve(50);

    // TODO calling service interface to get app list,
    // and now we can't distingush between `subpath` or `appid` for common configuration.
    using namespace Dtk::Core;
    QStringList appDirs = DConfigMeta::genericMetaDirs(localPrefix);
    const QStringList filterDirs {"overrides"};
    for (auto item : appDirs)
    {
        QDir appsDir(item);

        for (auto appid : appsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            if (filterDirs.contains(appid))
                continue;

            result.append(appid);
        }
    }
    return result;
}

static QSet<ResourcePath> resourcePathsForDirectory(const QString &dir)
{
    QSet<ResourcePath> result;
    QDirIterator iterator(dir, QDir::Files);
    while(iterator.hasNext()) {
        iterator.next();
        const QFileInfo &file(iterator.fileInfo());

        if (!file.fileName().endsWith(SUFFIX))
            continue;

        result.insert(file.absoluteFilePath());
    }
    return result;
}

static ResourceList resourcePathsForApp(const QString &appid, const QString &localPrefix = QString())
{
    QSet<ResourcePath> result;
    result.reserve(50);
    using namespace Dtk::Core;
    for (auto item : DConfigMeta::applicationMetaDirs(localPrefix, appid)) {
        result += resourcePathsForDirectory(item);
    }
    return result.values();
}

static ResourceList resourcesForApp(const QString &appid, const QString &localPrefix = QString())
{
    QSet<ResourceId> result;
    result.reserve(50);
    for (auto item : resourcePathsForApp(appid, localPrefix)) {
        const QFileInfo file(item);

        ResourceId resourceName = file.fileName().chopped(SUFFIX.size());
        result.insert(resourceName);
    }
    return result.values();
}

static ResourceList resourcesForAllApp(const QString &localPrefix = QString())
{
    QSet<ResourceId> result;
    result.reserve(50);
    using namespace Dtk::Core;
    for (auto item : DConfigMeta::genericMetaDirs(localPrefix)) {
        for (auto resourcePath : resourcePathsForDirectory(item)) {
            const QFileInfo file(resourcePath);

            ResourceId resourceName = file.fileName().chopped(SUFFIX.size());
            result.insert(resourceName);
        }
    }
    return result.values();
}

static SubpathList subpathsForResource(const AppId &appid, const ResourceId &resourceId, const QString &localPrefix = QString())
{
    SubpathList result;
    for (auto item : resourcePathsForApp(appid, localPrefix)) {
        QDir resourceDir(QFileInfo(item).absoluteDir());
        auto filters = QDir::Dirs | QDir::NoDotAndDotDot;
        resourceDir.setFilter(filters);
        QDirIterator iterator(resourceDir, QDirIterator::Subdirectories);

        while(iterator.hasNext()) {
            iterator.next();
            const QFileInfo &file(iterator.fileInfo());
            if (QDir(file.absoluteFilePath()).exists(resourceId + SUFFIX)) {
                auto subpath = file.absoluteFilePath().replace(resourceDir.absolutePath(), "");
                if (result.contains(subpath))
                    continue;
                result.append(subpath);
            }
        }
    }
    return result;
}

static bool existAppid(const QString &appid, const QString &localPrefix = QString())
{
    return !resourcesForApp(appid, localPrefix).isEmpty();
}

static bool existResource(const AppId &appid, const ResourceId &resourceId, const QString &localPrefix = QString())
{
    const ResourceList &resources = resourcesForApp(appid, localPrefix);
    if (resources.contains(resourceId))
        return true;

    const ResourceList &commons = resourcesForAllApp(localPrefix);
    if (commons.contains(resourceId)) {
        return true;
    }

    return false;
}

static QVariant decodeQDBusArgument(const QVariant &v)
{
    if (v.canConvert<QDBusArgument>()) {
        // we use QJsonValue to resolve all data type in DConfigInfo class, so it's type is equal QJsonValue::Type,
        // now we parse Map and Array type to QVariant explicitly.
        const QDBusArgument &complexType = v.value<QDBusArgument>();
        switch (complexType.currentType()) {
        case QDBusArgument::MapType: {
            QVariantMap list;
            complexType >> list;
            QVariantMap res;
            for (auto iter = list.begin(); iter != list.end(); iter++) {
                res[iter.key()] = decodeQDBusArgument(iter.value());
            }
            return res;
        }
        case QDBusArgument::ArrayType: {
            QVariantList list;
            complexType >> list;
            QVariantList res;
            res.reserve(list.size());
            for (auto iter = list.begin(); iter != list.end(); iter++) {
                res << decodeQDBusArgument(*iter);
            }
            return res;
        }
        default:
            qWarning("Can't parse the type, it maybe need user to do it, "
                     "QDBusArgument::ElementType: %d.", complexType.currentType());
        }
    }
    return v;
}

static QString qvariantToString(const QVariant &v)
{
    const auto &doc = QJsonDocument::fromVariant(v);
    return doc.isNull() ? v.toString() : doc.toJson();
}

static QString qvariantToStringCompact(const QVariant &v)
{
    const auto &doc = QJsonDocument::fromVariant(v);
    return doc.isNull() ? v.toString() : doc.toJson(QJsonDocument::Compact);
}

static QVariant stringToQVariant(const QString &s)
{
    QJsonParseError error;
    const auto &doc = QJsonDocument::fromJson(s.toUtf8(), &error);
    if (error.error == QJsonParseError::NoError)
        return doc.toVariant();
    
    // 将单个JSON值包装成数组后解析（支持数字、布尔、null、字符串等）
    const QString wrappedJson = QString("[") + s + QString("]");
    const auto &wrappedDoc = QJsonDocument::fromJson(wrappedJson.toUtf8(), &error);
    if (error.error == QJsonParseError::NoError) {
        QJsonArray array = wrappedDoc.array();
        if (!array.isEmpty())
            return array.first().toVariant();
    }
    
    // 解析失败则作为普通字符串处理
    return s;
}

static bool isValidTextJsonValue(const QString &s)
{
    QJsonParseError error;
    
    // 首先尝试作为JSON文档解析（对象或数组，包括多行格式）
    QJsonDocument::fromJson(s.toUtf8(), &error);
    if (error.error == QJsonParseError::NoError)
        return true;
    
    // 尝试包装成数组解析（支持数字、布尔、null、带引号的JSON字符串）
    const QString wrappedJson = QString("[") + s + QString("]");
    QJsonDocument::fromJson(wrappedJson.toUtf8(), &error);
    if (error.error == QJsonParseError::NoError)
        return true;
    
    // 如果字符串看起来像JSON结构（以{或[开头）但解析失败，说明是格式错误的JSON
    // 例如: [value1","value2"] 或 {"key":value} 等
    // 但普通文本如 "use {key} here" 不会被拒绝
    QString trimmed = s.trimmed();
    if (!trimmed.isEmpty() && (trimmed[0] == '{' || trimmed[0] == '['))
        return false;
    
    // 其他情况作为普通字符串接受
    return true;
}

static QString qvariantToCmd(const QVariant &v)
{
    auto stringValue = qvariantToStringCompact(v);
    auto jsonValue = QJsonValue::fromVariant(v);
    return jsonValue.isBool() || jsonValue.isDouble() ? stringValue : QString("'%1'").arg(stringValue);
}

static QStringList translationDirs()
{
    QStringList result;
    result << QCoreApplication::applicationDirPath();
    for (auto item : Dtk::Core::DStandardPaths::standardLocations(QStandardPaths::AppLocalDataLocation)) {
        result << item + "/translations";
    };
    return result;
}

static void loadTranslation(const QString &fileName)
{
    auto translator = new QTranslator(QCoreApplication::instance());
    for (auto item :translationDirs()) {
        if (translator->load(QLocale::system(), fileName, "_", item, ".qm")) {
            QCoreApplication::installTranslator(translator);
            break;
        }
    }
}

static QList<UserInfo> fetchUserInfos()
{
    QList<UserInfo> res;
    QFile file("/etc/passwd");
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    while (!file.atEnd()) {
        QByteArray line = file.readLine();
        const auto user = line.split(':');
        if (user.count() != 7)
            continue;
        const auto loginin = user.at(6);
        if (loginin.contains("nologin"))
            continue;

        res << UserInfo{user[0], user[2].toInt()};
    }
    return res;
}
