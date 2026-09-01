// SPDX-FileCopyrightText: 2021 - 2026 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <QBuffer>
#include <QFile>
#include <QLocale>
#include <QSignalSpy>
#include <QDir>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>

#include <gtest/gtest.h>

#include "dconfigresource.h"
#include "dconfigconn.h"
#include "dconfigrefmanager.h"
#include <memory>
#include "test_helper.hpp"

static const QString LocalPrefix = QDir::tempPath() + "/dde-app-services-test-" +
    QString::number(QCoreApplication::applicationPid()) + "/";
static constexpr char const *APP_ID = "org.foo.appid";
static constexpr char const *FILE_NAME = "example";

static QString configPath()
{
    const QString metaPath = QString("%1/usr/share/dsg/configs/%2").arg(LocalPrefix, APP_ID);
    return QString("%1/%2.json").arg(metaPath, FILE_NAME);
}
static QString noAppIdConfigPath()
{
    const QString metaPath = QString("%1/usr/share/dsg/configs/").arg(LocalPrefix);

    return QString("%1/%2.json").arg(metaPath, FILE_NAME);
}

static EnvGuard dsgDataDir;
class ut_DConfigResource : public testing::Test
{
protected:
    static void SetUpTestCase() {
        auto path = configPath();
        if (!QFile::exists(path)) {
            QDir("").mkpath(QFileInfo(path).path());
        }

        ASSERT_TRUE(QFile::copy(":/config/example.json", path));
        ASSERT_TRUE(QFile::copy(":/config/example.json", noAppIdConfigPath()));
        qputenv("DSG_CONFIG_CONNECTION_DISABLE_DBUS", "true");
        qputenv("STATE_DIRECTORY", LocalPrefix.toLocal8Bit());
        dsgDataDir.set("DSG_DATA_DIRS", "/usr/share/dsg");
    }
    static void TearDownTestCase() {
        QFile::remove(configPath());
        QFile::remove(noAppIdConfigPath());
        QDir(LocalPrefix).removeRecursively();
        qunsetenv("DSG_CONFIG_CONNECTION_DISABLE_DBUS");
        qunsetenv("STATE_DIRECTORY");
        dsgDataDir.restore();
    }
    virtual void SetUp() override {
        resource.reset(new DSGConfigResource(FILE_NAME, "", LocalPrefix));
    }
    virtual void TearDown() override {

    }

    QScopedPointer<DSGConfigResource> resource;
};

TEST_F(ut_DConfigResource, load) {

    ASSERT_TRUE(resource->load(APP_ID));
    ASSERT_TRUE(resource->load(VirtualInterAppId));
}
TEST_F(ut_DConfigResource, load_fail) {

    DSGConfigResource resource2("example_notexist", "");
    ASSERT_FALSE(resource2.load(APP_ID));
}
TEST_F(ut_DConfigResource, createConn) {

    resource->load(APP_ID);
    ASSERT_TRUE(resource->createConn(APP_ID, TestUid));

    resource->load(VirtualInterAppId);
    ASSERT_TRUE(resource->createConn(VirtualInterAppId, TestUid));
}
TEST_F(ut_DConfigResource, getConn) {

    resource->load(APP_ID);
    resource->createConn(APP_ID, TestUid);
    ASSERT_TRUE(resource->getConn(APP_ID, TestUid));

    resource->load(VirtualInterAppId);
    resource->createConn(VirtualInterAppId, TestUid);
    ASSERT_TRUE(resource->getConn(VirtualInterAppId, TestUid));

    ASSERT_EQ(resource->connSize(), 2);
}
TEST_F(ut_DConfigResource, fallbackToGenericConfig) {

    resource->load(APP_ID);
    ASSERT_TRUE(resource->fallbackToGenericConfig());
}

