// SPDX-FileCopyrightText: 2021 - 2026 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <gtest/gtest.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCoreApplication>
#include <QDBusArgument>
#include <QDBusVariant>
#include <QJsonDocument>
#include <QJsonValue>
#include <QTranslator>
#include <QStandardPaths>

#include <DConfigFile>

#include "dconfig_global.h"
#include "test_helper.hpp"

static const QString LocalPrefix = QDir::tempPath() + "/dde-app-services-test-" +
    QString::number(QCoreApplication::applicationPid()) + "/";
static constexpr char const *APP_ID = "org.foo.appid";
static constexpr char const *FILE_NAME = "example";

static EnvGuard dsgDataDir;
static EnvGuard stateDir;

class ut_helper : public testing::Test
{
protected:
    static void SetUpTestCase() {
        // App-specific config
        const QString appDir = QString("%1/usr/share/dsg/configs/%2").arg(LocalPrefix, APP_ID);
        QDir().mkpath(appDir);
        ASSERT_TRUE(QFile::copy(":/config/example.json", appDir + "/" + FILE_NAME + ".json"));

        // Subpath config for subpathsForResource testing
        const QString subDir = appDir + "/subdir";
        QDir().mkpath(subDir);
        ASSERT_TRUE(QFile::copy(":/config/example.json", subDir + "/" + FILE_NAME + ".json"));

        // Generic (NoAppId) config
        const QString genericDir = QString("%1/usr/share/dsg/configs").arg(LocalPrefix);
        ASSERT_TRUE(QFile::copy(":/config/example.json", genericDir + "/" + FILE_NAME + ".json"));

        qputenv("DSG_CONFIG_CONNECTION_DISABLE_DBUS", "true");
        dsgDataDir.set("DSG_DATA_DIRS", "/usr/share/dsg");
        stateDir.set("STATE_DIRECTORY", LocalPrefix.toLocal8Bit());
    }
    static void TearDownTestCase() {
        qunsetenv("DSG_CONFIG_CONNECTION_DISABLE_DBUS");
        dsgDataDir.restore();
        stateDir.restore();
        QDir(LocalPrefix).removeRecursively();
    }
    virtual void SetUp() override {}
    virtual void TearDown() override {}
};

// ---------------------------------------------------------------------------
// applications
// ---------------------------------------------------------------------------
TEST_F(ut_helper, applications_returnsNoAppIdAndAppDirs) {
    auto apps = applications(LocalPrefix);
    ASSERT_TRUE(apps.contains(NoAppId));
    ASSERT_TRUE(apps.contains(APP_ID));
}

TEST_F(ut_helper, applications_defaultPrefix) {
    auto apps = applications();
    ASSERT_TRUE(apps.contains(NoAppId));
}

TEST_F(ut_helper, applications_emptyPrefix) {
    auto apps = applications("");
    ASSERT_TRUE(apps.contains(NoAppId));
}

// ---------------------------------------------------------------------------
// resourcePathsForDirectory
// ---------------------------------------------------------------------------
TEST_F(ut_helper, resourcePathsForDirectory_returnsJsonFiles) {
    const QString dir = QString("%1/usr/share/dsg/configs/%2").arg(LocalPrefix, APP_ID);
    auto paths = resourcePathsForDirectory(dir);
    ASSERT_FALSE(paths.isEmpty());
    for (const auto &p : paths) {
        ASSERT_TRUE(p.endsWith(".json"));
    }
}

TEST_F(ut_helper, resourcePathsForDirectory_emptyDir) {
    const QString dir = QString("%1/usr/share/dsg/configs/nonexistent_dir").arg(LocalPrefix);
    auto paths = resourcePathsForDirectory(dir);
    ASSERT_TRUE(paths.isEmpty());
}

TEST_F(ut_helper, resourcePathsForDirectory_ignoresNonJson) {
    const QString dir = QString("%1/usr/share/dsg/configs/%2").arg(LocalPrefix, APP_ID);
    QFile txtFile(dir + "/notjson.txt");
    txtFile.open(QIODevice::WriteOnly);
    txtFile.write("test");
    txtFile.close();

    auto paths = resourcePathsForDirectory(dir);
    for (const auto &p : paths) {
        ASSERT_TRUE(p.endsWith(".json"));
        ASSERT_FALSE(p.endsWith(".txt"));
    }
    QFile::remove(dir + "/notjson.txt");
}

