# 构建和测试入口

## 当前状态

TASK-003 已建立根目录 C++ 工程及灰盒地图。Development Editor / Win64 实际构建成功。
运行证据及验收边界见 [TASK-003 交接](../handoffs/TASK-003.md)。打包、两机复现和完整 M0 仍为 NOT_RUN。

## TASK-003 本机执行入口

在 `G:/GameFactory` 使用 `.venv/Scripts/python.exe -X utf8` 执行以下 Python；
换机后显式替换两个绝对路径，并准备相同 UE、MSVC、SDK，先执行 `git lfs pull`。

```python
from engine_adapters.ue5 import UEClient
ue = UEClient(
    project_path="G:/GameFactory/Hearthward/Hearthward.uproject",
    ue_root="G:/UnrealEngine/UE_5.8",
)
result = ue.build.project(target="HearthwardEditor", configuration="Development", timeout=1200)
print(result)
assert result["ok"]
launch = ue.runtime.launch_editor(map_path="/Game/Hearthward/Bootstrap/L_Bootstrap")
print(launch)
# 保留本 Python 会话；完成编辑器操作并保存后，可关闭自己启动的进程：
# ue.runtime.stop_editor(launch["payload"]["process_id"])
```

实际底层构建命令为 `Build.bat HearthwardEditor Win64 Development -Project=G:/GameFactory/Hearthward/Hearthward.uproject -WaitMutex`，
通过 UEClient 调用。目标关联 5.8、BuildSettingsVersion.V7；MSVC 14.44.35228，SDK 10.0.22621.0。

地图打开后按 Play，单击视口，以 WASD 行走、鼠标转动镜头，Esc 结束 PIE。
再次 Play 检查输入。键位用于灰盒验收，正式输入方案 R23 保持 OPEN。
可在编辑器底部 Cmd 输入 `py "G:/GameFactory/Hearthward/docs/qa/evidence/TASK-003/verify_pie.py"`
执行两轮定量测试；结果生成于 `Saved/Task003/pie-results.json`。
此脚本使用 Enhanced Input 动作注入，实键与鼠标测试另行记录，不将它声称为键位映射测试。
`create_map.py` 仅用于初次创建；仓库已含地图时不重跑，脚本拒绝覆盖已有地图。

录屏使用同一 `ue` 实例的公开 API：

```python
from pipeline.common.paths import task_output_dir
out = task_output_dir("hearthward", "pipeline", "TASK-003", run_id="YOUR_UNIQUE_RUN_ID")
result = ue.playtest.record(
    output_dir=out / "keyboard",
    map_path="/Game/Hearthward/Bootstrap/L_Bootstrap",
    scenario="G:/GameFactory/Hearthward/docs/qa/evidence/TASK-003/keyboard-scenario.json",
    duration=15, fps=12, warmup=2, timeout=180,
)
print(result)
```

录屏会向游戏窗口发送真实键盘输入。该入口在未打包工程中使用 Editor `-game` 模式，
PIE 的两次启停由上述独立测量脚本验证。A3GamePlayable 源码已随工程保存，正常构建会编译插件。

## TASK-005 世界时钟验证

沿用上面的 `ue` 实例，实际原生测试入口：

```python
from pipeline.common.paths import task_output_dir
out = task_output_dir("hearthward", "pipeline", "TASK-005", run_id="YOUR_UNIQUE_RUN_ID")
result = ue.testing.run_automation_tests(
    "Hearthward.Time", report_dir=str(out / "automation"),
    extra_args=["-NullRHI"], timeout=240,
)
print(result)
assert result["ok"] and result["payload"]["tests_found"] == 2
```

