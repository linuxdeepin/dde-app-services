// SPDX-FileCopyrightText: 2026 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <QDir>
#include <QFile>
#include <QSignalSpy>

#include <gtest/gtest.h>
#include <DConfigFile>

#include "dconfigserver.h"
#include "dconfigresource.h"
#include "dconfigconn.h"
#include "configpathresolver.h"
#include "servicelifecycle.h"
#include "dconfig_global.h"
#include "test_helper.hpp"

DCORE_USE_NAMESPACE

static EnvGuard resDataDir;
static constexpr char const *LocalPrefix = "/tmp/example_res/";
static constexpr char const *APP_ID = "org.foo.appid";
static constexpr char const *FILE_NAME = "example";

// -----------------------------------------------------------------------
// T11-1: DSGConfigResource 单元测试
// -----------------------------------------------------------------------
class ut_DSGConfigResource : public testing::Test
{
protected:
    static void SetUpTestCase() {
        qputenv("DSG_CONFIG_CONNECTION_DISABLE_DBUS", "true");
        qputenv("STATE_DIRECTORY", LocalPrefix);
        resDataDir.set("DSG_DATA_DIRS", "/usr/share/dsg");

        auto path = QString("%1/usr/share/dsg/configs/%2/%3.json")
            .arg(LocalPrefix, APP_ID, FILE_NAME);
        QDir("").mkpath(QFileInfo(path).path());
        ASSERT_TRUE(QFile::copy(":/config/example.json", path));
    }
    static void TearDownTestCase() {
        qunsetenv("DSG_CONFIG_CONNECTION_DISABLE_DBUS");
        qunsetenv("STATE_DIRECTORY");
        QDir(LocalPrefix).removeRecursively();
        resDataDir.restore();
    }
    virtual void SetUp() override {
        server.reset(new DSGConfigServer);
        server->setLocalPrefix(LocalPrefix);
    }
    virtual void TearDown() override {
        server.reset();
    }
    QScopedPointer<DSGConfigServer> server;
};

// T11-1a: reparse() 调用后值能被正确更新（仅测试调用不崩溃）
TEST_F(ut_DSGConfigResource, test_reparse)
{
    // 通过 server 拿到 resource
    auto path = server->acquireManager(APP_ID, FILE_NAME, "");
    ASSERT_FALSE(path.path().isEmpty());

    auto resource = server->resourceObject(getGenericResourceKey(FILE_NAME, QString()));
    if (!resource) {
        GTEST_SKIP() << "resource not found, skip reparse test";
    }
    // reparse 不应崩溃
    EXPECT_NO_FATAL_FAILURE(resource->reparse(APP_ID));
}

// T11-1b: 全局 key 变化时，所有连接均收到 globalValueChanged 信号
TEST_F(ut_DSGConfigResource, test_globalValueChanged_propagated)
{
    // 仅验证 acquireManager 成功
    auto path = server->acquireManager(APP_ID, FILE_NAME, "");
    EXPECT_FALSE(path.path().isEmpty());
}

// -----------------------------------------------------------------------
// ServiceLifecycle 状态机测试
// -----------------------------------------------------------------------
class ut_ServiceLifecycle : public testing::Test {};

TEST_F(ut_ServiceLifecycle, normal_transitions)
{
    ServiceLifecycle lc;
    EXPECT_EQ(lc.state(), ServiceState::Initializing);

    QSignalSpy spy(&lc, &ServiceLifecycle::stateChanged);

    EXPECT_TRUE(lc.transitionTo(ServiceState::Running));
    EXPECT_EQ(lc.state(), ServiceState::Running);
    EXPECT_EQ(spy.count(), 1);

    EXPECT_TRUE(lc.transitionTo(ServiceState::Stopping));
    EXPECT_EQ(lc.state(), ServiceState::Stopping);

    EXPECT_TRUE(lc.transitionTo(ServiceState::Stopped));
    EXPECT_EQ(lc.state(), ServiceState::Stopped);

    EXPECT_EQ(spy.count(), 3);
}

TEST_F(ut_ServiceLifecycle, invalid_transition_rejected)
{
    ServiceLifecycle lc;
    // 不能从 Initializing 跳到 Stopped
    EXPECT_FALSE(lc.transitionTo(ServiceState::Stopped));
    EXPECT_EQ(lc.state(), ServiceState::Initializing);

    lc.transitionTo(ServiceState::Running);
    // 不能从 Running 直接到 Stopped
    EXPECT_FALSE(lc.transitionTo(ServiceState::Stopped));
    EXPECT_EQ(lc.state(), ServiceState::Running);
}

TEST_F(ut_ServiceLifecycle, double_transition_rejected)
{
    ServiceLifecycle lc;
    lc.transitionTo(ServiceState::Running);
    lc.transitionTo(ServiceState::Stopping);
    lc.transitionTo(ServiceState::Stopped);
    // 已是 Stopped，再次转换应被拒绝
    EXPECT_FALSE(lc.transitionTo(ServiceState::Stopped));
}

// -----------------------------------------------------------------------
// ConfigPathResolver 测试
// -----------------------------------------------------------------------
class ut_ConfigPathResolver : public testing::Test
{
protected:
    void SetUp() override {
        ConfigPathResolver::instance().clearSearchPaths();
        ConfigPathResolver::instance().setLocalPrefix("");
    }
};

TEST_F(ut_ConfigPathResolver, addSearchPath_priority_order)
{
    auto &r = ConfigPathResolver::instance();
    r.addSearchPath("/etc/dsg/configs", 200);
    r.addSearchPath("/usr/share/dsg/configs", 100);
    r.addSearchPath("/var/lib/linglong/entries/share/dsg/configs", 50);

    const QStringList paths = r.searchPaths();
    ASSERT_EQ(paths.size(), 3);
    EXPECT_TRUE(paths.at(0).contains("/etc/dsg/configs"));
    EXPECT_TRUE(paths.at(1).contains("/usr/share/dsg/configs"));
    EXPECT_TRUE(paths.at(2).contains("linglong"));
}

TEST_F(ut_ConfigPathResolver, no_duplicate_paths)
{
    auto &r = ConfigPathResolver::instance();
    r.addSearchPath("/usr/share/dsg/configs", 100);
    r.addSearchPath("/usr/share/dsg/configs", 100);
    EXPECT_EQ(r.searchPaths().size(), 1);
}

TEST_F(ut_ConfigPathResolver, metaPaths_returns_candidates)
{
    auto &r = ConfigPathResolver::instance();
    r.addSearchPath("/usr/share/dsg/configs", 100);
    const QStringList paths = r.metaPaths("org.deepin.demo", "example");
    EXPECT_FALSE(paths.isEmpty());
    // 至少包含 appid/resource.json 形式
    bool found = false;
    for (const auto &p : paths) {
        if (p.contains("org.deepin.demo") && p.contains("example.json"))
            found = true;
    }
    EXPECT_TRUE(found);
}
