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
#include <QFile>
#include <QLocale>
#include <QSignalSpy>
#include <QDir>

#include <gtest/gtest.h>

#include "dconfigresource.h"
#include "dconfigconn.h"

static constexpr char const *LocalPrefix = "/tmp/example/";
static constexpr char const *APP_ID = "org.foo.appid";
static constexpr char const *FILE_NAME = "example";

static QString configPath()
{
    const QString metaPath = QString("%1/opt/apps/%2/files/schemas/configs").arg(LocalPrefix, APP_ID);

    return QString("%1/%2.json").arg(metaPath, FILE_NAME);
}

class ut_DConfigResource : public testing::Test
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
    }
    virtual void SetUp() override {
        resource.reset(new DSGConfigResource("/example", LocalPrefix));
    }
    virtual void TearDown() override {

    }

    QScopedPointer<DSGConfigResource> resource;
};

TEST_F(ut_DConfigResource, load) {

    ASSERT_TRUE(resource->load(APP_ID, FILE_NAME, ""));
}
TEST_F(ut_DConfigResource, load_fail) {

    ASSERT_FALSE(resource->load(APP_ID, "example_notexist", ""));
}
TEST_F(ut_DConfigResource, createConn) {

    resource->load(APP_ID, FILE_NAME, "");
    ASSERT_TRUE(resource->createConn(0));
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
        qputenv("DSG_CONFIG_CONNECTION_DISABLE_DBUS", "true");
    }
    static void TearDownTestCase() {
        QFile::remove(configPath());
        QDir(LocalPrefix).removeRecursively();
        qunsetenv("DSG_CONFIG_CONNECTION_DISABLE_DBUS");
    }
    virtual void SetUp() override {
        resource.reset(new DSGConfigResource("/example", LocalPrefix));
        resource->load(APP_ID, FILE_NAME, "");
        conn = resource->createConn(0);
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
}

TEST_F(ut_DConfigConn, setValue) {
    conn->setValue("canExit", QDBusVariant{false});

    QSignalSpy spy(conn, &DSGConfigConn::valueChanged);
    conn->setValue("canExit", QDBusVariant{true});
    ASSERT_EQ(conn->value("canExit").variant(), true);
    ASSERT_EQ(spy.count(), 1);
}

TEST_F(ut_DConfigConn, reset) {
    conn->setValue("canExit", QDBusVariant{false});

    QSignalSpy spy(conn, &DSGConfigConn::valueChanged);
    conn->reset("canExit");
    ASSERT_EQ(conn->value("canExit").variant(), true);
    ASSERT_EQ(spy.count(), 1);
}

TEST_F(ut_DConfigConn, visibility) {

    ASSERT_EQ(conn->visibility("canExit"), "private");
}

TEST_F(ut_DConfigConn, release) {
    QSignalSpy spy(conn, &DSGConfigConn::releaseChanged);
    ASSERT_EQ(spy.count(), 0);

    conn->release();

    ASSERT_EQ(spy.count(), 1);
    conn->release();
}
