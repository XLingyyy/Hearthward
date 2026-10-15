# 共享写入与资产窗口

## 一次只允许一个写者

|共享对象|协调责任|消费者|
|---|---|---|
|存档格式、epoch、迁移|TASK-131唯一提供者|环境/合作/人物/天气/踪迹/陈列|
|gameplay.json、任务/实例目录|TASK-112指定整合者逐单导入|113/114/116/117/121—125/128—130|
|experience.json、全局混音|TASK-127串行整合|115/116/119/120/128|
|公共战斗入口|TASK-113—117按依赖顺序与精确文件交接|121/122/128|
|伙伴/通用交互入口|TASK-119唯一协调；126提供对齐|118/120/123—125/130|
|公共UI、输入、README|当次集成单一写者|各单只提交范围明确的请求|
|关卡/ExternalActors与UE包|121作坊、122粮窖各自唯一写者|其他单先交源码/制作源/数据请求|

初次激活时从任务拟定目录缩小到准确文件。本文定义职责，不表示已经指派真人/Agent或取得资产锁。未知改动保持不动，不批量清理历史QA。

## 2026-10-13 本地实施窗口

Owner=XLingyyy；唯一写者=当前Codex主会话；一次只激活一个生产写窗口。112正在进行文档登记，后续窗口按依赖串行。以下均为真实文件路径；未创建的专用文件明确标为拟新增。

|顺序/任务|精确共享文件|当前窗口|
|---|---|---|
|112|`README.md`、`docs/START_HERE.md`、`docs/PROJECT_STATE.md`；112—133任务/交接导入及本轮planning|OPEN，仅文档|
|131|`Source/Hearthward/Save/HearthwardSaveGame.h`、`.cpp`，`HearthwardSaveSubsystem.h`、`.cpp`，`HearthwardSaveCompatibility.cpp`|OPEN_TASK131；按实际需要启用，禁止整目录重构|
|131|拟新增 `Source/Hearthward/Interaction/HearthwardPersistentInteractionState.h`、`.cpp`；`Source/Hearthward/Tests/Round2PersistenceTests.cpp`|OPEN_TASK131|
|132|既有 `scripts/qa/run_engine_gate.py`；拟新增 `scripts/qa/round2/` 及 `scripts/tests/test_round2_metrics.py`|WAIT_112；复用原测量解析，不另造统计口径|
|126|拟新增 `Source/Hearthward/Interaction/HearthwardInteractionAlignment.h`、`.cpp`；`Source/Hearthward/Animation/HearthwardHeroAnimInstance.cpp`|WAIT_C_COOP；其他角色动画文件按首件实际需要登记|
|113→114→116→117|`Source/Hearthward/Combat/HearthwardCombatRules.h`、`HearthwardCombatComponent.h`、`.cpp`|SERIAL；每单结束并核对diff后转移|
|113→114→116→121→117→122→123→124→125→128→129→130|`Resources/Data/gameplay.json`|QUEUE；只导入当前单的已批准条目|
|127|`Resources/Data/experience.json`|WAIT_112；115/116/119/120/128只提交事件接入请求|
|119→120|`Source/Hearthward/Companion/HearthwardCompanionFixture.h`、`.cpp`|WAIT_131_126；共享调度器保持唯一|
|121→122|`Content/Hearthward/World/Natural/Rebuild/L_HearthwardWilds.umap` 及其确需修改的ExternalActors|CLOSED；两地点属于同一地图，必须串行，逐包登记并核验LFS锁后才开放|

写者窗口是本轮串行协作约定，不声称存在自动锁服务。模拟：113持有gameplay.json时114的请求排队；113交接并释放后才轮到114。同理121持有主地图时122等待；Content当前总禁写，双方均不能取得资产写窗口。正式内容和测试夹具不共用玩家档池。

制作路径优先 `art_source/TASK-126/`、`art_source/TASK-119/`、`art_source/TASK-120/`，通过Blender MCP制作首件；确切输出文件、UE包、许可和预算在首件选定后登记。现阶段新增运行资产清单为空，**不是已完成资产交付**；不凭空预填锁ID、作者或许可。