先结束已有 PIE，再通过 `ue.runtime.launch_editor` 加入参数：
`extra_args=["-ExecutePythonScript=G:/GameFactory/Hearthward/docs/qa/evidence/TASK-005/verify_clock_pie.py"]`。
地图仍为 `/Game/Hearthward/Bootstrap/L_Bootstrap`。
脚本自动执行两轮真实 PIE 的运行、暂停、恢复及行走回归，结果与截图生成到 `Saved/Task005/`。
完成后由同一 UEClient 会话关闭自己启动的编辑器；不重复运行地图创建脚本。

手动体验可打开工程进入 PIE，在控制台输入 `Hearthward.Clock` 获取一次时间快照；
通过原生 `Pause` 命令暂停／恢复，再查询快照。显示值为累计经过时间，不代表故事开局时刻。
默认无计时 HUD，也未实现睡眠、跳时、持久化或最终暂停菜单。

## TASK-006 五秒动作验证

沿用 UEClient 构建入口，原生测试筛选 `Hearthward.Actions`，必须实际执行2项且均通过。
两轮 PIE 脚本为 `docs/qa/evidence/TASK-006/verify_action_pie.py`，通过
`ue.runtime.launch_editor(map_path="/Game/Hearthward/Bootstrap/L_Bootstrap", extra_args=["-ExecutePythonScript=G:/GameFactory/Hearthward/docs/qa/evidence/TASK-006/verify_action_pie.py"])`
运行。报告和截图生成于 `Saved/Task006/`；完成后同一 UEClient 关闭自己启动的编辑器。

手动 PIE 控制台执行 `Hearthward.Action.Start`，等待5秒后执行 `Hearthward.Action` 查询。
运行中用 WASD 可中断；转动视角不会中断。`Pause` 暂停／恢复计时。
快照显示一次观测，保留10秒，不自动更新。伤害中断由测试脚本使用原生 `ApplyDamage` 验证。
开发入口不在 Shipping 注册；它只演示计时，不产生建筑、物资或救助结果。

## TASK-007 动作HUD验证

构建入口沿用 UEClient。HUD通过GameMode自动创建；使用006的开发命令开始计时，
底部实时显示“进行中”、剩余秒数和进度；暂停显示“已暂停”，中断短暂显示“已中断”。
空闲／计时完成后收起；不代表对应玩法完成。显示不改变鼠标、WASD或输入模式。

`docs/qa/evidence/TASK-007/verify_hud_pie.py` 通过同样的 `-ExecutePythonScript` 入口运行，
在两轮PIE捕获1280×720、1920×1080的HUD渲染证据，结果生成于 `Saved/Task007/`。
使用原生HighResShot指定输出分辨率；这些是渲染尺寸检查，不宣称测试了两种物理显示器或DPI。
动作回归继续运行006脚本与 `Hearthward.Actions` 两项原生测试。

## TASK-008 个人背包验证

沿用UEClient构建入口；原生测试筛选 `Hearthward.Inventory`，必须实际执行2项且全部通过。
通过 `-ExecutePythonScript=G:/GameFactory/Hearthward/docs/qa/evidence/TASK-008/verify_inventory_pie.py`
启动两轮PIE；报告和截图生成到 `Saved/Task008/`。入口仍为灰盒地图，结束后关闭本次启动的编辑器。

开发控制台：`Hearthward.Inventory.Add wood 50` 增加50木材，`Hearthward.Inventory.Remove wood 50` 扣除；
`Hearthward.Inventory` 查询五种物品数量及重量。支持ID为wood、stone、ore、meat、arrow，数量必须为正整数。
这些是明确的开发授予／扣除命令，不是拾取、丢弃、烹饪或任务奖励；Shipping不注册。
HUD左下显示实际重量；空载／半载／满载行走分别350／332.5／315cm/s。
新PIE容器为空；存档、装备、共享仓储及正式交互均未接入。

## TASK-009 背包查看与暂停验证

