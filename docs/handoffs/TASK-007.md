# TASK-007 交接：持续动作实时进度反馈

日期：2026-09-17；Owner：XLingyyy；当前会话单写。
任务：[TASK-007](../tasks/TASK-007.md)；分支 `codex/TASK-007-action-hud`。
父基线 `cbfdf78a0a0fbc03d332d89da96e6becd7c7b8d4`；依赖未合并TASK-006，按堆叠任务评审，禁止先合并。
测试时为UNCOMMITTED_WORKTREE，实现提交绑定在提交后补记；基线不包含本次HUD。

## 实现

新增 `AHearthwardHUD`，由现有GameMode创建；仅读取006组件的公开状态、经过秒数和持续时间常量。
底部居中显示实际进度和剩余秒数，暂停时显示“已暂停”；中断提示显示1.5游戏运行秒。
空闲、计时完成后收起；没有“建造成功”／“救助成功”文案，领域结算仍未实现。
UI仅保存上一显示状态、提示消失时刻和观察的Pawn弱引用，不复制动作进度、不发玩法命令。
更换Pawn清除旧提示；默认隐藏且不占用键鼠输入。004、原设计、资产、输入映射、006动作代码和模块依赖均未修改。
本单为原生游戏内显示，不扩展浏览器交付或完整HUD，不关闭R23等设计问题。

## 验证

- [构建](../qa/evidence/TASK-007/build.json)：UE5.8.1 Development Editor / Win64通过，MSVC14.44.35228、SDK10.0.22621.0。
- [原生测试](../qa/evidence/TASK-007/automation-index.json)：`Hearthward.Actions` 2项实际执行、2通过。[执行摘要](../qa/evidence/TASK-007/automation-result.json)保留启动阶段13条已知Condition failed诊断，不声称引擎日志无错误。
- [动作回归](../qa/evidence/TASK-007/action-regression.json)：复用未修改的006脚本，两轮32项通过，包含移动／伤害中断、暂停冻结、重复事件和从零重试。
- [HUD运行记录](../qa/evidence/TASK-007/hud-results.json)：两轮共10项状态检查与12张状态截图；使用HighResShot原生渲染1280×720及1920×1080。
- [画面检查](../qa/evidence/TASK-007/visual-review.md)记录中文可读性、状态、布局与显示消失检查。

首轮720p截图发现字号偏小，调整Canvas字体缩放为原来的1.6倍，然后重新构建并补跑两轮HUD检查。
原生动作测试及006回归发生于字号调整前；调整只涉及DrawText/GetTextSize的字体缩放，不触及动作或输入路径，因此没有重复运行这些测试。
新构建后的HUD截图对应最终字号。两种尺寸是实际渲染输出检查，不代表测试了两种显示器、物理窗口尺寸或DPI。

## 复现、权限与边界

运行入口见 [BUILD_AND_TEST](../qa/BUILD_AND_TEST.md)。[HUD脚本](../qa/evidence/TASK-007/verify_hud_pie.py)通过UEClient启动原生PIE；回归复用006的 `verify_action_pie.py`。
手动在PIE控制台执行 `Hearthward.Action.Start`，退出控制台观察；`Pause`暂停／恢复，WASD中断。开始入口仍为开发命令，正式交互键位和领域动作尚未接入。
未运行打包、两机复现、控制器／重绑定、完整玩法闭环。未新建资产或申请资产锁，原TASK-003锁保留原归属。
用户已授权本单完成后直接提交推送，不包含合并main；独立评审与Issue缺项，流程状态保持Blocked。
新任务不在父基线，范围核对基于当前用户授权的任务声明；不冒充正式基线审批认证。
回退本任务实现提交即可移除HUD及GameMode接线，005／006机制保留。
