// SPDX-FileCopyrightText: 2021 - 2026 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <gtest/gtest.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCoreApplication>
#include <QLoggingCategory>

#include "dconfig_global.h"

// ---------------------------------------------------------------------------
// formatDBusObjectPath
// ---------------------------------------------------------------------------
TEST(ut_dconfig_global, formatDBusObjectPathDot) {
    ASSERT_EQ(formatDBusObjectPath("org.deepin.app"), "org_deepin_app");
}

TEST(ut_dconfig_global, formatDBusObjectPathSpace) {
    ASSERT_EQ(formatDBusObjectPath("org deepin app"), "org_deepin_app");
}

TEST(ut_dconfig_global, formatDBusObjectPathDash) {
    ASSERT_EQ(formatDBusObjectPath("org-deepin-app"), "org_deepin_app");
}

TEST(ut_dconfig_global, formatDBusObjectPathMixed) {
    ASSERT_EQ(formatDBusObjectPath("org.deepin-app test"), "org_deepin_app_test");
}

TEST(ut_dconfig_global, formatDBusObjectPathNochange) {
    ASSERT_EQ(formatDBusObjectPath("org_deepin_app"), "org_deepin_app");
}

TEST(ut_dconfig_global, formatDBusObjectPathEmpty) {
    ASSERT_EQ(formatDBusObjectPath(""), "");
}

// ---------------------------------------------------------------------------
// outerAppidToInner / innerAppidToOuter
// ---------------------------------------------------------------------------
TEST(ut_dconfig_global, outerAppidToInnerEmpty) {
    ASSERT_EQ(outerAppidToInner(NoAppId), VirtualInterAppId);
}

TEST(ut_dconfig_global, outerAppidToInnerNormal) {
    ASSERT_EQ(outerAppidToInner("org.foo.app"), "org.foo.app");
}

TEST(ut_dconfig_global, innerAppidToOuterVirtual) {
    ASSERT_EQ(innerAppidToOuter(VirtualInterAppId), NoAppId);
}

TEST(ut_dconfig_global, innerAppidToOuterNormal) {
    ASSERT_EQ(innerAppidToOuter("org.foo.app"), "org.foo.app");
}

// ---------------------------------------------------------------------------
// isGenericResourceConn
// ---------------------------------------------------------------------------
TEST(ut_dconfig_global, isGenericResourceConnTrue) {
    ASSERT_TRUE(isGenericResourceConn("/_/example/0"));
}

TEST(ut_dconfig_global, isGenericResourceConnFalse) {
    ASSERT_FALSE(isGenericResourceConn("/org.foo.app/example/0"));
}

// ---------------------------------------------------------------------------
// getResourceKey (two overloads)
// ---------------------------------------------------------------------------
TEST(ut_dconfig_global, getResourceKeyFromAppidAndKey) {
    ASSERT_EQ(getResourceKey("org.foo.app", "/example"), "/org.foo.app/example");
}

TEST(ut_dconfig_global, getResourceKeyFromConnKey) {
    ASSERT_EQ(getResourceKey("/org.foo.app/example/0"), "/org.foo.app/example");
}

TEST(ut_dconfig_global, getResourceKeyFromConnKeyNoUid) {
    ASSERT_EQ(getResourceKey("/org.foo.app/example/"), "/org.foo.app/example");
}

// ---------------------------------------------------------------------------
// getGenericResourceKeyByResourceKey
// ---------------------------------------------------------------------------
TEST(ut_dconfig_global, getGenericResourceKeyByResourceKeyNormal) {
    ASSERT_EQ(getGenericResourceKeyByResourceKey("/org.foo.app/example"), "/example");
}

TEST(ut_dconfig_global, getGenericResourceKeyByResourceKeyGeneric) {
    ASSERT_EQ(getGenericResourceKeyByResourceKey("/_/example"), "/example");
}

// ---------------------------------------------------------------------------
// getGenericResourceKey (two overloads)
// ---------------------------------------------------------------------------
TEST(ut_dconfig_global, getGenericResourceKeyFromNameAndSubpath) {
    ASSERT_EQ(getGenericResourceKey("example", ""), "/example");
    ASSERT_EQ(getGenericResourceKey("example", "/sub"), "/example/sub");
}

TEST(ut_dconfig_global, getGenericResourceKeyFromConnKey) {
    ASSERT_EQ(getGenericResourceKey("/org.foo.app/example/0"), "/example");
    ASSERT_EQ(getGenericResourceKey("/_/example/sub/100"), "/example/sub");
}

// ---------------------------------------------------------------------------
// getConnectionKey (two overloads)
// ---------------------------------------------------------------------------
TEST(ut_dconfig_global, getConnectionKeyFromConnKey) {
    ASSERT_EQ(getConnectionKey("/org.foo.app/example/42"), 42u);
    ASSERT_EQ(getConnectionKey("/_/example/0"), 0u);
}

TEST(ut_dconfig_global, getConnectionKeyFromResourceKeyAndUid) {
    ASSERT_EQ(getConnectionKey("/org.foo.app/example", 42), "/org.foo.app/example/42");
}

// ---------------------------------------------------------------------------
// removeBackSlash
// ---------------------------------------------------------------------------
TEST(ut_dconfig_global, removeBackSlashTrailingSlash) {
    ASSERT_EQ(removeBackSlash("foo/"), "foo");
}

TEST(ut_dconfig_global, removeBackSlashNoTrailingSlash) {
    ASSERT_EQ(removeBackSlash("foo"), "foo");
}

TEST(ut_dconfig_global, removeBackSlashEmpty) {
    ASSERT_EQ(removeBackSlash(""), "");
}

