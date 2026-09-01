// SPDX-FileCopyrightText: 2021 - 2022 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "mainwindow.h"
#include "helper.hpp"

#include <DApplication>
#include <DWidgetUtil>

DWIDGET_USE_NAMESPACE


int main(int argc, char *argv[])
{
    setenv("DSG_DATA_DIRS", "/usr/share/dsg:/var/lib/linglong/entries/share/dsg", 0);

    DApplication a(argc, argv);
    a.setApplicationVersion(VERSION);
    a.setApplicationName("dde-dconfig-editor");
    a.setOrganizationName("deepin");
    a.loadTranslator();
    loadTranslation("dde-dconfig-editor");
    a.setProductIcon(QIcon::fromTheme(APP_ICON));
    a.setWindowIcon(QIcon::fromTheme(APP_ICON));
    MainWindow window;
    window.show();

    Dtk::Widget::moveToCenter(&window);

    return a.exec();
}
