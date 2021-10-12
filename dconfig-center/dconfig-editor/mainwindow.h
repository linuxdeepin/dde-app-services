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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <DMainWindow>

#include <DTreeWidget>
#include <DListView>
#include <QVBoxLayout>
#include <DAbstractDialog>
#include <DDialog>
class ValueHandler;
class ConfigGetter;
DWIDGET_USE_NAMESPACE

class LevelDelegate : public DStyledItemDelegate {
    Q_OBJECT
public:
    explicit LevelDelegate(QAbstractItemView *parent);
    ~LevelDelegate();
protected slots:
    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

class KeyContent : public QWidget{
    Q_OBJECT
public:
    KeyContent(const QString &key, QWidget *parent = nullptr);
    void setBaseInfo(ConfigGetter *getter, const QString &language);
    QString key() const;

private Q_SLOTS:
    void onDoubleValueChanged(double value);

Q_SIGNALS:
    void valueChanged(const QVariant &value);

private:
    QString m_key;
    QHBoxLayout *m_hLay = nullptr;
};

class Content : public QWidget {
    Q_OBJECT
public:
    explicit Content(QWidget *parent = nullptr);

    ~Content();

    void refreshResourceKeys(const QString &appid, const QString &resourceId, const QString &subpath, const QString &matchKeyId = QString());

    void clear();

    ValueHandler *getter();
    static bool isVisible(ConfigGetter *manager, const QString &key);

    static void remove(QLayout *layout);

    void setLanguage(const QString &language);

Q_SIGNALS:
    void sendValueUpdated(const QStringList &keyid, const QVariant &pre, const QVariant &now);

private Q_SLOTS:
    void onValueChanged(const QVariant &value);
private:
    QVBoxLayout *m_contentLayout = nullptr;
    QScopedPointer<ValueHandler> m_getter;
    QString m_language;
};

class HistoryDialog : public DDialog {
    Q_OBJECT
public:
    explicit HistoryDialog(QWidget *parent = nullptr);

public Q_SLOTS:
    void onSendValueUpdated(const QStringList &key, const QVariant &pre, const QVariant &now);

Q_SIGNALS:
    void refreshResourceKeys(const QString &appid, const QString &resourceId, const QString &subpath);
private:

    DListView *historyView = nullptr;
    int maxRows = 100;
};

class MainWindow : public DMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:

    void refreshApps(const QString &matchAppid = QString());

    void refreshAppResources(const QString &appid, const QString &matchResource = QString());

    void refreshResourceSubpaths(QStandardItemModel *model, const QString &appid, const QString &resourceId);

    void refreshResourceKeys(const QString &appid, const QString &resourceId, const QString &subpath, const QString &matchKeyId = QString());
private:
    void installTranslate();

private:
    QWidget *centralwidget;
    DListView *appListView;
    DListView *resourceListView;
    Content *contentView;

    HistoryDialog *historyView = nullptr;


};

#endif // MAINWINDOW_H
