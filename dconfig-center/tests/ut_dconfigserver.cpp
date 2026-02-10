// SPDX-FileCopyrightText: 2021 - 2023 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QThread>

#include <gtest/gtest.h>

#include <DConfigFile>

#include "dconfigserver.h"
#include "dconfigresource.h"
#include "dconfigconn.h"
#include "test_helper.hpp"

DCORE_USE_NAMESPACE
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
        ASSERT_TRUE(QFile::copy(":/config/example.json", noAppIdConfigPath()));
        qputenv("DSG_CONFIG_CONNECTION_DISABLE_DBUS", "true");
        qputenv("STATE_DIRECTORY", LocalPrefix);
        dsgDataDir.set("DSG_DATA_DIRS", "/usr/share/dsg");
    }
    static void TearDownTestCase() {
        QFile::remove(configPath());
        QFile::remove(noAppIdConfigPath());
        qunsetenv("DSG_CONFIG_CONNECTION_DISABLE_DBUS");
        qunsetenv("STATE_DIRECTORY");
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
    }
    static QString noAppIdConfigPath()
    {
        const QString metaPath = QString("%1/usr/share/dsg/configs/").arg(LocalPrefix);

        return QString("%1/%2.json").arg(metaPath, FILE_NAME);
    }
    QScopedPointer<DSGConfigServer> server;
};

void ut_DConfigServer::TearDown() {
}

TEST_F(ut_DConfigServer, acquireManager) {
    ASSERT_EQ(server->acquireManager(APP_ID, FILE_NAME, QString("")).path(),
              formatDBusObjectPath(QString("/%1/%2/%3").arg(APP_ID, FILE_NAME, QString::number(TestUid))));

    ASSERT_EQ(server->resourceSize(), 1);

    auto path2 = server->acquireManager(APP_ID, "example_noexist", QString("")).path();
    ASSERT_EQ(server->resourceObject(path2), nullptr);
    ASSERT_EQ(server->resourceSize(), 1);
}

TEST_F(ut_DConfigServer, acquireManagerV2) {
    ASSERT_EQ(server->acquireManagerV2(TestUid, APP_ID, FILE_NAME, QString("")).path(),
              formatDBusObjectPath(QString("/%1/%2/%3").arg(APP_ID, FILE_NAME, QString::number(TestUid))));
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
    ASSERT_EQ(server->resourceObject(getGenericResourceKey(path1)), server->resourceObject(getGenericResourceKey(path2)));
    ASSERT_EQ(server->resourceSize(), 1);
}

