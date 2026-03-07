// SPDX-FileCopyrightText: 2026 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QObject>

/**
 * @brief 服务生命周期状态机
 *
 * 状态转换规则：
 *   Initializing → Running → Stopping → Stopped
 *
 * 非法转换（如 Stopped → Running）会打印 warning 并被忽略。
 */
enum class ServiceState {
    Initializing,   ///< 服务正在初始化
    Running,        ///< 服务正常运行中
    Stopping,       ///< 服务正在停止（收到 SIGTERM 或 exit() 调用）
    Stopped         ///< 服务已完全停止
};

class ServiceLifecycle : public QObject
{
    Q_OBJECT
public:
    explicit ServiceLifecycle(QObject *parent = nullptr);

    ServiceState state() const { return m_state; }
    bool isRunning()   const { return m_state == ServiceState::Running; }
    bool isStopping()  const { return m_state == ServiceState::Stopping; }

    /**
     * @brief 请求状态转换
     * @return true 转换成功，false 转换被拒绝（非法路径）
     */
    bool transitionTo(ServiceState next);

    static const char *stateName(ServiceState s);

Q_SIGNALS:
    void stateChanged(ServiceState from, ServiceState to);

private:
    static bool isValidTransition(ServiceState from, ServiceState to);
    ServiceState m_state = ServiceState::Initializing;
};
