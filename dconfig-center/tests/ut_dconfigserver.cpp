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

#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QThread>

#include <gtest/gtest.h>

#include "dconfigserver.h"
#include "dconfigresource.h"
#include "dconfigconn.h"

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
    }
    static void TearDownTestCase() {
        QFile::remove(configPath());
        qunsetenv("DSG_CONFIG_CONNECTION_DISABLE_DBUS");
        QDir(LocalPrefix).removeRecursively();
    }
    virtual void SetUp() override {
        server.reset(new DSGConfigServer);
        server->setLocalPrefix(LocalPrefix);
    }
    virtual void TearDown() override;
    static QString configPath()
    {
        const QString metaPath = QString("%1/opt/apps/%2/files/schemas/configs").arg(LocalPrefix, APP_ID);

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
