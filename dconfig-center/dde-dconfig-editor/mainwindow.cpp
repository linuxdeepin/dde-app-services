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
#include "mainwindow.h"


#include "helper.hpp"
#include "valuehandler.h"
#include "iteminfo.h"
#include "exportdialog.h"
#include "oemdialog.h"

#include <QHBoxLayout>
#include <DLabel>
#include <DSwitchButton>
#include <DLineEdit>
#include <DSearchEdit>
#include <DSlider>
#include <DTitlebar>
#include <DStatusBar>
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
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    DMainWindow(parent)
{
    resize(800, 600);
    centralwidget = new QWidget(this);
    centralwidget->setObjectName(QStringLiteral("centralwidget"));

    auto layout = new QHBoxLayout(centralwidget);
    layout->setContentsMargins(0, 0, 0, 0);
    QSplitter *hSplitter = new QSplitter(Qt::Horizontal, centralwidget);
    hSplitter->setLineWidth(1);

    appListView = new DListView();
    appListView->setObjectName(QStringLiteral("appListView"));
    appListView->setTextElideMode(Qt::ElideRight);
    appListView->setMinimumWidth(200);
    appListView->setEditTriggers(QAbstractItemView::NoEditTriggers);

    auto appHeader = new DSearchEdit();
    appHeader->setPlaceHolder("appid");
    appHeader->setPlaceholderText("input filter appid");
    QObject::connect(appHeader, &DSearchEdit::textChanged, [this](const QString &appid){
        refreshApps(appid);
    });
    appListView->addHeaderWidget(appHeader);

    hSplitter->addWidget(appListView);

    resourceListView = new DListView();
    resourceListView->setObjectName(QStringLiteral("resourceListView"));
    resourceListView->setTextElideMode(Qt::ElideRight);
    resourceListView->setMinimumWidth(200);
    resourceListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    resourceListView->setItemDelegate(new LevelDelegate(resourceListView));
    resourceListView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    auto resourceHeader = new DSearchEdit();
    resourceHeader->setMinimumWidth(200);
    resourceHeader->setPlaceHolder("resource");
    resourceHeader->setPlaceholderText("input filter resource");
    QObject::connect(resourceHeader, &DSearchEdit::textChanged, [this](const QString &resourceid){
        const auto &appid = appListView->model()->data(appListView->currentIndex(), ConfigUserRole + 2).toString();
        refreshAppResources(appid, resourceid);
    });
    resourceListView->addHeaderWidget(resourceHeader);

    hSplitter->addWidget(resourceListView);

    auto contentViewWraper = new QWidget(this);
    auto contentViewLayout = new QVBoxLayout(contentViewWraper);
    contentViewLayout->setSpacing(0);
    contentViewLayout->setMargin(0);
    contentView = new Content();
    contentView->setObjectName(QStringLiteral("contentView"));

    auto contentHeader = new DSearchEdit();
    contentHeader->setPlaceHolder("keys");
    contentHeader->setPlaceholderText("input filter keys");
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
    contentViewLayout->addWidget(contentView);

    auto scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setWidget(contentView);

    contentViewLayout->addWidget(scrollArea);
    hSplitter->addWidget(contentViewWraper);

    layout->addWidget(hSplitter);
    hSplitter->setSizes({200, 200, 400});

    connect(appListView, &QListView::clicked, this, [this, resourceHeader](const QModelIndex &index){
        if (auto model = qobject_cast<QStandardItemModel*>(appListView->model())){
            this->refreshAppResources(model->data(index, ConfigUserRole + 2).toString(), resourceHeader->text());
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
                if (appid.isEmpty() || resourceId.isEmpty()) {
                    qWarning() << "error" << appid << resourceId;
                    return;
                }
                refreshResourceKeys(appid, resourceId, subpath, contentHeader->text());
            } else {
                contentView->clear();
            }
            scrollArea->setWidget(contentView);
        }
    });

    // set history
    DTitlebar *titlebar = this->titlebar();
    titlebar->setIcon(QIcon(APP_ICON));
    connect(titlebar->menu()->addAction("setting history"), &QAction::triggered, [this](){
        qInfo() << "show history view";
        historyView->show();

        const auto &topLeft = historyView->parentWidget()->geometry().topLeft();
        historyView->move(topLeft.x() - historyView->geometry().width(), topLeft.y());
    });
    historyView = new HistoryDialog(this);
    historyView->setFixedSize(QSize(400, 600));
    QObject::connect(contentView, &Content::sendValueUpdated, historyView, &HistoryDialog::onSendValueUpdated);
    QObject::connect(historyView, &HistoryDialog::refreshResourceKeys, this, [this, contentHeader](const QString &appid, const QString &resourceId, const QString &subpath){
        refreshResourceKeys(appid, resourceId, subpath, contentHeader->text());
    });

    connect(titlebar->menu()->addAction("export"), &QAction::triggered, [this]() {
        qInfo() << "export setting";
        exportView->loadData(contentView->language());
        exportView->show();
    });
    exportView = new ExportDialog(this);
    exportView->setFixedSize(QSize(400, 600));
    connect(titlebar->menu()->addAction("OEM"), &QAction::triggered, [this]() {
        oemView->loadData(contentView->language());
        oemView->show();
    });
    oemView = new OEMDialog(this);
    oemView->setFixedSize(QSize(800, 600));

    installTranslate();

    refreshApps(appHeader->text());
    setCentralWidget(centralwidget);
}