// ---------------------------------------------------------------------------
// resourcePathsForApp
// ---------------------------------------------------------------------------
TEST_F(ut_helper, resourcePathsForApp_returnsPaths) {
    auto paths = resourcePathsForApp(APP_ID, LocalPrefix);
    ASSERT_FALSE(paths.isEmpty());
    bool found = false;
    for (const auto &p : paths) {
        if (p.contains(APP_ID) && p.contains(FILE_NAME + QString(".json")))
            found = true;
    }
    ASSERT_TRUE(found);
}

TEST_F(ut_helper, resourcePathsForApp_nonExistentApp) {
    auto paths = resourcePathsForApp("nonexistent.app", LocalPrefix);
    ASSERT_TRUE(paths.isEmpty());
}

// ---------------------------------------------------------------------------
// resourcesForApp
// ---------------------------------------------------------------------------
TEST_F(ut_helper, resourcesForApp_returnsResourceNames) {
    auto resources = resourcesForApp(APP_ID, LocalPrefix);
    ASSERT_TRUE(resources.contains(FILE_NAME));
}

TEST_F(ut_helper, resourcesForApp_nonExistentApp) {
    auto resources = resourcesForApp("nonexistent.app", LocalPrefix);
    ASSERT_TRUE(resources.isEmpty());
}

// ---------------------------------------------------------------------------
// resourcesForAllApp
// ---------------------------------------------------------------------------
TEST_F(ut_helper, resourcesForAllApp_returnsGenericResources) {
    auto resources = resourcesForAllApp(LocalPrefix);
    ASSERT_TRUE(resources.contains(FILE_NAME));
}

TEST_F(ut_helper, resourcesForAllApp_defaultPrefix) {
    // Default prefix uses system DSG data dirs; verify deterministic result
    auto resources1 = resourcesForAllApp();
    auto resources2 = resourcesForAllApp();
    ASSERT_EQ(resources1.size(), resources2.size());
}

// ---------------------------------------------------------------------------
// availableResourcesForApp
// ---------------------------------------------------------------------------
TEST_F(ut_helper, availableResourcesForApp_withAppId) {
    auto resources = availableResourcesForApp(APP_ID, LocalPrefix);
    ASSERT_TRUE(resources.contains(FILE_NAME));
}

TEST_F(ut_helper, availableResourcesForApp_emptyAppid) {
    auto resources = availableResourcesForApp(NoAppId, LocalPrefix);
    ASSERT_TRUE(resources.contains(FILE_NAME));
}

TEST_F(ut_helper, availableResourcesForApp_nonExistentApp) {
    auto resources = availableResourcesForApp("nonexistent.app", LocalPrefix);
    ASSERT_TRUE(resources.contains(FILE_NAME));
}

// ---------------------------------------------------------------------------
// subpathsForResource
// ---------------------------------------------------------------------------
TEST_F(ut_helper, subpathsForResource_returnsSubpaths) {
    auto subpaths = subpathsForResource(APP_ID, FILE_NAME, LocalPrefix);
    ASSERT_FALSE(subpaths.isEmpty());
    bool foundSub = false;
    for (const auto &sp : subpaths) {
        if (sp.contains("subdir"))
            foundSub = true;
    }
    ASSERT_TRUE(foundSub);
}

TEST_F(ut_helper, subpathsForResource_noSubpaths) {
    auto subpaths = subpathsForResource(APP_ID, "nonexistent_resource", LocalPrefix);
    ASSERT_TRUE(subpaths.isEmpty());
}

// ---------------------------------------------------------------------------
// existAppid
// ---------------------------------------------------------------------------
TEST_F(ut_helper, existAppid_true) {
    ASSERT_TRUE(existAppid(APP_ID, LocalPrefix));
}

TEST_F(ut_helper, existAppid_false) {
    ASSERT_FALSE(existAppid("nonexistent.app", LocalPrefix));
}

// ---------------------------------------------------------------------------
// existResource
// ---------------------------------------------------------------------------
TEST_F(ut_helper, existResource_appSpecific_true) {
    ASSERT_TRUE(existResource(APP_ID, FILE_NAME, LocalPrefix));
}

