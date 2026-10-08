# 玩家弓弩动作与可弯曲长弓

沿用DSGN-004和继续完成授权，四段Hero原骨架动作由本地Blender原创制作：BowDraw 1秒、BowRelease 0.35秒、CrossbowAim 0.5秒、CrossbowReload 1.8秒。可编辑源为Hero-Ranged.blend，生成过程见author_player_ranged.py。兄弟、敌人和原Hero骨架包保持原绑定。

长弓复用已制作的木弓几何、UV和基础色贴图，新增四骨可弯曲副本。拉弦额外35厘米，弓梢后移10厘米，弦中点抬高3.3厘米让箭位于持弓手上方。PlayerBow.blend保存骨架、权重和动画；厘米导出、UE根缩放1，与Hero既有根缩放100分别处理。

运行时采用[Epic上身分层动画方式](https://dev.epicgames.com/documentation/unreal-engine/using-layered-animations-in-unreal-engine)，在现有跳跃/移动姿态上从spine_01覆盖上身；后续受击/攻击、倒地、起身和施救层保持其优先级。上身瞄准旋转读取真实控制方向，瞄准时角色使用控制器水平朝向，退出后恢复按移动朝向转身。

动画读取Combat.Action、Elapsed和既有装备/耐久。拉弓1秒达到最大姿态，提前松手从已有拉距恢复，松手后弦快速回弹；搭箭只在有箭且拉弓/待射时显示。弩装填使用原1.8秒规则，装填完成后显示弩箭，发射后隐藏。可见弩箭复用箭模型并沿长度缩至55%，不改变库存物品。

当前实际发射仍使用旧Combat固定生成点；改变为真实箭尖并补遮挡检查的方案已向Owner提出，未获答复前保持原弹道入口。此限制不视为动作和接触验收完成。

实机、原生、失败记录与未验边界见[运行报告](../../qa/TASK-095/player-ranged/REPORT.md)。