## 资产登记最小字段

AssetID、实际Content包、可编辑制作源、来源/作者/许可、引用地点、碰撞/导航影响、单写者任务、LFS锁ID/Owner/核对时间、引擎保存状态、视觉首件批准、LOD/贴图/声音预算和Cook证据。缺实测的栏位只能写未核对，不能写已锁定或合规。

所有任务当前forbidden_paths含Content/。登记准确包并获允许后，更新任务范围，删除总Content禁写项并以更精确禁止项保护其余资产；禁止只加allowed路径却遗留上级禁写而产生矛盾。

## 回退

新内容隐藏/未注册时不得破坏旧存档；状态版本迁移失败保留原件。回退源码不能强制旧版读取新版玩家档。范围化revert需当前授权，不使用reset --hard/clean/强推或批量解锁。

2026-10-13 Owner明确回复“采用，继续实施首批”，批准D112-01至D112-06作为首件实施基线；实际试玩与真人验收另行记录。
112文档用例通过，保持Active待独立审查；131生产窗口开启，其他生产写窗口尚未启用。

131编译等待期间，132仅开启scripts/qa/round2与scripts/tests/test_round2_metrics.py写窗口；共享C++和配置不变。

131首批原生与独立恢复通过后暂停生产写窗口；113打开CombatRules.h、CombatComponent.h/.cpp、HearthwardCharacter.cpp及Round2WeaponChoicesTests.cpp。范围增加仅为必要公开方法声明。

113原生测试通过后暂停写窗口；131恢复窗口处理实际旧档启动装备投影回归，增加GameplayComponent.cpp单写者。

2026-10-14：按Owner全量连续实施指令，114持有CampaignActor及BrotherAnimInstance窗口；实际新包编辑仍逐包登记。人工签收留到最终，不伪造门禁通过。

114本次Editor构建完成后，116接管CampaignActor窗口并新增NoiseInteraction；两者在下次联合构建统一验证。

2026-10-14 集成窗口：唯一写者仍为当前主会话；TASK-132 复用并扩展 docs/qa/TASK-102/run_standalone_joint.py，仅增加只读检查点输入、隔离输出目录和实际世界时钟记录。TASK-133 已激活集成验证。TASK-120 增补 ShortcutActor 的确定性移梁、承托区扫掠、取消复位；与 TASK-126/UI 近距离标签修复一起编译验证。

### 2026-10-14 实景碰撞修复

单写者 root；TASK-118/121/122/126/133。实际渲染夹具 scenes-01/02 复现斜坡站位压入 Landscape、粮窖与故乡村屋实心体重叠。新增精确窗口 Source/Hearthward/Building/HearthwardHometownFortress.cpp，仅放开占领阶段粮窖所处村屋的底层；序章保持原碰撞。机关接触点独立投影地面，保留原18厘米对齐限制和碰撞扫掠。工坊现有 home_workshops_16 接入带台阶的木台；身份、奖励和兵力不变。结果待修复后的实景复测。


2026-10-14 TASK-132 诊断窗口：CombatComponent、NoiseInteraction、InteractionAlignment、AnimalTraces、PresentationComponent 五个现有入口串行加入固定名称 CSV 计数。仅非 Shipping 编译，复用 UE CsvProfiler；无自由文本、文件队列或额外线程。Editor诊断构建通过，实际采集验证中。

2026-10-15：唯一写者 root。TASK-131 持有具名状态定义/校验及 Round2PersistenceTests，修复陈列归属并增加正式ID跨进程矩阵；TASK-116 复用 CampaignAIBehaviorTests 的导航夹具；TASK-123—125 补充 Round2PersonalStoryTests。TASK-132 在 CombatComponent 增加非Shipping目标查询/感知耗时，采样脚本增加生产交互的采集、营火建造/等待与结束后Widget渲染。每次编译和性能采集串行执行。

2026-10-15：TASK-132记录 Source/Hearthward/Gameplay/HearthwardActorQuery.h 的单写窗口，用于两处组件遍历共用UE默认活动关卡判定，保持已有过滤语义；先用隐藏/移除/关联/解关联关卡夹具复现，再修正Combat与AnimalMotion既有文件。不改模型、画质或玩家规则。

