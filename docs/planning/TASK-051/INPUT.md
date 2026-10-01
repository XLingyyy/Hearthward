# TASK-051｜默认操作与上下文消费表

状态：APPROVED；[D1／D2](../../design/DSGN-R23-input-traversal-acceptance.md)为权威语义。默认键保留当前入口，051分支已接入新增通行。重绑定界面及全部HUD提示须读取操作ID的当前绑定；不得只修改文字。

| 操作ID | 默认输入 | 上下文／行为 |
|---|---|---|
| move.forward/back/left/right | W/S/A/D | 世界移动，输入框内仅输入文字 |
| look | 鼠标移动 | 世界视角，灵敏度与反转沿用户设置 |
| sprint | Shift | 移动时按住冲刺；同帧重击修饰只付重击费 |
| jump | Space | 普通落地起跳，5耐力；不绕过战斗Busy与生命状态 |
| interact | E | 扶起、搬尸、回收箭、工作台、任务／站点、采集按D1优先 |
| traversal.vault | E | 低障攀越，显式提示选中后消费，无其他有效E对象 |
| combat.attack | 左键 | 近战轻击；远程状态由武器解释，不自动锁定命中 |
| combat.heavyModifier | Shift＋左键 | 近战重击；射箭不切近战，Shift是可映射修饰动作 |
| combat.guardAim | 右键 | 合法盾组合格挡，远程瞄准；弓松开取消未释放箭 |
| combat.bowRelease | 松开左键 | 瞄准时结束真实蓄力并发射；费用沿045 |
| combat.execute | F | 有效潜入目标统一3秒处决；HUD显示批准后的准确语义 |
| combat.stun | R | 同045／043等效击杀；有效潜入目标优先于仓储／装填 |
| storage.open | R | 可访问仓储且没有有效R处决对象 |
| combat.reload | R | 没有更高R动作且已装备可装填弩 |
| combat.dodge | Alt＋移动方向 | 闪避及后摇取消沿045，不添加新无敌时长 |
| combat.lock | 鼠标中键 | 锁最近合法活敌／解除，距离遮挡沿045 |
| combat.sense | V | 感应沿045，不永久写地图、不向弟弟泄露 |
| combat.throw | G，副键4 | 使用当前有效快捷投掷物；两个入口不能重复扣物 |
| survival.medicine | 1 | 真实用药，菜单快捷使用返回HUD计时 |
| survival.food | 2 | 实际消耗食物，不为设置自动生成补给 |
| body.drag/carry | E／Shift＋E | 选中尸体拖／背；同一输入只启动一种，费用沿045 |
| body.drop | E | 正在搬运时放下，不再采集前方对象 |
| companion.dialogue | T | 30m内交流；打开后世界继续，按T返回需输入框没有焦点 |
| companion.wait/follow/attack | Z/X/C | 原执行器的等待／跟随／协助进攻 |
| ui.inventory | Tab | 打开／关闭背包，服从普通菜单暂停设置 |
| ui.map/skills/journal | M/K/J | 打开地图／技能／任务日志，页面间只切换一次 |
| ui.save | F6 | 安全存档页，确认／捕获输入优先消费；保存禁用仍沿044 |
| ui.pause/back | Esc；PIE P | Esc先取消当前钓鱼／建筑预览，否则返回页／真暂停；PIE不覆盖编辑器Esc停止Play |
| building.catalogue | B | 建筑目录与营地管理按钮入口 |
| building.rotate | Q | 仅预览时旋转15°；世界Q不触发旧重击 |
| building.confirm/cancel | 左键／右键 | 仅预览生效，消费后不攻击或瞄准；Esc亦取消 |
| fishing.hold/release | 按住／松开左键 | 真实钓鱼张力；E发起交互、Esc取消，费用与失败沿048 |
| inventory.use/drop/repair | F/R/H | 当前背包物品使用／确认丢弃／跳转工作台维修 |
| crafting.commit/repair.commit | F | 当前页面有效配方或装备实例，一次真实提交 |
| storage.transfer | E | 当前仓储页面数量确认与真实转移 |
| skills.learn | F | 当前技能学习，不消费世界处决 |
| journal.locate/track | F/V | 当前任务定位地图／追踪，消费后不处决／感应 |
| map.marker/zoom/pan | 右键／滚轮／方向键 | 地图标记、缩放和平移；只写已允许的地图状态 |
| settings.defaults | R | 填入默认草稿，仍需应用；不进行世界动作 |
| ui.navigate/activate | 方向键／Enter | 当前菜单有效焦点，标题首次不自动选中危险项 |
| dialogue.send | Enter | 中文输入法完成选词后发送一次；正在选词的Enter不得提交 |
| quantity.decrease/increase | 页面−／+按钮 | 仓储、制作、设施等共用实际数量控件，键盘焦点可达 |
| development.layout | F10 | 仅Development，Ctrl＋S保存布局、Ctrl＋Z撤销；不进入Shipping玩家映射 |

页面内未列明的营地发展、分工、规则撤销、任务卡确认、设备换装、收获、种养等动作沿现有按钮入口，须可用方向键／Enter访问。统一键盘导航不意味着添加批量批准捷径；取消所有任务／约定和删除／载入存档依旧逐项确认。

重绑定动作冲突按可同时使用的上下文检查。副键4和G是同一动作，不能把它们当两个世界写入口。用户重绑处决／仓储／装填之后，若仍同键保留R优先语义；分成不同键则各自只触发对应动作。可重绑的UI动作与世界动作也须按当前页消费，不能只检查世界表。

事实来源：[Character](../../../Source/Hearthward/HearthwardCharacter.cpp)、[HUD](../../../Source/Hearthward/UI/HearthwardHUDDialogue.cpp)、[页面键盘](../../../Source/Hearthward/UI/HearthwardScreenWidget.cpp)、[设置](../../../Source/Hearthward/UI/HearthwardScreenSettings.cpp)、[钓鱼](../../../Source/Hearthward/Nature/HearthwardNatureFishing.cpp)。本表包含正式候选输入规范，并不宣称当前源码已完成全部焦点／重绑定行为。
