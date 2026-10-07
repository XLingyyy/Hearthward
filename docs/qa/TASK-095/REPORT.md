# TASK-095｜角色、持握与动作技术调查

## 2026-10-07 已批准方向后的进展

方向已由DSGN-004批准。已制作兄弟、守卫、射手、重兵五份可编辑源样板并渲染；骨架头尾和层级保持。射手仍带原网格连体短刀/盾，须继续清理，持握/拉弓/倒地/扶起和UE替换未完成。见samples/source-results.json。

以下为本单此前调查与验证记录；旧“待批准方向/暂缓制作/未改Content”的描述以本节更新为准，旧测试只保留其原版本信用。

状态：Active，2026-10-07。无设计依赖的源码/绑定盘点已实施；资产替换和 Owner 视觉签收暂缓。实际工作树为 `G:/GameFactory/Hearthward`、`codex/TASK-084-103-iteration`，参考 HEAD `6fcf5c22e965f0f7409438f19bc7b09e96ffb058`，含共享未提交改动。本单未提交、未推送、未发布；最终受测实现版本由主 Agent 的构建/运行报告绑定。

## 已执行与证据

- 运行 `../.venv/Scripts/python.exe -X utf8 docs/qa/TASK-095/audit_bindings.py`。有界读取实际 Animation、Character、CampaignActor、Combat、Equipment 源码及 gameplay/interface 配置，生成本单 [TECHNICAL_AUDIT.json](TECHNICAL_AUDIT.json)，并为096—098生成各自输出。未加载或编辑 UE 二进制，未扫描 Saved 或计算资产 SHA。
- 主角原生数组8段、弟弟6段，14个配置引用对应的文件均存在；二进制骨架兼容、片段实际播放与 Cook 依赖仍需 UE 验证。守卫和重兵使用6槽，其中两个槽回用 Idle；射手复用 Guard 网格与槽。数组中未见专用倒地、扶起、拉弓槽，这是源码事实，未声明所有实机状态已复现。
- 13个真实 weapon ID 已记录。当前 `HearthwardCharacter.cpp` 的 HeldAxe 分支只显示有耐久的 axe，远程模式会隐藏；挂点为 `hand_r`，初始旋转 `(0,0,-90)`。该分支未绑定其余12件武器，不能把文件已导入等同持握完成。
- 石斧播放位置读取权威 `Combat.Elapsed` 和 `StoneAxeMove` 的 `StoneAxeClipTime`。弟弟步行动画播放率读取实际 GroundSpeed；未增加 Notify 伤害/奖励，也未改变任何速度、判定窗口或体力规则。
- [094准确候选包](../../assets/TASK-094/TASK-095-PACKAGES.json)共127个，`selected_for_change=[]`、锁未取得。已存在070短刃/长矛源；盾、绑腿、箭袋及专用族人/射手成品来源仍未解决。候选清单不授予 Content 写入。

## 定向验证与验收前置

复用已注册的3条有效行为测试，执行权归主 Agent，现本轮结果已附下节：

- `Hearthward.Equipment055.HeldAxeFollowsCurrentInstanceAndMode`
- `Hearthward.Equipment055.StoneAxeLightUsesClipPhaseAndPhysicalBlade`
- `Hearthward.Equipment055.StoneAxeHeavyUsesCurrentMoveAndPhysicalBlade`

这些用例覆盖石斧实际实例/模式、轻重击有效片段和物理刃接触；不能覆盖其余武器、全角色骨架兼容或正常输入连续观感。未新增复制配置或重复现有测试的断言。

|验收项|本轮结果|已有静态证据及剩余前置|
|---|---|---|
|T095-C01 角色识别|NOT_RUN|已登记族人/射手复用模型；需批准角色样板、同光照近中远景实机与 Owner 判断。|
|T095-C02 持握|NOT_RUN|已定位石斧挂点及12件未覆盖的武器；先用现有石斧实机，其他武器需样板和准确写范围。|
|T095-C03 时序|NOT_RUN|已追溯石斧权威时钟；需3条复用测试及逐帧真实接触/单次结算证据。|
|T095-C04 移动|NOT_RUN|实际速度读取路径已核对；需空载/负重/重伤、坡面、停止转向实机。|
|T095-C05 倒地救援|NOT_RUN|原生数组缺专用槽已登记；需具体动作源和双方倒地/中断/恢复的运行证据。|
|T095-C06 Cook绑定|NOT_RUN|14引用文件存在；需 UE 骨架/材质加载及独立 Cook 运行，文件存在不等同通过。|
|T095-C07 连续观感|NOT_RUN|需首线连续战斗昼夜录像、真实输入和 Owner 签收。|

## 2026-10-07 实际定向原生结果

主 Agent 经公开UEClient构建实际成功后执行；[本单原始测试条目](NATIVE_REUSE.json)保留每条 entries、warnings、errors及设备，源报告为 `.agent-local/qa/TASK-085-099/native-first-20261007/index.json`（原报告时间 `2026.10.06-20.35.14`）。受测源码为参考HEAD上的共享未提交改动；此轮在092等待修复之前。范围 3/3 Success，warning 0，error 0。上述静态盘点生成时尚无运行结果，现由本节补充。

|实际过滤器|状态|Warnings|Errors|
|---|---|---|---|
|`Hearthward.Equipment055.HeldAxeFollowsCurrentInstanceAndMode`|Success|0|0|
|`Hearthward.Equipment055.StoneAxeHeavyUsesCurrentMoveAndPhysicalBlade`|Success|0|0|
|`Hearthward.Equipment055.StoneAxeLightUsesClipPhaseAndPhysicalBlade`|Success|0|0|

本单所选测试没有Warning。

警告原文保留，未把有警告的Success写为零警告；只按测试结果登记，不据警告推断正式输入或环境原因。这些是隔离原生行为证据，完整Cxx场景、正式地形、渲染/正常输入、Cook及Owner视觉仍为NOT_RUN。静态脚本编译、分单JSON/空选包/过滤器注册核对和定向 `git diff --check` 亦通过。

## 具体暂缓点与继续路径

Owner 待审项目：角色服装/色块/身份轮廓，普通兵、射手、重兵的首件样板；首线武器持握与倒地/扶起/拉弓的具体动作源。沿用既有身高/骨架比例，不自行选择新性别、脸或风格。该判断来自094风格记录和本单验收，未将全部工程工作标为人工阻塞。

技术范围需协调：真实持握入口 `HearthwardCharacter.cpp`、敌军/族人绑定入口 `CampaignActor.cpp` 当前不在095 allowed_paths。批准具体资产后若需修改这些入口，先由主 Agent 明确扩大到准确文件；不能把绑定代码绕放 Animation 以规避范围。

现阶段可继续复用石斧测试、只读 UE 查询14片段兼容、整理权威动作时间轴。新 Content 修改还需准确包授权、单写者窗口和锁；092真实路线与碰撞尚未完成冻结。工程调查、Owner、正常输入、Cook、真人验收分别登记。
