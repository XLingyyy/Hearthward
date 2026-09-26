# TASK-045 验证报告

2026-09-26，Windows／UE 5.8.2／Development Editor。工程基线main@7ce8262bd8721dc59fb0703d365dab2e10340f89，范围批准be113edd960a60c8f2282de08e51578da0efc049。核心受测源码`072889e41dd37e510d8c995bdae7925ad71b2cac`；最终受测源码`313122c30e40a4f6e4a52d0538f1fb3fb6bd2983`。运行后将同一源码树提交，之后仅整理证据文档。

## 实测结果

| 检查 | 结果 | 绑定与证据 |
|---|---|---|
| Development Editor构建 | PASS，零编译诊断 | 最终源码；[构建记录](evidence/build-result.json) |
| 相关原生回归组 | 20/20，零警告、零失败 | 核心源码；[测试索引](evidence/automation-index.json) |
| 最终战斗定向复测 | 2/2，零警告、零失败 | 最终源码；[战斗索引](evidence/combat-final-index.json) |
| 真实PIE | 27/27 | 最终源码；[运行记录](evidence/pie-results.json) |
| 仓库工具自测 | 33/33 | `python -m unittest discover -s scripts/tests -v`；脚本未改动 |
| 仓库／任务路径检查 | PASS，0错误 | [检查日志](evidence/repo-check.txt)；基线见上 |

原生测试在2.99与3.001A秒验证清敌边界；PIE注入输入到观察完成为约3.367A秒，包含输入派发与帧采样量化，不将其表述为每帧恰好3.000秒。截图复核：提示已清除，尸体按碰撞地面落下。证据清单见[manifest](evidence/manifest.json)。

- [处决中](evidence/execution.png)
- [完成后尸体](evidence/corpse.png)
- [读档恢复](evidence/restored.png)

## 运行方法

通过GameFactory公开UEClient指定Hearthward工程，调用build.project(target="HearthwardEditor", configuration="Development")；调用testing.run_automation_tests，过滤Hearthward.Combat、Survival、Save、Inventory、Time、Gameplay。原生测试使用NullRHI、culture=en及Engine Entry启动地图，隔离测试存档。

PIE使用[verify_combat_pie.py](verify_combat_pie.py)，由runtime.launch_editor在L_GrayboxValidation执行。测试创建既有开发伙伴／敌人夹具，通过真实Enhanced Input动作入口注入F、R和V，调用原生伤害／菜单／存档API；完成后同一UEClient停止其Editor进程。没有改地图、没有物理键鼠或自然地图完整试玩的声明。

## 重点覆盖

- 零耐力处决开始、2.99秒未清敌、3秒边界完成、锁定期间CanAct拒绝、取消释放、重复请求／尸体命中不重复奖励。
- 重型当前血量等于或高于偷袭阈值，普通正伤害打断；盾举起／破防／松键／耐力阈值，闪避窗口；部位护甲本击破损和下一击失效。
- 1／30／120fps真实碰撞跨窗，单次扣费和命中；费用包含既有负重修正。
- 两观察者发现条不相加；尸体传播3秒、受击中断、同尸不续期、持续目击阻止过期；伙伴命中不暴露玩家坐标。
- 真实投射物被墙阻挡、射空落地箭随快照恢复、一次回收；感应重复拒绝、范围高度过滤、冷却恢复。
- PIE中F／R相同行为、实际动画状态、原动画无声音Notify、目标无反击、菜单冻结A计时、取消后重试和存读档。

## 已处理失败

首轮新增快照读取在无Owner的原有组件单测中触发空指针，已修复；低帧率测试最初漏算既有负重修正，现按同一正式费用公式验证。扩展Unity编译组合暴露旧Floor／Chest常量遮蔽冲突，局部改名解决，未改变资产路径。PIE最初将伙伴摆到地板外导致安全保存拒绝，调整夹具落点后继续；Python读取感应字段所需反射已补齐，动画Notify检查改用公开AnimationLibrary接口。截图发现处决提示残留和灰盒尸体悬浮，分别清除过期提示并按实际地面／包围盒落尸，再定向复测。

## 范围限制

- 不声称整个C01—C20矩阵已全覆盖：导航诱饵全路线、搬尸复杂地形、区域交界和多人同时命中顺序仍需正式关卡验收。
- 专用处决动画、不同武器专用动画未制作；目前以既有攻击片段按3秒播放验证流程，灰盒目标无正式受击／死亡演出。感应是屏幕投影轮廓。
- 正式弓弩／投掷速度、重力和装备绝对值未配表。缺弹道字段时拒绝发射，原生弹道夹具数值仅在测试内。自然地图敌人、区域照明／巡逻／藏匿未配置。
- 快照覆盖已加载战斗目标；完整World Partition未加载目标持久化和043刷新／代次生成尚待世界系统集成。
- Shipping、独立打包、长时性能、慢放录像及Owner动作质量签收：NOT_RUN。引擎启动使用NoSound；静音依据执行代码没有声音／听觉调用及动画Notify检查，未宣称做过扬声器录音。
