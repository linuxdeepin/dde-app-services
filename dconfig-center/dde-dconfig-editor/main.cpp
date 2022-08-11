// SPDX-FileCopyrightText: 2021 - 2022 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "mainwindow.h"

#include <DApplication>
#include <DWidgetUtil>

DWIDGET_USE_NAMESPACE


int main(int argc, char *argv[])
{
    DApplication a(argc, argv);
    a.setApplicationVersion("1.0.0");
    a.setApplicationName("dde-dconfig-editor");
    a.setOrganizationName("deepin");
    a.setProductIcon(QIcon::fromTheme(APP_ICON));
    a.setWindowIcon(QIcon::fromTheme(APP_ICON));

    MainWindow window;
    window.show();

    Dtk::Widget::moveToCenter(&window);

    return a.exec();
}