class ut_DConfigConn : public testing::Test
{
protected:
    static void SetUpTestCase() {
        auto path = configPath();
        if (!QFile::exists(path)) {
            QDir("").mkpath(QFileInfo(path).path());
        }

        ASSERT_TRUE(QFile::copy(":/config/example.json", path));
        ASSERT_TRUE(QFile::copy(":/config/example.json", noAppIdConfigPath()));
        qputenv("DSG_CONFIG_CONNECTION_DISABLE_DBUS", "true");
        qputenv("STATE_DIRECTORY", LocalPrefix.toLocal8Bit());
        dsgDataDir.set("DSG_DATA_DIRS", "/usr/share/dsg");
    }
    static void TearDownTestCase() {
        QFile::remove(configPath());
        QFile::remove(noAppIdConfigPath());
        QDir(LocalPrefix).removeRecursively();
        qunsetenv("DSG_CONFIG_CONNECTION_DISABLE_DBUS");
        qunsetenv("STATE_DIRECTORY");
        dsgDataDir.restore();
    }
    virtual void SetUp() override {
        resource.reset(new DSGConfigResource("example", "", LocalPrefix));
        resource->load(APP_ID);
        conn = resource->createConn(APP_ID, TestUid);
        ASSERT_TRUE(conn);
    }
    virtual void TearDown() override {

    }
    DSGConfigConn* conn;
    QScopedPointer<DSGConfigResource> resource;
};

TEST_F(ut_DConfigConn, description_name) {
    ASSERT_EQ(conn->description("canExit", ""), "我是描述");
    ASSERT_EQ(conn->description("canExit", "en_US"), "I am description");
    ASSERT_EQ(conn->name("canExit", ""), "I am name");
    ASSERT_EQ(conn->name("canExit", "zh_CN"), QString("我是名字"));
    ASSERT_EQ(conn->name("canExit", QLocale(QLocale::Japanese).name()), "");
    ASSERT_EQ(conn->flags("canExit"), 0);
}

TEST_F(ut_DConfigConn, value) {
    conn->setValue("canExit", QDBusVariant{true});
    ASSERT_EQ(conn->value("canExit").variant(), true);
    const QStringList array{"value1", "value2"};
    QVariantMap map;
    map.insert("key1", "value1");
    map.insert("key2", "value2");
    ASSERT_EQ(conn->value("array").variant().toStringList(), array);
    ASSERT_EQ(conn->value("map").variant().toMap(), map);

    // array_map is an ARRAY containing one MAP: [{"key1":"value1","key2":"value2"}]
    QVariantList arrayMap = conn->value("array_map").variant().toList();
    ASSERT_EQ(arrayMap.size(), 1);
    QVariantMap mapInArray = arrayMap.first().toMap();
    ASSERT_EQ(mapInArray.value("key1").toString(), "value1");
    ASSERT_EQ(mapInArray.value("key2").toString(), "value2");
}

TEST_F(ut_DConfigConn, value_default) {
    ASSERT_EQ(conn->value("canExit").variant(), true);
}

TEST_F(ut_DConfigConn, setValue_andGetValue) {
    conn->setValue("canExit", QDBusVariant{false});
    ASSERT_EQ(conn->value("canExit").variant(), false);
    conn->setValue("canExit", QDBusVariant{true});
    ASSERT_EQ(conn->value("canExit").variant(), true);
}

TEST_F(ut_DConfigConn, permissions) {
    ASSERT_EQ(conn->permissions("canExit"), "readwrite");
    ASSERT_EQ(conn->permissions("key2"), "readwrite");
}

TEST_F(ut_DConfigConn, visibility) {
    // canExit has "visibility": "private" in example.json
    ASSERT_EQ(conn->visibility("canExit"), "private");
    ASSERT_EQ(conn->visibility("key2"), "public");
}

TEST_F(ut_DConfigConn, key) {
    ASSERT_FALSE(conn->key().isEmpty());
}

TEST_F(ut_DConfigConn, resourceKey) {
    ASSERT_FALSE(getResourceKey(conn->key()).isEmpty());
}

TEST_F(ut_DConfigConn, appId) {
    // When calledFromDBus() is false (test env with DSG_CONFIG_CONNECTION_DISABLE_DBUS=true),
    // getAppid() returns "testappid"
    ASSERT_EQ(conn->getAppid(), QString("testappid"));
}

TEST_F(ut_DConfigConn, uid) {
    ASSERT_EQ(getConnectionKey(conn->key()), TestUid);
}

TEST_F(ut_DConfigConn, file) {
    ASSERT_NE(conn->file(), nullptr);
}

TEST_F(ut_DConfigConn, meta) {
    ASSERT_NE(conn->meta(), nullptr);
}

