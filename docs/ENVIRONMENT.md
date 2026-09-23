# 环境与复现

当前锁文件为 [toolchain.lock.json](../config/toolchain.lock.json)，字段 null 代表未决定。Hearthward 工程已初始化，当前统一目标为 UE 5.8.2。

## 2026-09-23 当前统一工具链

- UE 5.8.2 Installed Build，Build.version changelist 56702186，路径 `G:/UnrealEngine/UE_5.8`。
- MSVC 工具目录 14.44.35207，cl.exe 19.44.35228.0；Windows SDK 10.0.22621.0。
- 本机版本与仓库目标一致。当前干净提交的 Editor Development 构建失败，原因与结果见 [TASK-040 clean-tree review](qa/evidence/TASK-040/CLEAN_TREE_REVIEW.md)。
- 已完成任务的验证记录保留执行当时的引擎版本，仅用于追溯；当前新增和复验目标统一为 UE 5.8.2。

## 2026-09-17 历史安装快照

- UE：5.8.1，Installed Build，Build.version changelist 56057345。
- Visual Studio Community 2026：18.9.12120.119。
- 选定编译器候选：MSVC 工具目录 14.44.35207，cl.exe 文件版本 19.44.35228.0。
- Windows SDK：10.0.22621.0；本机另装 10.0.26100.0，不自动切换。
- Git：2.54.0.windows.1；Git LFS：3.7.1；仓库工具 Python：3.13.5，仅依赖标准库。
- 以上是安装与文件版本核验；本项目编译、运行和打包仍为 NOT_RUN。

本机路径保存在被 Git 忽略的 `.agent-local/environment.json`，不包含凭据。
在项目根目录用其中 python 路径执行 `-X utf8 -m unittest discover -s scripts/tests -v`。
用户已授权读取设计资料，可执行 `-X utf8 scripts/validate_repo.py` 全量文档校验。
该脚本包含归档清单要求的完整性检查；实际结果见本次启动提交验证记录。

当前不需要另建 venv 或安装依赖。GameFactory 资产接口复用已有环境；不修改全局 Python。
不要直接运行上层 activate.ps1，它会把默认 UE 工程绑定到旧的 GameFactoryUE 宿主。
后续游戏创建在本仓库，生成资产和中间产物仍通过 GameFactory 的 paths API 分配后按任务导入。

## Git 与 GitHub

origin 已配置为 `https://github.com/XLingyyy/Hearthward.git`，fetch 成功。
GitHub 连接器确认仓库公开、当前连接账户具备管理权限；未修改可见性、保护或成员权限。
本机 gh CLI 返回 HTTP 401；若后续使用 gh，先通过 `gh auth login --hostname github.com --web` 恢复登录。
不要把 token 写入文件或聊天。仓库级作者采用已核实账号 XLingyyy 及其 GitHub noreply 邮箱；未修改全局 Git 配置。
Git LFS 本地安装、pre-push hook、远端锁查询及 locksverify 已配置；双账号锁演练与资产上传未执行。

用户已授权提交推送本次工作流基线和首个任务单；按 WORKFLOW 第20.1节初始化 main。
后续实施 TASK-003 时，从最新 main 创建独立任务分支与工作目录。

## 每人必须一致

UE精确版本及Launcher/源码构建来源、目标平台、编译器和SDK版本、工程插件版本、
项目标识和路径、Git/LFS版本。游戏内模型还需标识、修订、量化、校验值和许可。
不把网页默认展示的最新引擎当作团队版本；不同版本保存资产可能造成额外迁移工作。

## 本地化配置

UE安装绝对路径、模型权重路径、实例端口、临时输出位置与认证保存在本机，不提交密钥。
每任务分开Saved、生成文件、测试存档、向量索引和端口。共享模型文件只读。
不复用其他任务的真实游戏存档作为未声明的测试前置。

## 验证顺序

先跑仓库工具，再按 [BUILD_AND_TEST](qa/BUILD_AND_TEST.md)落实项目构建。
两名成员在干净克隆上验证同一SHA。检查不只有C++：蓝图、LFS资产、场景引用和打包启动都要覆盖。
成功命令、版本、测试地图、测试组和产物放入该文档，失败照实记录。

## 工程升级

单独任务与PR，指定Owner、旧新版本、资产变更量、插件兼容、存档兼容、回滚方式。
功能Agent不得自行升级UE或执行全工程资源重保存。