TEST_F(ut_helper, existResource_generic_true) {
    ASSERT_TRUE(existResource(NoAppId, FILE_NAME, LocalPrefix));
}

TEST_F(ut_helper, existResource_nonExistentResource_false) {
    ASSERT_FALSE(existResource(APP_ID, "nonexistent_resource", LocalPrefix));
}

TEST_F(ut_helper, existResource_nonExistentApp_false) {
    ASSERT_FALSE(existResource("nonexistent.app", "nonexistent_resource", LocalPrefix));
}

// ---------------------------------------------------------------------------
// decodeQDBusArgument
// ---------------------------------------------------------------------------
TEST_F(ut_helper, decodeQDBusArgument_plainVariant) {
    QVariant v(42);
    auto result = decodeQDBusArgument(v);
    ASSERT_EQ(result.toInt(), 42);
}

TEST_F(ut_helper, decodeQDBusArgument_stringVariant) {
    QVariant v("hello");
    auto result = decodeQDBusArgument(v);
    ASSERT_EQ(result.toString(), "hello");
}

// QDBusArgument created in write mode (not from a real D-Bus message)
// returns UnknownType from currentType(), so decodeQDBusArgument hits the
// default branch and returns the original variant unchanged. MapType and
// ArrayType branches require a QDBusArgument in read mode (from an actual
// D-Bus message), which cannot be constructed without a D-Bus daemon.
TEST_F(ut_helper, decodeQDBusArgument_writeMode_defaultBranch) {
    QDBusArgument arg;
    arg.beginMap(QMetaType::QString, QMetaType::QVariant);
    arg.beginMapEntry();
    arg << QString("key1");
    arg << QDBusVariant("value1");
    arg.endMapEntry();
    arg.endMap();

    QVariant v = QVariant::fromValue(arg);
    ASSERT_TRUE(v.canConvert<QDBusArgument>());
    auto result = decodeQDBusArgument(v);
    // Default branch returns the original variant unchanged
    ASSERT_TRUE(result.canConvert<QDBusArgument>());
}

TEST_F(ut_helper, decodeQDBusArgument_nonQDBusArgument) {
    QVariantMap map;
    map.insert("key1", "value1");
    QVariant v(map);
    ASSERT_FALSE(v.canConvert<QDBusArgument>());
    auto result = decodeQDBusArgument(v);
    // Non-QDBusArgument variants pass through unchanged
    ASSERT_EQ(result.toMap().value("key1").toString(), "value1");
}

// ---------------------------------------------------------------------------
// qvariantToString
// ---------------------------------------------------------------------------
TEST_F(ut_helper, qvariantToString_map) {
    QVariantMap map;
    map["key"] = "value";
    QString result = qvariantToString(map);
    ASSERT_TRUE(result.contains("key"));
    ASSERT_TRUE(result.contains("value"));
}

TEST_F(ut_helper, qvariantToString_string) {
    QVariant v("hello");
    QString result = qvariantToString(v);
    ASSERT_EQ(result, "hello");
}

TEST_F(ut_helper, qvariantToString_int) {
    QVariant v(42);
    QString result = qvariantToString(v);
    ASSERT_EQ(result, "42");
}

// ---------------------------------------------------------------------------
// qvariantToStringCompact
// ---------------------------------------------------------------------------
TEST_F(ut_helper, qvariantToStringCompact_map) {
    QVariantMap map;
    map["key"] = "value";
    QString result = qvariantToStringCompact(map);
    ASSERT_FALSE(result.contains("\n"));
    ASSERT_TRUE(result.contains("key"));
}

TEST_F(ut_helper, qvariantToStringCompact_string) {
    QVariant v("hello");
    QString result = qvariantToStringCompact(v);
    ASSERT_EQ(result, "hello");
}

// ---------------------------------------------------------------------------
// stringToQVariant
// ---------------------------------------------------------------------------
TEST_F(ut_helper, stringToQVariant_validJson) {
    auto result = stringToQVariant("{\"key\":\"value\"}");
    ASSERT_TRUE(result.canConvert<QVariantMap>());
    ASSERT_EQ(result.toMap().value("key").toString(), "value");
}

TEST_F(ut_helper, stringToQVariant_invalidJson) {
    auto result = stringToQVariant("hello");
    ASSERT_EQ(result.toString(), "hello");
}