2026-10-15 TASK-117由同一主会话串行扩展非Shipping实景检查入口 HearthwardScreenRound2SceneTest.inl：正式两关卡入口灯走既有CompleteInteraction，随后独立进程Continue核对四灯状态、可见性、检测与组件数量。保留预清敌/角色布置夹具标识，不修改生产灯规则，不计正常路线或实听。

2026-10-15 TASK-115/127主会话串行补充 CombatAudioFeedbackTests.cpp 边界用例：真实表现消费者不得派发敌人调查、伤害或库存事务；真实命中反馈不改变世界/角色时钟倍率。无生产代码改动；脚步回执为测试输入，不计动画接触或真人听感验收。

2026-10-15 TASK-117实景复现地窖入口灯被第二级石阶包围（灯98050,57880,23407；石阶X98020—98084/Y57760—57940/Z23354—23442）。主会话在CampaignRound2.cpp将该灯局部Y从-620移至-420，移到阶梯旁；身份、状态、灯强/范围与检测规则不变。候选03因此成为修复前版本，待实景恢复通过后重建候选04。

2026-10-15 TASK-128：主会话复用非Shipping实景夹具，增加独立Continue的天气种子/日历恢复及真实暂停/恢复检查；不改变天气规则与Shipping实现。

2026-10-15 TASK-127/128：同一主会话复用EnvironmentAudioLifecycleTests.cpp，使用明确非Shipping时间夹具进入雨天，再验证真实音频组件暂停、连续LoadPoint清理与单实例恢复；不计自然天气等待、睡眠或实际听感。

2026-10-15 TASK-123/124/125：同一主会话复用EnvironmentAudioLifecycleTests.cpp已有落地角色/存档夹具，补三人两分支实际SavePoint/LoadPoint及对白回退；现场完成/胜利仍明确为夹具前置，不计正常路线或跨进程。

2026-10-15 TASK-126复用EnvironmentAudioLifecycleTests.cpp验证三武器真实灯具交互后的实例存读档；TASK-129复用非Shipping实景读档入口核对原保存踪迹的独立进程/连续载入，均由当前主会话串行执行。

2026-10-15 TASK-119/120：同一主会话在原实景夹具检查活动移梁、绞盘接近及维持时Capture按原规则拒绝保存；额外保存合法操作前/取消后节点并以UI选择特定节点独立恢复。未改变危险保存策略。

2026-10-15 TASK-123/124/130同一主会话复用已有三人两分支存档夹具，增加六个正式陈列ID的真实Mesh组件，核对选择前隐藏、选择后对应资产恢复；不计实景路径/营地三档外观验收。

2026-10-15：唯一写者root；TASK-129复用 EnvironmentAudioLifecycleTests.cpp 的既有真实存档世界夹具，补死亡/拾取/重载数量守恒。TASK-132复用run_standalone_joint.py，经营场景按既有MenuPause=false与真实岗位生产验证，不修改游戏暂停或请求取消策略。

2026-10-15：TASK-122复用 EnvironmentAudioLifecycleTests.cpp 验证原粮窖物资实际满背包拒绝、有限领取、连续LoadPoint和回档守恒；唯一写者root。

2026-10-15：TASK-132登记下一串行窗口：HearthwardNPCContextProjection.cpp 与 LocalAIContextProjectionTests.cpp。当前CPU完整暖窗口第4→5→6请求仅navigation_rebuilding翻转，发生约516 token重预填充及35秒延迟。采样完成后才修改JSON字段顺序，将易变观察置后，保留全部字段/值/层级和模型预算；以定向投影回归与真实请求验证。唯一写者root。

2026-10-15 TASK-114：主会话串行修复实景确认的弟弟战役守卫协助缺口，窗口为Gameplay/HearthwardGameplayComponent.cpp及Tests/EnvironmentAudioLifecycleTests.cpp；复用已交战目标和既有距离策略，不增加全图敌情。
