# TASK-098｜设施、图标与实际岗位技术调查

## 2026-10-07 已批准方向后的进展

方向已由DSGN-004批准。三个新设施已取得12个精确包锁、通过UEClient导入并绑定gameplay；UE实测高88/96.75/86cm，底部与原放置占地匹配。真实PIE工作台/仓储入口支付72木和12木4石、生成新网格、登记设施及储物组件均通过，原成本/碰撞/事务规则不变。四类SVG/PNG武器图标已接入，10个刃/矛ID已分流。锻造实付建造、DPI/缺图、存读档/Cook和Owner视觉尚未验。见samples/paid-placement-pie.json、ue-stage.json。

以下为本单此前调查与验证记录；旧“待批准方向/暂缓制作/未改Content”的描述以本节更新为准，旧测试只保留其原版本信用。

状态：Active，2026-10-07。已实施设施/碰撞/岗位配置和 UI 文件/UV 检查；设施与图标新样板待 Owner 审定。工作树 `G:/GameFactory/Hearthward`、分支 `codex/TASK-084-103-iteration`、参考 HEAD `6fcf5c22e965f0f7409438f19bc7b09e96ffb058`，含共享未提交改动。未提交、未推送、未发布；最终运行版本由主 Agent 绑定。

## 已执行与证据

运行 `../.venv/Scripts/python.exe -X utf8 docs/qa/TASK-095/audit_bindings.py`，生成 [TECHNICAL_AUDIT.json](TECHNICAL_AUDIT.json)，未改 gameplay/interface/provenance 或任何二进制。

|真实设施ID|当前可见源|原根碰撞半尺寸cm|岗位/事务结果|
|---|---|---|---|
|workbench、forge、warehouse_access|028 wood_table_model|70×76×36|实际岗位/交互运行 NOT_RUN|
|campfire、smelter、cooking|028 campfire_model|54×56×27|实际岗位/交互运行 NOT_RUN|
|bed、medical_area|028 rope_wood_bed_model|105×57×18|实际岗位/交互运行 NOT_RUN|

- 已记录8个真实设施Kind ID和每一份 part 的源/位置/旋转/缩放。建筑实例ID由原事务产生，不能新增第二套。原生 root box 承担实体碰撞；每个可见 StaticMesh part 设置 NoCollision 且不影响导航。模型替换不应擅自改变实际占地或交互距离。
- CampaignWorld 的 PresentLabor 读取真实 Region、Enabled、有效工作场所、Batch.Active 和 Work<Required。该呈现入口没有增产或推进日历逻辑；岗位与结果验证继续复用059/062。
- interface 中187个图片定义：文件均存在、PNG头尺寸可读、所有给定UV矩形均在尺寸内；`static_file_uv_errors=[]`。gameplay 中已有物品 icon 键都能在 interface 找到，缺键0。
- 13个 weapon ID 仍共用 axe 图标，这是语义区分缺口；键和裁切范围有效不能证明每个物品图示已正确，也不能证明小尺寸、DPI、透明边缘或缺图 fallback 已通过。
- [094准确候选包](../../assets/TASK-094/TASK-098-PACKAGES.json)含98个 Content 候选，`selected_for_change=[]`、锁未取得。028家具归098唯一写者，096为消费方；当前未修改这些共享资产。

## 定向验证与验收前置

复用3条已注册测试，执行权归主 Agent，现本轮结果已附下节：

- `Hearthward.Camp059.WorkerAssignmentPresentation`
- `Hearthward.Farming062.CropProgressAndHarvestCapacity`
- `Hearthward.Farming062.PenProductiveAnimalAndPausePresentation`

它们保护实际岗位呈现、采收容量和生产/暂停状态；不代替真实设施模型、鼠标交互、UI图示或两营地读档。未新增复制配置字段的测试。

|验收项|本轮结果|已执行静态子检查及剩余前置|
|---|---|---|
|T098-C01 用途辨识|NOT_RUN|8设施复用外形已登记；需批准设施首件样板及正常场景识别。|
|T098-C02 实际交互|NOT_RUN|根碰撞/可见part职责已核对；需真实建造、岗位、睡眠、医护、储物等交互。|
|T098-C03 升级外观|NOT_RUN|原Kind/实例/岗位状态沿用；需真实阶段/升级/第二营地对照。|
|T098-C04 图标映射|NOT_RUN|文件/UV/键静态检查 PASS；13武器共用axe需新图示样板，缺图fallback运行未执行。|
|T098-C05 UI缩放|NOT_RUN|PNG头和UV检查 PASS；需实际DPI/大字模式/小图边缘/透明裁切与信息占位。|
|T098-C06 保存Cook|NOT_RUN|源/候选包已登记；需建筑材料/布局/两营地恢复、UE依赖闭合及独立Cook。|

## 2026-10-07 实际定向原生结果

主 Agent 经公开UEClient构建实际成功后执行；[本单原始测试条目](NATIVE_REUSE.json)保留每条 entries、warnings、errors及设备，源报告为 `.agent-local/qa/TASK-085-099/native-first-20261007/index.json`（原报告时间 `2026.10.06-20.35.14`）。受测源码为参考HEAD上的共享未提交改动；此轮在092等待修复之前。范围 3/3 Success，warning 3，error 0。上述静态盘点生成时尚无运行结果，现由本节补充。

|实际过滤器|状态|Warnings|Errors|
|---|---|---|---|
|`Hearthward.Camp059.WorkerAssignmentPresentation`|Success|1|0|
|`Hearthward.Farming062.CropProgressAndHarvestCapacity`|Success|1|0|
|`Hearthward.Farming062.PenProductiveAnimalAndPausePresentation`|Success|1|0|

- `Hearthward.Camp059.WorkerAssignmentPresentation`：LogEnhancedInput: UEnhancedInputLocalPlayerSubsystem for Local Player 'LocalPlayer_0' does not have a valid PlayerInput object. Failed to load user settings.
- `Hearthward.Farming062.CropProgressAndHarvestCapacity`：LogEnhancedInput: UEnhancedInputLocalPlayerSubsystem for Local Player 'LocalPlayer_1' does not have a valid PlayerInput object. Failed to load user settings.
- `Hearthward.Farming062.PenProductiveAnimalAndPausePresentation`：LogEnhancedInput: UEnhancedInputLocalPlayerSubsystem for Local Player 'LocalPlayer_3' does not have a valid PlayerInput object. Failed to load user settings.

警告原文保留，未把有警告的Success写为零警告；只按测试结果登记，不据警告推断正式输入或环境原因。这些是隔离原生行为证据，完整Cxx场景、正式地形、渲染/正常输入、Cook及Owner视觉仍为NOT_RUN。静态脚本编译、分单JSON/空选包/过滤器注册核对和定向 `git diff --check` 亦通过。

## 具体暂缓点与继续路径

Owner 待审的是工作台/锻造/熔炼/医疗/储物等设施用途可辨的首件外形，以及石斧/刃/矛/钝器等关键物品图示样板和制作来源。暂缓新风格制作与替换，现有玩法、价格、占地、奖励和产出继续沿用。

仍可执行3条真实状态原生测试、187图示的渲染/大字检查、实际设施岗位操作及存档/Cook验证；不存在新增设计决定的部分由主 Agent继续运行。094来源许可未解决项和包锁属于单列工程前置。本单未编辑 BuildingComponent、CampaignWorld、WorldPresentation、Content 或界面运行配置。