TEST_F(ut_DConfigServer, releaseResource) {

    auto path1 = server->acquireManager(APP_ID, FILE_NAME, QString("")).path();
    auto path2 = server->acquireManager(APP_ID, FILE_NAME, QString("")).path();

    ASSERT_EQ(server->resourceSize(), 1);
    QSignalSpy spy(server.data(), &DSGConfigServer::releaseResource);

    {
        auto resource = server->resourceObject(getGenericResourceKey(path1));
        ASSERT_TRUE(resource);
        auto conn = resource->getConn(APP_ID, TestUid);
        ASSERT_TRUE(conn);
        conn->release();
    }
    ASSERT_EQ(spy.count(), 0);

    {
        auto resource = server->resourceObject(getGenericResourceKey(path2));
        ASSERT_TRUE(resource);
        auto conn = resource->getConn(APP_ID, TestUid);
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
        auto resource = server->resourceObject(getGenericResourceKey(path1));
        ASSERT_TRUE(resource);
        auto conn = resource->getConn(APP_ID, TestUid);
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

TEST_F(ut_DConfigServer, acquireManagerGeneric) {
    ASSERT_EQ(server->acquireManager(NoAppId, FILE_NAME, QString("")).path(),
              formatDBusObjectPath(QString("/%1/%2/%3").arg(VirtualInterAppId, FILE_NAME, QString::number(TestUid))));

    ASSERT_EQ(server->resourceSize(), 1);
    server->acquireManager(APP_ID, FILE_NAME, QString(""));
    ASSERT_EQ(server->resourceSize(), 1);

    const auto path1 = server->acquireManager(NoAppId, FILE_NAME, QString("")).path();
    const auto path2 = server->acquireManager(APP_ID, FILE_NAME, QString("")).path();
    const auto resource = server->resourceObject(getGenericResourceKey(path1));
    ASSERT_EQ(resource, server->resourceObject(getGenericResourceKey(path2)));
    auto conn = resource->getConn(VirtualInterAppId, TestUid);
    ASSERT_TRUE(conn);
    ASSERT_EQ(resource->connSize(), 2);
}

TEST_F(ut_DConfigServer, removeUserData) {
    // 测试用户ID
    const uint testUid1 = 1001;
    const uint testUid2 = 1002;
    
    // 创建多个用户的连接
    auto path1 = server->acquireManagerV2(testUid1, APP_ID, FILE_NAME, QString("")).path();
    auto path2 = server->acquireManagerV2(testUid2, APP_ID, FILE_NAME, QString("")).path();
    auto path3 = server->acquireManagerV2(TestUid, APP_ID, FILE_NAME, QString("")).path();
    
    // 验证初始状态
    ASSERT_EQ(server->resourceSize(), 1);
    auto resource = server->resourceObject(getGenericResourceKey(path1));
    ASSERT_TRUE(resource);
    ASSERT_EQ(resource->connSize(), 3); // 三个用户的连接
    
    // 验证所有连接都存在
    auto conn1 = resource->getConn(APP_ID, testUid1);
    auto conn2 = resource->getConn(APP_ID, testUid2);
    auto conn3 = resource->getConn(APP_ID, TestUid);
    ASSERT_TRUE(conn1);
    ASSERT_TRUE(conn2);
    ASSERT_TRUE(conn3);
    
    // 测试删除用户1的数据
    server->removeUserData(testUid1);
    
    // 验证用户1的连接被删除
    ASSERT_FALSE(resource->getConn(APP_ID, testUid1));
    ASSERT_EQ(resource->connSize(), 2); // 剩余两个连接
    
    // 验证其他用户的连接不受影响
    ASSERT_TRUE(resource->getConn(APP_ID, testUid2));
    ASSERT_TRUE(resource->getConn(APP_ID, TestUid));
    ASSERT_EQ(server->resourceSize(), 1); // 资源仍然存在
    
    // 测试删除用户2的数据
    server->removeUserData(testUid2);
    
    // 验证用户2的连接被删除
    ASSERT_FALSE(resource->getConn(APP_ID, testUid2));
    ASSERT_EQ(resource->connSize(), 1); // 剩余一个连接
    
    // 验证剩余用户的连接不受影响
    ASSERT_TRUE(resource->getConn(APP_ID, TestUid));
    ASSERT_EQ(server->resourceSize(), 1); // 资源仍然存在
    
    // 测试删除最后一个用户的数据
    server->removeUserData(TestUid);
    
    // 验证所有连接都被删除，资源也被清理
    ASSERT_EQ(server->resourceSize(), 0); // 资源被删除
}

TEST_F(ut_DConfigServer, removeUserDataMultipleResources) {
    // 测试删除用户数据时涉及多个资源的情况
    const uint testUid = 1003;
    
    // 为同一用户创建多个资源的连接
    auto path1 = server->acquireManagerV2(testUid, APP_ID, FILE_NAME, QString("")).path();
    auto path2 = server->acquireManagerV2(testUid, "org.foo.appid2", FILE_NAME, QString("")).path();
    
    // 验证初始状态
    int initialResourceCount = server->resourceSize();
    ASSERT_GE(initialResourceCount, 1);
    
    // 创建其他用户的连接以验证不受影响
    auto path3 = server->acquireManagerV2(TestUid, APP_ID, FILE_NAME, QString("")).path();
    
    // 删除目标用户的数据
    server->removeUserData(testUid);
    
    // 验证目标用户的所有连接都被删除
    auto resource1 = server->resourceObject(getGenericResourceKey(path1));
    if (resource1) {
        ASSERT_FALSE(resource1->getConn(APP_ID, testUid));
    }
    
    // 验证其他用户的连接不受影响
    auto resource3 = server->resourceObject(getGenericResourceKey(path3));
    if (resource3) {
        ASSERT_TRUE(resource3->getConn(APP_ID, TestUid));
    }
}

TEST_F(ut_DConfigServer, removeUserDataEdgeCases) {
    // 测试边界情况
    
    // 删除不存在的用户数据（不应该崩溃）
    server->removeUserData(9999);
    ASSERT_EQ(server->resourceSize(), 0);
    
    // 创建一个连接然后删除两次
    const uint testUid = 1004;
    server->acquireManagerV2(testUid, APP_ID, FILE_NAME, QString(""));
    ASSERT_EQ(server->resourceSize(), 1);
    
    // 第一次删除
    server->removeUserData(testUid);
    ASSERT_EQ(server->resourceSize(), 0);
    
    // 第二次删除同一用户（不应该崩溃）
    server->removeUserData(testUid);
    ASSERT_EQ(server->resourceSize(), 0);
    
    // 删除用户ID为0的数据（TestUid）
    server->acquireManagerV2(TestUid, APP_ID, FILE_NAME, QString(""));
    ASSERT_EQ(server->resourceSize(), 1);
    
    server->removeUserData(TestUid);
    ASSERT_EQ(server->resourceSize(), 0);
}

TEST_F(ut_DConfigServer, removeUserDataGenericConfig) {
    // 测试删除用户数据对通用配置的影响
    // 使用TestUid (0) 来创建连接，因为在测试环境中这是有效的用户ID
    
    // 创建普通应用的连接
    auto path1 = server->acquireManagerV2(TestUid, APP_ID, FILE_NAME, QString("")).path();
    // 创建通用配置的连接
    auto path2 = server->acquireManagerV2(TestUid, NoAppId, FILE_NAME, QString("")).path();
    
    ASSERT_EQ(server->resourceSize(), 1);
    auto resource = server->resourceObject(getGenericResourceKey(path1));
    ASSERT_TRUE(resource);
    ASSERT_EQ(resource->connSize(), 2);
    
    // 验证连接都存在  
    ASSERT_TRUE(resource->getConn(APP_ID, TestUid));
    ASSERT_TRUE(resource->getConn(VirtualInterAppId, TestUid));
    
    // 删除用户数据
    server->removeUserData(TestUid);
    
    // 验证所有连接都被删除，资源被清理
    ASSERT_EQ(server->resourceSize(), 0);
}

TEST_F(ut_DConfigServer, removeUserDataFiles) {
    // 测试删除用户数据时缓存文件也被删除
    // 通过重新访问配置验证：如果返回默认值，说明缓存文件被正确删除
    const uint testUid1 = 1001;
    const uint testUid2 = 1002;
    
    // 创建用户连接
    auto path1 = server->acquireManagerV2(testUid1, APP_ID, FILE_NAME, QString("")).path();
    auto path2 = server->acquireManagerV2(testUid2, APP_ID, FILE_NAME, QString("")).path();
    
    auto resource = server->resourceObject(getGenericResourceKey(path1));
    ASSERT_TRUE(resource);
    
    // 获取连接并设置非默认值（默认值是true）
    auto conn1 = resource->getConn(APP_ID, testUid1);
    auto conn2 = resource->getConn(APP_ID, testUid2);
    ASSERT_TRUE(conn1);
    ASSERT_TRUE(conn2);
    
    // 设置非默认值：用户1设置为false，用户2设置为false
    conn1->setValue("canExit", QDBusVariant{false});
    conn2->setValue("canExit", QDBusVariant{false});
    
    // 验证设置成功
    ASSERT_EQ(conn1->value("canExit").variant().toBool(), false);
    ASSERT_EQ(conn2->value("canExit").variant().toBool(), false);
    
    // 保存缓存
    resource->save();
    
    // 删除用户1的数据
    server->removeUserData(testUid1);
    
    // 验证用户1的连接被删除
    ASSERT_FALSE(resource->getConn(APP_ID, testUid1));
    ASSERT_TRUE(resource->getConn(APP_ID, testUid2));
    
    // 重新为用户1创建连接，验证缓存文件是否被删除
    auto newPath1 = server->acquireManagerV2(testUid1, APP_ID, FILE_NAME, QString("")).path();
    auto newResource = server->resourceObject(getGenericResourceKey(newPath1));
    ASSERT_TRUE(newResource);
    
    auto newConn1 = newResource->getConn(APP_ID, testUid1);
    ASSERT_TRUE(newConn1);
    
    qWarning() << "*******" << configPrefixPath();
    // 关键验证：如果缓存文件被正确删除，应该返回默认值(true)，而不是之前设置的false
    ASSERT_EQ(newConn1->value("canExit").variant().toBool(), true) 
        << "Cache file was not properly deleted - expected default value (true) but got cached value (false)";
    
    // 验证用户2的配置仍然是之前设置的值（证明删除操作的隔离性）
    ASSERT_EQ(conn2->value("canExit").variant().toBool(), false) 
        << "User2's cache should not be affected by removing User1's data";
    
    // 删除用户2的数据并验证
    server->removeUserData(testUid2);
    
    // 重新为用户2创建连接，验证其缓存文件也被删除
    auto newPath2 = server->acquireManagerV2(testUid2, APP_ID, FILE_NAME, QString("")).path();
    auto newResource2 = server->resourceObject(getGenericResourceKey(newPath2));
    ASSERT_TRUE(newResource2);
    
    auto newConn2 = newResource2->getConn(APP_ID, testUid2);
    ASSERT_TRUE(newConn2);
    
    // 验证用户2的缓存文件也被删除
    ASSERT_EQ(newConn2->value("canExit").variant().toBool(), true) 
        << "User2's cache file was not properly deleted";
}

TEST_F(ut_DConfigServer, removeUserDataWithSubpath) {
    // 测试带有子路径的配置删除
    const uint testUid = 1006;
    const QString subpath = "test/subdir";
    
    // 创建带有子路径的连接
    auto path = server->acquireManagerV2(testUid, APP_ID, FILE_NAME, subpath).path();
    
    auto resource = server->resourceObject(getGenericResourceKey(path));
    ASSERT_TRUE(resource);
    
    // 获取连接并设置配置值
    auto conn = resource->getConn(APP_ID, testUid);
    ASSERT_TRUE(conn);
    
    conn->setValue("canExit", QDBusVariant{true});
    
    // 验证初始状态
    ASSERT_EQ(resource->connSize(), 1);
    ASSERT_TRUE(resource->getConn(APP_ID, testUid));
    ASSERT_EQ(server->resourceSize(), 1);
    
    // 删除用户数据
    server->removeUserData(testUid);
    
    // 验证连接和资源被清理
    ASSERT_EQ(server->resourceSize(), 0);
}

TEST_F(ut_DConfigServer, removeUserDataSimpleValidation) {
    // 简单直接的验证：确保删除用户数据后，同一服务器实例中的新连接返回默认值
    const uint testUid = TestUid;
    
    // 1. 创建连接并设置非默认值
    auto path1 = server->acquireManagerV2(testUid, APP_ID, FILE_NAME, QString("")).path();
    auto resource1 = server->resourceObject(getGenericResourceKey(path1));
    ASSERT_TRUE(resource1);
    
    auto conn1 = resource1->getConn(APP_ID, testUid);
    ASSERT_TRUE(conn1);
    
    // 验证默认值
    ASSERT_EQ(conn1->value("canExit").variant().toBool(), true) << "Default value should be true";
    
    // 设置非默认值
    conn1->setValue("canExit", QDBusVariant{false});
    ASSERT_EQ(conn1->value("canExit").variant().toBool(), false) << "Value should be set to false";
    resource1->save();
    
    // 2. 删除用户数据
    server->removeUserData(testUid);
    
    // 验证连接被删除
    ASSERT_FALSE(resource1->getConn(APP_ID, testUid));
    
    // 3. 在同一服务器实例中创建新连接
    auto path2 = server->acquireManagerV2(testUid, APP_ID, FILE_NAME, QString("")).path();
    auto resource2 = server->resourceObject(getGenericResourceKey(path2));
    ASSERT_TRUE(resource2);
    
    auto conn2 = resource2->getConn(APP_ID, testUid);
    ASSERT_TRUE(conn2);
    
    // 4. 验证新连接返回默认值（这证明缓存被正确清除）
    ASSERT_EQ(conn2->value("canExit").variant().toBool(), true) 
        << "New connection should return default value after removeUserData";
}