沿用UEClient构建入口和 `Hearthward.Inventory` 两项原生回归。
通过 `-ExecutePythonScript=G:/GameFactory/Hearthward/docs/qa/evidence/TASK-009/verify_panel_pie.py`
启动两轮PIE，结果和截图在 `Saved/Task009/`；结束后由同一UEClient关闭本次编辑器。
此脚本注入Enhanced Input动作并核对Tab映射，未声称发送物理键盘事件。
截图输出1280×720、1920×1080，覆盖空/混合/满载以及关闭状态；不代表物理窗口或DPI兼容测试。

手动进入PIE后，Tab打开/关闭背包。界面只显示持有的普通物品名称、数量、单重及实际负重。
默认打开暂停世界，Tab仍可关闭并恢复；若打开前已暂停，关闭后保留原有暂停。
可用008开发命令授予物品，再用006命令启动动作；打开背包观察“已暂停”及冻结的剩余秒数。
Tab为灰盒临时键位；R23正式输入方案和R09暂停选项例外尚未确定。

## TASK-010 共享仓储与转移验证

沿用UEClient构建入口，筛选 `Hearthward.Inventory` 共4项原生测试（含008两项回归和010两项）。
以 `-ExecutePythonScript=G:/GameFactory/Hearthward/docs/qa/evidence/TASK-010/verify_storage_pie.py` 启动编辑器，
运行两轮PIE，报告与背包截图生成于 `Saved/Task010/`；结束后用同一UEClient关闭本次编辑器。
脚本调用开发命令 `Hearthward.Storage.CreateTestAccess` 创建两个运行期Actor，随后通过两个访问组件存取同一库存。
该命令重复执行不会继续添加访问点，不生成物品、营地美术或修改地图；Shipping不注册。
正式营地解锁和仓储操作UI尚未接入，个人Tab界面仍只访问随身背包。

接口：`HearthwardStorageAccessComponent.Transfer(Personal, ToCamp, ItemId, Count, OperationId, TimelineEpoch)`。
每次新操作生成GUID；重试复用原GUID和载荷，epoch取世界StorageSubsystem。
结果含Result、本次MovedCount与Replayed；重试成功返回MovedCount=0，不能再次结算外部奖励。
`AdvanceTimeline`只失效旧请求并清空当前去重记录，不读档、不清空库存，也不宣称已实现保存。

## TASK-011 距离交互验证

沿用UEClient构建入口，原生测试筛选 `Hearthward.Actions`（2项）。
使用 `-ExecutePythonScript=G:/GameFactory/Hearthward/docs/qa/evidence/TASK-011/verify_interaction_pie.py` 启动两轮PIE，
报告和截图位于 `Saved/Task011/`，结束后由同一UEClient关闭自己启动的编辑器。
脚本带PROTOTYPE_ONLY标签：距离200cm、2木材消费仅是独立测试值，不写入正式配方或地图。

手动PIE控制台执行 `Hearthward.Interaction.CreateTestTarget`，在角色前方创建一个测试方块，未授予物资；
站定后按临时E键交互，观察五秒进度。WASD/受伤可中断，Tab打开背包暂停，目标移远或销毁会取消。
显示“交互计时完成”只表示条件满足，不代表建筑或奖励已生成。材料结算消费者只在自动验证脚本中挂接。
测试命令Shipping不注册；正式目标MaxDistance默认0，必须由后续任务明确配置。
E是灰盒临时键位，当前选取方式为范围内最近目标，完整遮挡/选取及正式键位仍未定。
脚本用Enhanced Input动作注入并核对E映射；不声称进行了物理键盘操作。

## TASK-012 伙伴委托验证

沿用UEClient构建入口，原生筛选 `Hearthward.Companion`（2项）与 `Hearthward.Inventory`（4项局部回归）。
以 `-ExecutePythonScript=G:/GameFactory/Hearthward/docs/qa/evidence/TASK-012/verify_companion_pie.py` 启动编辑器；
两轮PIE报告和截图写入 `Saved/Task012/`，结束后同一UEClient关闭自己启动的进程。
此脚本直接提供结构化候选，真实模型评测另见本任务 `model-evaluation.json`，不能等价为自然语言通过。

