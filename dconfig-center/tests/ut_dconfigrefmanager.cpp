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

#include "dconfigrefmanager.h"

class ut_DConfigRefServer : public testing::Test
{
protected:
    static void SetUpTestCase() {
    }
    static void TearDownTestCase() {
    }
    virtual void SetUp() override {
        server.reset(new RefManager);
        server->setDelayReleaseTime(0);
    }
    virtual void TearDown() override {

    }
    QScopedPointer<RefManager> server;


    const char* Service1 = "service1";
    const char* Service2 = "service2";

    const char* Resource1 = "resource1";
    const char* Resource2 = "resource2";
    const char* Resource3 = "resource3";
};

TEST_F(ut_DConfigRefServer, refResource) {

    server->refResource(Service1, Resource1);
    server->refResource(Service1, Resource2);
    server->refResource(Service1, Resource2);
    server->refResource(Service1, Resource3);

    server->refResource(Service2, Resource1);
    server->refResource(Service2, Resource2);

    ASSERT_EQ(server->getServiceCount(), 2);
    ASSERT_EQ(server->getResourceCount(), 3);

    ASSERT_EQ(server->getServiceCountOnTheResource(Resource1), 2);
    ASSERT_EQ(server->getServiceCountOnTheResource(Resource2), 2);
    ASSERT_EQ(server->getServiceCountOnTheResource(Resource3), 1);

    ASSERT_EQ(server->getResourceCountOnTheService(Service1), 3);
    ASSERT_EQ(server->getResourceCountOnTheService(Service2), 2);

    ASSERT_EQ(server->getRefResourceCountOnAllService(Resource1), 2);
    ASSERT_EQ(server->getRefResourceCountOnAllService(Resource2), 3);
    ASSERT_EQ(server->getRefResourceCountOnAllService(Resource3), 1);

    ASSERT_EQ(server->getRefResourceCountOnTheService(Service1), 4);
    ASSERT_EQ(server->getRefResourceCountOnTheService(Service2), 2);

    ASSERT_EQ(server->getRefResourceCountOnTheSR(Service1, Resource1), 1);
    ASSERT_EQ(server->getRefResourceCountOnTheSR(Service1, Resource2), 2);
    ASSERT_EQ(server->getRefResourceCountOnTheSR(Service1, Resource3), 1);
}


TEST_F(ut_DConfigRefServer, derefResource) {

    server->refResource(Service1, Resource1);
    server->refResource(Service1, Resource2);
    server->refResource(Service1, Resource2);
    server->refResource(Service1, Resource3);

    server->refResource(Service2, Resource1);
    server->refResource(Service2, Resource2);

    server->derefResource(Service1, Resource1);
    server->derefResource(Service1, Resource2);
    server->derefResource(Service1, Resource3);
    server->derefResource(Service2, Resource1);

    ASSERT_EQ(server->getServiceCount(), 2);
    ASSERT_EQ(server->getResourceCount(), 1);

    ASSERT_EQ(server->getServiceCountOnTheResource(Resource1), 0);
    ASSERT_EQ(server->getServiceCountOnTheResource(Resource2), 2);
    ASSERT_EQ(server->getServiceCountOnTheResource(Resource3), 0);

    ASSERT_EQ(server->getResourceCountOnTheService(Service1), 1);
    ASSERT_EQ(server->getResourceCountOnTheService(Service2), 1);

    ASSERT_EQ(server->getRefResourceCountOnAllService(Resource1), 0);
    ASSERT_EQ(server->getRefResourceCountOnAllService(Resource2), 2);
    ASSERT_EQ(server->getRefResourceCountOnAllService(Resource3), 0);

    ASSERT_EQ(server->getRefResourceCountOnTheService(Service1), 1);
    ASSERT_EQ(server->getRefResourceCountOnTheService(Service2), 1);

    ASSERT_EQ(server->getRefResourceCountOnTheSR(Service1, Resource1), 0);
    ASSERT_EQ(server->getRefResourceCountOnTheSR(Service1, Resource2), 1);
    ASSERT_EQ(server->getRefResourceCountOnTheSR(Service1, Resource3), 0);
}