TEST_F(ut_DConfigConn, setResource_updatesResource) {
    auto oldFile = conn->file();
    ASSERT_NE(oldFile, nullptr);

    auto resource2 = std::make_unique<DSGConfigResource>(FILE_NAME, "", LocalPrefix);
    resource2->load(APP_ID);
    conn->setResource(resource2.get());

    auto newFile = conn->file();
    ASSERT_NE(newFile, nullptr);
    ASSERT_NE(newFile, oldFile);

    // Restore original resource pointer to avoid use-after-free on TearDown
    conn->setResource(resource.data());
}

// Coverage-9: specificAppConns
TEST_F(ut_DConfigResource, specificAppConns_returnsOnlyAppConns) {
    resource->load(APP_ID);
    resource->createConn(APP_ID, TestUid);
    auto conns = resource->specificAppConns();
    ASSERT_FALSE(conns.isEmpty());
}

// Coverage-9: cacheExist
TEST_F(ut_DConfigResource, cacheExist_true) {
    resource->load(APP_ID);
    resource->createConn(APP_ID, TestUid);
    auto resourceKey = getResourceKey(APP_ID, resource->key());
    ASSERT_TRUE(resource->cacheExist(resourceKey));
}

TEST_F(ut_DConfigResource, cacheExist_false) {
    ASSERT_FALSE(resource->cacheExist("/nonexistent/resource"));
}

// Coverage-9: cachesOfTheResource
TEST_F(ut_DConfigResource, cachesOfTheResource_returnsCaches) {
    resource->load(APP_ID);
    resource->createConn(APP_ID, TestUid);
    auto resourceKey = getResourceKey(APP_ID, resource->key());
    auto caches = resource->cachesOfTheResource(resourceKey);
    ASSERT_FALSE(caches.isEmpty());
}

TEST_F(ut_DConfigResource, cachesOfTheResource_emptyForNonExistent) {
    auto caches = resource->cachesOfTheResource("/nonexistent/resource");
    ASSERT_TRUE(caches.isEmpty());
}

// Coverage-9: connsOfTheResource
TEST_F(ut_DConfigResource, connsOfTheResource_returnsConns) {
    resource->load(APP_ID);
    resource->createConn(APP_ID, TestUid);
    auto resourceKey = getResourceKey(APP_ID, resource->key());
    auto conns = resource->connsOfTheResource(resourceKey);
    ASSERT_EQ(conns.size(), 1);
}

TEST_F(ut_DConfigResource, connsOfTheResource_emptyForNonExistent) {
    auto conns = resource->connsOfTheResource("/nonexistent/resource");
    ASSERT_TRUE(conns.isEmpty());
}

// Coverage-9: repareCache
TEST_F(ut_DConfigResource, repareCache_sameMeta_preservesKeys) {
    resource->load(APP_ID);
    resource->createConn(APP_ID, TestUid);
    auto connKey = resource->getConnKey(APP_ID, TestUid);
    auto cache = resource->getCache(connKey);
    ASSERT_NE(cache, nullptr);
    auto file = resource->getFile(getResourceKey(APP_ID, resource->key()));
    ASSERT_NE(file, nullptr);
    auto meta = file->meta();
    auto keysBefore = cache->keyList();
    resource->repareCache(cache, meta, meta);
    auto keysAfter = cache->keyList();
    ASSERT_EQ(keysAfter.size(), keysBefore.size());
}

TEST_F(ut_DConfigResource, getConnectionsByUid) {
    resource->load(APP_ID);
    resource->createConn(APP_ID, TestUid);
    resource->load(VirtualInterAppId);
    resource->createConn(VirtualInterAppId, TestUid);

    auto conns = resource->getConnectionsByUid(TestUid);
    ASSERT_EQ(conns.size(), 2);
}

TEST_F(ut_DConfigResource, getConnectionsByUid_noMatch) {
    resource->load(APP_ID);
    resource->createConn(APP_ID, TestUid);

    auto conns = resource->getConnectionsByUid(99999);
    ASSERT_TRUE(conns.isEmpty());
}

TEST_F(ut_DConfigResource, isEmptyConn) {
    ASSERT_TRUE(resource->isEmptyConn());
    resource->load(APP_ID);
    resource->createConn(APP_ID, TestUid);
    ASSERT_FALSE(resource->isEmptyConn());
}

TEST_F(ut_DConfigResource, noAppidCache) {
    resource->load(APP_ID);
    resource->load(VirtualInterAppId);
    resource->createConn(VirtualInterAppId, TestUid);
    auto cache = resource->noAppidCache(TestUid);
    ASSERT_NE(cache, nullptr);
}

