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

## TASK-013 本地模型验证

开发准备：`python scripts/local_ai/prepare_bundle.py`，固定下载llama.cpp b10964与Unsloth转换的Qwen3.5-4B Q4_K_M；脚本验证发行散列并支持中断续传。玩家运行不调用此脚本。
沿用UEClient构建；原生筛选 `Hearthward.LocalAI`（2项）和 `Hearthward.Companion`（2项相关回归）。提取包与许可证回归：`python -m unittest scripts.tests.test_local_ai_setup -v`。

使用 `-ExecutePythonScript=G:/GameFactory/Hearthward/docs/qa/evidence/TASK-013/verify_local_ai_pie.py` 启动两轮PIE。
该脚本运行真实模型，覆盖明确/模糊/危险/虚报/冲突/多目标输入、暂停、取消、覆盖、旧epoch与离开30米范围；结果在 `Saved/Task013/`。
GPU实测启动参数为 `-HearthwardAIBackend=vulkan -HearthwardAIGpuLayers=32`；默认配置是CPU，不能将GPU报告冒充默认CPU表现。
推理使用一个隐藏的本机子进程、动态loopback端口和临时认证，加载4096上下文、单并发、关闭thinking，最长等待120秒。PIE/世界结束时释放模型。

手动控制台先执行 `Hearthward.Companion.CreateTest`，再执行 `Hearthward.AI.Say 帮我收集十份木材，分几趟运回营地仓库。`。
自然语言通过模型生成候选后，伙伴实际采集、返营、入库。`Hearthward.AI.CancelReply`仅取消推理；取消已接受动作可说“取消刚才的采集委托”。
全部开发控制台入口不在Shipping注册；正式输入面板归TASK-015，持久化/认知快照归迁移后的TASK-016。
非Editor构建要求本地模型包完整；Build.cs将权重、server及DLL、知识和许可证登记为NonUFS运行依赖。登记成功不能替代完整打包运行验收，实际结果见 [TASK-013交接](../handoffs/TASK-013.md)。

## TASK-014 独立灰盒场景

地图为 `/Game/Hearthward/Tests/Graybox/L_GrayboxValidation`，不修改默认启动地图。
通过UEClient的 `launch_editor(map_path=..., extra_args=["-ExecutePythonScript=G:/GameFactory/Hearthward/docs/qa/evidence/TASK-014/verify_graybox.py"])` 运行两轮PIE。
结果与截图写入 `Saved/Task014/`；脚本同时检查6个项目资产的引用闭包，以及实际角色移动、墙体/门洞碰撞、坡道、暂停与新PIE出生。
`create_graybox.py`仅记录初始生成过程；已有资产时拒绝覆盖，后续编辑以已保存UE资产为准，并先核对本单LFS锁。

物理键盘短片沿用公开 `ue.playtest.record`，map_path取014地图，scenario指向本单 `walkthrough-scenario.json`，duration=23、fps=8、warmup=2。
这段录像使用已有角色与操作，不启动LLM；模型权重缺失的干净源码克隆也可验证本单地图。干净克隆先取回Git LFS资产，再用UEClient构建HearthwardEditor并运行同一验证脚本。
本机独立克隆不等于另一真人/第二台机器验证；T-003双账号锁竞争继续独立登记。

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

## TASK-015 伙伴UI

开发模式执行 `Hearthward.Companion.CreateTest` 后，30米内T打开、Enter发送、Esc关闭。Widget接真实本地模型和伙伴执行器；夹具准备与正式地图隔离。背包Tab打开仍默认暂停，对话本身不暂停。
通过UEClient构建HearthwardEditor并用launch_editor的extra_args传入 `-ExecutePythonScript=G:/GameFactory/Hearthward/docs/qa/evidence/TASK-015/verify_ui_pie.py`，运行两轮PIE；真实模型验证可追加 `-HearthwardAIBackend=vulkan`、`-HearthwardAIGpuLayers=32`。原始结果与截图写入Saved/Task015，正式证据见[015交接](../handoffs/TASK-015.md)。
自动测试调用实际Widget事件处理入口；物理键盘/鼠标另行实测，不能混为同一种输入证据。TASK015_INTERACTIVE=1只用于测试结束后保留PIE供手动检查，默认测试自动结束PIE。

