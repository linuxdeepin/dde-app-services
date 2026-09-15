// SPDX-FileCopyrightText: 2021 - 2023 Uniontech Software Technology Co.,Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "mainwindow.h"


#include "helper.hpp"
#include "resourcecatalogclient.h"
#include "valuehandler.h"
#include "iteminfo.h"
#include "exportdialog.h"
#include "oemdialog.h"

#include <QHBoxLayout>
#include <DLabel>
#include <DIconButton>
#include <DSwitchButton>
#include <DLineEdit>
#include <DSearchEdit>
#include <DSlider>
#include <DTitlebar>
#include <DStatusBar>
#include <DStyle>
#include <QScrollArea>
#include <QSplitter>
#include <QTime>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <DSpinBox>
#include <DDBusSender>
#include <QDBusPendingReply>
#include <QFileDialog>
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QActionGroup>
#include <QScreen>
#include <dinputdialog.h>

#include <algorithm>

namespace {
QStringList sortedStrings(QStringList values)
{
    std::sort(values.begin(), values.end(), [](const QString &left, const QString &right) {
        const int insensitiveOrder = QString::compare(left, right, Qt::CaseInsensitive);
        return insensitiveOrder == 0 ? left < right : insensitiveOrder < 0;
    });
    return values;
}

bool normalizeDynamicSubpath(const QString &input, QString *normalized)
{
    QString subpath = input.trimmed();
    if (subpath.isEmpty() || subpath.contains(QLatin1Char('\\')))
        return false;

    const QStringList components = subpath.split(QLatin1Char('/'), Qt::SkipEmptyParts);
    if (components.isEmpty())
        return false;

    for (const QString &component : components) {
        if (component == QLatin1String(".") || component == QLatin1String(".."))
            return false;
    }

    if (!subpath.startsWith(QLatin1Char('/')))
        subpath.prepend(QLatin1Char('/'));
    *normalized = QDir::cleanPath(subpath);
    return true;
}
}

