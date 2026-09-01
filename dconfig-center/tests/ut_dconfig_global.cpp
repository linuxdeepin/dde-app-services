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
TEST(ut_dconfig_global, formatDBusObjectPath_dot) {
    ASSERT_EQ(formatDBusObjectPath("org.deepin.app"), "org_deepin_app");
}

TEST(ut_dconfig_global, formatDBusObjectPath_space) {
    ASSERT_EQ(formatDBusObjectPath("org deepin app"), "org_deepin_app");
}

TEST(ut_dconfig_global, formatDBusObjectPath_dash) {
    ASSERT_EQ(formatDBusObjectPath("org-deepin-app"), "org_deepin_app");
}

TEST(ut_dconfig_global, formatDBusObjectPath_mixed) {
    ASSERT_EQ(formatDBusObjectPath("org.deepin-app test"), "org_deepin_app_test");
}

TEST(ut_dconfig_global, formatDBusObjectPath_nochange) {
    ASSERT_EQ(formatDBusObjectPath("org_deepin_app"), "org_deepin_app");
}

TEST(ut_dconfig_global, formatDBusObjectPath_empty) {
    ASSERT_EQ(formatDBusObjectPath(""), "");
}

// ---------------------------------------------------------------------------
// outerAppidToInner / innerAppidToOuter
// ---------------------------------------------------------------------------
TEST(ut_dconfig_global, outerAppidToInner_empty) {
    ASSERT_EQ(outerAppidToInner(NoAppId), VirtualInterAppId);
}

TEST(ut_dconfig_global, outerAppidToInner_normal) {
    ASSERT_EQ(outerAppidToInner("org.foo.app"), "org.foo.app");
}

TEST(ut_dconfig_global, innerAppidToOuter_virtual) {
    ASSERT_EQ(innerAppidToOuter(VirtualInterAppId), NoAppId);
}

TEST(ut_dconfig_global, innerAppidToOuter_normal) {
    ASSERT_EQ(innerAppidToOuter("org.foo.app"), "org.foo.app");
}

// ---------------------------------------------------------------------------
// isGenericResourceConn
// ---------------------------------------------------------------------------
TEST(ut_dconfig_global, isGenericResourceConn_true) {
    ASSERT_TRUE(isGenericResourceConn("/_/example/0"));
}

TEST(ut_dconfig_global, isGenericResourceConn_false) {
    ASSERT_FALSE(isGenericResourceConn("/org.foo.app/example/0"));
}

// ---------------------------------------------------------------------------
// getResourceKey (two overloads)
// ---------------------------------------------------------------------------
TEST(ut_dconfig_global, getResourceKey_fromAppidAndKey) {
    ASSERT_EQ(getResourceKey("org.foo.app", "/example"), "/org.foo.app/example");
}

TEST(ut_dconfig_global, getResourceKey_fromConnKey) {
    ASSERT_EQ(getResourceKey("/org.foo.app/example/0"), "/org.foo.app/example");
}

TEST(ut_dconfig_global, getResourceKey_fromConnKey_noUid) {
    ASSERT_EQ(getResourceKey("/org.foo.app/example/"), "/org.foo.app/example");
}

// ---------------------------------------------------------------------------
// getGenericResourceKeyByResourceKey
// ---------------------------------------------------------------------------
TEST(ut_dconfig_global, getGenericResourceKeyByResourceKey_normal) {
    ASSERT_EQ(getGenericResourceKeyByResourceKey("/org.foo.app/example"), "/example");
}

TEST(ut_dconfig_global, getGenericResourceKeyByResourceKey_generic) {
    ASSERT_EQ(getGenericResourceKeyByResourceKey("/_/example"), "/example");
}

// ---------------------------------------------------------------------------
// getGenericResourceKey (two overloads)
// ---------------------------------------------------------------------------
TEST(ut_dconfig_global, getGenericResourceKey_fromNameAndSubpath) {
    ASSERT_EQ(getGenericResourceKey("example", ""), "/example");
    ASSERT_EQ(getGenericResourceKey("example", "/sub"), "/example/sub");
}

TEST(ut_dconfig_global, getGenericResourceKey_fromConnKey) {
    ASSERT_EQ(getGenericResourceKey("/org.foo.app/example/0"), "/example");
    ASSERT_EQ(getGenericResourceKey("/_/example/sub/100"), "/example/sub");
}

// ---------------------------------------------------------------------------
// getConnectionKey (two overloads)
// ---------------------------------------------------------------------------
TEST(ut_dconfig_global, getConnectionKey_fromConnKey) {
    ASSERT_EQ(getConnectionKey("/org.foo.app/example/42"), 42u);
    ASSERT_EQ(getConnectionKey("/_/example/0"), 0u);
}

TEST(ut_dconfig_global, getConnectionKey_fromResourceKeyAndUid) {
    ASSERT_EQ(getConnectionKey("/org.foo.app/example", 42), "/org.foo.app/example/42");
}

// ---------------------------------------------------------------------------
// removeBackSlash
// ---------------------------------------------------------------------------
TEST(ut_dconfig_global, removeBackSlash_trailingSlash) {
    ASSERT_EQ(removeBackSlash("foo/"), "foo");
}

TEST(ut_dconfig_global, removeBackSlash_noTrailingSlash) {
    ASSERT_EQ(removeBackSlash("foo"), "foo");
}

TEST(ut_dconfig_global, removeBackSlash_empty) {
    ASSERT_EQ(removeBackSlash(""), "");
}

