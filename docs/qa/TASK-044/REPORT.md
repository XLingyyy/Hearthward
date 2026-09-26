# TASK-044 实施验证

- 日期：2026-09-26。受测源码：`8f512a7dc4cf46e33d70c67c9294b6a2f3d8da77`。后续提交仅更新文档、状态与证据。
- 环境：Windows 11、UE 5.8.2、Win64 Development Editor、RTX 4060 Laptop。工作树为本任务独立目录；测试保存使用独立随机HearthwardSaveTestPool，不覆盖用户档。
- 结果：Editor构建PASS；相关原生18/18 PASS，0警告／失败；真实PIE 23/23 PASS；仓库工具33/33 PASS（本轮早先执行，工具源码未改变）；仓库结构／路径检查0错误。

## 可复现入口

从GameFactory用UEClient并显式指定本任务Hearthward.uproject和G:/UnrealEngine/UE_5.8：

```python
u.build.project(target="HearthwardEditor", configuration="Development", timeout=1200)
u.testing.run_automation_tests(
    "Hearthward.Survival+Hearthward.Save+Hearthward.Inventory+Hearthward.Time+Hearthward.Gameplay",
    report_dir="<task>/Saved/Task044/final-automation",
    extra_args=["-NullRHI", "-culture=en",
      "-ini:Engine:[/Script/EngineSettings.GameMapsSettings]:EditorStartupMap=/Engine/Maps/Entry"],
    timeout=300)
u.runtime.launch_editor(
    map_path="/Game/Hearthward/Tests/Graybox/L_GrayboxValidation",
    extra_args=["-NoSound", "-culture=en", "-HearthwardSaveTestPool=<unique-id>",
      "-ExecutePythonScript=<task>/docs/qa/TASK-044/verify_survival_pie.py"])
# 读取Saved/Task044/pie/results.json，使用同一UEClient实例stop_editor(process_id)。
```

仓库内：`python scripts/validate_repo.py --task TASK-044 --base 0ef6b18`；工具自测：`python -X utf8 -m unittest discover -s scripts/tests -v`。

原始证据：[构建](build.json)、[自动化命令](automation-command.json)、[原生测试结果](automation.json)、[PIE检查结果](results.json)、[PIE脚本](verify_survival_pie.py)。

## 覆盖范围

- 原生：W跳时饥饿／严重期限／致死停止、120秒倒地、独立残伤致死、持续药A时间、预留药不可转移／制作、同帧取消与伤害、半份重量／恢复、重复事件、满血提交、持续效果刷新与弱效拒绝、稀有物撤权及半份继承、救援截止同刻优先、10%扶起；现有库存／存档迁移／玩法／时钟回归。
- PIE：真实Actor、WorldClock、库存、NativeDamage、Enhanced Input移动／镜头、暂停菜单、兄弟互救、待结算药品保存恢复、双方倒地失败和回档恢复。测试布置双方位置与生命值；救援过程由游戏组件持续完成，不伪造救援结果。此证据不声称物理键鼠或真实模型推理验收。
- 目视检查：[倒地HUD与弟弟扶起倒计时](player-downed.png)、[失败后暂停的回档页](failure-load.png)。文字可见；没有新增倒地／扶起动画，角色仍使用既有站立表现。

## 失败历史与修正

- 原生夹具曾重复初始化UWorld导致崩溃，移除重复初始化。
- 初轮原生17/18通过；药品失败定位为库存Exchange拒绝空产出，消耗无剩余药量改走Remove，随后相关6/6通过，最终18/18通过。
- PIE首轮因未取回现有灰盒地面LFS资源持续下落，保存正确拒绝；补齐既有资源，无资产改写。
- Python夹具曾调用未暴露设置／字段／输入查询，修正为实际公开接口和只读字段。
- 一次反向救援启动失败；当次缺少位置证据，未定为游戏缺陷。后续记录显示弟弟在首次救援后自由活动；最终将反向测试作为独立布置，恢复150cm间距后验证5秒真实扶起。
- 截图原先同帧请求与状态切换导致拍到前后状态，增加渲染等待后重新获取并目视确认。

## 尚未验证或配置

正式坠落曲线、游泳耗耐力数值、新持续药配方和治疗区域未配表；代码入口与夹具通过不等于地图已配置。完整043床睡眠／生产／刷新未集成；044只有W生存边界入口。危险救援的复杂路径和火力组合未做全图穷举。Shipping打包、自然地图全流程、长时性能及Owner体验验收NOT_RUN。本结果绑定任务分支，未声明main已集成。

工程参考：Epic [Enhanced Input Python API](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/EnhancedInputLocalPlayerSubsystem?application_version=5.7)核验输入注入方法；实际兼容性以本机5.8.2运行结果为准。