TEST_F(ut_helper, stringToQVariant_validJsonArray) {
    auto result = stringToQVariant("[1, 2, 3]");
    ASSERT_TRUE(result.canConvert<QVariantList>());
    ASSERT_EQ(result.toList().size(), 3);
}

// ---------------------------------------------------------------------------
// validateTextInput
// ---------------------------------------------------------------------------
TEST_F(ut_helper, validateTextInput_empty) {
    QString errorMsg;
    ASSERT_TRUE(validateTextInput("", errorMsg));
    ASSERT_TRUE(validateTextInput("   ", errorMsg));
}

TEST_F(ut_helper, validateTextInput_plainText) {
    QString errorMsg;
    ASSERT_TRUE(validateTextInput("hello world", errorMsg));
    ASSERT_TRUE(errorMsg.isEmpty());
}

TEST_F(ut_helper, validateTextInput_validJsonObject) {
    QString errorMsg;
    ASSERT_TRUE(validateTextInput("{\"key\":\"value\"}", errorMsg));
    ASSERT_TRUE(errorMsg.isEmpty());
}

TEST_F(ut_helper, validateTextInput_validJsonArray) {
    QString errorMsg;
    ASSERT_TRUE(validateTextInput("[1, 2, 3]", errorMsg));
    ASSERT_TRUE(errorMsg.isEmpty());
}

TEST_F(ut_helper, validateTextInput_invalidJsonObject) {
    QString errorMsg;
    ASSERT_FALSE(validateTextInput("{key: value}", errorMsg));
    ASSERT_FALSE(errorMsg.isEmpty());
}

TEST_F(ut_helper, validateTextInput_invalidJsonArray) {
    QString errorMsg;
    ASSERT_FALSE(validateTextInput("[1, 2,]", errorMsg));
    ASSERT_FALSE(errorMsg.isEmpty());
}

// ---------------------------------------------------------------------------
// qvariantToCmd
// ---------------------------------------------------------------------------
TEST_F(ut_helper, qvariantToCmd_bool) {
    ASSERT_EQ(qvariantToCmd(QVariant(true)), "true");
    ASSERT_EQ(qvariantToCmd(QVariant(false)), "false");
}

TEST_F(ut_helper, qvariantToCmd_int) {
    QString result = qvariantToCmd(QVariant(42));
    ASSERT_FALSE(result.startsWith("'"));
    ASSERT_TRUE(result.contains("42"));
}

TEST_F(ut_helper, qvariantToCmd_double) {
    QString result = qvariantToCmd(QVariant(3.14));
    ASSERT_FALSE(result.startsWith("'"));
}

TEST_F(ut_helper, qvariantToCmd_string) {
    QString result = qvariantToCmd(QVariant("hello"));
    ASSERT_TRUE(result.startsWith("'"));
    ASSERT_TRUE(result.endsWith("'"));
    ASSERT_TRUE(result.contains("hello"));
}

// ---------------------------------------------------------------------------
// translationDirs
// ---------------------------------------------------------------------------
TEST_F(ut_helper, translationDirs_nonEmpty) {
    auto dirs = translationDirs();
    ASSERT_FALSE(dirs.isEmpty());
    ASSERT_TRUE(dirs.contains(QCoreApplication::applicationDirPath()));
}

// ---------------------------------------------------------------------------
// loadTranslation
// ---------------------------------------------------------------------------
TEST_F(ut_helper, loadTranslation_nonExistent_noCrash) {
    loadTranslation("nonexistent_translation_file");
    // No .qm file installed for this name; loadTranslation() iterates
    // translationDirs() and simply skips when translator->load() fails.
    // Only verify the call does not crash.
    SUCCEED();
}

// ---------------------------------------------------------------------------
// fetchUserInfos
// ---------------------------------------------------------------------------
TEST_F(ut_helper, fetchUserInfos_nonEmpty) {
    auto users = fetchUserInfos();
    ASSERT_FALSE(users.isEmpty());
    bool hasRoot = false;
    for (const auto &user : users) {
        ASSERT_FALSE(user.first.isEmpty());
        if (user.first == "root")
            hasRoot = true;
    }
    ASSERT_TRUE(hasRoot);
}


