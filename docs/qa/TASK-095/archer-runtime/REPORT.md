# TASK-095 射手运行绑定与可见拉弓

日期：2026-10-08。环境：Windows 11、UE 5.8.2、RTX 4060 Laptop、Blender 5.2.0 LTS。
范围基线：`df52f80f`；受测实现为该基线上的本目录对应代码与资产增量，提交后补记完整实现 SHA。

## 实现

- Campaign 射手使用独立 `SK_Archer_Combat` 与27骨骨架；保留原22骨参考姿态，新增弓身、弓梢、弓弦及持箭控制。Guard原骨架未改。
- Idle/Walk/Run复用现有Guard下肢动作，持弓上肢重新制作；Shoot为2.2秒动作，0.6秒放箭。身体按原身高缩放，弓箭袋边界不再改变角色高度。
- Owner已批准[可见前摇](../../../assets/TASK-095/ARCHER_TIMING.md)。攻击开始仅登记待发箭，前摇结束沿原投射物链释放一次；受击不能行动、目标失效、时间线切换或放箭路径受阻时取消。既有伤害、3000 cm/s箭速和射程保持。
- 未增加动画Notify伤害结算。已发箭继续使用原投射物逻辑；新的骨骼原点对齐箭尖。
- 六个新UE包与 `Archer-Animated.blend` 在写入前锁定。弓、箭、箭袋原材质补齐骨骼网格用途并保存。

## 实际验证

|验证|结果|证据|
|---|---|---|
|Development Editor构建|PASS|[build.json](build.json)|
|3项Task095原生|3/3 PASS，0 warning、0 error|[native.json](native.json)|
|UE重开后的六个材质绑定|PASS，均为正式Game资产|[pie.json](pie.json)|
|真实Campaign射击代码的隔离PIE|PASS，记录66帧、两支已生成箭、玩家原伤害链生效|[连续录像](continuous-shooting.mp4)|
|拉弓位置与持箭显隐|原生断言PASS，连续帧复核通过|[拉弓](draw.png)、[放箭后](release.png)、[收弓](recovery.png)|
|Blender源与FBX|4段完成，27骨，单位按既有导出器转换为厘米|[source-results.json](source-results.json)|

原生新增用例 `Hearthward.Iteration.Task095.ArcherWindupReleaseAndInterrupt` 覆盖0.6秒前摇、无提前箭、单次释放、受击取消、读档取消、骨骼位置及持箭显隐；其余两项保留预览参考姿态和玩家/敌方箭轨迹验证。材质修正之后实际重开UE并运行PIE；该修正不改变已通过的动画与C++逻辑。

PIE为未保存的灰盒诊断夹具，使用真实CampaignActor与伤害代码；不计作自然通关或真人操作。原始采样间隔见pie.json，录像按原时间戳保持采样帧并编码到30fps，不能据此声称游戏达到30fps。截图导出与编辑器开销不用于性能验收。

## 本轮发现并修正

1. 30 Hz动作插值使箭在0.59秒提前缩小；Shoot改为120 Hz烘焙，把显隐过渡缩短到一个60 Hz渲染帧以内，原失败断言重新通过。
2. 材质数组更新未可靠保存；逐元素写回、明确保存资产后，重新打开的PIE确认绑定。
3. 原静态弓箭装备材质没有骨骼用途标记；补齐后颜色恢复，最终日志不再出现对应缺失用途警告。
4. 后台CPU降频使首轮录制约3fps；仅本次编辑器会话关闭后台降频后重新取样，未保存用户全局偏好。

启动期在Engine初始化之前仍记录13条 `LogAutomationTest: Error: Condition failed`，原文保留于[startup-errors.txt](startup-errors.txt)。其来源尚未定位，不能将整份编辑器日志标为零错误；本次具名原生用例报告为3/3 PASS，PIE脚本完成且材料用途警告已消失。

## 复现与边界

- 可编辑源：`art_source/TASK-095/Archer-Animated.blend`。
- 制作：打开 `Archer-Repaired.blend`，先运行 `author_archer_motion.py` 并把返回字典放入 `bpy.app.driver_namespace['archer']`，再运行 `bake_archer_motion.py`。
- 现有Guard动作由[UE导出脚本](export_guard_motion.py)导出；资产由[导入脚本](import_archer_motion.py)导入并保存。UE生命周期使用公开UEClient。
- [PIE脚本](record_archer_pie.py)通过UEClient的ExecutePythonScript入口运行，隔离存档池；运行后由同一客户端关闭其启动的编辑器。
- 本轮仅完成射手这一项运行增量。其他角色服装落地、其余武器持握、倒地与扶起、完整昼夜/远近/移动验收、实际新版本Cook、Owner视觉签收仍未完成。TASK-095保持Active。
- 原人物源的Tripo来源/许可沿用既有登记，未新增授权结论；原创装备源不含新增外部素材。
