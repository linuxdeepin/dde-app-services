// SPDX-FileCopyrightText: 2026 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "servicelifecycle.h"
#include <QDebug>

ServiceLifecycle::ServiceLifecycle(QObject *parent)
    : QObject(parent)
{
}

bool ServiceLifecycle::transitionTo(ServiceState next)
{
    if (!isValidTransition(m_state, next)) {
        qWarning("[ServiceLifecycle] invalid transition: %s → %s (ignored)",
                 stateName(m_state), stateName(next));
        return false;
    }
    const ServiceState prev = m_state;
    m_state = next;
    qInfo("[ServiceLifecycle] %s → %s", stateName(prev), stateName(next));
    emit stateChanged(prev, next);
    return true;
}

bool ServiceLifecycle::isValidTransition(ServiceState from, ServiceState to)
{
    switch (from) {
    case ServiceState::Initializing: return to == ServiceState::Running;
    case ServiceState::Running:      return to == ServiceState::Stopping;
    case ServiceState::Stopping:     return to == ServiceState::Stopped;
    case ServiceState::Stopped:      return false;
    }
    return false;
}

const char *ServiceLifecycle::stateName(ServiceState s)
{
    switch (s) {
    case ServiceState::Initializing: return "Initializing";
    case ServiceState::Running:      return "Running";
    case ServiceState::Stopping:     return "Stopping";
    case ServiceState::Stopped:      return "Stopped";
    }
    return "Unknown";
}