TEST_F(ut_DConfigResource, noAppidFile) {
    resource->load(APP_ID);
    resource->load(VirtualInterAppId);
    resource->createConn(VirtualInterAppId, TestUid);
    auto file = resource->noAppidFile();
    ASSERT_NE(file, nullptr);
}

TEST_F(ut_DConfigResource, removeConn) {
    resource->load(APP_ID);
    auto conn = resource->createConn(APP_ID, TestUid);
    ASSERT_EQ(resource->connSize(), 1);
    resource->removeConn(conn->key());
    ASSERT_EQ(resource->connSize(), 0);
}

TEST_F(ut_DConfigResource, save_preservesData) {
    resource->load(APP_ID);
    auto conn = resource->createConn(APP_ID, TestUid);
    ASSERT_TRUE(conn);
    conn->setValue("canExit", QDBusVariant{false});
    ASSERT_EQ(conn->value("canExit").variant(), false);
    resource->save();
    ASSERT_EQ(conn->value("canExit").variant(), false);
    ASSERT_EQ(resource->connSize(), 1);
    // Assert the user cache file was actually written to disk.
    // save() passes m_localPrefix to cache->save(), which prepends it to the
    // cachePathPrefix (configPrefixPath() + "/<uid>") in applicationCacheDir().
    QString userCachePath = QDir::cleanPath(LocalPrefix + "/" + configPrefixPath() + "/" +
        QString::number(TestUid) + "/" + APP_ID + "/" + FILE_NAME + ".json");
    ASSERT_TRUE(QFile::exists(userCachePath));
}

TEST_F(ut_DConfigResource, save_withAppid_preservesData) {
    resource->load(APP_ID);
    auto conn = resource->createConn(APP_ID, TestUid);
    ASSERT_TRUE(conn);
    conn->setValue("canExit", QDBusVariant{true});
    ASSERT_EQ(conn->value("canExit").variant(), true);
    resource->save(APP_ID);
    ASSERT_EQ(conn->value("canExit").variant(), true);
    ASSERT_EQ(resource->connSize(), 1);
    // Assert the user cache file was actually written to disk.
    QString userCachePath = QDir::cleanPath(LocalPrefix + "/" + configPrefixPath() + "/" +
        QString::number(TestUid) + "/" + APP_ID + "/" + FILE_NAME + ".json");
    ASSERT_TRUE(QFile::exists(userCachePath));
}

TEST_F(ut_DConfigResource, reparse_existingResource) {
    resource->load(APP_ID);
    resource->createConn(APP_ID, TestUid);
    // reparse() calls newMeta->load() with empty localPrefix, using DSG_DATA_DIRS.
    // Override to LocalPrefix so the meta file is found by reparse.
    EnvGuard localDsgDir;
    localDsgDir.set("DSG_DATA_DIRS", (LocalPrefix + "/usr/share/dsg").toLocal8Bit());
    ASSERT_TRUE(resource->reparse(APP_ID));
}

TEST_F(ut_DConfigResource, reparse_metaLoadFailure_returnsFalse) {
    resource->load(APP_ID);
    resource->createConn(APP_ID, TestUid);
    // reparse() calls newMeta->load() with empty localPrefix, using DSG_DATA_DIRS.
    // Override to LocalPrefix so reparse looks for meta in the test fixtures.
    EnvGuard localDsgDir;
    localDsgDir.set("DSG_DATA_DIRS", (LocalPrefix + "/usr/share/dsg").toLocal8Bit());
    QString appMetaPath = configPath();
    QString genericMetaPath = noAppIdConfigPath();
    MetaFileGuard appGuard(appMetaPath);
    MetaFileGuard genericGuard(genericMetaPath);
    QFile::remove(appMetaPath);
    QFile::remove(genericMetaPath);
    EXPECT_FALSE(resource->reparse(APP_ID));
}

TEST_F(ut_DConfigResource, doGlobalValueChanged_withoutSyncCache_noCrash) {
    resource->load(APP_ID);
    auto conn = resource->createConn(APP_ID, TestUid);
    ASSERT_TRUE(conn);
    QSignalSpy spy(conn, &DSGConfigConn::valueChanged);
    auto resourceKey = getResourceKey(APP_ID, resource->key());
    resource->doGlobalValueChanged("array", resourceKey);
    ASSERT_EQ(spy.count(), 1);
}

