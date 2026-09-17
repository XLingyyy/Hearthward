# TASK-007 持续动作实时进度反馈

Owner：XLingyyy；当前会话单写。本地实现与验证完成；流程Blocked（Issue、独立评审未落实）。
分支 `codex/TASK-007-action-hud`，基于已推送 TASK-006，作为堆叠任务开发。

## 目标与设计依据

GDD v0.3 Q043、Q135要求五秒读条，第14章要求真实状态反馈。
给现有持续动作增加原生HUD，只消费006已实现的公开状态、经过秒数和持续时间常量。
运行时显示实时进度与剩余秒数；暂停显示“已暂停”，数值保持不变；中断显示“已中断”1.5游戏运行秒。
1.5秒为界面呈现时长，不改变任何动作判定；暂停期间提示保留。
空闲与计时完成时隐藏，不声称建筑完成、材料结算或扶起成功。
HUD不占用输入、不设新键位，不创建第二份动作计时；更换Pawn或重进PIE清除旧提示。

## 范围与验收

- 新增原生AHUD并在现有GameMode选择；不新增UI资产或模块依赖。
- 底部居中小型进度条，中文提示可读，至少两种渲染尺寸内不裁切；保留场景与角色可见。
- 实际PIE检查空闲、运行、暂停、恢复、完成、移动中断、伤害中断与再次进入。
- 构建通过；006原生动作测试及两轮真实PIE回归；截图证据与交接入库。
- 不改持续动作机制、输入映射、004资产范围、原设计、库存／保存契约；不交付浏览器前端或完整游戏HUD。

用户授权完成后直接提交推送，不合并main。精确路径见 [机器任务单](TASK-007.json)。
参考 Epic [HUD与Canvas](https://dev.epicgames.com/documentation/unreal-engine/user-interfaces-and-huds-in-unreal-engine)及
[AHUD](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/AHUD)。现阶段使用已有Engine依赖完成小型只读显示。
