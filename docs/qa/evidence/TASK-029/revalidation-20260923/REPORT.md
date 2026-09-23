# AI NPC 分支本机复验

2026-09-23；受测提交 `bf5fb97eb466d38aeef7b6f092a2c6822926e6e1`，分支 `codex/ai-npc-vnext-rework-01-fix`。

本轮重新构建和运行后的结论：AI NPC 在显式 Development 夹具内的核心功能通过。自然地图尚未接入 AI NPC；模型对部分危险或模糊输入的原始分类仍有偏差，当前由确定性校验阻止执行。不能将本报告视为正式地图、Shipping 或任意自然语言的完整验收。

工作树：`G:/GameFactory/Hearthward-ai-npc-fix`。原 `G:/GameFactory/Hearthward` 保留角色动画分支及其未提交改动。测试期间没有修改游戏 C++、模型、配置、地图或资产，没有提交、推送或合并。

## 本轮实际结果

| 验证 | 结果 | 证据 |
|---|---|---|
| UE 5.8.2 Editor Development 构建 | PASS，退出码 0 | [build.json](build.json) |
| 仓库检查 / Python 工具测试 | 0 errors / 31 tests PASS | [repository.log](repository.log)、[python-tests.log](python-tests.log) |
| 原生 Hearthward 自动化 | 41/41 Success，0 failed、0 not-run；包含 Schema 2→3 实际文件迁移 | [native-index.json](native-index.json) |
| 采集、途中保存恢复、建议刷新 | 23/23 checks PASS | [smoke.json](smoke.json) |
| 执行器 | 50/50 checks PASS | [executor.json](executor.json) |
| 动态资源点移动后自动恢复 | 13/13 checks PASS，含完成后恢复巡营 | [recovery.json](recovery.json) |
| 主动消息 | 17/17 checks PASS | [initiative.json](initiative.json) |
| 协助、保护、回撤战术 | 17/17 checks PASS | [tactical.json](tactical.json) |
| 自主巡营及任务抢占、读档 | 27/27 checks PASS | [routine.json](routine.json) |
| 真实 Qwen 普通 / 压力上下文 | 32/32 safety，核心 raw 20/20；所有逐项断言通过 | [model-matrix.json](model-matrix.json) |
| 上下文降档 / 超限 | 263/263 checks PASS，其中含压力夹具准备断言 | [context-boundary.json](context-boundary.json) |
| 真实键鼠对话闭环 | 6/6 checks PASS | [ui-result.json](ui-result.json)、[ui-trace.json](ui-trace.json) |

原生测试数、模型用例数和 PIE 检查点数分别统计，未相加为统一“测试总数”。相比历史脚本，运行脚本增加了显式启用夹具检查，因此部分检查数增加。

执行器覆盖实际材料扣除、制作和维修、多趟交付、途中回档重建计划、耗尽后仅交付真实数量、取消后的实物保留和替换委托。战术测试验证真实伤害、低血量保护、受围攻时向玩家回撤、倒地后停止追击。巡营测试验证真实移动、等待/跟随的显式覆盖、任务期间导航占用、取消任务和回档后恢复。

## 真实模型与安全边界

- 使用现有 Qwen3.5-4B Q4_K_M / llama.cpp b10964，Vulkan，GPU layers 32；未下载或修改运行包。
- 普通与压力各 16 项，共 32 次生成；输入 1765～2045 tokens，单项均为一次生成。
- 核心 M01～M10：20/20 原始结构符合预期，包含采集、制作、维修、等待、跟随、协助、巡营、玩家库存报告、库存询问和任务回忆。
- 所有原始结构：24/32 达到脚本理想分类。两种上下文的 M11/M12/M14/M16 共 8 项仍可能输出偏宽意图；UE 分别以数量不明、条件未解决或规则冲突阻止候选执行。对应前后权威状态均相同。
- 本轮延迟中位数 3.82 秒，最小 3.49 秒，最大 11.79 秒；最大值包含首次模型加载，不代表纯推理时间。
- CTX-03：降到 `compact_relevant`，2836 tokens，generation=1，未知地点和口头安全限制保留，没有可执行候选。
- CTX-04：`required_minimal` 仍为 4019 tokens，返回 `CONTEXT_OVERFLOW`，generation=0，保留原话，世界状态未改变。

这些结果证明本轮用例中的校验有效；原始模型分类偏差仍是质量限制。

## 实际界面操作

使用 computer-use 的真实键盘/鼠标输入；观察脚本只准备隔离夹具、记录状态并截图，没有调用发送文本或确认任务 API。

