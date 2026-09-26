# TASK-052 验证记录

- 日期：2026-09-26，操作者：Codex；Owner / Reviewer：XLingyyy。
- tested_commit：`81aed96d7f9544e96f243970b1152f59c4791d42`。
- 环境：Windows、UE 5.8.2、VS 14.44.35228、Windows SDK 10.0.22621.0；Editor Development，NullRHI 原生测试。
- 工作树：`G:/GameFactory/Hearthward/.agent-local/TASK-052`；上层 UEClient 从 `G:/GameFactory` 的 `.venv/Scripts/python.exe -X utf8` 调用。
- 本记录后的提交仅包含文档及测试证据；无新的源码变化。

## 已运行

| 检查 | 实际结果 | 证据／范围 |
|---|---|---|
| UE Editor Development 构建 | PASS，exit 0 | [build.json](build.json) |
| Hearthward.Time + Hearthward.Save | PASS，10/10，0失败、0警告、0跳过；进程exit 0，诊断为空 | [index.json](index.json)、[runner.json](runner.json) |
| 仓库自检 | PASS，0 errors | `python -X utf8 scripts/validate_repo.py` |
| 任务范围检查 | PASS，0 errors | `python -X utf8 scripts/validate_repo.py --task TASK-052 --base f34be1ed8527a75eb4eec5b955b86df41961e7b7` |
| 工具自测 | PASS，33/33 | `python -X utf8 -m unittest discover -s scripts/tests -v`；未修改工具代码 |
| diff whitespace | PASS | `git diff --check` |

调用使用现有公开 API，遵循 [Epic Automation Tests 文档](https://dev.epicgames.com/documentation/unreal-engine/run-automation-tests-in-unreal-engine)，没有新建测试框架。

```python
from engine_adapters.ue5 import UEClient
ue = UEClient(
    project_path="G:/GameFactory/Hearthward/.agent-local/TASK-052/Hearthward.uproject",
    ue_root="G:/UnrealEngine/UE_5.8",
)
ue.build.project(target="HearthwardEditor", configuration="Development", timeout=1200)
ue.testing.run_automation_tests(
    "Hearthward.Time+Hearthward.Save",
    report_dir="G:/GameFactory/Hearthward/.agent-local/TASK-052/Saved/Task052/automation-complete-assets",
    extra_args=[
        "-NullRHI", "-culture=en",
        "-ini:Engine:[/Script/EngineSettings.GameMapsSettings]:EditorStartupMap=/Engine/Maps/Entry",
    ], timeout=300,
)
```

## 10 项用例的实际覆盖

- 时间比例、分帧一致性与多世界隔离。
- 20:00 + 480 分钟到次日04:00，再睡到同日12:00；A不增加；跨多个周期与分段跳时一致；独立A/W回滚、非法输入不改变状态。
- 真实UWorld中的采集结算和库存：部分采集无计时、耗尽2880分钟到期、暂停不推进、重复耗尽回调无奖励、建筑占地延后、多周期不积累产出、拆除建筑后一次恢复、恢复旧采集快照撤销未来再生、满背包无消耗。
- 存档池保护、文件完整性及原子替换、真实旧自然档夹具、旧NPC schema1/2兼容、schema3缺省属性迁移为schema4、独立日历和刷新截止时间落盘往返、新档损坏字段严格拒绝且不覆盖旧文件。

## 修复与环境诊断

1. 首次编译失败：新增头文件引用写成 `Misc/LexFromString.h`。已查UE5.8源码，修正为 `String/LexFromString.h`，后续构建通过。
2. 首次自动化10项通过，其中世界销毁警告1项：夹具没有登记WorldContext。补齐实际上下文生命周期；最终测试0警告。
3. 稀疏工作树启动时缺角色动画和斧头资产：按日志补齐 `Content/Characters` 和 `Content/Hearthward/Assets/TASK-028/props/stone_bone_axe` 的本地LFS对象，没有改资产。启动地图通过进程参数使用Engine Entry，没有修改配置文件。
4. 中文环境启动时出现UE内建Smoke测试的 `Condition failed`。本机源码 `Engine/Source/Runtime/Core/Tests/Experimental/UnifiedError/UnifiedErrorTests.cpp:485` 起有英文字符串硬编码检查，日志中的实际字符串已本地化为中文。仅本次测试增加 `-culture=en` 后，进程diagnostics为空；游戏测试仍10/10。未关闭测试、过滤错误或修改引擎／系统语言。

## 验证限制

- 这些是原生状态、真实UWorld领域结算及文件测试；未跑自然主地图键鼠试玩、渲染画面、Shipping或长时性能。
- 世界层睡眠／篝火明确拒绝未接入的生存结算。只有纯时钟的480分钟运算已验证；不表示正式睡眠可以使用。
- M0／开场日期、昼夜光照配置、生产、动物、敌人／击晕及传送联动未完成，整单仍Active。
- 建筑阻塞验证对应当前已建建筑的旋转Box占地；全地图建筑流送与视觉验收未覆盖。
- 新格式写入后旧程序不能读取HWS4；若回退程序，需要保留新档并使用升级前备份。未运行破坏性存档替换，也未接触用户原工作区存档。