MainWindow::MainWindow(QWidget *parent) :
    DMainWindow(parent)
{
    appIdToNameMaps[NoAppId] = VirtualAppName;
    const QSize available = screen() ? screen()->availableGeometry().size() : QSize(1366, 768);
    const QSize defaultSize(qMin(1200, qMax(900, available.width() * 9 / 10)),
                            qMin(760, qMax(600, available.height() * 9 / 10)));
    resize(defaultSize.boundedTo(QSize(1100, 700)));
    setMinimumSize(QSize(800, 520));
    centralwidget = new QWidget(this);
    centralwidget->setObjectName(QStringLiteral("centralwidget"));

    auto layout = new QVBoxLayout(centralwidget);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);
    auto navigationWidget = new QWidget(centralwidget);
    navigationWidget->setObjectName(QStringLiteral("navigationWidget"));
    navigationWidget->setMinimumHeight(32);
    navigationWidget->setBackgroundRole(QPalette::AlternateBase);
    navigationWidget->setAutoFillBackground(true);
    auto navigationLayout = new QHBoxLayout(navigationWidget);
    navigationLayout->setContentsMargins(12, 4, 12, 4);
    navigationLayout->setSpacing(8);

    navigationAppLabel = new DLabel(tr("Select an application and configuration"), navigationWidget);
    navigationResourceLabel = new DLabel(navigationWidget);
    navigationSubpathLabel = new DLabel(navigationWidget);
    navigationResourceSeparator = new DLabel(QStringLiteral(">"), navigationWidget);
    navigationSubpathSeparator = new DLabel(QStringLiteral(">"), navigationWidget);

    QFont navigationFont = navigationAppLabel->font();
    navigationFont.setBold(true);
    const QList<DLabel *> navigationLabels{navigationAppLabel, navigationResourceLabel, navigationSubpathLabel};
    for (DLabel *label : navigationLabels) {
        label->setFont(navigationFont);
        label->setElideMode(Qt::ElideMiddle);
        label->setAlignment(Qt::AlignCenter);
        label->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
        label->setMaximumWidth(320);
    }
    navigationResourceSeparator->hide();
    navigationSubpathSeparator->hide();
    navigationResourceLabel->hide();
    navigationSubpathLabel->hide();
    navigationLayout->addStretch();
    navigationLayout->addWidget(navigationAppLabel);
    navigationLayout->addWidget(navigationResourceSeparator);
    navigationLayout->addWidget(navigationResourceLabel);
    navigationLayout->addWidget(navigationSubpathSeparator);
    navigationLayout->addWidget(navigationSubpathLabel);
    navigationLayout->addStretch();
    QSplitter *hSplitter = new QSplitter(Qt::Horizontal, centralwidget);
    hSplitter->setLineWidth(1);
    hSplitter->setHandleWidth(6);

    appListView = new DListView();
    appListView->setObjectName(QStringLiteral("appListView"));
    appListView->setTextElideMode(Qt::ElideMiddle);
    appListView->setWordWrap(false);
    appListView->setMinimumWidth(180);
    appListView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    auto appHeaderWidget = new QWidget(appListView);
    auto appHeaderLayout = new QHBoxLayout(appHeaderWidget);
    appHeaderLayout->setContentsMargins(0, 0, 8, 0);
    appHeaderLayout->setSpacing(2);

    auto appHeader = new DSearchEdit(appHeaderWidget);
    appHeader->setPlaceHolder(tr("appid"));
    appHeader->setPlaceholderText(tr("input filter appid"));
    QObject::connect(appHeader, &DSearchEdit::textChanged, [this](const QString &appid){
        refreshApps(appid);
    });
    appHeaderLayout->addWidget(appHeader);

    auto addAppidButton = new DIconButton(DStyle::SP_AddButton, appHeaderWidget);
    addAppidButton->setToolTip(tr("specify appid"));
    addAppidButton->setAccessibleName(tr("specify appid"));
    addAppidButton->setFlat(true);
    addAppidButton->setFixedSize(32, 32);
    addAppidButton->setIconSize(QSize(20, 20));
    connect(addAppidButton, &DIconButton::clicked, this, &MainWindow::addAppid);
    appHeaderLayout->addWidget(addAppidButton);

    appListView->addHeaderWidget(appHeaderWidget);

    hSplitter->addWidget(appListView);

    resourceListView = new DListView();
    resourceListView->setObjectName(QStringLiteral("resourceListView"));
    resourceListView->setTextElideMode(Qt::ElideMiddle);
    resourceListView->setMinimumWidth(220);
    resourceListView->setSpacing(3);
    resourceListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    resourceListView->setItemDelegate(new LevelDelegate(resourceListView));
    resourceListView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    resourceListView->setModel(new QStandardItemModel());
    auto resourceHeader = new DSearchEdit();
    resourceHeader->setMinimumWidth(200);
    resourceHeader->setPlaceHolder(tr("resource"));
    resourceHeader->setPlaceholderText(tr("input filter resource"));
    QObject::connect(resourceHeader, &DSearchEdit::textChanged, [this](const QString &resourceid){
        const auto &appid = appListView->model()->data(appListView->currentIndex(), ConfigUserRole + 2).toString();
        refreshAppResources(appid, resourceid);
    });
    resourceListView->addHeaderWidget(resourceHeader);

    hSplitter->addWidget(resourceListView);

    auto contentViewWraper = new QWidget(this);
    contentViewWraper->setMinimumWidth(380);
    auto contentViewLayout = new QVBoxLayout(contentViewWraper);
    contentViewLayout->setSpacing(0);
    contentViewLayout->setContentsMargins(0, 0, 0, 0);
    contentView = new Content();
    contentView->setObjectName(QStringLiteral("contentView"));

    auto contentHeader = new DSearchEdit();
    contentHeader->setPlaceHolder(tr("keys"));
    contentHeader->setPlaceholderText(tr("input filter keys"));
    QObject::connect(contentHeader, &DSearchEdit::textChanged, [this](const QString &keyid){
        const auto &appid = appListView->model()->data(appListView->currentIndex(), ConfigUserRole + 2).toString();
        const auto &resourceId = resourceListView->model()->data(resourceListView->currentIndex(), ConfigUserRole + 3).toString();
        if (appid.isEmpty() || resourceId.isEmpty()) {
            return;
        }
        const auto &subpath = resourceListView->model()->data(resourceListView->currentIndex(), ConfigUserRole + 4).toString();
        refreshResourceKeys(appid, resourceId, subpath, keyid);
    });
    QObject::connect(contentView, &Content::requestRefreshResourceKeys, this, [this, contentHeader](){
        const auto &appid = appListView->model()->data(appListView->currentIndex(), ConfigUserRole + 2).toString();
        const auto &resourceId = resourceListView->model()->data(resourceListView->currentIndex(), ConfigUserRole + 3).toString();
        if (appid.isEmpty() || resourceId.isEmpty()) {
            return;
        }
        const auto &subpath = resourceListView->model()->data(resourceListView->currentIndex(), ConfigUserRole + 4).toString();
        refreshResourceKeys(appid, resourceId, subpath, contentHeader->text());
    });

    contentViewLayout->addWidget(contentHeader);
    auto scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setWidget(contentView);

    contentViewLayout->addWidget(scrollArea);
    hSplitter->addWidget(contentViewWraper);

    layout->addWidget(hSplitter, 1);
    layout->addWidget(navigationWidget);
    hSplitter->setChildrenCollapsible(false);
    hSplitter->setStretchFactor(0, 24);
    hSplitter->setStretchFactor(1, 28);
    hSplitter->setStretchFactor(2, 48);
    hSplitter->setSizes({240, 280, 500});

    connect(appListView, &QListView::clicked, this, [this, resourceHeader](const QModelIndex &index){
        if (auto model = qobject_cast<QStandardItemModel*>(appListView->model())){
            const QString appid = model->data(index, ConfigUserRole + 2).toString();
            this->refreshAppResources(appid, resourceHeader->text());
            updateNavigation(appid);
            emit resourceListView->clicked(resourceListView->currentIndex());
        }
    });

    connect(resourceListView, &QListView::clicked, this, [this, scrollArea, contentHeader](const QModelIndex &index){
        if (auto model = qobject_cast<QStandardItemModel*>(resourceListView->model())){
            const auto &type = model->data(index, ConfigUserRole + 1).toInt();
            if (type & ConfigType::ResourceType || type == ConfigType::SubpathType) {
                const auto &appid = model->data(index, ConfigUserRole + 2).toString();
                const auto &resourceId = model->data(index, ConfigUserRole + 3).toString();
                const auto &subpath = model->data(index, ConfigUserRole + 4).toString();
                if (resourceId.isEmpty()) {
                    qWarning() << "error" << appid << resourceId;
                    return;
                }
                refreshResourceKeys(appid, resourceId, subpath, contentHeader->text());
                updateNavigation(appid, resourceId, subpath);
            } else {
                contentView->clear();
            }
            scrollArea->setWidget(contentView);
        }
    });

    resourceListView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(resourceListView, &QWidget::customContextMenuRequested, this, [this](const QPoint &position) {
        if (auto model = qobject_cast<QStandardItemModel*>(resourceListView->model())){
            const auto index = resourceListView->indexAt(position);
            if (!index.isValid())
                return;
            resourceListView->setCurrentIndex(index);
            const auto &type = model->data(index, ConfigUserRole + 1).toInt();
            if (type & ConfigType::ResourceType || type == ConfigType::SubpathType) {
                const auto &appid = model->data(index, ConfigUserRole + 2).toString();
                const auto &resourceId = model->data(index, ConfigUserRole + 3).toString();
                const auto &subpath = model->data(index, ConfigUserRole + 4).toString();
                if (resourceId.isEmpty()) {
                    qWarning() << "error" << appid << resourceId;
                    return;
                }
                onCustomResourceMenuRequested(appid, resourceId, subpath,
                                              type != ConfigType::SubpathType);
            }
        }
    });

    // set history
    DTitlebar *titlebar = this->titlebar();
    titlebar->setIcon(QIcon(APP_ICON));
    auto refreshButton = new DIconButton(QStyle::SP_BrowserReload, titlebar);
    refreshButton->setFlat(true);
    refreshButton->setFixedSize(32, 32);
    refreshButton->setIconSize(QSize(20, 20));
    refreshButton->setToolTip(tr("refresh configuration"));
    refreshButton->setAccessibleName(tr("refresh configuration"));
    titlebar->addWidget(refreshButton, Qt::AlignLeft);
    connect(refreshButton, &DIconButton::clicked, this, [this, appHeader] {
        const QString selectedApp = appListView->model()
                ? appListView->model()->data(appListView->currentIndex(), ConfigUserRole + 2).toString()
                : QString();
        ResourceCatalogClient::instance().refresh();
        refreshApps(appHeader->text());

        auto model = appListView->model();
        QModelIndex target;
        for (int row = 0; model && row < model->rowCount(); ++row) {
            const QModelIndex index = model->index(row, 0);
            const QString appid = model->data(index, ConfigUserRole + 2).toString();
            if ((!selectedApp.isEmpty() && appid == selectedApp)
                    || (!target.isValid() && !appid.isEmpty())) {
                target = index;
                if (appid == selectedApp)
                    break;
            }
        }
        if (target.isValid()) {
            appListView->setCurrentIndex(target);
            emit appListView->clicked(target);
        } else {
            resourceListView->model()->removeRows(0, resourceListView->model()->rowCount());
            contentView->clear();
            navigationAppLabel->setText(tr("Select an application and configuration"));
            navigationResourceLabel->hide();
            navigationSubpathLabel->hide();
            navigationResourceSeparator->hide();
            navigationSubpathSeparator->hide();
        }
    });
    connect(titlebar->menu()->addAction(tr("setting history")), &QAction::triggered, [this](){
        qInfo() << "show history view";
        historyView->show();

        const auto &topLeft = historyView->parentWidget()->geometry().topLeft();
        historyView->move(topLeft.x() - historyView->geometry().width(), topLeft.y());
    });
    historyView = new HistoryDialog(this);
    historyView->setMinimumSize(QSize(380, 520));
    historyView->resize(QSize(520, 680));
    QObject::connect(contentView, &Content::sendValueUpdated, historyView, &HistoryDialog::onSendValueUpdated);
    QObject::connect(historyView, &HistoryDialog::refreshResourceKeys, this, [this, contentHeader](const QString &appid, const QString &resourceId, const QString &subpath){
        refreshResourceKeys(appid, resourceId, subpath, contentHeader->text());
    });

    connect(titlebar->menu()->addAction(tr("export")), &QAction::triggered, [this]() {
        qInfo() << "export setting";
        exportView->loadData(contentView->language());
        exportView->show();
    });
    exportView = new ExportDialog(this);
    exportView->setMinimumSize(QSize(720, 520));
    exportView->resize(QSize(960, 680));
    connect(titlebar->menu()->addAction(tr("OEM")), &QAction::triggered, [this]() {
        oemView->loadData(contentView->language());
        oemView->show();
    });
    oemView = new OEMDialog(this);
    oemView->setMinimumSize(QSize(820, 560));
    oemView->resize(QSize(1100, 720));

    installTranslate();

    refreshApps(appHeader->text());
    if (auto model = appListView->model()) {
        for (int row = 0; row < model->rowCount(); ++row) {
            const QModelIndex index = model->index(row, 0);
            if (!model->data(index, ConfigUserRole + 2).toString().isEmpty()) {
                appListView->setCurrentIndex(index);
                emit appListView->clicked(index);
                break;
            }
        }
    }
    setCentralWidget(centralwidget);
}