TEST_F(ut_DConfigRefServer, releaseService) {

    server->refResource(Service1, Resource1);
    server->refResource(Service1, Resource2);
    server->refResource(Service1, Resource2);
    server->refResource(Service1, Resource3);

    server->refResource(Service2, Resource1);
    server->refResource(Service2, Resource2);

    server->releaseService(Service1);

    ASSERT_EQ(server->getServiceCount(), 1);
    ASSERT_EQ(server->getResourceCount(), 2);

    ASSERT_EQ(server->getServiceCountOnTheResource(Resource1), 1);
    ASSERT_EQ(server->getServiceCountOnTheResource(Resource2), 1);
    ASSERT_EQ(server->getServiceCountOnTheResource(Resource3), 0);

    ASSERT_EQ(server->getResourceCountOnTheService(Service1), 0);
    ASSERT_EQ(server->getResourceCountOnTheService(Service2), 2);

    ASSERT_EQ(server->getRefResourceCountOnAllService(Resource1), 1);
    ASSERT_EQ(server->getRefResourceCountOnAllService(Resource2), 1);
    ASSERT_EQ(server->getRefResourceCountOnAllService(Resource3), 0);

    ASSERT_EQ(server->getRefResourceCountOnTheService(Service1), 0);
    ASSERT_EQ(server->getRefResourceCountOnTheService(Service2), 2);

    ASSERT_EQ(server->getRefResourceCountOnTheSR(Service1, Resource1), 0);
    ASSERT_EQ(server->getRefResourceCountOnTheSR(Service1, Resource2), 0);
    ASSERT_EQ(server->getRefResourceCountOnTheSR(Service1, Resource3), 0);
}


TEST_F(ut_DConfigRefServer, setDelayReleaseTime) {

    server->refResource(Service1, Resource1);
    server->refResource(Service1, Resource2);
    server->refResource(Service1, Resource2);
    server->refResource(Service1, Resource3);

    server->refResource(Service2, Resource1);
    server->refResource(Service2, Resource2);

    server->setDelayReleaseTime(10);

    QSignalSpy spy(server.data(), &RefManager::releaseResource);

    server->releaseService(Service1);

    ASSERT_EQ(server->getServiceCount(), 1);
    ASSERT_EQ(server->getResourceCount(), 3);

    ASSERT_EQ(server->getServiceCountOnTheResource(Resource1), 1);
    ASSERT_EQ(server->getServiceCountOnTheResource(Resource2), 1);
    ASSERT_EQ(server->getServiceCountOnTheResource(Resource3), 0);

    ASSERT_EQ(server->getResourceCountOnTheService(Service1), 0);
    ASSERT_EQ(server->getResourceCountOnTheService(Service2), 2);

    ASSERT_EQ(server->getRefResourceCountOnAllService(Resource1), 1);
    ASSERT_EQ(server->getRefResourceCountOnAllService(Resource2), 1);
    ASSERT_EQ(server->getRefResourceCountOnAllService(Resource3), 0);

    ASSERT_EQ(server->getRefResourceCountOnTheService(Service1), 0);
    ASSERT_EQ(server->getRefResourceCountOnTheService(Service2), 2);

    ASSERT_EQ(server->getRefResourceCountOnTheSR(Service1, Resource1), 0);
    ASSERT_EQ(server->getRefResourceCountOnTheSR(Service1, Resource2), 0);
    ASSERT_EQ(server->getRefResourceCountOnTheSR(Service1, Resource3), 0);

    ASSERT_EQ(spy.count(), 0);

    spy.wait(server->delayReleaseTime());

    ASSERT_EQ(spy.count(), 1);

    ASSERT_EQ(server->getServiceCount(), 1);
    ASSERT_EQ(server->getResourceCount(), 2);

    ASSERT_EQ(server->getServiceCountOnTheResource(Resource1), 1);
    ASSERT_EQ(server->getServiceCountOnTheResource(Resource2), 1);
    ASSERT_EQ(server->getServiceCountOnTheResource(Resource3), 0);

    ASSERT_EQ(server->getResourceCountOnTheService(Service1), 0);
    ASSERT_EQ(server->getResourceCountOnTheService(Service2), 2);

    ASSERT_EQ(server->getRefResourceCountOnAllService(Resource1), 1);
    ASSERT_EQ(server->getRefResourceCountOnAllService(Resource2), 1);
    ASSERT_EQ(server->getRefResourceCountOnAllService(Resource3), 0);

    ASSERT_EQ(server->getRefResourceCountOnTheService(Service1), 0);
    ASSERT_EQ(server->getRefResourceCountOnTheService(Service2), 2);

    ASSERT_EQ(server->getRefResourceCountOnTheSR(Service1, Resource1), 0);
    ASSERT_EQ(server->getRefResourceCountOnTheSR(Service1, Resource2), 0);
    ASSERT_EQ(server->getRefResourceCountOnTheSR(Service1, Resource3), 0);
}
