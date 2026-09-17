# TASK-005 正常运行世界时钟与暂停冻结

状态：本地实现与验证完成；流程快照为 Blocked，独立评审和 Issue 尚未落实。Owner：XLingyyy；唯一写者：当前 Codex 会话。
分支：`codex/TASK-005-world-clock`，基于 TASK-003 已推送成果 `5ec0648`；不合并 main，不执行 TASK-004。
用户已授权拆单并直接实施。机器快照：[TASK-005.json](TASK-005.json)。

## 目标与设计依据

建立所有后续计时功能可以读取的单一正常运行时钟。原始 [GDD v0.3](../归火Hearthward游戏设计文档_v0.3.docx) 第4章，Q025—028、Q152—156：
24 实玩分钟 = 1 游戏日，即 1 实玩秒 = 1 游戏分钟；暂停时计时冻结。建造／扶起等短动作仍按实玩秒计量，不能乘上日历倍率。

本单只实现累积实玩秒数和对应日历时长，不确定开局日期／时刻、昼夜分界、太阳运动或天气。
R02／R03 的睡眠、篝火跳时和生产结算仍待定；R09 的界面暂停设置优先级不在本单决定。
计时范围为 UE 正常运行、默认时间倍率，读取世界 DeltaTime，不使用 UTC 或进程启动时间补算暂停。

## 实施

- 新增游戏世界时钟子系统，仅在 Game／PIE 世界开始游戏后累计；编辑状态不创建有效游戏时钟。
- 只读快照提供 ActivePlaySeconds、ElapsedCalendarMinutes、ElapsedDays、MinuteOfDay。
  ElapsedDays 从 0 起表示经过整日，MinuteOfDay 表示剩余日历分钟；两者均是经过时长，不宣称开局为午夜。
- 子系统生命周期限当前世界；重进 PIE 从零计时。跨地图、存档恢复和快进接口留给相应任务，不伪造持久化。
- 原生 C++ 计时测试检查比例、跨日、分帧累积和新实例隔离；PIE 检查运行、暂停、恢复与再次进入。
- 提供开发控制台 `Hearthward.Clock` 快照查询，便于查看真实运行值；默认不显示调试信息，不新增最终 HUD 或按键方案。
- 沿用现有灰盒地图和行走代码，不修改任何二进制资产，也不占用 TASK-004 资产目录。

## 验收

1. Development Editor / Win64 构建通过；原生 Automation 筛选 `Hearthward.Time` 实际执行且全通过。
2. 60 实玩秒对应 60 日历分钟；1440 实玩秒对应 1 整日；跨日余数、浮点分帧累积不丢失时间。
3. PIE 正常运行累计值与 UE 实际世界时间增量相符；真实暂停至少 2 秒期间快照不变；恢复后继续增加且不补算暂停。
4. 第二轮 PIE 时钟独立，从零开始；编辑世界不产生游戏时钟，避免双计时。
5. 两轮均能生成原角色，移动回归通过；控制台快照有实际画面证据。
6. 交接记录实际代码状态、命令、环境、测量与日志。未接入的睡眠、生产、存档等明确 NOT_RUN。

## 范围与权限

只修改 `Source/Hearthward/Time/`、`Source/Hearthward/Tests/WorldClockTests.cpp`、本任务文档与证据，
以及 AGENTS、START_HERE、PROJECT_STATE、BUILD_AND_TEST 的当前状态。不存在其他系统消费者，不改现有库存／伙伴／存档契约。
后续消费者接入时需评审上述只读接口；本任务不自行批准跨模块契约。
用户已追加授权本任务实现、任务单及验证证据提交推送。不得执行资产采购、生成、导入、合并或远端设置变更。

参考：Epic [AHUD 调试入口](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/AHUD/ShowDebugInfo)用于比较引擎已有调试能力；
实际实现采用原生控制台快照，无需引入 HUD 类或 UI 依赖。
交接输出为 `docs/handoffs/TASK-005.md`，证据位于 `docs/qa/evidence/TASK-005/`。