MainWindow::~MainWindow()
{
}

void MainWindow::updateNavigation(const QString &appid, const QString &resourceId,
                                  const QString &subpath)
{
    const QString appText = appid.isEmpty() ? VirtualAppName : appid;
    navigationAppLabel->setText(appText);
    navigationAppLabel->setToolTip(appText);

    const bool hasResource = !resourceId.isEmpty();
    navigationResourceLabel->setVisible(hasResource);
    navigationResourceSeparator->setVisible(hasResource);
    if (hasResource) {
        navigationResourceLabel->setText(resourceId);
        navigationResourceLabel->setToolTip(resourceId);
    }

    const bool hasSubpath = hasResource && !subpath.isEmpty();
    navigationSubpathLabel->setVisible(hasSubpath);
    navigationSubpathSeparator->setVisible(hasSubpath);
    if (hasSubpath) {
        navigationSubpathLabel->setText(subpath);
        navigationSubpathLabel->setToolTip(subpath);
    }
}

void MainWindow::refreshApps(const QString &matchAppid)
{
    auto model = new QStandardItemModel(this);
    auto apps = sortedStrings(ResourceCatalogClient::instance().applications());
    for (const QString &appid : dynamicAppids) {
        if (!apps.contains(appid))
            apps.append(appid);
    }
    apps = sortedStrings(apps);
    apps.removeAll(NoAppId);
    apps.prepend(NoAppId);
    for (auto app : apps) {
        if (!matchAppid.isEmpty() && !app.contains(matchAppid, Qt::CaseInsensitive)) {
            continue;
        }

        if (ResourceCatalogClient::instance().resourcesForApp(app).isEmpty()
                && !dynamicAppids.contains(app)) {
            continue;
        }

        DStandardItem *item = new DStandardItem(app);
        item->setSizeHint(QSize(220, 52));
        item->setToolTip(app);
        item->setData(ConfigType::AppType, ConfigUserRole + 1);
        item->setData(app, ConfigUserRole + 2);
        model->appendRow(item);
    }
    appListView->setModel(model);
    translateAppName();
    refreshAppTranslate();
}