TEST_F(ut_DConfigResource, reparse_withRemovedKey_removesFromCache) {
    resource->load(APP_ID);
    auto conn = resource->createConn(APP_ID, TestUid);
    ASSERT_TRUE(conn);
    // reparse() calls newMeta->load() with empty localPrefix, using DSG_DATA_DIRS.
    // Override to LocalPrefix so reparse finds the modified meta file.
    EnvGuard localDsgDir;
    localDsgDir.set("DSG_DATA_DIRS", (LocalPrefix + "/usr/share/dsg").toLocal8Bit());
    conn->setValue("canExit", QDBusVariant{false});
    ASSERT_EQ(conn->value("canExit").variant(), false);
    QString metaPath = configPath();
    {
        MetaFileGuard guard(metaPath);
        QFile readFile(metaPath);
        ASSERT_TRUE(readFile.open(QIODevice::ReadOnly));
        QJsonDocument doc = QJsonDocument::fromJson(readFile.readAll());
        readFile.close();
        QJsonObject root = doc.object();
        QJsonObject contents = root.value("contents").toObject();
        contents.remove("canExit");
        root["contents"] = contents;
        doc.setObject(root);
        QFile writeFile(metaPath);
        ASSERT_TRUE(writeFile.open(QIODevice::WriteOnly | QIODevice::Truncate));
        writeFile.write(doc.toJson());
        writeFile.close();
        ASSERT_TRUE(resource->reparse(APP_ID));
        EXPECT_FALSE(conn->value("canExit").variant().isValid());
    }
}

TEST_F(ut_DConfigResource, reparse_withPermissionChange_removesFromCache) {
    resource->load(APP_ID);
    auto conn = resource->createConn(APP_ID, TestUid);
    ASSERT_TRUE(conn);
    // reparse() calls newMeta->load() with empty localPrefix, using DSG_DATA_DIRS.
    // Override to LocalPrefix so reparse finds the modified meta file.
    EnvGuard localDsgDir;
    localDsgDir.set("DSG_DATA_DIRS", (LocalPrefix + "/usr/share/dsg").toLocal8Bit());
    conn->setValue("canExit", QDBusVariant{false});
    ASSERT_EQ(conn->value("canExit").variant(), false);
    QString metaPath = configPath();
    {
        MetaFileGuard guard(metaPath);
        QFile readFile(metaPath);
        ASSERT_TRUE(readFile.open(QIODevice::ReadOnly));
        QJsonDocument doc = QJsonDocument::fromJson(readFile.readAll());
        readFile.close();
        QJsonObject root = doc.object();
        QJsonObject contents = root.value("contents").toObject();
        QJsonObject canExit = contents.value("canExit").toObject();
        canExit["permissions"] = "readonly";
        contents["canExit"] = canExit;
        root["contents"] = contents;
        doc.setObject(root);
        QFile writeFile(metaPath);
        ASSERT_TRUE(writeFile.open(QIODevice::WriteOnly | QIODevice::Truncate));
        writeFile.write(doc.toJson());
        writeFile.close();
        ASSERT_TRUE(resource->reparse(APP_ID));
        conn->setValue("canExit", QDBusVariant{false});
        EXPECT_EQ(conn->value("canExit").variant(), true);
    }
}

TEST_F(ut_DConfigResource, getConn_byKey) {
    resource->load(APP_ID);
    auto conn = resource->createConn(APP_ID, TestUid);
    ASSERT_EQ(resource->getConn(conn->key()), conn);
}

TEST_F(ut_DConfigResource, getConn_nonexistent) {
    resource->load(APP_ID);
    ASSERT_EQ(resource->getConn("nonexistent_key"), nullptr);
}

TEST_F(ut_DConfigResource, getFile) {
    resource->load(APP_ID);
    auto resourceKey = getResourceKey(APP_ID, resource->key());
    auto file = resource->getFile(resourceKey);
    ASSERT_NE(file, nullptr);
}

TEST_F(ut_DConfigResource, getFile_nonexistent) {
    auto file = resource->getFile("/nonexistent/resource");
    ASSERT_EQ(file, nullptr);
}

TEST_F(ut_DConfigResource, getCache_nonexistent) {
    auto cache = resource->getCache("/nonexistent/conn");
    ASSERT_EQ(cache, nullptr);
}

