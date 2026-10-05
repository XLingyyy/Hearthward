# TASK-076 新版UI与玩法整合验证

日期：2026-10-05。受测对象为本报告同一交付中的整合源码，构建、运行先于提交：main `ac6a302ba24ea6531ccdaf880d83e9a03779c51e` + UI `fa1828ed79d26518a5163f522ee0dbf124097209` + TASK-076冲突修复及TASK-075整理。两个来源SHA不是整合实现SHA；最终实现以包含本报告的Git提交追溯。代码最后变更仅修正提示测试的历史文案断言，生产代码的前述回归结果仍适用。

环境：Windows、UE 5.8.2、MSVC 14.44、Windows SDK 10.0.22621.0。引擎生命周期使用GameFactory公开UEClient；图形验证采用独立UserDir和GUID存档池，不覆盖人工存档。最后构建见[完整构建结果](build-result.json)。

## 结果

| 验证 | 实际结果 | 证据 |
|---|---|---|
| HearthwardEditor / Win64 / Development | PASS | [构建](build-result.json) |
| 生存、战斗、库存、存档、玩法、时间、攀越及UI原生回归 | 53项全部通过 | [逐项结果](native-results.json) |
| 世界地图真实Widget渲染与探索/大字号检查 | 2项通过，含125%/150%侧栏、滚动和缩放 | [渲染结果](map-render-results.json) |
| 正常独立游戏的标题、输入、页面、加载、返回 | 144项通过 | [输入报告](input-report.json) |
| 背包拖放、装备、行装管理及返回 | 307项通过 | [背包报告](drag-report.json) |
| 四栏道具与动作互斥 | 65项通过 | [道具报告](quick-report.json) |
| 提示重触发、两秒消失、淡出、暂停和持续动作 | 17项通过 | [提示报告](feedback-report.json) |
| 仓库工具单元测试 | 33项通过 | [日志](tool-tests.txt) |
| 仓库文档/元数据检查 | 77任务快照，0错误 | [日志](repo-check.txt) |

53项原生结果按测试全名合并本轮实际运行，来源保留在source_run中：native-fixed的玩法结果，ui-native-final的地图/输入结果，ui-native-pass的暂停/确认修复结果。旧暂停页目标排版测试按新版设计改为34任务状态下的菜单操作验证，覆盖正常/倒地和100%/150%字号；未把旧布局要求强加给新版设计。原生夹具存在清理及未初始化PlayerInput警告，选中测试没有失败。

图形检查使用真实UE渲染及Widget/Slate事件，未声称本轮完成真人通关、第二机器或发布包验证。已查看本轮标题、夜袭HUD、150%行装管理及150%世界地图截图：

- [标题](boot-title.png)
- [夜袭中的新HUD](cycle_2_gameplay.png)
- [150%行装管理](management-player-150.png)
- [150%世界地图](world-map-150.png)

## 冲突与修复

14个初始文件冲突全部解决，31个双方共同改动文件按功能检查。保留main统一日夜、伙伴攀越、生存失败/救援/治疗设施、epoch和存档校验，接入新UI的拖放、四栏道具、进食计时及提示修订号。

新版局部地图为默认入口，另接回世界地图的探索雾、已发现地点、任务指引、路标、驻军筛选与传送；日志定位进入世界地图。输入、确认和大字号实际失败及修复见[回归记录](regression-findings.json)。提示专项首次因会话中断未生成报告，重跑显示2项历史文案断言不匹配；主干已改为具体满血用药拒绝原因，修订断言后17项全部通过。原始失败、运行日志、启动/退出记录留在`.agent-local/qa/TASK-076/`，不覆盖或伪装原始失败。

## 复现入口

在准备好工程的Python环境通过UEClient执行 `build.project(target="HearthwardEditor", configuration="Development", timeout=1200)`。原生使用 `testing.run_automation_tests`，过滤器为：

```text
Hearthward.Survival+Hearthward.Gameplay+Hearthward.Combat+Hearthward.Save+Hearthward.Inventory+Hearthward.Traversal053+Hearthward.Time+Hearthward.Map064+Hearthward.UI069+Hearthward.Experience
```

原生逻辑测试传`-NullRHI`；地图渲染去掉该参数，使用`-RenderOffscreen -Map064Render`，筛选`Hearthward.Map064+Hearthward.UI069.MapScaledGuidanceKeepsCompleteText`。每次使用新报告目录、UserDir和HearthwardSaveTestPool GUID。

独立游戏使用 `runtime.launch_editor`，地图Bootstrap，参数`-game -RenderOffscreen -unattended -nosound -CoreLimit=4 -windowed -ResX=1600 -ResY=1000`。分别通过HearthwardInputVerify、HearthwardDragVerify、HearthwardQuickVerify、HearthwardFeedbackVerify指定报告路径；后三组增加HearthwardHUDPreview。相应现成入口为`scripts/ui/verify_input_client.py`、`verify_drag_client.py --suite equipment-return`、`verify_quick_client.py`和`verify_feedback_client.py`。逐组运行，结束后通过同一UEClient关闭自己启动的进程。

## 交付边界

用户已明确授权提交、推送并更新main。现有Content、Config、Runtime、其他工作树和正式存档保持。没有强推、历史清理、删除开发分支或修改远端保护。来源UI的4,235份QA历史变更仍保留原分支，本轮仅提升上述必要证据。Shipping安装包、模型理解矩阵、完整路线/性能与美术人工验收沿用原任务限制，不能据本次整合标成通过。