void MainWindow::addAppid()
{
    QString appid;
    while (true) {
        bool accepted = false;
        DInputDialog dialog(this);
        dialog.setInputMode(DInputDialog::TextInput);
        dialog.setTitle(tr("appid"));
        dialog.setMessage(tr("Specify appid"));
        dialog.setTextValue(appid);
        dialog.setOkButtonText(tr("ok"));
        dialog.setCancelButtonText(tr("cancel"));

        accepted = dialog.exec() == QDialog::Accepted;
        appid = dialog.textValue().trimmed();
        if (!accepted)
            return;

        if (!appid.isEmpty() && !appid.contains(QLatin1Char('/'))
                && !appid.contains(QLatin1Char('\\')))
            break;

        DDialog warning(this);
        warning.setTitle(tr("invalid appid"));
        warning.setMessage(tr("Enter a non-empty appid without '/' or '\\'."));
        warning.addButton(tr("ok"), true, DDialog::ButtonNormal);
        warning.exec();
    }

    if (!dynamicAppids.contains(appid))
        dynamicAppids.append(appid);

    refreshApps();
    const auto model = appListView->model();
    for (int row = 0; row < model->rowCount(); ++row) {
        const QModelIndex index = model->index(row, 0);
        if (model->data(index, ConfigUserRole + 2).toString() == appid) {
            appListView->setCurrentIndex(index);
            emit appListView->clicked(index);
            return;
        }
    }
}

void MainWindow::refreshAppResources(const QString &appid, const QString &matchResource)
{
    resourceListView->reset();
    auto model = qobject_cast<QStandardItemModel *>(resourceListView->model());
    model->clear();

    auto appResources = appid == NoAppId
            ? ResourceList()
            : sortedStrings(ResourceCatalogClient::instance().resourcesForApp(appid));

    const auto commons = sortedStrings(ResourceCatalogClient::instance().resourcesForAllApp());
    QStringList resources = sortedStrings(appResources);
    // Keep application resources together, followed by common resources.
    const QStringList commonResources = sortedStrings(commons);
    for (const QString &resource : commonResources) {
        if (!appResources.contains(resource))
            resources.append(resource);
    }

    for (const QString &resource : resources) {
        if (!matchResource.isEmpty() && !resource.contains(matchResource, Qt::CaseInsensitive)) {
            continue;
        }

        const bool isAppResource = appResources.contains(resource);
        auto resourceItem = new DStandardItem();
        resourceItem->setSizeHint(QSize(200, 45));
        resourceItem->setToolTip(resource);
        resourceItem->setData(isAppResource ? ConfigType::AppResourceType
                                            : ConfigType::CommonResourceType,
                              ConfigUserRole + 1);
        resourceItem->setData(appid, ConfigUserRole + 2);
        resourceItem->setData(resource, ConfigUserRole + 3);
        resourceItem->setText(resource);

        model->appendRow(resourceItem);

        if (isAppResource)
            refreshResourceSubpaths(model, appid, resource);
    }

    if (model->rowCount() > 0) {
        resourceListView->setCurrentIndex(resourceListView->model()->index(0, 0));
    }
}

void MainWindow::refreshResourceSubpaths(QStandardItemModel *model, const QString &appid, const QString &resourceId)
{
    const auto installedSubpaths = ResourceCatalogClient::instance().subpathsForResource(appid, resourceId);
    const auto runtimeSubpaths = dynamicSubpaths.value(appid).value(resourceId);
    QStringList subpaths = installedSubpaths;
    for (const QString &subpath : runtimeSubpaths) {
        if (!subpaths.contains(subpath))
            subpaths.append(subpath);
    }
    subpaths = sortedStrings(subpaths);

    for (const QString &subpath : subpaths) {

        auto subpathItem = new DStandardItem();
        subpathItem->setSizeHint(QSize(200, 45));
        subpathItem->setData(ConfigType::SubpathType, ConfigUserRole + 1);
        subpathItem->setData(appid, ConfigUserRole + 2);
        subpathItem->setData(resourceId, ConfigUserRole + 3);
        subpathItem->setData(subpath, ConfigUserRole + 4);
        subpathItem->setText(subpath);
        if (runtimeSubpaths.contains(subpath))
            subpathItem->setToolTip(tr("subpath: %1").arg(subpath));

        model->appendRow(subpathItem);
    }
}

void MainWindow::refreshResourceKeys(const QString &appid, const QString &resourceId, const QString &subpath, const QString &matchKeyId)
{
    contentView->refreshResourceKeys(appid, resourceId, subpath, matchKeyId);
}