## TASK-016 世界知识快照

先创建012伙伴夹具，执行 `Hearthward.Save enable`，再执行 `Hearthward.Save new` 建立初始节点。`manual` / `auto` 新增节点，`list` 查看全池，`load <GUID>` 加载，`lock <GUID>` / `unlock <GUID>` / `delete <GUID>` 管理节点。入口均属于非Shipping原型。

原生筛选 `Hearthward.Save`，预期2项；PIE脚本为 `docs/qa/evidence/TASK-016/verify_save_pie.py`。通过UEClient launch_editor加入 `-ExecutePythonScript=<脚本绝对路径>`、`-HearthwardSaveTestPool=<新生成GUID>`、`-HearthwardAIBackend=vulkan`、`-HearthwardAIGpuLayers=32`。唯一测试池必须显式指定，防止污染人工存档。脚本运行两轮PIE，包含真实一分钟自动保存、真实模型HTTP中回档和磁盘损坏拒绝，结果输出Saved/Task016。

保存协调器公开状态通过Blueprint读取；`SetAutoMinutes`限定1—60，`SetPrototypeSafety`仅作为当前缺少生产危险系统时的显式夹具输入，不代表已经识别真实战斗/溺水/倒地。未接入模块、地图不同或参与者缺失时不能声称完整恢复。

## TASK-017 存档管理UI验证

沿用UEClient Editor构建和 `Hearthward.Save` 两项原生测试。两轮PIE脚本为 `docs/qa/evidence/TASK-017/verify_save_ui_pie.py`；通过UEClient `launch_editor` 加入 `-ExecutePythonScript=<绝对路径>` 和每次全新的 `-HearthwardSaveTestPool=<GUID>`，地图Bootstrap。报告/截图写入Saved/Task017；设置进程环境TASK017_INTERACTIVE=1可在第二轮后保留窗口做实键检查。测试只操作独立池。

手动先创建伙伴夹具，F6打开，点击启用存档，再新进度或读取；确认/取消使用鼠标和Esc，Tab切到背包。菜单打开暂停，关闭仅释放自己取得的暂停。原型安全标志决定实际禁存原因；禁止将其说成已接入生产危险识别。

实键录像通过PIE脚本的Slate回调调用原生 `Shot showui`，与JSON状态轨迹绑定，再用现有FFmpeg编码。原生playtest recorder未声明暂停tick，本单菜单暂停期间使用此截图路径保留UI证据；不修改框架录制器。

独立窗口使用相同UEClient `launch_editor(..., extra_args=["-game", "-windowed", "-ResX=1280", "-ResY=720", "-ForceRes", "-HearthwardSaveTestPool=<新GUID>"])`，另测1920×1080。临时测试夹具通过控制台创建，不保存地图。退出确认应关闭游戏且存档文件大小/修改时间不变。

## TASK-018 玩家采集入库验证

沿用UEClient Editor构建；原生筛选 `Hearthward.Resource`，共2项。`docs/qa/evidence/TASK-018/verify_resource_pie.py`通过launch_editor的ExecutePythonScript参数运行，Bootstrap地图，每轮独立HearthwardSaveTestPool GUID。报告写入Saved/Task018。完整模式两轮39项；TASK018_CAMERA_ONLY=1只执行镜头修正后的4项，不替代完整模式。TASK018_INTERACTIVE=1完成后保留视口；.agent-local/task018-record存在时每约0.35秒请求原生Shot showui并记录状态。

显式执行Hearthward.Companion.CreateTest，150cm内E进行五秒动作，资源点每次1木材进背包，营地整批木材入库。数量、时间、距离均原型参数，未定规则不转为正式设计；覆盖与录像见018交接。

