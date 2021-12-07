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
#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H

#include <DAbstractDialog>
#include <DDialog>
#include <DStandardItem>
#include <DSuggestButton>

class ValueHandler;
class QTreeView;

DWIDGET_USE_NAMESPACE

class ExportDialog : public DDialog {
    Q_OBJECT
public:
    explicit ExportDialog(QWidget *parent = nullptr);
    void loadData(const QString &language);

public Q_SLOTS:
    void treeItemChanged(QStandardItem *item);
    bool hasChildItemChecked();
    void saveFile();

private:
    void checkAllChild(QStandardItem *item, bool check);
    QList<QStringList> exportData();

private:
    QTreeView *m_exportView = nullptr;
    DSuggestButton *m_exportBtn = nullptr;
    QScopedPointer<ValueHandler> m_getter;
    QList<DStandardItem *> m_rootItems, m_childItems;
};

#endif // EXPORTDIALOG_H
