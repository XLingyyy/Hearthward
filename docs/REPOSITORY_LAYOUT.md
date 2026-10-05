# 仓库目录与文件存放规则

核对基线：main `ac6a302b`，2026-10-05。TASK-075整理补丁现随 `codex/TASK-076-ui-gameplay-integration` 接续，随本轮整合交付main。

## 按用途找文件

| 用途 | 位置 | 维护方式 |
|---|---|---|
| 游戏入口 | 根 `Hearthward.uproject`、[README](../README.md) | 从Bootstrap进入游戏 |
| 游戏源码、插件 | `Source/`、`Plugins/` | 按模块维护 |
| UE导入资产、地图 | `Content/` | 通过UE编辑器管理引用，修改前核对LFS锁 |
| UE配置 | `Config/` | 共享配置需协调 |
| 游戏数据、UI布局、美术、字体 | `Resources/` | 由Build.cs作为运行依赖打包，不放开发报告 |
| 本地AI运行包 | `Runtime/LocalAI/` | 许可证和配置跟踪；权重、可执行文件本地准备 |
| 模型、贴图、动作、参考图等制作源 | [art_source](../art_source/README.md) | 与已导入的Content分开维护，二进制遵循LFS |
| 工具链锁定信息 | `config/` | 与UE的Config目录用途不同，保持现有路径 |
| 当前集成状态 | [PROJECT_STATE](PROJECT_STATE.md)、[分支整合](planning/BRANCH_INTEGRATION.md) | 按真实提交更新 |
| 设计、契约、任务、交接 | `docs/design/`、`contracts/`、`tasks/`、`handoffs/` | 任务编号唯一；历史授权保留原文 |
| 验证报告与必要证据 | [docs/qa](qa/README.md) | 按任务索引，报告绑定实际受测版本 |
| 正式发布记录 | `docs/releases/` | 成品包使用Release或约定的制品存储 |
| 执行脚本 | `scripts/` | 通用入口在此，任务专用验证脚本可随QA记录保存 |
| 本机状态、试验与未筛选输出 | `.agent-local/`、`.agent-cache/` | Git忽略；不作为团队唯一交付 |
| 生成文件 | `Binaries/`、`Intermediate/`、`DerivedDataCache/`、`.vs/`、`*.sln`、`*.slnx` | Git忽略；按需生成 |
| 项目测试端 | `TestClient/` | 仅启动说明与入口入库；Profile、Runs、Logs保留本地 |
| 本地运行数据 | `Saved/` | 包含存档及配置，不能按缓存一并清空 |

UE目录职责参考[Epic官方目录说明](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-directory-structure)。本次没有移动UE资产、游戏源码、配置和打包依赖。

## 2026-10-05旧路径迁移

| 原位置 | 当前位置 | 处理 |
|---|---|---|
| `Resource/Tripo/主角/`、`Resource/Tripo/弟弟/` | `art_source/TASK-004/Tripo/`下同名角色目录 | 3份ZIP和2份局部LFS规则迁入；5个逐字节相同的图片/模型/预览归并到既有制作源 |
| 弟弟outputs内3份不同的JSON | 同一outputs子目录内`*.legacy-resource.json` | 保留原始元数据，现有`task.json`与`rig_pipeline.json`不覆盖 |
| `ui pic/`九张原始设计图 | `art_source/ui-reference/TASK-020/` | 作为旧版UI视觉依据，不作为当前运行时UI |
| `2026-09-22/EvoX-21e5b7f8/` | `docs/assets/requests/2026-09-22/EvoX-21e5b7f8/` | 原始资产需求清单归档 |

历史任务授权、原始JSON绝对路径和受测记录保留当时写法；查找旧文件时按此表映射。不要根据历史记录重建重复目录。`resourceSummary.md`仍是既有资源清单入口，原链接保持兼容。

## 开发材料入库

新验证运行默认写入 `.agent-local/qa/<任务编号>/<运行名称>/`。整理交付时保留报告、复现脚本和报告引用的结果、关键截图；失败结果若用于解释已知限制也应保留。试跑中间版本、整份部署镜像和无引用重复输出留在本地。现有QA材料保持原位置，以保全提交和报告引用。

大型制作源继续放art_source并使用LFS，不能因文件数量多而删掉可编辑源。后续如要迁出至独立素材仓库或制品存储，需先安排可访问的永久目的地，并修改导入、验证与克隆恢复流程。

同步前保存的旧本地改动仍位于stash，记录见[TASK-075交接](handoffs/TASK-075.md)。不要把旧引擎关联或旧UI删除记录直接应用到最新main。