手动进入灰盒PIE，站在开阔平地，在控制台依次执行：

```text
Hearthward.Companion.CreateTest
Hearthward.Companion.Collect 10
```

开发命令只在非Shipping注册；运行期创建伙伴圆柱、营地薄板、有限资源方块，不保存地图。
场景参数明确为PROTOTYPE_ONLY：16份木材、每趟重量4、五秒采集、180cm/s直线碰撞移动、50cm到达范围。
观察伙伴行走、采集、返营，以及右上角实际交付从0/10变成4/10、8/10、10/10；全程30米内显示测试状态。
Tab暂停/恢复世界与伙伴动作。`Hearthward.Companion.Cancel` 仅30米内生效，未入库物资仍留在携带容器。
再次下单用真实剩余资源继续；没有自动刷新或补给。路线受阻时尝试返营；回程也被挡时保持真实位置及物资，通路恢复后继续。
当前直线测试通路没有全地图寻路、坡面导航或战斗避险；`bSourceSafe` 是开发场景的显式世界判断，未实现生产环境危险识别。
弟弟正式容量、长期安排与迟到文字展示仍待R14/R18/R20；100容量的复用容器仅为测试宿主。

## 仓库检查命令

```bash
python scripts/validate_repo.py
python -m unittest discover -s scripts/tests -v
python scripts/validate_repo.py --launch-ready
```

最后一项在初始未配置状态应失败，不要为了绿色删掉门槛。
提交前可在真实Git任务分支运行：

```bash
python scripts/validate_repo.py --task TASK-010 --base origin/main
```

此命令用基线中已合入的任务范围检查已提交、已暂存、未暂存和未跟踪改动；
新任务尚未合入基线时会拒绝自动范围认证，先进行任务审批。

## 待M0填充的项目命令

| 项目 | 真实命令 / 值 | 最近有效SHA / 结果 |
|---|---|---|
| 工程.uproject路径 | Hearthward.uproject | TASK-003 已提交 |
| Editor Target编译 | 上述 UEClient build.project | PASS，实现提交见交接 |
| 蓝图编译与资产加载 | 待选择测试地图后填 | NOT_RUN |
| 自动化测试组与非零用例数 | 待实现注册后填 | NOT_RUN |
| 目标平台打包 | 待定 | NOT_RUN |
| 可执行包启动及闭环 | 待定 | NOT_RUN |

Epic官方有编辑器与命令行自动化入口；锁定版本核对后，测试命令须导出报告，并确认实际执行用例数、
通过／失败和异常退出。不能只看进程返回或没有报错。参考 [S14](../references/OFFICIAL_SOURCES.md)。
本包不提供一个假装有工程路径的build.bat，不把不存在的`TribeGame.Tests`声称已接通。

## 证据要求

使用 [测试报告模板](../templates/test-report.md)，包括tested_commit、环境、前置数据、命令、
实际执行用例数、结果、日志/产物地址和SHA。模块测试、PIE与打包运行分别记录。
无UE环境的Agent标NOT_RUN，并把验证交给具备环境的人，不伪造截图与日志。

## 检查器的覆盖边界

`validate_repo.py`检查必需文件、JSON任务结构、依赖循环、R/测试编号引用、本文使用的行内相对文件链接、
原设计归档哈希或LFS指针OID。它不完整解析所有Markdown语法、不验证外部URL可用性、不自动证明角色审批。
可选任务范围检查从指定base读取已审批范围，覆盖已提交、暂存、未暂存及未跟踪路径；基线必须由团队指定，
不能自己挑一个更宽松的base。CI默认只执行结构检查和工具自测，**未自动接入每个PR的任务范围核验**；
该核验先由评审者按真实任务、可信基线执行。后续接入CI属于单独工程任务。