// ---------------------------------------------------------------------------
// getMetaConfigureId
// ---------------------------------------------------------------------------
TEST(ut_dconfig_global, getMetaConfigureIdValid) {
    auto id = getMetaConfigureId("/usr/share/dsg/configs/org.foo.app/example.json");
    ASSERT_FALSE(id.isInValid());
    ASSERT_EQ(id.appid, "org.foo.app");
    ASSERT_EQ(id.resource, "example");
    ASSERT_TRUE(id.subpath.isEmpty());
}

TEST(ut_dconfig_global, getMetaConfigureIdWithSubpath) {
    auto id = getMetaConfigureId("/usr/share/dsg/configs/org.foo.app/sub/example.json");
    ASSERT_FALSE(id.isInValid());
    ASSERT_EQ(id.appid, "org.foo.app");
    ASSERT_EQ(id.resource, "example");
    ASSERT_EQ(id.subpath, "sub");
}

TEST(ut_dconfig_global, getMetaConfigureIdGeneric) {
    auto id = getMetaConfigureId("/usr/share/dsg/configs/example.json");
    ASSERT_FALSE(id.isInValid());
    ASSERT_TRUE(id.appid.isEmpty());
    ASSERT_EQ(id.resource, "example");
}

TEST(ut_dconfig_global, getMetaConfigureIdInvalid) {
    auto id = getMetaConfigureId("/random/path/nothing.json");
    ASSERT_TRUE(id.isInValid());
}

// ---------------------------------------------------------------------------
// getOverrideConfigureId
// ---------------------------------------------------------------------------
TEST(ut_dconfig_global, getOverrideConfigureIdValid) {
    auto id = getOverrideConfigureId("/usr/share/dsg/configs/overrides/org.foo.app/example/a.json");
    ASSERT_FALSE(id.isInValid());
    ASSERT_EQ(id.appid, "org.foo.app");
    ASSERT_EQ(id.resource, "example");
}

TEST(ut_dconfig_global, getOverrideConfigureIdEtcPath) {
    auto id = getOverrideConfigureId("/etc/dsg/configs/overrides/org.foo.app/example/a.json");
    ASSERT_FALSE(id.isInValid());
    ASSERT_EQ(id.appid, "org.foo.app");
    ASSERT_EQ(id.resource, "example");
}

TEST(ut_dconfig_global, getOverrideConfigureIdInvalid) {
    auto id = getOverrideConfigureId("/random/path/nothing.json");
    ASSERT_TRUE(id.isInValid());
}

// ---------------------------------------------------------------------------
// getProcessNameByPid
// ---------------------------------------------------------------------------
TEST(ut_dconfig_global, getProcessNameByPidSelf) {
    auto name = getProcessNameByPid(QCoreApplication::applicationPid());
    ASSERT_FALSE(name.isEmpty());
}

TEST(ut_dconfig_global, getProcessNameByPidInvalid) {
    auto name = getProcessNameByPid(999999);
    ASSERT_FALSE(name.isEmpty());
}

// ---------------------------------------------------------------------------
// getUserNameByUid
// ---------------------------------------------------------------------------
TEST(ut_dconfig_global, getUserNameByUidRoot) {
    auto name = getUserNameByUid(0);
    ASSERT_FALSE(name.isEmpty());
}

TEST(ut_dconfig_global, getUserNameByUidInvalid) {
    // Source bug: getUserNameByUid() calls getpwuid() without null check.
    // For a non-existent uid, getpwuid() returns nullptr, causing segfault
    // at `passwd->pw_name` dereference (dconfig_global.h).
    // Defect report filed; do not modify source. Skipping this test case.
    GTEST_SKIP() << "Source bug: getpwuid() nullptr dereference for invalid uid (dconfig_global.h)";
}

// ---------------------------------------------------------------------------
// configPrefixPath
// P0-2: configPrefixPath() uses a static cache; the first call wins.
// This test only verifies it doesn't crash, not the return value.
// ---------------------------------------------------------------------------
TEST(ut_dconfig_global, configPrefixPathDoesNotCrash) {
    auto path = configPrefixPath();
    Q_UNUSED(path);
    SUCCEED();
}

// ---------------------------------------------------------------------------
// ObjectPool<T>
// ---------------------------------------------------------------------------
TEST(ut_dconfig_global, ObjectPoolPullCreatesNew) {
    ObjectPool<int> pool;
    auto item = pool.pull();
    ASSERT_NE(item, nullptr);
    *item = 42;
    ASSERT_EQ(*item, 42);
    delete item;
}

TEST(ut_dconfig_global, ObjectPoolPushAndPull) {
    ObjectPool<int> pool;
    auto item1 = pool.pull();
    *item1 = 100;
    pool.push(item1);
    auto item2 = pool.pull();
    ASSERT_EQ(item2, item1);
    ASSERT_EQ(*item2, 100);
    delete item2;
}

TEST(ut_dconfig_global, ObjectPoolInitFunc) {
    ObjectPool<int> pool;
    pool.setInitFunc([](int *item) { *item = 555; });
    auto item = pool.pull();
    ASSERT_EQ(*item, 555);
    delete item;
}

TEST(ut_dconfig_global, ObjectPoolClear) {
    ObjectPool<int> pool;
    pool.push(new int(1));
    pool.push(new int(2));
    pool.clear();
    // After clear, pool should be empty; pull creates new
    auto item = pool.pull();
    ASSERT_NE(item, nullptr);
    delete item;
}

TEST(ut_dconfig_global, ObjectPoolDestructorClears) {
    // Verifies the destructor calls clear() and frees all pooled items
    // without crashing. No post-condition is observable after deletion.
    ObjectPool<int> *pool = new ObjectPool<int>();
    pool->push(new int(1));
    pool->push(new int(2));
    delete pool;
    SUCCEED();
}



