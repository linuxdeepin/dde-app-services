// SPDX-FileCopyrightText: 2026 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "dconfig_global.h"
#include <QObject>

struct ConfigSyncBatchRequest;

/**
 * @brief ConfigSyncPolicy 写盘策略抽象接口
 *
 * 提供三种实现策略（由具体子类决定）：
 * - ImmediateSyncPolicy：立即写盘（用于关键配置）
 * - DelayedSyncPolicy：批量延迟写盘（默认，即原 ConfigSyncRequestCache）
 * - DeferredSyncPolicy：仅在关闭时写盘（节能模式）
 */
class ConfigSyncPolicy : public QObject
{
    Q_OBJECT
public:
    explicit ConfigSyncPolicy(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~ConfigSyncPolicy() override = default;

    /// 调度一个写盘请求（非立即执行，具体时机由子类决定）
    virtual void schedule(const ConfigCacheKey &key) = 0;

    /// 立即将所有待写盘请求刷入磁盘（阻塞，服务关闭前调用）
    virtual void flush() = 0;

    /// 清空所有待写盘请求（不写盘，仅清队列）
    virtual void clear() = 0;

Q_SIGNALS:
    void syncConfigRequest(const ConfigSyncBatchRequest &request);
};