void MainWindow::onCustomResourceMenuRequested(const QString &appid, const QString &resource,
                                               const QString &subpath, bool canAddDynamicSubpath)
{
     QMenu menu(resourceListView);

     if (canAddDynamicSubpath) {
         QAction *dynamicSubpathAction = menu.addAction(tr("specify subpath"));
         connect(dynamicSubpathAction, &QAction::triggered, this, [this, appid, resource] {
             addDynamicSubpath(appid, resource);
         });
         menu.addSeparator();
     }

     QAction *resetCmdAction = menu.addAction(tr("reset value"));

     connect(resetCmdAction, &QAction::triggered, this, [this, appid, resource, subpath] {
        QScopedPointer<ValueHandler> getter(new ValueHandler(appid, resource, subpath));
        QScopedPointer<ConfigGetter> manager(getter->createManager());
        if (!manager) {
            qWarning() << "Failed to create manager for reset command";
            return;
        }
        const auto keys = manager->keyList();
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        for (const auto &item : std::as_const(keys)) {
#else
        for (const auto &item : qAsConst(keys)) {
#endif
            manager->reset(item);
        }
        refreshResourceKeys(appid, resource, subpath);
     });
     menu.exec(QCursor::pos());
}

void MainWindow::addDynamicSubpath(const QString &appid, const QString &resource)
{
    QString subpath;
    QString input;
    while (true) {
        bool accepted = false;
        DInputDialog dialog(this);
        dialog.setInputMode(DInputDialog::TextInput);
        dialog.setTitle(resource);
        dialog.setMessage(tr("Specify subpath"));
        dialog.setTextValue(input);
        dialog.setOkButtonText(tr("ok"));
        dialog.setCancelButtonText(tr("cancel"));

        accepted = dialog.exec() == QDialog::Accepted;
        input = dialog.textValue();
        if (!accepted)
            return;

        if (normalizeDynamicSubpath(input, &subpath))
            break;

        DDialog warning(this);
        warning.setTitle(tr("invalid subpath"));
        warning.setMessage(tr("Enter a non-empty path without '.' or '..' components or backslashes."));
        warning.addButton(tr("ok"), true, DDialog::ButtonNormal);
        warning.exec();
    }

    auto model = qobject_cast<QStandardItemModel *>(resourceListView->model());
    if (!model)
        return;

    for (int row = 0; row < model->rowCount(); ++row) {
        const QModelIndex index = model->index(row, 0);
        if (model->data(index, ConfigUserRole + 2).toString() == appid
                && model->data(index, ConfigUserRole + 3).toString() == resource
                && model->data(index, ConfigUserRole + 4).toString() == subpath) {
            resourceListView->setCurrentIndex(index);
            emit resourceListView->clicked(index);
            return;
        }
    }

    QStringList &runtimeSubpaths = dynamicSubpaths[appid][resource];
    if (!runtimeSubpaths.contains(subpath))
        runtimeSubpaths.append(subpath);

    const QModelIndex resourceIndex = resourceListView->currentIndex();
    int insertRow = resourceIndex.row() + 1;
    while (insertRow < model->rowCount()) {
        const QModelIndex index = model->index(insertRow, 0);
        if (model->data(index, ConfigUserRole + 1).toInt() != ConfigType::SubpathType
                || model->data(index, ConfigUserRole + 2).toString() != appid
                || model->data(index, ConfigUserRole + 3).toString() != resource) {
            break;
        }
        ++insertRow;
    }

    auto subpathItem = new DStandardItem();
    subpathItem->setSizeHint(QSize(200, 45));
    subpathItem->setData(ConfigType::SubpathType, ConfigUserRole + 1);
    subpathItem->setData(appid, ConfigUserRole + 2);
    subpathItem->setData(resource, ConfigUserRole + 3);
    subpathItem->setData(subpath, ConfigUserRole + 4);
    subpathItem->setText(subpath);
    subpathItem->setToolTip(tr("subpath: %1").arg(subpath));
    model->insertRow(insertRow, subpathItem);

    const QModelIndex subpathIndex = model->index(insertRow, 0);
    resourceListView->setCurrentIndex(subpathIndex);
    emit resourceListView->clicked(subpathIndex);
}

void MainWindow::installTranslate()
{
    DTitlebar *titlebar = this->titlebar();
    auto languageMenu = titlebar->menu()->addMenu(tr("config language"));
    auto defaultAction = languageMenu->addAction(tr("default"));
    defaultAction->setCheckable(true);
    auto chineseAction = languageMenu->addAction(tr("chinese"));
    chineseAction->setCheckable(true);
    auto englishAction = languageMenu->addAction(tr("english"));
    englishAction->setCheckable(true);
    QActionGroup *languageGroup = new QActionGroup(this);
    languageGroup->addAction(defaultAction);
    languageGroup->addAction(chineseAction);
    languageGroup->addAction(englishAction);

    connect(defaultAction, &QAction::toggled, [this](){
        contentView->setLanguage("");
    });
    connect(chineseAction, &QAction::toggled, [this](){
        contentView->setLanguage("zh_CN");
    });
    connect(englishAction, &QAction::toggled, [this](){
        contentView->setLanguage("en_US");
    });
    connect(contentView, &Content::languageChanged, this, [this](){
        emit resourceListView->clicked(resourceListView->currentIndex());
    });
    const auto userInfos = fetchUserInfos();
    if (!userInfos.isEmpty()) {
        auto userMenu = titlebar->menu()->addMenu(tr("Switch User"));
        QActionGroup *userGroup = new QActionGroup(this);
        for (const auto user : userInfos) {
            const auto uid = user.second;
            auto action = userMenu->addAction(user.first);
            action->setProperty("uid", uid);
            action->setCheckable(true);
            if (ValueHandler::currentUid() == uid) {
                action->setChecked(true);
            }
            userGroup->addAction(action);
        }
        connect(userGroup, &QActionGroup::triggered, this, [this] (QAction *action) {
            const auto uid = action->property("uid").toInt();
            qDebug() << "Switch to the user" << action->text() << ", uid:" << uid;
            ValueHandler::setCurrentUid(uid);
        });
    }

    const auto systemLanguage = QLocale::system().name();
    qDebug() << systemLanguage;
    if (systemLanguage == "zh_CN") {
        chineseAction->setChecked(true);
    } else if (systemLanguage == "en_US") {
        englishAction->setChecked(true);
    } else {
        defaultAction->setChecked(true);
    }
}

void MainWindow::translateAppName()
{
    ItemInfo::registerMetaType();

    DTK_USE_NAMESPACE;
    QDBusPendingCall call = DDBusSender()
            .service("com.deepin.dde.daemon.Launcher")
            .interface("com.deepin.dde.daemon.Launcher")
            .path("/com/deepin/dde/daemon/Launcher")
            .method(QString("GetAllItemInfos"))
            .call();

    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(call, this);

    QObject::connect(watcher, &QDBusPendingCallWatcher::finished,
                     this, [this](QDBusPendingCallWatcher* call){

        QDBusPendingReply<ItemInfoList> reply = *call;
        if (!reply.isError()) {
            for (auto item : reply.value()) {
                appIdToNameMaps[item.m_key] = item.m_name;
            }
            refreshAppTranslate();
        }
        call->deleteLater();
    });
}

void MainWindow::refreshAppTranslate()
{
    if (auto model = qobject_cast<QStandardItemModel*>(appListView->model())) {
        for (int i = 0; i < model->rowCount(); i++) {
            auto item = model->item(i);
            const QString appId = item->data(ConfigUserRole + 2).toString();
            const QString displayName = appIdToNameMaps.value(appId, appId);
            if (appId.isEmpty()) {
                item->setText(VirtualAppName);
            } else if (displayName == appId) {
                item->setText(appId);
            } else {
                item->setText(QString("%1  ·  %2").arg(displayName, appId));
            }
            item->setToolTip(appId);
        }
    }
}

LevelDelegate::LevelDelegate(QAbstractItemView *parent)
    : DStyledItemDelegate(parent)
{

}

LevelDelegate::~LevelDelegate()
{

}

void LevelDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    opt.text = QString();
    DStyledItemDelegate::paint(painter, opt, index);
    auto level = static_cast<ConfigType>(index.data(ConfigUserRole + 1).toInt());
    bool isSelected = option.state & QStyle::State_Selected;

    // draw text
    switch (level) {
    case ConfigType::AppResourceType: {
        QColor pen = option.palette.color(isSelected ? QPalette::HighlightedText : QPalette::BrightText);
        painter->setPen(pen);
        painter->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T4, QFont::Medium, opt.font));
        QRect rect = opt.rect.marginsRemoved(QMargins(10, 0, 10, 0));
        auto text = opt.fontMetrics.elidedText(index.data().toString(), Qt::ElideMiddle, rect.width());
        painter->drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, text);
        break;
    }
    case ConfigType::CommonResourceType: {
        QColor pen = option.palette.color(isSelected ? QPalette::HighlightedText : QPalette::BrightText);
        painter->setPen(pen);
        painter->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T4, QFont::ExtraBold, opt.font));
        QRect rect = opt.rect.marginsRemoved(QMargins(10, 0, 10, 0));
        auto text = opt.fontMetrics.elidedText(index.data().toString(), Qt::ElideMiddle, rect.width());
        painter->drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, text);
        break;
    }
    case ConfigType::SubpathType: {
        QColor pen = option.palette.color(isSelected ? QPalette::HighlightedText : QPalette::WindowText);
        painter->setPen(pen);
        auto rect = option.rect.marginsRemoved(QMargins(30, 0, 10, 0));
        auto text = opt.fontMetrics.elidedText(index.data().toString(), Qt::ElideMiddle, rect.width());
        painter->setFont(opt.font);
        painter->drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, text);
        break;
    }
    default :
        break;
    }
}

void LevelDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
    DStyledItemDelegate::initStyleOption(option, index);
    option->features &= ~QStyleOptionViewItem::HasDisplay;

    auto level = static_cast<ConfigType>(index.data(ConfigUserRole + 1).toInt());

    if (level & ConfigType::ResourceType) {
        option->font = DFontSizeManager::instance()->get(DFontSizeManager::T4, option->font);
        option->font.setBold(true);
        option->fontMetrics = QFontMetrics(option->font);
    }
}

Content::Content(QWidget *parent)
    : QWidget(parent)
{
    m_contentLayout = new QVBoxLayout(this);
}

Content::~Content() {
}

void Content::remove(QLayout *layout)
{
    if (!layout)
        return;

    while (auto item = layout->takeAt(0)) {
        if (auto widget = item->widget() ) {
            widget->deleteLater();
        }
        remove(item->layout());
    }
}

void Content::setLanguage(const QString &language)
{
    m_language = language;
    emit languageChanged();
}

void Content::refreshResourceKeys(const QString &appid, const QString &resourceId, const QString &subpath, const QString &matchKeyId)
{
    remove(m_contentLayout);

    m_getter.reset(new ValueHandler(appid, resourceId, subpath));
    QScopedPointer<ConfigGetter> manager(m_getter->createManager());
    if(!manager) {
        return;
    }
    for (auto key : manager->keyList()) {

        if (!matchKeyId.isEmpty() && !key.contains(matchKeyId, Qt::CaseInsensitive)) {
            continue;
        }

        if (!isVisible(manager.get(), key)) {
            // TODO visiblity
            //            continue;
            ;
        }

        auto keyItem = new KeyContent(key, this);
        keyItem->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);


        keyItem->setBaseInfo(manager.get(), m_language);

        connect(keyItem, &KeyContent::valueChanged, this, &Content::onValueChanged);

        m_contentLayout->addWidget(keyItem, 0, Qt::AlignTop);
        keyItem->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(keyItem, &QWidget::customContextMenuRequested, this, [this, keyItem, appid, resourceId, subpath, key](const QPoint&) {
            onCustomContextMenuRequested(keyItem, appid, resourceId, subpath, key);
        });
    }
    m_contentLayout->addStretch();
}

void Content::clear()
{
    remove(m_contentLayout);
}

ValueHandler *Content::getter()
{
    return m_getter.get();
}

bool Content::isVisible(ConfigGetter *manager, const QString &key)
{
    const QString &visiblity = manager->visibility(key);
    return  visiblity == "public" ? true : false;
}

