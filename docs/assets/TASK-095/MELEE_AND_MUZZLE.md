# TASK-095 已批准的近战与出箭时序

2026-10-09，Owner 在当前会话明确选择两项推荐方案：

- 弟弟近战增加 0.25 秒可中断前摇，保留原 1.2 秒起手周期和原武器伤害。跟随战斗和狩猎使用同一入口；受击、生存行动、目标离开、武器 GUID 变化或读档时间线变化取消未结算的一击。取消不返还已经经过的冷却时间，未命中不扣武器耐久。
- 玩家弓弩从实际可见箭尖发射，检查身体到箭尖及前方 5 厘米的阻挡。正常射击仍按原规则消耗箭、体力、耐久和弩装填；阻挡调用同一投射物命中逻辑，落在墙上的箭按原规则可回收。投掷物保留原发射路径。

弟弟长枪使用 Blender MCP 原创双手突刺，保持现有 Brother 骨架、根缩放与身高；60 fps、1 秒动作，0.25 秒达到前伸峰值。源文件为 `art_source/TASK-095/Brother-Spear.blend` 与 `author_brother_spear.py`，复用本项目原创 Hero 双手 IK 制作函数。新动作不构成原人物模型来源权利的补充授权。

阻挡实现参考 Epic 的 [Single Line Trace](https://dev.epicgames.com/documentation/unreal-engine/using-a-single-line-trace-raycast-by-channel-in-unreal-engine?lang=en-US) 与 [Projectile](https://dev.epicgames.com/documentation/unreal-engine/implementing-projectiles-in-unreal-engine?lang=en-US) 文档，沿用当前游戏的 Visibility 碰撞通道及唯一伤害事件。

构建、定向原生、实际渲染和 Cook 证据分别记录；本设计批准不替代 Owner 视觉验收或完整实玩。
