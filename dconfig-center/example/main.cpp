/*
 * Copyright (C) 2021 Uniontech Technology Co., Ltd.
 *
 * Author:     yeshanshan <yeshanshan@uniontech.com>
 *
 * Maintainer: yeshanshan <yeshanshan@uniontech.com>
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

#include <QCoreApplication>
#include <QDebug>
#include <QTimer>

#include <csignal>
#include <DConfig>
DCORE_USE_NAMESPACE

class DConfigExample {
public:
    explicit DConfigExample(const QString &fileName)
        :fileName(fileName)
    {
        baseAPI();
        watchValueChanged();
        subpath();
    }

    void watchValueChanged()
    {
        heapConfig.reset(new DConfig(fileName));
        if (!heapConfig->isValid()) {
            qWarning() << QString("DConfig is invalide, name:[%1], subpath[%2].").
                          arg(heapConfig->name(), heapConfig->subpath());
            return;
        }

        // 监听值改变的信号
        const auto &oldValue = heapConfig->value("canExit").toBool();
        QObject::connect(heapConfig.get(), &DConfig::valueChanged, [oldValue, this](const QString &key){
            qDebug() << "value changed, oldValue:" << oldValue << ", newValue:" << heapConfig->value(key).toBool();
        });

        heapConfig->setValue("canExit", !oldValue);
    }

    void baseAPI()
    {
        // 构造DConfig
        DConfig config(fileName);

        // 判断是否有效
        if (!config.isValid()) {
            qWarning() << QString("DConfig is invalide, name:[%1], subpath[%2].").
                          arg(config.name(), config.subpath());
            return;
        }

        // 获取所有配置项的key
        qDebug() << "All key items." << config.keyList();

        // 获取指定配置项的值
        qDebug() << "key item's value:" << config.value("canExit").toBool();

        // 设置值指定配置项的值
        config.setValue("canExit", false);
    }

    void subpath()
    {
        // 构造DConfig
        DConfig config(fileName, "/a");

        // 判断是否有效
        if (!config.isValid()) {
            qWarning() << QString("DConfig is invalide, name:[%1], subpath[%2].").
                          arg(config.name(), config.subpath());
            return;
        }

        // 获取指定配置项的值
        qDebug() << "subpath's key item's value:" << config.value("key3").toString();

        qDebug() << "no subpath's key item's value:" << DConfig(fileName).value("key3").toString();
    }

private:
    QScopedPointer<DConfig> heapConfig;
    QString fileName;
};

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    DConfigExample example("example");

    // 异常处理，调用QCoreApplication::exit，使DConfig正常析构。
    std::signal(SIGINT, &QCoreApplication::exit);
    std::signal(SIGABRT, &QCoreApplication::exit);
    std::signal(SIGTERM, &QCoreApplication::exit);
    std::signal(SIGKILL, &QCoreApplication::exit) ;

    return a.exec();
}