MainWindow::~MainWindow()
{
}

void MainWindow::refreshApps(const QString &matchAppid)
{
    auto model = new QStandardItemModel(this);
    const auto &apps = applications();
    for (auto app : apps) {
        if (!matchAppid.isEmpty() && !app.contains(matchAppid, Qt::CaseInsensitive)) {
            continue;
        }

        if (resourcesForApp(app).isEmpty()) {
            continue;
        }

        DStandardItem *item = new DStandardItem(app);
        item->setSizeHint(QSize(200, 45));
        item->setToolTip(app);
        item->setData(ConfigType::AppType, ConfigUserRole + 1);
        item->setData(app, ConfigUserRole + 2);
        model->appendRow(item);
    }
    appListView->setModel(model);
    translateAppName();
}

void MainWindow::refreshAppResources(const QString &appid, const QString &matchResource)
{
    auto model = new QStandardItemModel(resourceListView);
    resourceListView->reset();

    const auto &resources = resourcesForApp(appid);

    for (auto resource : resources) {
        if (!matchResource.isEmpty() && !resource.contains(matchResource, Qt::CaseInsensitive)) {
            continue;
        }

        auto resourceItem = new DStandardItem();
        resourceItem->setSizeHint(QSize(200, 45));
        resourceItem->setData(ConfigType::AppResourceType, ConfigUserRole + 1);
        resourceItem->setData(appid, ConfigUserRole + 2);
        resourceItem->setData(resource, ConfigUserRole + 3);
        resourceItem->setText(resource);

        model->appendRow(resourceItem);

        refreshResourceSubpaths(model, appid, resource);
    }

    const auto &commons = resourcesForAllApp();

    for (auto resource : commons) {
        if (!matchResource.isEmpty() && !resource.contains(matchResource, Qt::CaseInsensitive)) {
            continue;
        }

        auto resourceItem = new DStandardItem();
        resourceItem->setSizeHint(QSize(200, 45));
        resourceItem->setToolTip(resource);
        resourceItem->setData(ConfigType::CommonResourceType, ConfigUserRole + 1);
        resourceItem->setData(appid, ConfigUserRole + 2);
        resourceItem->setData(resource, ConfigUserRole + 3);
        resourceItem->setText(resource);

        model->appendRow(resourceItem);

    }

    resourceListView->setModel(model);
    if (model->rowCount() > 0) {
        resourceListView->setCurrentIndex(resourceListView->model()->index(0, 0));
    }
}