## TASK-019 营地仓储管理验证

UEClient构建HearthwardEditor后，以Bootstrap地图和 `-ExecutePythonScript=G:/GameFactory/Hearthward/docs/qa/evidence/TASK-019/verify_storage_pie.py`、每次全新的 `-HearthwardSaveTestPool=<GUID>` 启动。默认两轮PIE共49项（含五类选项强制GC回归），结果和截图位于Saved/Task019。TASK019_PHYSICAL_ONLY=1只运行最终UI的3项定向检查，TASK019_INTERACTIVE=1结束后保留视口。测试物品来自显式脚本授予，未写入地图或正式经济。

实键：150cm内R打开营地仓储，选物品、输入整数、点击存入/取出；Esc/R关闭，Tab/F6切换。世界暂停，仅释放自己取得的暂停；每次转移复核当前访问与epoch，回档关闭菜单。`.agent-local/task019-record`存在时按018方式连续保存原生截图及状态轨迹。正式键位、营地解锁和兄弟转交未实现。

## TASK-020 九页UI及配套玩法

UE 5.8.1，Bootstrap地图。通过GameFactory公开UEClient构建HearthwardEditor Development；原生测试筛选Hearthward.，17项。图形测试使用真实渲染，不能以NullRHI代替截图。

```python
from engine_adapters.ue5 import UEClient
from pathlib import Path
import uuid
root = Path("G:/GameFactory/Hearthward")
client = UEClient(project_path=str(root / "Hearthward.uproject"), ue_root="G:/UnrealEngine/UE_5.8")
client.build.project(target="HearthwardEditor", configuration="Development", timeout=1200)
# 选择一个脚本；每个新测试必须使用独立的存档池。
script = root / "docs/qa/evidence/TASK-020/verify_ui_pie.py"
launch = client.runtime.launch_editor(map_path="/Game/Hearthward/Bootstrap/L_Bootstrap", extra_args=[
    "-ExecutePythonScript=" + str(script), "-HearthwardSaveTestPool=" + str(uuid.uuid4())])
# 等待Saved/Task020/verification.json更新，检查passed与每项checks，再停止本次进程。
# client.runtime.stop_editor(launch["payload"]["process_id"])
```

verify_ui_pie.py覆盖80项真实玩法与UI事务，生成resume.json记录池ID和具体节点；verify_ui_reload.py需在新编辑器进程使用该池ID，验证8项跨进程恢复。verify_ui_visual.py仅用于视觉改动后的33项入口/截图/存取检查，不能替代80项玩法测试。

verify_ui_extension.py验证旧档迁移与图鉴，共32项。先将本目录evidence/TASK-020/legacy-task019.hws复制到Saved/SaveGames/HearthwardPrototype/test-<新GUID去连字符并大写>.hws，再以该GUID启动。测试只修改副本。旧档来自019的真实隔离测试池，不依赖人工玩家档。

原生CaptureUI以线性浮点目标渲染，再转sRGB写PNG；HUD截图只含UI层，实际三维画面见physical-final-hud.png。1672×941为设计尺寸，另验证1280×720与Windows 150% DPI独立窗口。实键截图来自computer-use的sky窗口捕获，自动化脚本调用Widget命令的结果单独记录。

17项Automation报告均Success；进程启动阶段另有LogAutomationTest的Condition failed诊断，发生于Engine初始化及Hearthward测试开始之前。保留automation-result.json及automation-startup-excerpt.txt，不将进程日志表述为零错误。Shipping打包、完整三维美术、两台机器及独立评审未运行。

## TASK-021 伙伴导航

构建目标HearthwardEditor / Development，使用上层UEClient公开API。以独立 `-HearthwardSaveTestPool=<UUID>` 启动Bootstrap，分别通过 `-ExecutePythonScript=<绝对路径>` 执行证据目录中的 `verify_navigation.py`、`verify_navigation_edges.py`。脚本只在未保存的编辑器世界放置临时障碍，复制到PIE进行真实库存与导航验证，不保存Content资产。结果在Saved/Task021，包含JSON及真实3D截图；不要把UI截图当作导航轨迹。

