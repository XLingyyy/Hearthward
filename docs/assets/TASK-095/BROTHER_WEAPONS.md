# 弟弟装备呈现

2026-10-08，沿用DSGN-004和持续完成授权。实际实例GUID、耐久、倒地及施救/工作状态驱动右手武器显示，覆盖现有13种近战装备。复用已导入的短刃、长刃、长枪、战斧及原石斧，不修改攻击伤害、间隔或新增远程攻击。

弟弟原待机右手位于腹前，直接挂接长刃和战斧会穿过肩部。新增原创 `A_Brother_WeaponCarry`，将右手放到身体右前侧；仅对 `upperarm_r` 及其子骨骼分层，原行走腿部继续播放，工作/攻击/倒地/施救层保留更高优先级。原石斧沿用既有持握；它与新版握点不同，不套用新的携带姿势。分层方法参考 [Epic Layered Animations](https://dev.epicgames.com/documentation/unreal-engine/using-layered-animations-in-unreal-engine)。

制作源 `art_source/TASK-095/Brother-WeaponCarry.blend`，配方 `author_brother_carry.py` 复用本任务原创双骨IK与掌心坐标计算。该资产为固定携带姿势，非步行动捕；61骨参考层级、角色比例和原移动规则保持。输出通过框架 `task_output_dir` 分配，动画单独导入现有 `SK_Brother_Skeleton`，兼容既有根缩放100。新姿势为本任务原创，底层角色网格的原来源/许可状态仍沿用094登记。

动态攻击接触、弟弟长枪专用突刺和完整Cook仍需后续完成。护卫/狩猎增加0.25秒前摇的设计问题另待Owner答复，本增量不提前改变命中时序。
