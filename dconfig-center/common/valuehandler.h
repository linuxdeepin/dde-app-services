/*
* Copyright (C) 2021 ~ 2021 Uniontech Software Technology Co.,Ltd.
*
* Author:     Ye ShanShan <yeshanshan@uniontech.com>
*
* Maintainer: Ye ShanShan <yeshanshan@uniontech.com>>
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
#ifndef VALUEHANDLER_H
#define VALUEHANDLER_H

#include <QString>
#include <QVariant>

class ConfigGetter {
public:
    virtual ~ConfigGetter();

    virtual QStringList keyList() const = 0;
    virtual void setValue(const QString &key, const QVariant &value) = 0;
    virtual QVariant value(const QString &key) const = 0;
    virtual void reset(const QString &key) = 0;
    virtual QString permissions(const QString &key) const = 0;
    virtual QString visibility(const QString &key) const = 0;
    virtual QString displayName(const QString &key, const QString &locale) = 0;
    virtual QString description(const QString &key, const QString &locale) = 0;
    virtual QString version() const = 0;

    virtual void release() = 0;
};

class ValueHandler : public QObject
{
    Q_OBJECT
public:
    explicit ValueHandler(const QString &appid, const QString &fileName, const QString &subpath);
    ~ValueHandler();

    ConfigGetter *createManager();

Q_SIGNALS:
    void valueChanged(const QString &key);

public:
    const QString appid;
    const QString fileName;
    const QString subpath;
};

#endif // VALUEHANDLER_H
