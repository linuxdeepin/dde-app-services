// SPDX-FileCopyrightText: 2026 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include "configinfo.h"
#include "dconfig_global.h"

#include <QObject>

class DSGConfigCatalog : public QObject
{
    Q_OBJECT
public:
    explicit DSGConfigCatalog(QObject *parent = nullptr);

    void setLocalPrefix(const QString &localPrefix);

public Q_SLOTS:
    ConfigInfoList configurations() const;

Q_SIGNALS:
    void configurationsChanged();

private:
    QString m_localPrefix;
};