1. 点击游戏视口，按 T 打开伙伴对话。
2. 点击输入框，输入“帮我采集两份木材带回营地。”，按 Enter。
3. 模型产生 `collect / wood / 2 / additional_acquired / S1`，界面显示待确认任务卡；此时 requested/acquired/delivered=0，资源16、仓库0。
4. 点击“确认这项任务”，NPC 实际采集、返营、入库。最终 requested/acquired/delivered=2，carried=0，资源14、仓库2。

![确认前任务卡](ui-candidate.png)

![完成后进度2/2](ui-completed.png)

## 发现和处理

### 测试启动入口过时

原 `verify_model_matrix_pie.py` 调用标题页 `new`。当前该操作会进入自然地图并销毁 Bootstrap World，脚本随后访问旧 World，产生 `Cannot nativize 'World'`，实际模型用例数为 0。[首次失败报告](model-matrix-original-failed.json)保留。

本轮只修正 `TASK-040/verify_model_matrix_pie.py` 和 `verify_ctx03_04_pie.py` 的初始化：显式创建伙伴、启用开发玩法、读取现有 loadout、启用原型存档、创建新进度并打开 HUD。原有模型与边界断言不变，两份脚本随后真实通过。

旧执行器/恢复/主动消息/战术/巡营脚本使用相同旧入口。本目录保留实际执行的脚本副本，统一使用上述初始化，原历史脚本未覆盖。

### 恢复测试的旧断言与巡营状态冲突

初次恢复测试已完成自动寻路、真实扣料和交付，但旧断言要求 `BlockReason` 为空。当前巡营实现会在任务完成后将其写为“自由活动 · patrol”，因此旧断言失败。见[保留的失败结果](recovery-obsolete-assertion.json)。

核对 `HearthwardCompanionBehavior.cpp` 的巡营状态写入后，本轮脚本分别检查：执行动作已为 None、没有残留 REPLANNING、巡营授权和 tactical intent 已恢复、显示文字严格匹配当前 routine activity。复测 13/13 通过；未修改游戏逻辑。

### 手动指南与正式地图边界

旧 `TASK-030/MANUAL_UE_VALIDATION.md` 的“Play 后选择新游戏自动生成伙伴”已经过时。当前正式新游戏进入自然地图，README 也明确伙伴等功能尚未接入该地图。本次通过范围是 Bootstrap 的显式测试夹具。

### 构建和日志说明

第一次构建被用户已打开编辑器的 Live Coding 阻止。用户保存并关闭后，同一公开 UEClient 构建成功，未绕过构建保护。

原生运行日志在引擎初始化完成前出现 13 条 `Condition failed`；Hearthward 测试在其后开始，导出报告 41 项全部 Success、退出码0。保留[初始化诊断时序](native-startup-diagnostics.log)，不将此次运行描述为“日志零错误”。

## 运行方式与证据绑定

引擎操作使用 GameFactory `UEClient` 的 `build.project`、`testing.run_automation_tests`、`runtime.launch_editor` 与 `runtime.stop_editor`。自动化入口参照 [Epic 官方说明](https://dev.epicgames.com/documentation/unreal-engine/run-automation-tests-in-unreal-engine)。

[environment.json](environment.json)记录环境和源码绑定；[commands.json](commands.json)记录实际启动命令、脚本、存档 UUID 和进程信息。完整本地进程日志在工作树 `Saved/NPCValidation/`；本目录保留本轮 JSON、运行脚本和关键截图。所有 PIE 会话各用全新 `HearthwardSaveTestPool`，不操作人工存档、不保存夹具地图。测试编辑器均已关闭。

需要手动体验时，用本工作树启动 Bootstrap，并以 `-HearthwardAIBundlePath=G:/GameFactory/Hearthward/Runtime/LocalAI -HearthwardAIBackend=vulkan -HearthwardAIGpuLayers=32 -HearthwardSaveTestPool=<新UUID>` 指定本机模型及隔离档池。进入 Play 后，在 UE Python 控制台执行以下夹具初始化，再点击视口按 T：

```python
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pc = unreal.GameplayStatics.get_player_controller(w, 0)
unreal.SystemLibrary.execute_console_command(w, "Hearthward.Companion.CreateTest", pc)
p = unreal.GameplayStatics.get_player_pawn(w, 0)
p.get_component_by_class(unreal.HearthwardGameplayComponent).enable_adventure()
s = next(x for x in unreal.ObjectIterator(unreal.HearthwardSaveSubsystem) if x.get_outer() == w)
assert s.enable_prototype() and s.start_new_progress()
pc.get_hud().screen.open_page("hud")
```

未运行：Shipping 打包、CPU 后端复测、第二台机器、自然地图的 AI 集成。未提交或推送本轮测试脚本修订和报告。