开发导航为Recast Dynamic，半径34cm、高180cm覆盖伙伴胶囊；临时边界从已有碰撞网格范围生成。正式地图可直接放置NavMeshBoundsVolume。导航路径不进入存档；读档清理控制器路径与速度，沿保存的任务阶段重新求路。

## TASK-022 自由建造

通过上述 UEClient `build.project` 构建 HearthwardEditor Development。
定向原生测试筛选 `Hearthward.Gameplay`，包含现有玩法快照回归与新增建筑字段兼容测试。
通过 `runtime.launch_editor` 加 `-ExecutePythonScript=G:/GameFactory/Hearthward/docs/qa/evidence/TASK-022/verify_building.py`
及唯一 `-HearthwardSaveTestPool=<UUID>` 运行真实PIE；结果在 `Saved/Task022/verification.json`。
测试会创建未保存的临时支撑平台，使用隔离档池；完成后关闭此测试进程，禁止保存临时地图。
覆盖放置、旋转、碰撞、地面边缘、材料整批结算、中断、暂停、存读档、跨PIE磁盘恢复及实际采集到建造。
手动：新游戏→E采集木材或R从仓储取木材→B选设施→Q旋转→左键施工，右键取消；五秒后出现独立建筑。
工作台/篝火目前为灰盒建筑，不含加工、生产、睡眠和升级；配方/半径为独立原型参数。

## TASK-023 工作台即时制作

构建沿用UEClient HearthwardEditor Development。原生测试筛选 `Hearthward.Gameplay.Crafting`，2项：库存整批交换（包含最终容量、失败无部分扣费/产出、整数溢出）及实际配方目录约束。
通过UEClient `launch_editor` 在Bootstrap执行 `docs/qa/evidence/TASK-023/verify_crafting.py`，每次加入独立 `-HearthwardSaveTestPool=<UUID>`。脚本完成真实工作台建造、五秒采集得到制作材料、即时批量制作、访问失效与两轮PIE磁盘恢复，报告/截图在Saved/Task023。建造材料由夹具显式提供，制作材料来自真实采集，不冒充零物资全程建造测试。
`observe_crafting_input.py`只搭建隔离工作台并监听状态；手工/电脑操作工具按E、鼠标点击+、F、返回，结果写physical-input.json。两张皮革面板和操作组均可通过F10独立编辑，背景没有烘焙页面控件。
使用同一UEClient停止自己启动的编辑器；不保存测试墙体到地图，不触碰用户正常档池。引擎初始化前的既有Condition failed诊断单独保留；目标测试报告和进程退出码另行核对。

## TASK-024 装备维修

沿用UEClient构建HearthwardEditor Development，定向原生筛选 `Hearthward.Gameplay.Repair`，1项配方有效性与整批扣料测试。
Bootstrap通过 `-ExecutePythonScript=G:/GameFactory/Hearthward/docs/qa/evidence/TASK-024/verify_repair.py` 和独立 `-HearthwardSaveTestPool=<UUID>` 运行59项PIE验证，报告/截图输出Saved/Task024。显式夹具提供8建造木材、设置装备破损；维修材料通过真实采集木材和023制作绳索获取。脚本绑定库存通知观察完整扣料/耐久状态，并尝试重入维修和保存；随后实际攻击敌人验证武器恢复使用，再验证读档和新PIE磁盘恢复。
`observe_repair_input.py`只布置工作台/破损石斧与材料，监听E打开、鼠标切维修、F修复、返回。用电脑操作工具发送真实输入，physical-input.json与窗口截图单独归档。所有测试都使用独立档池，不保存地图，不触碰人工档。
维修页面新增3个独立可编辑组件组；14页59组。背包H打开维修页，再F确认费用；不再沿用营地任意位置直接扣通用木材/矿石的入口。