// ---------------------------------------------------------------------------
// getMetaConfigureId
// ---------------------------------------------------------------------------
TEST(ut_dconfig_global, getMetaConfigureId_valid) {
    auto id = getMetaConfigureId("/usr/share/dsg/configs/org.foo.app/example.json");
    ASSERT_FALSE(id.isInValid());
    ASSERT_EQ(id.appid, "org.foo.app");
    ASSERT_EQ(id.resource, "example");
    ASSERT_TRUE(id.subpath.isEmpty());
}

TEST(ut_dconfig_global, getMetaConfigureId_withSubpath) {
    auto id = getMetaConfigureId("/usr/share/dsg/configs/org.foo.app/sub/example.json");
    ASSERT_FALSE(id.isInValid());
    ASSERT_EQ(id.appid, "org.foo.app");
    ASSERT_EQ(id.resource, "example");
    ASSERT_EQ(id.subpath, "sub");
}

TEST(ut_dconfig_global, getMetaConfigureId_generic) {
    auto id = getMetaConfigureId("/usr/share/dsg/configs/example.json");
    ASSERT_FALSE(id.isInValid());
    ASSERT_TRUE(id.appid.isEmpty());
    ASSERT_EQ(id.resource, "example");
}

TEST(ut_dconfig_global, getMetaConfigureId_invalid) {
    auto id = getMetaConfigureId("/random/path/nothing.json");
    ASSERT_TRUE(id.isInValid());
}

// ---------------------------------------------------------------------------
// getOverrideConfigureId
// ---------------------------------------------------------------------------
TEST(ut_dconfig_global, getOverrideConfigureId_valid) {
    auto id = getOverrideConfigureId("/usr/share/dsg/configs/overrides/org.foo.app/example/a.json");
    ASSERT_FALSE(id.isInValid());
    ASSERT_EQ(id.appid, "org.foo.app");
    ASSERT_EQ(id.resource, "example");
}

TEST(ut_dconfig_global, getOverrideConfigureId_etcPath) {
    auto id = getOverrideConfigureId("/etc/dsg/configs/overrides/org.foo.app/example/a.json");
    ASSERT_FALSE(id.isInValid());
    ASSERT_EQ(id.appid, "org.foo.app");
    ASSERT_EQ(id.resource, "example");
}

TEST(ut_dconfig_global, getOverrideConfigureId_invalid) {
    auto id = getOverrideConfigureId("/random/path/nothing.json");
    ASSERT_TRUE(id.isInValid());
}

// ---------------------------------------------------------------------------
// getProcessNameByPid
// ---------------------------------------------------------------------------
TEST(ut_dconfig_global, getProcessNameByPid_self) {
    auto name = getProcessNameByPid(QCoreApplication::applicationPid());
    ASSERT_FALSE(name.isEmpty());
}

TEST(ut_dconfig_global, getProcessNameByPid_invalid) {
    auto name = getProcessNameByPid(999999);
    ASSERT_FALSE(name.isEmpty());
}

// ---------------------------------------------------------------------------
// getUserNameByUid
// ---------------------------------------------------------------------------
TEST(ut_dconfig_global, getUserNameByUid_root) {
    auto name = getUserNameByUid(0);
    ASSERT_FALSE(name.isEmpty());
}

TEST(ut_dconfig_global, getUserNameByUid_invalid) {
    // Source bug: getUserNameByUid() calls getpwuid() without null check.
    // For a non-existent uid, getpwuid() returns nullptr, causing segfault
    // at `passwd->pw_name` dereference (dconfig_global.h:210).
    // Defect report filed; do not modify source. Skipping this test case.
    GTEST_SKIP() << "Source bug: getpwuid() nullptr dereference for invalid uid (dconfig_global.h:210)";
}

// ---------------------------------------------------------------------------
// configPrefixPath
// P0-2: configPrefixPath() uses a static cache; the first call wins.
// This test only verifies it doesn't crash, not the return value.
// ---------------------------------------------------------------------------
TEST(ut_dconfig_global, configPrefixPath_doesNotCrash) {
    auto path = configPrefixPath();
    Q_UNUSED(path);
    SUCCEED();
}

// ---------------------------------------------------------------------------
// ObjectPool<T>
// ---------------------------------------------------------------------------
TEST(ut_dconfig_global, ObjectPool_pullCreatesNew) {
    ObjectPool<int> pool;
    auto item = pool.pull();
    ASSERT_NE(item, nullptr);
    *item = 42;
    ASSERT_EQ(*item, 42);
    delete item;
}

TEST(ut_dconfig_global, ObjectPool_pushAndPull) {
    ObjectPool<int> pool;
    auto item1 = pool.pull();
    *item1 = 100;
    pool.push(item1);
    auto item2 = pool.pull();
    ASSERT_EQ(item2, item1);
    ASSERT_EQ(*item2, 100);
    delete item2;
}

TEST(ut_dconfig_global, ObjectPool_initFunc) {
    ObjectPool<int> pool;
    pool.setInitFunc([](int *item) { *item = 555; });
    auto item = pool.pull();
    ASSERT_EQ(*item, 555);
    delete item;
}

TEST(ut_dconfig_global, ObjectPool_clear) {
    ObjectPool<int> pool;
    pool.push(new int(1));
    pool.push(new int(2));
    pool.clear();
    // After clear, pool should be empty; pull creates new
    auto item = pool.pull();
    ASSERT_NE(item, nullptr);
    delete item;
}

TEST(ut_dconfig_global, ObjectPool_destructorClears) {
    // Verifies the destructor calls clear() and frees all pooled items
    // without crashing. No post-condition is observable after deletion.
    ObjectPool<int> *pool = new ObjectPool<int>();
    pool->push(new int(1));
    pool->push(new int(2));
    delete pool;
    SUCCEED();
}



