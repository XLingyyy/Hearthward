# TASK-097｜自然路线、生态与LOD技术调查

2026-10-08 两个草种资产的接触阴影修复已通过重新启动后的渲染验证：草片下方矩形黑斑消失，建筑/树阴影保留，全局接触阴影仍开启。修改仅为GT_Meadow与GT_S1_Meadow的CastContactShadow=false。首次保存跳过未标脏资产的失败已保留，显式保存后重启读取/实际组件均为false。见[草簇修复](grass-contact/REPORT.md)。本次为PIE局部画面验证，候选8尚未包含这次修复，完整自然路线/性能/Owner验收仍未完成。

## 2026-10-07 已批准方向后的进展

方向已由DSGN-004批准。野猪基于既有R3猪源延长吻部、添加同骨骼獠牙/鬃毛并用材质调色；原骨架层级不变。三种作物各有幼株/成熟两形态，共六份FBX制作源。六个作物网格及材料共20包已锁定、导入、显式保存并在重启前确认落盘；NatureActor按现有权威日历选择幼株/成熟网格与缩放，成长时间、产量和采收事务不变。Editor构建成功；CropGeometryConsumesCalendarStage与CropProgressAndHarvestCapacity实际2/2 Success、1 warning、0 error，证据见docs/qa/TASK-097/samples/crop-native-20261007.json。警告为测试LocalPlayer缺少PlayerInput，未隐去。已补充作物目录的AlwaysCook配置，实际Cook仍NOT_RUN。野猪新网格及烘焙毛色、三材质共五包已锁定并导入，复用原50骨R3 Skeleton与PhysicsAsset，原Skeleton未标脏且Git未变化；家猪保持原网格，野猪继续使用原27动作/行为参数，活动范围仍按原网格计算。Editor编译成功；BoarKeepsR3AndPig原生测试通过（0警告/0错误），核对家猪/野猪网格区分、共享物理资产与动作、活动边界，并比较静候/奔跑/顶撞/倒地四个真实动作中间帧的逐骨姿态。首轮夹具重复初始化World失败保留，修正为CreateWorld单次初始化后通过。连续自然路线、完整Cook及Owner视觉仍未验，原Tripo输入权利状态仍PENDING。见samples/boar-source.json及crop-source.json。

以下为本单此前调查与验证记录；旧“待批准方向/暂缓制作/未改Content”的描述以本节更新为准，旧测试只保留其原版本信用。

状态：Active，2026-10-07。已完成无设计依赖的自然资产引用、动物配置与来源边界调查；新野猪/作物/植被样板的 Owner 审核暂缓。工作树 `G:/GameFactory/Hearthward`、分支 `codex/TASK-084-103-iteration`、参考 HEAD `6fcf5c22e965f0f7409438f19bc7b09e96ffb058`，含共享未提交改动。本单未提交、未推送、未发布；主 Agent 后续运行报告绑定最终受测版本。

## 已执行与证据

运行 `../.venv/Scripts/python.exe -X utf8 docs/qa/TASK-095/audit_bindings.py`，生成 [TECHNICAL_AUDIT.json](TECHNICAL_AUDIT.json)。核对实际 animal_motion、NatureActor、AnimalMotionComponent、原生作物几何和094来源登记。

- 当前14个 R3 物种配置共有303段片段，全部采用既有 mesh/skeleton 路径。AnimalMotionComponent 实际加载时比较动画与网格 Skeleton；当前本单只核对源码和配置，UE 配对结果仍为 NOT_RUN。默认保留 R3/303，不重制动画或改速度/行为规则。
- 真实别名为 `deer→stag_a`、`boar→pig`。当前野猪复用猪源；外形合适性需单独 Owner 样板，不能只凭可加载判为物种表达通过。
- 作物外观读取现有成长/成熟状态生成茎叶形状；原几何测试可验证随权威日历变化。物种专用形态和成熟阶段辨识仍需批准样板及实机，不能变更成长、掉落、刷新和碰撞范围。
- [094准确候选包](../../assets/TASK-094/TASK-097-PACKAGES.json)共539个，`selected_for_change=[]`、锁未取得。NaturalWorld/Rebuild 中124条仅配对到制作源家族；对应包未逐一确认来源、许可或制作参数，未升级成 EXACT。PolyHaven CC0 仅作用于已匹配源，其他 UNKNOWN 如实保留。
- NatureActor 的动物根碰撞与 R3 可见网格职责不同；动物网格不影响导航，资源/作物原交互沿稳定ID。正式装饰避让依据084/092路线，091未重排，当前不冻结自然地形 Z 或净空。

