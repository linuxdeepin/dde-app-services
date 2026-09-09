# dde-app-services v20/v25 代码整合方案

## 1. 目标

将 `gerrit/develop/eagle` 中仍需保留的功能整合到
`origin/master`，使同一套 dde-app-services 源码能够使用 DTK5/Qt5
或 DTK6/Qt6 编译。

systemd service、D-Bus activation/config 和 policy 不随 Qt 主版本切换，
全部沿用 v25 文件及安装规则。本次不验证 v20 系统运行及 Debian 打包，只
验证 DTK5、DTK6 编译、安装清单和现有单元测试。

缓存兼容逻辑在 dtkcore 中实现。DConfig 设置新缓存路径后，每次加载均以
旧缓存为基础，再使用新缓存覆盖同名键；缓存保存继续使用 dtkcore 已有的
`DConfigCache::save()` 实现。

## 2. 基线与 worktree

### dde-app-services

- 原始仓库：`/home/work/dde-app-services`
- worktree：`/tmp/dde-app-services-unified`
- 基线：`origin/master`
- 工作分支：`feat/v20-v25-unification`
- 对比分支：`gerrit/develop/eagle`
- 共同祖先：`687af602410343bc89f915e567d6dcecd20dacc6`

### dtkcore

- 原始仓库：`/home/work/dtkcore`
- worktree：`/tmp/dtkcore-cache-compat`
- 基线：本地 `master`
- 工作分支：`feat/dconfig-cache-compat`
- eagle 来源：`gerrit/develop/eagle`

dtkcore 使用本地 master 而不是重置到 origin/master，以保留本地已有的
开发包依赖提交。

## 3. 分支差异处理原则

`origin/master` 作为统一主线，不直接合并 eagle 分支。eagle 中需要保留的
提交使用 `git cherry-pick -x` 按原顺序摘取，以保留作者、提交说明、
Change-Id 和来源 commit hash。

发生冲突时，在 cherry-pick 过程中基于 origin 当前实现解决冲突，然后执行
`git cherry-pick --continue`。不得将原提交改写成无来源信息的新提交，也不将
多个 eagle 提交 squash。

### 3.1 dtkcore 摘取列表

1. `b1e3ac4c547682544474102e9acd078e026e23f4`
   `chore: add Permissions function`

该提交增加 `AuthorizedReadOnly`、`AuthorizedReadWrite` 枚举，并解析
`authorizedreadonly`、`authorizedreadwrite` 权限字符串。

### 3.2 dde-app-services 摘取列表

按以下顺序执行：

1. `7f00fdc3b408b89ff2993ac151c5c1bd6997d13a`
   `chore: add Permissions type of AuthorizedReadOnly and AuthorizedReadWrite`
2. `721cb5a3aedc7ac57a13d79fe443ccd3afd420ef`
   `fix: OEM 对话框交互修复`
3. `a699f1c209b17369f0d492847850cd79f227bd81`
   `fix: oem 导出的数组类型不对导致 overrides 失效`
4. `29198c6f15e6194842c737cdf3d41ec07a6be6ad`
   `fix: 限制 D-Bus 接口仅允许 root 用户调用`
5. `e809ff37a2ec1b924905a722fa09e2ffe9d84cb1`
   `refactor: 使用 qAsConst 替换 std::as_const 并更新文件时间戳处理`

第 5 个提交解决 `QFileInfo::metadataChangeTime(QTimeZone::UTC)` 无法在
Qt5 编译的问题。整合分支启用 C++17 后，容器遍历最终仍使用
`std::as_const`，避免 Qt6 的 `qAsConst` 弃用警告；时间戳 API 和必要头文件
保留该 cherry-pick 的兼容修复。

### 3.3 不重复摘取的提交

以下改动已经被 origin 中的等价或更新实现覆盖，只在差异审计中记录：

- 配置重新解析的 UAF 修复。
- 负浮点值修复。
- 编辑器空索引崩溃修复。
- 全局缓存同步资源键转换修复。
- 配置 reload 和 Debian trigger 系列提交。
- systemd 加固、D-Bus policy 和服务身份相关提交；运行环境统一保留 v25
  实现，不摘取 eagle 资源文件。

## 4. dtkcore 缓存兼容设计

### 4.1 加载规则

保持 `DConfigCache` 公共方法和缓存 JSON 格式不变。

- 未调用 `setCachePathPrefix()`：只加载原默认路径，行为不变。
- 已调用 `setCachePathPrefix()`：
  1. 按原默认路径加载旧缓存。
  2. 按 `cachePathPrefix` 加载新缓存。
  3. 先写入旧缓存的 `contents`，再写入新缓存的 `contents`。
  4. 同名键无条件由新缓存覆盖，不比较 serial 或时间。

旧缓存解析失败时只记录告警，继续尝试新缓存；新缓存解析失败时保持当前
`load()` 失败语义。旧缓存文件始终只读，不删除、不改名、不修改。

### 4.2 保存规则