void MainWindow::refreshResourceSubpaths(QStandardItemModel *model, const QString &appid, const QString &resourceId)
{

    const auto &subpaths = subpathsForResource(appid, resourceId);
    for (auto subpath : subpaths) {
        auto subpathItem = new DStandardItem();
        subpathItem->setData(ConfigType::SubpathType, ConfigUserRole + 1);
        subpathItem->setData(appid, ConfigUserRole + 2);
        subpathItem->setData(resourceId, ConfigUserRole + 3);
        subpathItem->setData(subpath, ConfigUserRole + 4);
        subpathItem->setText(subpath);

        model->appendRow(subpathItem);
    }
}

void MainWindow::refreshResourceKeys(const QString &appid, const QString &resourceId, const QString &subpath, const QString &matchKeyId)
{
    contentView->refreshResourceKeys(appid, resourceId, subpath, matchKeyId);
}

void MainWindow::installTranslate()
{
    DTitlebar *titlebar = this->titlebar();
    auto languageMenu = titlebar->menu()->addMenu("config language");
    auto defaultAction = languageMenu->addAction("default");
    defaultAction->setCheckable(true);
    auto chineseAction = languageMenu->addAction("chinese");
    chineseAction->setCheckable(true);
    auto englishAction = languageMenu->addAction("english");
    englishAction->setCheckable(true);
    QActionGroup *languageGroup = new QActionGroup(this);
    languageGroup->addAction(defaultAction);
    languageGroup->addAction(chineseAction);
    languageGroup->addAction(englishAction);
    defaultAction->setChecked(true);

    connect(defaultAction, &QAction::triggered, [this](){
        contentView->setLanguage("");
    });
    connect(chineseAction, &QAction::triggered, [this](){
        contentView->setLanguage("zh_CN");
    });
    connect(englishAction, &QAction::triggered, [this](){
        contentView->setLanguage("en_US");
    });
    connect(contentView, &Content::languageChanged, this, [this](){
        emit resourceListView->clicked(resourceListView->currentIndex());
    });

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
            if (appIdToNameMaps.contains(item->text())) {
                item->setText(appIdToNameMaps.value(item->text()));
            }
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
        auto text = opt.fontMetrics.elidedText(index.data().toString(), Qt::ElideRight, rect.width());
        painter->drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, text);
        break;
    }
    case ConfigType::CommonResourceType: {
        QColor pen = option.palette.color(isSelected ? QPalette::HighlightedText : QPalette::BrightText);
        painter->setPen(pen);
        painter->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T4, QFont::ExtraBold, opt.font));
        QRect rect = opt.rect.marginsRemoved(QMargins(10, 0, 10, 0));
        auto text = opt.fontMetrics.elidedText(index.data().toString(), Qt::ElideRight, rect.width());
        painter->drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, text);
        break;
    }
    case ConfigType::SubpathType: {
        QColor pen = option.palette.color(isSelected ? QPalette::HighlightedText : QPalette::WindowText);
        painter->setPen(pen);
        auto rect = option.rect.marginsRemoved(QMargins(30, 0, 10, 0));
        auto text = opt.fontMetrics.elidedText(index.data().toString(), Qt::ElideRight, rect.width());
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

    // TODO
    emit sendValueUpdated(QStringList() << m_getter->appid << m_getter->fileName << m_getter->subpath << keyContent->key(), old, value);
}