void Content::onValueChanged(const QVariant &value)
{
    if (!m_getter) {
        return;
    }
    auto keyContent = qobject_cast<KeyContent*>(sender());
    if (!keyContent) {
        return;
    }
    QScopedPointer<ConfigGetter> manager(m_getter->createManager());
    if(!manager) {
        return;
    }
    const auto &old = manager->value(keyContent->key());
    if (old == value) {
        qInfo() << "old == value" << old << value;
        return;
    }
    manager->setValue(keyContent->key(), value);
    keyContent->updateContent(manager.data());

    // TODO
    emit sendValueUpdated(QStringList() << m_getter->appid << m_getter->fileName << m_getter->subpath << keyContent->key(), old, value);
}

void Content::onCustomContextMenuRequested(QWidget *widget, const QString &appid, const QString &resource, const QString &subpath, const QString &key)
{
    m_getter.reset(new ValueHandler(appid, resource, subpath));
    QScopedPointer<ConfigGetter> manager(m_getter->createManager());
    if (!manager) {
        qWarning() << "Failed to create manager for context menu";
        return;
    }
    const QString &value = qvariantToCmd(manager.get()->value(key));
    const QString &description = manager.get()->description(key, m_language);

    QMenu *menu = new QMenu(widget);

    QAction *exportAction = menu->addAction(tr("export"));
    QAction *copyFieldAction = menu->addAction(tr("copy field name"));
    QAction *copyValueAction = menu->addAction(tr("copy value"));
    QAction *copyCmdAction = menu->addAction(tr("convert to cmd"));
    QAction *resetCmdAction = menu->addAction(tr("reset value"));

    QString setCmd = QString("dde-dconfig set %1 -r %2 %3 -v %4").arg(appid).arg(resource).arg(key).arg(value);
    QString getCmd = QString("dde-dconfig get %1 -r %2 %3").arg(appid).arg(resource).arg(key);
    if (!subpath.isEmpty()) {
        setCmd.append(QString(" -s %1").arg(subpath));
        getCmd.append(QString(" -s %1").arg(subpath));
    }
    connect(exportAction, &QAction::triggered, this, [this, appid, resource, subpath, key, value, description, setCmd, getCmd]{
        QString fileName = QFileDialog::getSaveFileName(this, tr("export current configuration"), "", tr("file(*.csv)"));
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            QStringList headers = (QStringList() << "appid" << "resource" << "subpath" << "key" << "value" <<"description" << "set command" <<  "get command");
            stream << headers.join(',');
            stream << "\n";
            QStringList datas = QStringList() << appid <<  resource << subpath << key << value << description << setCmd << getCmd;
            stream << datas.join(',');
            stream << "\n";
            stream.flush();
            file.close();
            if (stream.status() != QTextStream::Ok) {
                qWarning() << "stream.status:" << stream.status();
                DDialog dialog("save failed", "",this);
                dialog.addButton("ok", true, DDialog::ButtonWarning);
                dialog.exec();
            }
        }
    });
    QClipboard *clip = QApplication::clipboard();
    connect(copyFieldAction, &QAction::triggered, this, [clip, key] {
        clip->setText(key);
    });
    connect(copyValueAction, &QAction::triggered, this, [clip, value] {
        clip->setText(value);
    });
    connect(copyCmdAction, &QAction::triggered, this, [clip, setCmd] {
        clip->setText(setCmd);
    });

    connect(resetCmdAction, &QAction::triggered, this, [this, key, widget] {
        QScopedPointer<ConfigGetter> manager(m_getter->createManager());
        if (!manager) {
            qWarning() << "Failed to create manager for reset";
            return;
        }
        const auto &old = manager->value(key);
        manager->reset(key);
        if (auto contentWidget = qobject_cast<KeyContent *>(widget)) {
            contentWidget->updateContent(manager.get());
        }
        const auto value = manager->value(key);
        if (old != value) {
            emit sendValueUpdated(QStringList() << m_getter->appid << m_getter->fileName << m_getter->subpath << key, old, value);
        }
    });
    menu->exec(QCursor::pos());
}

KeyContent::KeyContent(const QString &key, QWidget *parent)
    : QWidget (parent),
      m_key(key),
      m_hLay(new QHBoxLayout(this))
{
}