## 定向验证与验收前置

复用下列3条已注册用例，执行权归主 Agent，现本轮结果已附下节：

- `Hearthward.Animals.FrameBoundaryAndEscape`
- `Hearthward.Farming062.IndividualGrowthAndProductProgress`
- `Hearthward.Farming062.CropGeometryConsumesCalendarStage`

它们分别验证动物边界/逃离、个体成长/产品进度、原作物几何读取日历状态。原生夹具结果不替代 R3 连续动画、正式物种辨识、地标可读性或性能。

|验收项|本轮结果|已有静态证据及剩余前置|
|---|---|---|
|T097-C01 路线辨识|NOT_RUN|084既有路线已采用；需连续救援往返、近远景/昼夜地标实机。|
|T097-C02 交互通行|NOT_RUN|装饰/动物网格导航职责已核对；需092真实局部净空和资源/动物交互回归。|
|T097-C03 自然阶段|NOT_RUN|14物种/303片段、野猪别名、作物权威状态已登记；需批准外形与成熟阶段样板。|
|T097-C04 LOD材质|NOT_RUN|只读包候选和来源已登记；需 UE 查询实际LOD/Nanite/材质以及固定路线连续远近景。|
|T097-C05 性能对照|NOT_RUN|需固定场景、配置和输入的原始帧时/资源/Streaming记录；未采集或宣称性能达标。|
|T097-C06 持久恢复|NOT_RUN|源码读取既有状态，本单无资源/刷新规则修改；需采集、动物状态读档和不重复生成验证。|
|T097-C07 Cook来源|NOT_RUN|制作源/许可 UNKNOWN 与家族配对已细分；需准确配对、依赖闭合及 Cook 运行。|

## 2026-10-07 实际定向原生结果

主 Agent 经公开UEClient构建实际成功后执行；[本单原始测试条目](NATIVE_REUSE.json)保留每条 entries、warnings、errors及设备，源报告为 `.agent-local/qa/TASK-085-099/native-first-20261007/index.json`（原报告时间 `2026.10.06-20.35.14`）。受测源码为参考HEAD上的共享未提交改动；此轮在092等待修复之前。范围 3/3 Success，warning 1，error 0。上述静态盘点生成时尚无运行结果，现由本节补充。

|实际过滤器|状态|Warnings|Errors|
|---|---|---|---|
|`Hearthward.Animals.FrameBoundaryAndEscape`|Success|0|0|
|`Hearthward.Farming062.CropGeometryConsumesCalendarStage`|Success|0|0|
|`Hearthward.Farming062.IndividualGrowthAndProductProgress`|Success|1|0|

- `Hearthward.Farming062.IndividualGrowthAndProductProgress`：LogEnhancedInput: UEnhancedInputLocalPlayerSubsystem for Local Player 'LocalPlayer_2' does not have a valid PlayerInput object. Failed to load user settings.

警告原文保留，未把有警告的Success写为零警告；只按测试结果登记，不据警告推断正式输入或环境原因。这些是隔离原生行为证据，完整Cxx场景、正式地形、渲染/正常输入、Cook及Owner视觉仍为NOT_RUN。静态脚本编译、分单JSON/空选包/过滤器注册核对和定向 `git diff --check` 亦通过。

## 具体暂缓点与继续路径

Owner 待审的是野猪是否采用既有猪源的明确衍生方案、首线作物成长/成熟形态、自然环境首件样板。沿既定北方山林方向，不自行改变物种ID或玩法。二进制包锁、来源配对未闭合、真实净空和性能采集属于工程前置，分别处理。

可继续复用3条测试，进行现有 R3 骨架/LOD/材质只读查询和固定路线性能采集。094候选里共享 `M_RockScan` 的唯一写者为097，096仅消费；本单尚未取得写锁，未改 Content、animal_motion 或 WorldPresentation，也未新生成风格资产。