void Content::onCustomContextMenuRequested(QWidget *widget, const QString &appid, const QString &resource, const QString &subpath, const QString &key)
{
    m_getter.reset(new ValueHandler(appid, resource, subpath));
    QScopedPointer<ConfigGetter> manager(m_getter->createManager());
    const QString &value =  manager.get()->value(key).toString();
    const QString &description = manager.get()->description(key, m_language);

    QMenu *menu = new QMenu(widget);

    QAction *exportAction = menu->addAction("导出");
    QAction *copyAction = menu->addAction("复制");
    QAction *copyCmdAction = menu->addAction("复制命令");
    QAction *resetCmdAction = menu->addAction("重置");

    QString setCmd = QString("dde-dconfig --set -a %1 -r %2 -k %3 -v %4").arg(appid).arg(resource).arg(key).arg(value);
    QString getCmd = QString("dde-dconfig --get -a %1 -r %2  -k %3").arg(appid).arg(resource).arg(key);
    if (!subpath.isEmpty()) {
        setCmd.append(QString(" -s %1").arg(subpath));
        getCmd.append(QString(" -s %1").arg(subpath));
    }
    connect(exportAction, &QAction::triggered, this, [this, appid, resource, subpath, key, value, description, setCmd, getCmd]{
        QString fileName = QFileDialog::getSaveFileName(this, tr("导出当前配置"), "", tr("文件(*.csv)"));
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
                DDialog dialog("文件保存失败!", "",this);
                dialog.addButton("确定", true, DDialog::ButtonWarning);
                dialog.exec();
            }
        }
    });
    QClipboard *clip = QApplication::clipboard();
    connect(copyAction, &QAction::triggered, this, [clip, key] {
        clip->setText(key);
    });
    connect(copyCmdAction, &QAction::triggered, this, [clip, setCmd] {
        clip->setText(setCmd);
    });

    connect(resetCmdAction, &QAction::triggered, this, [this, key] {
        QScopedPointer<ConfigGetter> manager(m_getter->createManager());
        const auto &old = manager->value(key);
        manager->reset(key);
        const auto value = manager->value(key);
        if (old != value) {
            emit sendValueUpdated(QStringList() << m_getter->appid << m_getter->fileName << m_getter->subpath << key, old, value);
            requestRefreshResourceKeys();
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

    DLabel *labelWidget = new DLabel(QString("%1 [%2]").arg(getter->displayName(m_key, language), m_key));
    labelWidget->setToolTip(getter->description(m_key, language));
    m_hLay->addWidget(labelWidget);
    QWidget *valueWidget = nullptr;
    if (v.type() == QVariant::Bool) {
        auto widget = new DSwitchButton(this);
        widget->setChecked(v.toBool());
        widget->setEnabled(canWrite);
        connect(widget, &DSwitchButton::clicked, widget, [this, widget](bool checked){
            widget->clearFocus();
            emit valueChanged(checked);
        });
        valueWidget = widget;
    } else if (v.type() == QVariant::Double) {
        auto widget = new DDoubleSpinBox(this);
        widget->setRange(std::numeric_limits<double>::min(), std::numeric_limits<double>::max());
        widget->setValue(v.toDouble());
        widget->setEnabled(canWrite);
        connect(widget, SIGNAL(valueChanged(double)), this, SLOT(onDoubleValueChanged(double)));
        valueWidget = widget;
    } else {
        auto widget = new DLineEdit(this);
        widget->setText(qvariantToString(v));
        widget->setEnabled(canWrite);
        connect(widget, &DLineEdit::editingFinished, widget, [this, widget](){
            widget->clearFocus();
            emit valueChanged(stringToQVariant(widget->text()));
        });
        valueWidget = widget;
    }
    if (valueWidget) {
        m_hLay->addWidget(valueWidget);
    }
}

QString KeyContent::key() const
{
    return m_key;
}

void KeyContent::onDoubleValueChanged(double value)
{
    if (auto widget = qobject_cast<QWidget*>(sender())) {
        widget->clearFocus();
    }
    emit valueChanged(value);
}

HistoryDialog::HistoryDialog(QWidget *parent)
    : DDialog( parent)
{
    historyView = new DListView();
    historyView->setModel(new QStandardItemModel());
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