- 新缓存缺少旧缓存中的键时，将内存缓存标记为 dirty。
- 后续由现有 `DConfigCache::save()` 将合并结果写入 prefix 指向的新路径。
- 不在 dde-app-services 中复制缓存文件或实现 JSON 合并。
- 不创建迁移状态文件，不记录已接管键。

无状态叠加存在一个明确限制：如果新缓存删除了某键、旧缓存仍保存该键，
下次加载时旧值会再次进入内存并写回新缓存。本方案接受这一行为。

### 4.3 路径范围

兼容以下缓存形态：

- 用户缓存。
- 全局缓存。
- 全局缓存不可用时的 fake-global 缓存。
- 空 appid 的通用配置。
- 带 subpath 的配置。
- 测试使用的 `localPrefix`。

## 5. dde-app-services 适配

- 保留现有 `DTK_VERSION=5|6` 构建入口和 Qt/DTK 映射。
- C++ 标准调整为 C++17，解决 `std::optional` 在 C++14 下编译失败。
- 使用无时区参数的 `QFileInfo::metadataChangeTime()`，兼容 Qt5/Qt6。
- Authorized 权限继续使用 eagle 的 SID2、D-Bus UID/PID 和 root 校验逻辑。
- SID2 最多读取缓冲区容量减一，确保安全标签始终有 NUL 终止符。
- Qt5、Qt6 构建均使用 origin/master 的 v25 systemd、D-Bus 和 policy
  文件及原有 CMake 安装规则，不增加运行环境分支。
- 新增 include 只使用 Qt、DTK或系统公共头文件。

## 6. 提交规则

- eagle 原提交一律使用 `git cherry-pick -x`，不重建、不 squash。
- cherry-pick 冲突解决包含在对应的 cherry-pick 提交内。
- 缓存双加载、DTK5适配、测试和本文档属于新改动，使用
  `git-commit-helper` 分仓库提交。
- 提交后推送到个人 GitHub fork，并分别向上游仓库创建 PR，请求
  `BLumia`、`mhduiy` review。

## 7. 测试与验收

所有编译最多使用 6 个 CPU 核心。

### dtkcore

- DTK5：`cmake -DDTK5=ON`，全量编译并运行相关 `ut-DtkCore` 测试。
- DTK6：`cmake -DDTK5=OFF`，全量编译并运行相关测试。
- 缓存定向测试覆盖：旧/新缓存加载、不同键合并、同键新值覆盖、合并后
  使用现有 `save()` 写新路径、用户/全局/空 appid/subpath。
- 权限测试覆盖 Authorized 枚举及两个权限字符串的解析。

### dde-app-services

- 使用本次修改的 dtkcore DTK5 staging 构建，执行
  `cmake -DDTK_VERSION=5` 和全量编译。
- 使用本次修改的 dtkcore DTK6 staging 构建，执行
  `cmake -DDTK_VERSION=6` 和回归编译。
- 运行现有单元测试；Authorized 权限字符串解析由 dtkcore 新增测试覆盖。

本次不执行 v20 服务启动、系统升级或 Debian 包构建验收。

## 8. 实施结果

### 8.1 已保留的 cherry-pick

dtkcore：

- `1486870` ← `b1e3ac4c547682544474102e9acd078e026e23f4`

dde-app-services：

- `3327602` ← `7f00fdc3b408b89ff2993ac151c5c1bd6997d13a`
- `00e7085` ← `721cb5a3aedc7ac57a13d79fe443ccd3afd420ef`
- `6be9d40` ← `a699f1c209b17369f0d492847850cd79f227bd81`
- `1f62df4` ← `29198c6f15e6194842c737cdf3d41ec07a6be6ad`
- `8b56631` ← `e809ff37a2ec1b924905a722fa09e2ffe9d84cb1`

上述提交均保留原作者、提交说明、Change-Id，并带
`(cherry picked from commit ...)` 来源记录。

### 8.2 新增提交

- dtkcore `b1800e7`：旧/新缓存双加载、新缓存覆盖、保存到新路径及测试。
- dde-app-services：C++17/Qt5 兼容、SID2 边界修复和本文档；运行环境文件
  保持 v25 基线。

### 8.3 验证结果

- dtkcore DTK5 Release：全量编译通过；5 个定向 DConfigFile 用例通过。
- dtkcore DTK6 Release：全量编译通过；相同 5 个定向用例通过。
- dde-app-services DTK5 Release：使用本次 dtkcore staging，全量编译通过。
- dde-app-services DTK6 Release：使用本次 dtkcore staging，全量编译通过。
- DTK5、DTK6 的 dde-app-services 非环境依赖用例各 27 个，全部通过。
- 完整 dde-app-services 测试共 34 个；当前容器不存在 UID 1001～1004，导致
  5 个 `removeUserData*` 用例无法建立测试资源而失败，属于测试环境前置条件，
  与本次改动无关。
- 安装清单已核对：DTK5、DTK6 均安装相同的 v25 systemd 和 D-Bus 文件，
  不安装 eagle 的 PolicyKit policy。
