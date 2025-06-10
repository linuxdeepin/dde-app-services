\mainpage DConfig

# 配置策略

## 简介

配置策略是DTK提供的一套关于配置管理的规范，类似与gsettings，qsettings等配置管理。

## 名词解释

- 配置描述文件： 此类文件由安装包携带，用于描述配置项的元信息，以及携带配置项的默认值。
- appid：应用程序的唯一ID。
- 配置id：配置描述文件的唯一标识，是配置描述文件的文件名。
- 应用无关配置： 与应用无关的通用配置，用于提供所有应用共享的配置。
- 子目录：配置描述文件安装可包含子路径，支持配置描述文件的优先级。

详细描述参考 [配置文件规范 - 名词解释](https://github.com/linuxdeepin/deepin-specifications/blob/master/unstable/%E9%85%8D%E7%BD%AE%E6%96%87%E4%BB%B6%E8%A7%84%E8%8C%83.md#%E5%90%8D%E8%AF%8D%E8%A7%A3%E9%87%8A)

## 示例

- [示例代码 - meta文件](https://github.com/linuxdeepin/dde-app-services/tree/master/dconfig-center/example/configs/example.json)
- [示例代码 - override文件](https://github.com/linuxdeepin/dde-app-services/tree/master/dconfig-center/example/configs/dconf-example.override.json)
- [示例代码 - cmake工程中DConfig使用](https://github.com/linuxdeepin/dde-app-services/tree/master/dconfig-center/example/CMakeLists.txt)
- [示例代码 - qmake工程中DConfig使用](https://github.com/linuxdeepin/dde-app-services/tree/master/dconfig-center/example/example.pro)
- [示例代码 - cpp中DConfig使用](https://github.com/linuxdeepin/dde-app-services/tree/master/dconfig-center/example/main.cpp)
- [示例代码 - dde-dconfig](https://github.com/linuxdeepin/dde-app-services/tree/master/dconfig-center/example/dde-dconfig.sh)
- [示例代码 - dbus-dconfig](https://github.com/linuxdeepin/dde-app-services/tree/master/dconfig-center/example/dbus-dconfig.sh)

## 使用
主要分为三步：1.编写配置描述文件，2.配置描述文件安装到指定位置，3.在程序中调用提供的接口对配置项进行读写操作。
### 编写配置描述文件
配置描述文件主要用来描述配置项的元信息，包括默认值，权限等信息。
包括`meta文件` 和 `override文件`[***可选***]。

- 对单个配置项主要有以下属性
  - `meta文件`格式
    - value：配置项的默认值，可使用 json 支持的各种数据类型，如字符串、数字、数组、对象等
    - permissions：配置项的权限，readonly：不允许修改，readwrite：可读可写
    - visibility：配置项的可见性，private为仅程序内部可见，public为外部程序可见
    - flags: 配置项的特性，是字符串数组类型，值`nooverride`：将表明则此配置项不可被覆盖，值`global`：表明此配置为系统级配置
  - `override文件`，主要是对`meta文件`配置项属性的覆盖
    - value: 可以覆盖`meta文件`中配置项的value属性
    - permissions： 可覆盖`meta文件`中配置项的permissions属性

- 详细文件格式
  - 详细的`meta文件`格式，可参考[配置描述文件 - contents](https://github.com/linuxdeepin/deepin-specifications/blob/master/unstable/%E9%85%8D%E7%BD%AE%E6%96%87%E4%BB%B6%E8%A7%84%E8%8C%83.md#%E9%85%8D%E7%BD%AE%E6%8F%8F%E8%BF%B0%E6%96%87%E4%BB%B6)。
  - 详细的`override文件`，可参考[override 机制 - 属性](https://github.com/linuxdeepin/deepin-specifications/blob/master/unstable/%E9%85%8D%E7%BD%AE%E6%96%87%E4%BB%B6%E8%A7%84%E8%8C%83.md#override-%E6%9C%BA%E5%88%B6)。

### 安装配置描述文件

指定配置描述文件的安装路径，及各相关配置描述文件的优先级。

- 遵循自定义安装
  - `meta文件`安装到 `/usr/share/dsg/configs` 目录下，其中
  应用无关配置直接安装到此目录下，而应用的配置安装到 `$appid` 目录下。
  详细的配置描述文件安装路径，及目录优先级，可参考[配置描述文件 - 安装路径](https://github.com/linuxdeepin/deepin-specifications/blob/master/unstable/%E9%85%8D%E7%BD%AE%E6%96%87%E4%BB%B6%E8%A7%84%E8%8C%83.md#%E9%85%8D%E7%BD%AE%E6%8F%8F%E8%BF%B0%E6%96%87%E4%BB%B6)。

  - `override文件`通常安装到 `/etc/dsg/configs/overrides/$appid/$配置id/` 目录下。
  详细的配置描述文件安装路径，及目录优先级，可参考[override 机制 - override 文件放置路径](https://github.com/linuxdeepin/deepin-specifications/blob/master/unstable/%E9%85%8D%E7%BD%AE%E6%96%87%E4%BB%B6%E8%A7%84%E8%8C%83.md#override-%E6%9C%BA%E5%88%B6)。

- 使用DTK提供的配置工具去安装，目前支持cmake和qmake安装方法。

    cmake安装
    ```

    #FILES       - deployed files.
    #BASE        - used to get subpath, if it's empty, only copy files, and ignore it's subpath structure.
    #APPID       - working for the app.
    #COMMONID    - working for common.

    #/a为子目录

    dtk_add_config_meta_files(APPID dconfigexample BASE ./configs FILES ./configs/example.json ./configs/a/example.json)

    ```

     qmake安装
    ```

    #files       - deployed files.
    #base        - used to get subpath, if it's empty, only copy files, and ignore it's subpath structure.
    #appid       - working for the app, if it's empty, depending on `TEMPLATE`.
    #commonid    - working for common, if it's empyt, depending on `TEMPLATE`.

    #/a为子目录

    meta_file.files += \
    configs/example.json \
    configs/a/example.json

    meta_file.base = $$PWD/configs

    DCONFIG_META_FILES += meta_file
    load(dtk_install_dconfig)

    ```

### 程序使用API接口

提供了两类API接口，c++ API 和DBus API，其中c++ API功能较为单一，能满足大多数的需求，若需要操作更具体的信息，可以使用DBus API。

#### c++ API

`libdtkcore-dev`提供配置文件读写工具类[DTK::Core::DConfig](https://linuxdeepin.github.io/dtkcore/classDtk_1_1Core_1_1DConfig.html)

***当不指定appid构造DConfig时，会使用DSGApplication::id接口获取appid, 可能使用QCoreApplication::applicationName当作appid，当它与期望的appid不一致时，可能出现配置不一致的情况。***

#### DBus API
dde-dconfig-daemon中提供的dbus接口，管理配置描述文件，详细接口参照
 [org.desktopspec.ConfigManager 接口说明](https://github.com/linuxdeepin/dde-app-services/tree/master/dconfig-center/docs/org.desktopspec.ConfigManager.xml)和
 [org.desktopspec.ConfigManager.Manager 接口说明](https://github.com/linuxdeepin/dde-app-services/tree/master/dconfig-center/docs/org.desktopspec.ConfigManager.Manager.xml)

DBus管理配置文件，DBus调用者分为两步进行。
1. 获取配置描述文件所对应的DBus path。
  - 使用`org.desktopspec.ConfigManager` 的DBus接口，path: `/`
interface: `desktopspec.ConfigManager`， Method: `org.desktopspec.ConfigManager.acquireManager`。
  - 例如：
    ``` bash
    DCONFIG_RESOURCE_PATH=$(dbus-send --system --type=method_call --print-reply=literal --dest=org.desktopspec.ConfigManager / org.desktopspec.ConfigManager.acquireManager string:'dconfig-example' string:'example' string:'')
    ```
2. 根据配置描述文件的DBus path操作配置描述文件对应的配置信息。
 - 使用`org.desktopspec.ConfigManager.Manager` 的DBus接口, path:
为步骤1返回的`DBus path`，interface: `rg.desktopspec.ConfigManager.Manager`，Method: `org.desktopspec.ConfigManager.Manager.value`。
  - 例如：
    ``` bash
    dbus-send --system --type=method_call --print-reply --dest=org.desktopspec.ConfigManager $DCONFIG_RESOURCE_PATH org.desktopspec.ConfigManager.Manager.value string:'canExit' 
    ```

***在使用此服务时，需要考虑在执行第二个步骤时，不应该释放第一个步骤的连接，否则可能获取的DBus path已经失效***

#### 用户配置数据删除接口

为了解决在删除用户时，dde-dconfig-daemon的用户配置信息没有被删除的问题，提供了一个新的D-Bus接口 `removeUserData`，用于安全地删除指定用户的所有配置信息。

##### 接口定义

- **服务名**: `org.desktopspec.ConfigManager`
- **对象路径**: `/`
- **接口名**: `org.desktopspec.ConfigManager`
- **方法名**: `removeUserData`

```xml
<method name='removeUserData'>
  <arg type='u' name='uid' direction='in'/>
</method>
```

**参数说明**:
- 输入参数: `uid` (uint32) - 要删除配置数据的用户ID
- 返回值: 无返回值（void）

##### 安全机制

权限控制通过D-Bus安全策略实现：
- 只有 `root` 用户和 `deepin-daemon` 用户可以调用此接口
- 其他用户的调用将被D-Bus安全策略拒绝

##### 功能说明

此接口将删除指定用户的以下数据：

1. **内存中的连接和缓存**:
   - 所有属于该用户的配置连接 (DSGConfigConn)
   - 用户特定的配置缓存 (DConfigCache)
   - 相关的资源引用

2. **文件系统中的配置文件**:
   - 用户特定的配置缓存目录: `${STATE_DIRECTORY}/.config/{uid}/`

**不删除的内容**:
- 应用程序的系统级配置（全局配置）
- 其他用户的配置数据
- 应用程序的元数据文件

##### 使用示例

```bash
# 删除用户ID为1000的配置
dbus-send --system --type=method_call --print-reply \
    --dest=org.desktopspec.ConfigManager / \
    org.desktopspec.ConfigManager.removeUserData uint32:1000
```

##### 注意事项

⚠️ **重要警告**:
- 此操作是不可逆的，删除的配置无法恢复

🔒 **安全提醒**:
- 只有授权用户才能调用此接口
- 操作会被记录到系统日志中
- 建议定期审查配置删除操作的日志

## 调试

在安装`dde-dconfig-daemon`时，会创建`dde-dconfig-daemon用户`，并且家目录$HOME_DIR为`/var/lib/dde-dconfig-daemon`，默认情况下使用`dde-dconfig-daemon用户`去运行dde-dconfig-daemon。

- 重启dde-dconfig-daemon
    - 手动指定用户重启
    ``` bash
    sudo pkill dde-dconfig-dae && sudo -u dde-dconfig-daemon dde-dconfig-daemon
    ```

    - 通过system管理
    ``` bash
    sudo systemctl restart dde-dconfig-daemon.service
    ```

- 开启详细日志信息
    - 运行时通过api设置日志策略
    ``` bash
    dbus-send --system --type=method_call --print-reply=literal --dest=org.desktopspec.ConfigManager / org.desktopspec.ConfigManager.enableVerboseLogging
    ```
    - 启动前通过环境变量设置日志策略
    ``` bash
    sudo -u dde-dconfig-daemon QT_LOGGING_RULES="*dsg.config.debug=true" dde-dconfig-daemon
    ```

- 查看日志

    - 日志文件存储在目录`$HOME_DIR/.cache/deepin/dde-dconfig-daemon`
    - 通过journal查看日志
```
sudo journalctl -u dde-dconfig-daemon.service -f -b
```

- 配置缓存文件
配置缓存文件在目录`$HOME_DIR/.config`，其中系统配置项的缓存文件在`$HOME_DIR/.config/global`，用户级配置项在`$HOME_DIR/.cache/$uid`。

***dde-dconfig-daemon中的`DBus path链接`是共享的，释放链接做了延迟处理，可能导致有些现象不一致，例如`进程A`和`进程B`使用相同的`appid`和`配置id`去访问配置，若`进程A`先访问配置，虽然主动释放了`DBus path链接`，但若还在延迟周期内，此时`进程B`去访问此`DBus path链接`，会激活`进程A`产生的`DBus path链接`，否则才会重新创建`DBus path链接`***

## 内置工具

### dde-dconfig
dde-config 是一个命令行工具，主要用于通过命令行的方式，浏览和设置应用的配置项，可根据`dde-dconfig --help`获取帮助信息。

以下以应用名为`dconfig-example`，配置id为 `example`， 配置项名称为 `key1`示例：
- 设置配置项
``` bash
dde-dconfig --set -a dconfig-example -r example -k key1 -v 0
```
- 查看配置项
``` bash
dde-dconfig --get -a dconfig-example -r example -k key1
```
- 浏览应用所有配置:
``` bash
dde-dconfig --list -a dconfig-example
```
- 启动 dde-dconfig-editor
``` bash
dde-dconfig --gui
```

### dde-dconfig-editor
dde-dconfig-editor 是一个gui工具，需要单独安装。
``` bash
sudo apt install dde-dconfig-editor
```
- 选中单个配置项右键可以看到常用功能。
    - 重置：清除配置项缓存
    - 复制命令：复制dde-dconfig命令行到剪切板
- 菜单项
    - OEM： 导出override配置文件
    - config language： 切换配置项描述语言，默认为英文


***应用列表是根据$DSG_DATA_DIRS/configs或者默认的/usr/share/dsg/configs中的子目录名来获取的，所以对于自定义的appid，应用无关配置无法通过此工具设置。***