TEST_F(ut_DConfigConn, doSyncConfigCache_savesCache) {
    conn->setValue("canExit", QDBusVariant{false});
    ASSERT_EQ(conn->value("canExit").variant(), false);
    ConfigCacheKey userKey = ConfigSyncRequestCache::userKey(conn->key());
    ConfigSyncRequestCache syncCache(resource.data());
    resource->setSyncRequestCache(&syncCache);
    resource->doSyncConfigCache(userKey);
    auto connKey = resource->getConnKey(APP_ID, TestUid);
    auto cache = resource->getCache(connKey);
    ASSERT_NE(cache, nullptr);
    ASSERT_EQ(conn->value("canExit").variant(), false);
}



// Coverage-11: Branch coverage tests for DSGConfigConn

// Branch: description() with non-existent key returns ""
TEST_F(ut_DConfigConn, description_nonExistentKey_returnsEmpty) {
    ASSERT_EQ(conn->description("nonexistent_key", ""), QString());
    ASSERT_EQ(conn->description("nonexistent_key", "en_US"), QString());
}

// Branch: name() with non-existent key returns ""
TEST_F(ut_DConfigConn, name_nonExistentKey_returnsEmpty) {
    ASSERT_EQ(conn->name("nonexistent_key", ""), QString());
    ASSERT_EQ(conn->name("nonexistent_key", "zh_CN"), QString());
}

// Branch: value() with non-existent key returns empty QDBusVariant
TEST_F(ut_DConfigConn, value_nonExistentKey_returnsEmpty) {
    auto result = conn->value("nonexistent_key");
    ASSERT_FALSE(result.variant().isValid());
}

// Branch: setValue() with non-existent key does nothing (early return)
TEST_F(ut_DConfigConn, setValue_nonExistentKey_noChange) {
    QSignalSpy valueSpy(conn, &DSGConfigConn::valueChanged);
    QSignalSpy globalSpy(conn, &DSGConfigConn::globalValueChanged);
    conn->setValue("nonexistent_key", QDBusVariant{42});
    ASSERT_EQ(valueSpy.count(), 0);
    ASSERT_EQ(globalSpy.count(), 0);
}

// Branch: setValue() with global flag key emits globalValueChanged
TEST_F(ut_DConfigConn, setValue_globalKey_emitsGlobalValueChanged) {
    QSignalSpy valueSpy(conn, &DSGConfigConn::valueChanged);
    QSignalSpy globalSpy(conn, &DSGConfigConn::globalValueChanged);
    conn->setValue("array", QDBusVariant{QStringList{"new1", "new2"}});
    ASSERT_EQ(globalSpy.count(), 1);
    ASSERT_EQ(valueSpy.count(), 1);
}

// Branch: setValue() with non-global key emits valueChanged
TEST_F(ut_DConfigConn, setValue_nonGlobalKey_emitsValueChanged) {
    // Ensure canExit is in a known state to avoid side effects from previous tests
    // (e.g. doSyncConfigCache_savesCache may have persisted canExit=false)
    conn->setValue("canExit", QDBusVariant{true});
    QSignalSpy valueSpy(conn, &DSGConfigConn::valueChanged);
    QSignalSpy globalSpy(conn, &DSGConfigConn::globalValueChanged);
    conn->setValue("canExit", QDBusVariant{false});
    ASSERT_EQ(valueSpy.count(), 1);
    ASSERT_EQ(globalSpy.count(), 0);
}

// Branch: reset() with existing non-global key emits valueChanged
TEST_F(ut_DConfigConn, reset_existingNonGlobalKey_emitsValueChanged) {
    conn->setValue("canExit", QDBusVariant{false});
    ASSERT_EQ(conn->value("canExit").variant(), false);
    QSignalSpy valueSpy(conn, &DSGConfigConn::valueChanged);
    QSignalSpy globalSpy(conn, &DSGConfigConn::globalValueChanged);
    conn->reset("canExit");
    ASSERT_EQ(valueSpy.count(), 1);
    ASSERT_EQ(globalSpy.count(), 0);
}

// Branch: reset() with existing global key emits globalValueChanged
TEST_F(ut_DConfigConn, reset_globalKey_emitsGlobalValueChanged) {
    conn->setValue("array", QDBusVariant{QStringList{"new1", "new2"}});
    QSignalSpy valueSpy(conn, &DSGConfigConn::valueChanged);
    QSignalSpy globalSpy(conn, &DSGConfigConn::globalValueChanged);
    conn->reset("array");
    ASSERT_EQ(globalSpy.count(), 1);
    ASSERT_EQ(valueSpy.count(), 1);
}

