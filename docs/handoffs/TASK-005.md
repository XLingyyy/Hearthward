# TASK-005 交接：正常运行世界时钟

日期：2026-09-17。Owner：XLingyyy；唯一写者：当前 Codex 会话。
任务单：[TASK-005](../tasks/TASK-005.md)。分支：`codex/TASK-005-world-clock`。
基线：`5ec06486bdf4bb4f3d1aeff1a2e6b3b975690529`；tested_commit 为 **UNCOMMITTED_WORKTREE**，基线 SHA 不包含本次实现。

## 实现与设计边界

- `UHearthwardWorldClockSubsystem` 随 Game／PIE 世界生命周期创建，开始游戏后累计实际 World DeltaTime，暂停时不 Tick。
- 只读 `GetSnapshot()` 提供实玩秒、经过日历分钟、经过整日和日内余数；1 实玩秒 = 1 日历分钟，1440 秒 = 1 日。
- 日历结果表示经过时长，未确定开局日期、时刻、昼夜边界。世界重建后从零开始；尚未实现存档或跨图持久化。
- 独立 C++ 状态不依赖 HUD。`Hearthward.Clock` 仅提供开发环境控制台快照和日志，默认不显示，不修改时间，Shipping 不注册此调试命令。
- 本次未新增短动作倒计时消费者。后续建造／扶起读取实玩秒差，不能把日历倍率用于五秒动作。
- 睡眠、篝火跳时、生产、饥饿、资源刷新及最终暂停菜单均未接入；R02／R03／R09 保持 OPEN。
- 未修改任何资产、输入、角色或原设计文件；TASK-004 任务单和资产范围未触及。

## 真实验证

- [构建记录](../qa/evidence/TASK-005/build.json)：UE 5.8.1 Development Editor / Win64 构建退出 0。MSVC 14.44.35228，SDK 10.0.22621.0。
- [原生测试报告](../qa/evidence/TASK-005/automation-index.json)：筛选 `Hearthward.Time`，2 项实际执行，2 项通过，0 失败。覆盖比例、日边界、多日跨度、小数余量、60 fps 分帧累计及新实例隔离。
- [执行摘要](../qa/evidence/TASK-005/automation-result.json)保留完整命令、非零用例数及诊断；完整进程输出保留于本机 task005-output 指向的运行目录。
- [PIE 测量](../qa/evidence/TASK-005/pie-results.json)：同一编辑器中两轮 PIE，共 16 项检查；运行增量与 UE 世界时间一致、暂停 2.5 秒冻结、恢复不补算暂停、第二轮重新计时，行走回归通过。
- [运行日志摘录](../qa/evidence/TASK-005/pie-log-excerpt.txt)与[运行](../qa/evidence/TASK-005/session1-running.png)、[暂停](../qa/evidence/TASK-005/session1-paused.png)、[恢复](../qa/evidence/TASK-005/session1-resumed.png)截图记录状态。首轮暂停前后均为 3.344342600554228 秒，恢复后为 5.83952309563756 秒。

首轮编译的头文件搜索路径错误已通过同目录／相对引用修正，没有修改模块构建配置。
第一次高分辨率截图省略了调试文字，改为 UE 原生 `Shot SHOWUI` 后补采；只重跑相关 PIE 脚本，没有重复原生测试或编译。
引擎初始化仍有 TASK-003 已记录的 13 条 `LogAutomationTest: Error: Condition failed`；发生于用例执行前。
Automation 原始报告的两项业务测试均为 Success；没有据此宣称整个引擎日志无错误，也未改动上层框架掩盖诊断。

## 复现与后续

入口见 [BUILD_AND_TEST](../qa/BUILD_AND_TEST.md)，PIE 脚本为 [verify_clock_pie.py](../qa/evidence/TASK-005/verify_clock_pie.py)。
手动打开原灰盒地图、进入 PIE，在控制台执行 `Hearthward.Clock`，以原生 `Pause` 暂停／恢复并再次查询。
观测值是一次快照，会保留十秒；它不会自动刷新，实时测量使用 `GetSnapshot()`。

用户已追加授权 TASK-005 实现、任务单与证据提交推送。未合并 main；独立评审人和 Issue 尚未落实，流程状态保持 Blocked。
新任务未在既有基线中，标准 `--task TASK-005 --base` 无法认证任务范围；当前按用户拆单并执行授权核对本单声明范围，明确区分本地范围检查与正式基线认证。
未运行打包、两机复现、24 分钟墙钟等待或完整游戏验证；1440 秒日边界由实际原生状态测试验证，暂停由真实 PIE 验证。
