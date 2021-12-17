/*
* Copyright (C) 2021 Uniontech Software Technology Co.,Ltd.
*
* Author:     Wang fei <wangfeia@uniontech.com>
*
* Maintainer: Wang fei <wangfeia@uniontech.com>>
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
#ifndef OEMDIALOG_H
#define OEMDIALOG_H

#include <DDialog>
#include <DStandardItem>
#include <DSuggestButton>
#include <QCloseEvent>

class ValueHandler;
class QTreeView;
class ConfigGetter;
DWIDGET_USE_NAMESPACE

class OEMDialog : public DDialog {
    Q_OBJECT
public:
    explicit OEMDialog(QWidget *parent = nullptr);
    void loadData(const QString &language);

public Q_SLOTS:
    void treeItemChanged(DStandardItem *item);
    void saveFiles();
    void displayChangedResult();
protected:
    void closeEvent(QCloseEvent *event) override;
private:
    void createJsonFile(const QString &fileName, const QList<DStandardItem *> &items);
    QWidget* getItemWidget(ConfigGetter *getter, DStandardItem *item);
    void saveOverridesFiles(const QString &dirName);
    void saveCSVFile(const QString &dirName);
private:
    QTreeView *m_exportView = nullptr;
    QStandardItemModel *m_model = nullptr;
    DSuggestButton *m_exportBtn = nullptr;
    QScopedPointer<ValueHandler> m_getter;
    QMap<QString, QList<DStandardItem *>> m_overrides;
};

#endif // OEMDIALOG_H