// Branch: reset() with non-existent key does nothing
TEST_F(ut_DConfigConn, reset_nonExistentKey_noChange) {
    QSignalSpy valueSpy(conn, &DSGConfigConn::valueChanged);
    QSignalSpy globalSpy(conn, &DSGConfigConn::globalValueChanged);
    conn->reset("nonexistent_key");
    ASSERT_EQ(valueSpy.count(), 0);
    ASSERT_EQ(globalSpy.count(), 0);
}

// Branch: isDefaultValue() returns true for unset key (default)
TEST_F(ut_DConfigConn, isDefaultValue_defaultKey_returnsTrue) {
    ASSERT_TRUE(conn->isDefaultValue("canExit"));
}

// Branch: isDefaultValue() returns false after value is set
TEST_F(ut_DConfigConn, isDefaultValue_setKey_returnsFalse) {
    conn->setValue("canExit", QDBusVariant{false});
    ASSERT_FALSE(conn->isDefaultValue("canExit"));
}

// Branch: isDefaultValue() with non-existent key returns false
TEST_F(ut_DConfigConn, isDefaultValue_nonExistentKey_returnsFalse) {
    ASSERT_FALSE(conn->isDefaultValue("nonexistent_key"));
}

// Branch: contains() with non-existent key returns false
TEST_F(ut_DConfigConn, contains_nonExistentKey_returnsFalse) {
    ASSERT_FALSE(conn->contains("nonexistent_key"));
}

// Branch: contains() with existing key returns true
TEST_F(ut_DConfigConn, contains_existingKey_returnsTrue) {
    ASSERT_TRUE(conn->contains("canExit"));
}

// Branch: containsWithoutProp() with non-existent key returns false
TEST_F(ut_DConfigConn, containsWithoutProp_nonExistentKey_returnsFalse) {
    ASSERT_FALSE(conn->containsWithoutProp("nonexistent_key"));
}

// Branch: containsWithoutProp() with existing key returns true
TEST_F(ut_DConfigConn, containsWithoutProp_existingKey_returnsTrue) {
    ASSERT_TRUE(conn->containsWithoutProp("canExit"));
}

// Branch: visibility() with non-existent key returns ""
TEST_F(ut_DConfigConn, visibility_nonExistentKey_returnsEmpty) {
    ASSERT_EQ(conn->visibility("nonexistent_key"), QString());
}

// Branch: permissions() with non-existent key returns ""
TEST_F(ut_DConfigConn, permissions_nonExistentKey_returnsEmpty) {
    ASSERT_EQ(conn->permissions("nonexistent_key"), QString());
}

// Branch: path() returns formatted DBus path
TEST_F(ut_DConfigConn, path_returnsFormattedPath) {
    ASSERT_EQ(conn->path(), formatDBusObjectPath(conn->key()));
}

// Branch: version() returns version string
TEST_F(ut_DConfigConn, version_returnsVersionString) {
    QString ver = conn->version();
    ASSERT_FALSE(ver.isEmpty());
    ASSERT_TRUE(ver.contains('.'));
}

// Branch: keyList() returns list of keys
TEST_F(ut_DConfigConn, keyList_returnsKeys) {
    QStringList keys = conn->keyList();
    ASSERT_TRUE(keys.contains("canExit"));
    ASSERT_TRUE(keys.contains("key2"));
    ASSERT_TRUE(keys.contains("array"));
}

// Branch: flags() with global key returns non-zero
TEST_F(ut_DConfigConn, flags_globalKey_returnsNonZero) {
    ASSERT_EQ(conn->flags("canExit"), 0);
    ASSERT_NE(conn->flags("array"), 0);
}

// Branch: release() emits releaseChanged signal
TEST_F(ut_DConfigConn, release_emitsReleaseChanged) {
    QSignalSpy spy(conn, &DSGConfigConn::releaseChanged);
    conn->release();
    ASSERT_EQ(spy.count(), 1);
}

// Branch: hasPermissionByUid() returns true in non-DBus (test) env
TEST_F(ut_DConfigConn, hasPermissionByUid_returnsTrueInTestEnv) {
    ASSERT_TRUE(conn->hasPermissionByUid("canExit"));
    ASSERT_TRUE(conn->hasPermissionByUid("key2"));
}
