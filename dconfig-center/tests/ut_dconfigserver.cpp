// SPDX-FileCopyrightText: 2021 - 2022 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QThread>

#include <gtest/gtest.h>

#include "dconfigserver.h"
#include "dconfigresource.h"
#include "dconfigconn.h"
#include "test_helper.hpp"


static EnvGuard dsgDataDir;
static constexpr char const *LocalPrefix = "/tmp/example/";
static constexpr char const *APP_ID = "org.foo.appid";
static constexpr char const *FILE_NAME = "example";

class ut_DConfigServer : public testing::Test
{
protected:
    static void SetUpTestCase() {
        auto path = configPath();
        if (!QFile::exists(path)) {
            QDir("").mkpath(QFileInfo(path).path());
        }

        ASSERT_TRUE(QFile::copy(":/config/example.json", path));
        qputenv("DSG_CONFIG_CONNECTION_DISABLE_DBUS", "true");
        dsgDataDir.set("DSG_DATA_DIRS", "/usr/share/dsg");
    }
    static void TearDownTestCase() {
        QFile::remove(configPath());
        qunsetenv("DSG_CONFIG_CONNECTION_DISABLE_DBUS");
        QDir(LocalPrefix).removeRecursively();
        dsgDataDir.restore();
    }
    virtual void SetUp() override {
        server.reset(new DSGConfigServer);
        server->setLocalPrefix(LocalPrefix);
        server->setDelayReleaseTime(0);
    }
    virtual void TearDown() override;
    static QString configPath()
    {
        const QString metaPath = QString("%1/usr/share/dsg/configs/%2").arg(LocalPrefix, APP_ID);

        return QString("%1/%2.json").arg(metaPath, FILE_NAME);
    }    QScopedPointer<DSGConfigServer> server;
};

void ut_DConfigServer::TearDown() {
}

TEST_F(ut_DConfigServer, acquireManager) {

    auto abc = server->acquireManager(APP_ID, FILE_NAME, QString(""));

    ASSERT_EQ(server->acquireManager(APP_ID, FILE_NAME, QString("")).path(),
              DSGConfigServer::validDBusObjectPath(QString("/%1/%2/%3").arg(APP_ID, FILE_NAME, QString::number(0))));

    ASSERT_EQ(server->resourceSize(), 1);

    auto path2 = server->acquireManager(APP_ID, "example_noexist", QString("")).path();
    ASSERT_EQ(server->resourceObject(path2), nullptr);
    ASSERT_EQ(server->resourceSize(), 1);

}

TEST_F(ut_DConfigServer, resourceSize) {

    auto path1 = server->acquireManager(APP_ID, FILE_NAME, QString("")).path();
    auto path2 = server->acquireManager(APP_ID, FILE_NAME, QString("")).path();

    ASSERT_EQ(path1, path2);
    ASSERT_EQ(server->resourceSize(), 1);
    ASSERT_EQ(server->resourceObject(path1), server->resourceObject(path2));
    ASSERT_EQ(server->resourceSize(), 1);
}

TEST_F(ut_DConfigServer, releaseResource) {

    auto path1 = server->acquireManager(APP_ID, FILE_NAME, QString("")).path();
    auto path2 = server->acquireManager(APP_ID, FILE_NAME, QString("")).path();

    ASSERT_EQ(server->resourceSize(), 1);
    QSignalSpy spy(server.data(), &DSGConfigServer::releaseResource);

    {
        auto resource = server->resourceObject(getResourceKey(path1));
        ASSERT_TRUE(resource);
        auto conn = resource->connObject(getConnectionKey(path1));
        ASSERT_TRUE(conn);
        conn->release();
    }
    ASSERT_EQ(spy.count(), 0);

    {
        auto resource = server->resourceObject(getResourceKey(path2));
        ASSERT_TRUE(resource);
        auto conn = resource->connObject(getConnectionKey(path2));
        ASSERT_TRUE(conn);
        conn->release();
    }
    ASSERT_EQ(spy.count(), 1);
}

TEST_F(ut_DConfigServer, setDelayReleaseTime) {

    auto path1 = server->acquireManager(APP_ID, FILE_NAME, QString("")).path();

    ASSERT_EQ(server->resourceSize(), 1);
    server->setDelayReleaseTime(10);
    // 等待延迟释放时间，10ms.
    QSignalSpy spy(server.data(), &DSGConfigServer::releaseResource);

    {
        auto resource = server->resourceObject(getResourceKey(path1));
        ASSERT_TRUE(resource);
        auto conn = resource->connObject(getConnectionKey(path1));
        ASSERT_TRUE(conn);
        conn->release();
    }
    ASSERT_EQ(spy.count(), 0);
    spy.wait(server->delayReleaseTime());

    ASSERT_EQ(spy.count(), 1);
}

TEST_F(ut_DConfigServer, metaPathToConfigureId) {
    QStringList appPaths {
        "/usr/share/dsg/configs/example.json",
        "/usr/share/dsg/configs/dconfig-example_abc.d/example.json",
        "/usr/share/dsg/configs/dconfig-example/example.json",
        "/usr/share/dsg/configs/dconfig-example/a/b/example.json"
    };

    for (auto path : appPaths) {
        const auto configureId = getMetaConfigureId(path);
        ASSERT_FALSE(configureId.isInValid());
    }
}

TEST_F(ut_DConfigServer, overridePathToConfigureId) {
    QStringList paths {
        "/usr/share/dsg/configs/overrides/example/a.json",
        "/usr/share/dsg/configs/overrides/dconfig-example/example/a.json",
        "/usr/share/dsg/configs/overrides/dconfig-example_abc.d/example/a.json",
        "/usr/share/dsg/configs/overrides/dconfig-example/example/a/b/a.json",
        "/etc/dsg/configs/overrides/example/a.json",
        "/etc/dsg/configs/overrides/dconfig-example/example/a.json",
        "/etc/dsg/configs/overrides/dconfig-example/example/a/b/a.json"
    };

    for (auto path : paths) {
        const auto configureId = getOverrideConfigureId(path);
        ASSERT_FALSE(configureId.isInValid());
    }
}