void KeyContent::setBaseInfo(ConfigGetter *getter, const QString &language)
{
    const QVariant &v = getter->value(m_key);

    const QString &permissions = getter->permissions(m_key);
    bool canWrite = permissions == "readwrite" ? true : false;
    qDebug() << "key and value " << m_key << v;

    QString displayName = getter->displayName(m_key, language);
    if (displayName.isEmpty()) {
        displayName = getter->displayName(m_key, QString());
        if (displayName.isEmpty()) {
            displayName = m_key;
        }
    }
    QString description = getter->description(m_key, language);
    if (description.isEmpty()) {
        description = getter->description(m_key, QString());
    }

    auto labelContainer = new QWidget(this);
    labelContainer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    labelContainer->setMinimumWidth(150);
    labelContainer->setMaximumWidth(280);
    auto labelLayout = new QVBoxLayout(labelContainer);
    labelLayout->setContentsMargins(0, 0, 0, 0);
    labelLayout->setSpacing(2);

    if (displayName != m_key) {
        auto nameLabel = new DLabel(displayName, labelContainer);
        nameLabel->setObjectName("name-label");
        nameLabel->setWordWrap(false);
        nameLabel->setElideMode(Qt::ElideMiddle);
        nameLabel->setToolTip(description.isEmpty() ? displayName
                                                     : QString("%1\n%2").arg(displayName, description));
        nameLabel->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(nameLabel, &QWidget::customContextMenuRequested,
                this, &QWidget::customContextMenuRequested);
        labelLayout->addWidget(nameLabel);
    }

    auto keyLabel = new DLabel(m_key, labelContainer);
    keyLabel->setObjectName("key-label");
    keyLabel->setWordWrap(false);
    keyLabel->setElideMode(Qt::ElideMiddle);
    keyLabel->setToolTip(m_key);
    keyLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    keyLabel->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(keyLabel, &QWidget::customContextMenuRequested,
            this, &QWidget::customContextMenuRequested);
    labelLayout->addWidget(keyLabel);

    m_hLay->setContentsMargins(8, 6, 8, 6);
    m_hLay->setSpacing(12);
    auto modifiedIndicator = new DLabel(QStringLiteral("*"), this);
    modifiedIndicator->setObjectName("modified-indicator");
    modifiedIndicator->setToolTip(tr("modified"));
    modifiedIndicator->setFixedWidth(modifiedIndicator->fontMetrics().horizontalAdvance(QLatin1Char('*')) + 4);
    modifiedIndicator->clear();
    modifiedIndicator->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_hLay->addWidget(modifiedIndicator, 0, Qt::AlignVCenter);
    m_hLay->addWidget(labelContainer, 1);
    QWidget *valueWidget = nullptr;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    const auto valueType = v.type();
#else
    const auto valueType = v.typeId();
#endif
    if (valueType == QVariant::Bool) {
        auto widget = new DSwitchButton(this);
        widget->setEnabled(canWrite);
        connect(widget, &DSwitchButton::clicked, widget, [this, widget](bool checked){
            widget->clearFocus();
            emit valueChanged(checked);
        });
        valueWidget = widget;
    } else if (valueType == QVariant::Double) {
        auto widget = new DDoubleSpinBox(this);
        widget->setRange(std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max());
        widget->setAlignment(Qt::AlignRight);
        widget->setEnabled(canWrite);
        connect(widget, SIGNAL(valueChanged(double)), this, SLOT(onDoubleValueChanged(double)));
        valueWidget = widget;
    } else {
        auto widget = new DLineEdit(this);
        widget->lineEdit()->setAlignment(Qt::AlignRight);
        widget->setEnabled(canWrite);
        connect(widget, &DLineEdit::editingFinished, widget, [this, widget](){
            QString errorMsg;
            if (!validateTextInput(widget->text(), errorMsg)) {
                qWarning() << errorMsg;
                return;
            }
            widget->clearFocus();
            emit valueChanged(stringToQVariant(widget->text()));
        });
        valueWidget = widget;
    }
    if (valueWidget) {
        valueWidget->setObjectName("value-view");
        if (qobject_cast<DLineEdit *>(valueWidget)) {
            valueWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            m_hLay->addWidget(valueWidget, 3);
        } else {
            valueWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
            m_hLay->addWidget(valueWidget, 0, Qt::AlignRight | Qt::AlignVCenter);
        }
    }
    updateContent(getter);
}

QString KeyContent::key() const
{
    return m_key;
}

void KeyContent::updateContent(ConfigGetter *getter)
{
    if (auto indicator = findChild<DLabel *>("modified-indicator"))
        indicator->setText(getter->isDefaultValue(m_key) ? QString() : QStringLiteral("*"));
    if (auto viewWidget = findChild<QWidget *>("value-view")) {
        const QVariant &v = getter->value(m_key);
        if (auto widget = qobject_cast<DSwitchButton*>(viewWidget)) {
            widget->setChecked(v.toBool());
        } else if (auto widget = qobject_cast<DDoubleSpinBox *>(viewWidget)) {
            widget->setValue(v.toDouble());
        } else if (auto widget = qobject_cast<DLineEdit *>(viewWidget)) {
            widget->setText(qvariantToString(v));
        }
    }
}

void KeyContent::onDoubleValueChanged(double value)
{
    emit valueChanged(value);
}

HistoryDialog::HistoryDialog(QWidget *parent)
    : DDialog( parent)
{
    historyView = new DListView();
    historyView->setModel(new QStandardItemModel(this));
    historyView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    addContent(historyView);
    connect(historyView, &DListView::doubleClicked, this, [this](const QModelIndex &index){
        const auto &appid = historyView->model()->data(index, ConfigUserRole + 2).toString();
        const auto &resourceid = historyView->model()->data(index, ConfigUserRole + 3).toString();
        const auto &subpath = historyView->model()->data(index, ConfigUserRole + 4).toString();
        ValueHandler handler(appid, resourceid, subpath);

        bool hasReset = false;
        {

            QScopedPointer<ConfigGetter> manager(handler.createManager());
            if (!manager) {
                qWarning() << "Failed to create manager for history";
                return;
            }
            if (manager) {

                const auto &key = historyView->model()->data(index, ConfigUserRole + 5).toString();
                const auto &pre = historyView->model()->data(index, ConfigUserRole + 6);
                qInfo() << "reset" << appid << resourceid << subpath << key << pre;
                manager->setValue(key, pre);
                historyView->model()->removeRow(index.row());
                hasReset = true;
            }
        }
        if (hasReset) {
            emit refreshResourceKeys(appid, resourceid, subpath);
        }
    });
}

void HistoryDialog::onSendValueUpdated(const QStringList &key, const QVariant &pre, const QVariant &now)
{
    auto model = qobject_cast<QStandardItemModel*>(historyView->model());

    auto item = new DStandardItem();

    item->setText(QString("%1: [%2][%3][%4] \n[%5] from [%6] to [%7].").
                  arg(QTime::currentTime().toString(Qt::ISODate)).
                  arg(key[0]).arg(key[1]).arg(key[2]).arg(key[3]).
                  arg(qvariantToString(pre)).
                  arg(qvariantToString(now)));

    item->setData(key[0], ConfigUserRole + 2);
    item->setData(key[1], ConfigUserRole + 3);
    item->setData(key[2], ConfigUserRole + 4);
    item->setData(key[3], ConfigUserRole + 5);
    item->setData(pre, ConfigUserRole + 6);

    if (model->rowCount() >= maxRows) {
        model->removeRow(0);
    }

    model->appendRow(item);
}
