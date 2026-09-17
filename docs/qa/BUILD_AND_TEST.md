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
