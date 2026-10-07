# Hearthward｜TASK-084—103 Agent任务单合订版

2026-10-06 · 20份任务 · 142条细化验收用例 · 规划基线 `6fcf5c22e965f0f7409438f19bc7b09e96ffb058`

本文件供连续阅读；Agent按`docs/tasks/`中的单任务MD+JSON执行，权限/路径以Owner实际批准快照为准。所有任务Backlog，所有运行结果NOT_RUN；未提交/未推送/未发布。

[任务索引](docs/planning/TASK-084-103/README.md) · [执行约定](docs/planning/TASK-084-103/EXECUTION_GUIDE.md) · [依赖与写入调度](docs/planning/TASK-084-103/DISPATCH.md) · [共享契约草案](docs/contracts/CT-TASK-085-presentation-readmodels.md)

## 目录

- [TASK-084｜连续体验基线、断点复现与本轮范围冻结](#task-084)
- [TASK-085｜共享显示契约、动作语义与UI增量接入基础](#task-085)
- [TASK-086｜弟弟工作状态卡、停工原因和恢复入口](#task-086)
- [TASK-087｜自然语言委托可靠性、取消回退与真实模型全矩阵](#task-087)
- [TASK-088｜制作配方查找、可制作筛选与缺料追踪](#task-088)
- [TASK-089｜背包与仓储操作一致性、实例详情和可读性](#task-089)
- [TASK-090｜HUD与日志目标层级、首次准备引导](#task-090)
- [TASK-091｜地图可读性与关键入口转折引导](#task-091)
- [TASK-092｜首次救援往返路线与遭遇灰盒打磨](#task-092)
- [TASK-093｜营地准备、回营成长反馈与岗位可视状态](#task-093)
- [TASK-094｜关键资产盘点、风格样板与逐包制作清单](#task-094)
- [TASK-095｜兄弟与敌人辨识、武器持握和关键动作收尾](#task-095)
- [TASK-096｜石堡开场必经区域、近景材质与夜袭氛围](#task-096)
- [TASK-097｜救援路线自然环境、生态外观与远近景收尾](#task-097)
- [TASK-098｜营地核心设施、道具与关键图标统一](#task-098)
- [TASK-099｜动作音效、环境声与事件反馈同步](#task-099)
- [TASK-100｜四区差异化内容与首版全流程接续](#task-100)
- [TASK-101｜存读档、时间线和跨页面集成回归](#task-101)
- [TASK-102｜游戏与本地模型联合性能、内存及运行优化](#task-102)
- [TASK-103｜候选包、真人验收与有条件发行交接](#task-103)


---

<a id="task-084"></a>

# TASK-084｜连续体验基线、断点复现与本轮范围冻结

> 状态：Backlog（任务单已编写，实施未派发）。优先级：P0。阶段：A 基线。日期：2026-10-06。Owner：XLingyyy。Reviewer／Issue：未指派。建议分支：`codex/TASK-084-playable-baseline`（未创建）。

[本批总入口](docs/planning/TASK-084-103/README.md) · [执行约定](docs/planning/TASK-084-103/EXECUTION_GUIDE.md) · [元数据](docs/tasks/TASK-084.json) · [交接模板](docs/handoffs/TASK-084.md)

## 1. 目标与预期结果

从当前正式入口取得一条连续游玩基线，确定后续所有任务共同使用的救援往返路线、真实稳定ID和可复现缺陷；本单调查，不顺手改玩法。

## 2. 当前基础与事实边界

读取时 main 为 6fcf5c22e965f0f7409438f19bc7b09e96ffb058；当前预览版为 0.2.0-preview.20261006.2。已有独立包烟雾检查不等于完整救援切片或四区通关。TASK-083 保持 Active 不能解释为发行未完成；用实际发行证据判断。

本单参考基线为 `main@6fcf5c22e965f0f7409438f19bc7b09e96ffb058`；实际开工必须复核届时HEAD与依赖产物。这里不是本轮运行结果，旧报告PASS/FAIL均绑定原受测实现。新行为是本单计划，不能写成当前已经存在。

## 3. 依赖、开工门槛与最小必读

前置：无本批前置；先核实当前main/现有发行。

激活084仅授权调查与测试资料，不自动授权修改运行时。依赖的旧任务按已集成SHA读取，不要求把旧单人工验收伪填Done。

共同必读：`AGENTS.md`、`WORKFLOW.md`、`docs/START_HERE.md`、`docs/PROJECT_STATE.md`、`docs/design/CURRENT.md`及本单MD/JSON，然后执行本批指南中的接手/路径核对。

本单额外入口（路径为只读定位，不等于修改授权；前置任务产物需先实际生成）：

- `docs/design/DSGN-003-first-release-slice.md`
- `docs/qa/TASK-077/REPORT.md`
- `docs/qa/TASK-081/REPORT.md`
- `Resources/Data/gameplay.json`
- `Resources/Data/experience.json`

## 4. 修改范围与协作边界

除本单MD/JSON、handoff、QA目录、专用规划目录和根`README.md`固定收尾外，候选范围如下；最终以**已获准基线JSON**为准。

- `docs/planning/TASK-084-103/BASELINE.md`
- `docs/planning/TASK-084-103/ROUTE_MANIFEST.json`
- `docs/planning/TASK-084-103/DEFECTS.csv`
- `docs/planning/TASK-084-103/DISPATCH.md`

Content与Save等未授权领域只读；例外仅以JSON实际范围为准，不能用必读清单扩大写权限。

拟新增辅助/测试文件不表示仓库已存在；先搜索可复用类型。共享UI配置、公共头文件、WorldPresentation/CampaignWorld等按[写入调度](docs/planning/TASK-084-103/DISPATCH.md)串行。临时输出使用 `.agent-local/qa/TASK-084/<唯一run>/`，不得写入用户原档。

## 5. Agent具体实施步骤

### 1. 固定受测对象

记录仓库根、完整 HEAD、分支、未提交清单、UE/编译器版本、模型锁文件、实际运行入口、包版本及源码关联。开发源码和预览包分开编号；无法证明对应时不混用结果。不要运行地图重建脚本。

### 2. 准备隔离测试档

使用 launch_test_client.py 已有 --fresh-profile 入口或发行报告同等隔离 UserDir；记录绝对目录，确认不是用户正式档。只创建当前任务的测试数据，先检查现有编辑器与 Live Coding 状态。

### 3. 走序章

正常标题页新游戏，按实际键鼠取遗物、叫弟弟跟随，经卧室—门口—回廊—楼梯—庭院—侧门撤离。连续记录迷路点、卡碰撞点、伙伴掉队、画面与输入。禁止逐点传送或直接切阶段。

### 4. 走营地救援切片

从夜袭后营地控制开始计时，完成一次真实分工、准备工具/补给、前往现有首救地点、侦察/处理既有遭遇、带回一人、工作台I与营地S2条件、保存退出并继续。沿正式配表，任务领取/提交按现行界面执行；未能到达的阶段记录 BLOCKED 和首个阻塞。

### 5. 冻结路线清单

从 gameplay.json 与 Campaign 的现有对象解析任务/地点/人/敌人/设施ID、入口出口、空间坐标与触发条件，写 ROUTE_MANIFEST.json；坐标必须在本机读出，不能照示例编造。各节点列既有来源、后续负责任务和验收场景；不另造一条平行主线。

### 6. 建立缺陷账本

DEFECTS.csv 每条含编号、受测SHA/包、初始档哈希、操作、期望、实际、复现次数/尝试数、截图/日志、严重性、所属新旧任务。无法重现写 NOT_REPRODUCED；历史失败只作回归线索。先分崩溃/坏档、流程阻断、误导/卡顿、视觉四级，不用数量代替影响。

### 7. 登记基线而非调优

在营地、序章、路线各录一段有场景与输入说明的帧时/内存及真实模型请求；仅作 102 的可比基线，不以三段采样宣布性能通过。对 UI 和关键美术保存昼夜、100%/150%字号参考。

### 8. 完成派单映射

将缺陷分派至 086—103，标注哪些已存在且只需复验。写出 G0 基线完成条件、共享写窗口及外部阻塞；完整实玩没有完成也可交付完整调查，但不得把被阻塞路线记 PASS。

## 6. 不可破坏的规则

- 本单不需要人为设定30—60分钟达标；时长仅记录，正式5名新玩家验收在103。
- 每个失败必须保留最短复现与证据；没有UE/实际输入能力就记录 NOT_RUN，不由源码推导实玩结果。
- 不修改正式存档、资源数值、奖励、地图和模型；新增只读辅助采样也先在本单允许范围内实现。

所有更改继续遵守只读显示、原事务执行、稳定ID、时间线隔离和不伪填验收规则。规则未明确的新玩法不硬编码；不删测试/吞错误/全仓重构来让当前检查通过。

## 7. 验收用例与执行方式

每条用例在隔离档/明确诊断夹具中建立前置，实际记录操作与断言。夹具、注入输入、真实OS键鼠、真实模型、真人样本分层统计；下面所有结果尚未运行。

|局部编号|场景|前置/具体操作|通过标准|初始结果|
|---|---|---|---|---|
|T084-C01|入口与版本|分别从开发入口和可获得的当前独立包启动；记录标题版本/加载地图/对应指纹。|版本关系可追溯；不可获得的包明确 NOT_RUN。|NOT_RUN|
|T084-C02|序章连续性|不使用控制台，依次完成取物、跟随、下楼和撤离。|完整路线录像或首个阻塞复现，阶段推进来自真实交互。|NOT_RUN|
|T084-C03|首次分工|在安全营地执行个人定量委托，另开独立路线验证队伍持续生产。|任务参数、实际交付、岗位产量各有前后快照，不混为同一任务。|NOT_RUN|
|T084-C04|救援成长|走到现有首救点，带回族人并完成批准的工作台I/S2流程。|人口20→21、奖励/扣料与等阶变化有真实证据；失败不补材料掩盖。|NOT_RUN|
|T084-C05|继续游戏|手动保存、自然退出、独立重启继续。|地点、任务、伙伴、库存、人口与营地阶段前后对照，差异逐项登记。|NOT_RUN|
|T084-C06|路线数据|将清单每个稳定ID反查配置/运行对象。|不含虚构坐标、未定义ID或未来故事泄漏；下游任务可直接定位。|NOT_RUN|
|T084-C07|调查交付|检查缺陷分类、责任任务、未运行项与历史引用。|下游Agent能只凭记录复现；不将调查完成写成游戏验收完成。|NOT_RUN|


全局挂接索引：T-002, T-007, T-011, T-021。它们来自现有TEST_MATRIX；正式数值以已批准增量/ACCEPTANCE和本单细化步骤为准，不恢复历史灰盒规则。

本单以调查/资料、实际运行或真人验证为主，不为凑测试数量编造原生用例；使用现有有效回归并注明执行层。

公共检查：`python -X utf8 scripts/validate_repo.py`、`python -X utf8 -m unittest discover -s scripts/tests -v`、`git diff --check`；在包含获批本单快照的实际基线上运行 `python -X utf8 scripts/validate_repo.py --task TASK-084 --base <实际完整SHA>`。UE操作/证据要求见执行约定；缺环境则明确NOT_RUN，不能将文件编写完成等同测试通过。

## 8. 交付物与完成定义

- BASELINE.md：版本/环境/现状/已复现与未复现问题
- ROUTE_MANIFEST.json：现有路线及对象清单
- DEFECTS.csv：可复现缺陷及负责任务
- REPORT.md、阶段计时CSV、必要截图/日志/录像索引

固定收尾：更新`README.md`当前说明、同名MD/JSON与`docs/handoffs/TASK-084.md`；在`docs/qa/TASK-084/REPORT.md`绑定最终完整SHA、资产/模型指纹、环境、命令、逐用例结果和明确限制。该REPORT为未来执行产物，本包不预填。

工程实现完成、Owner视觉、真人体验、二机、性能与发布各自登记。依赖下游可用产物写入handoff，不把未验项目涂为PASS。没有授权时交付本地改动和证据并写“未提交/未推送/未发布”，不得自增权限。

## 9. 本单明确不做

- 玩法代码修复和跨模块重构
- 扩大地图、新建任务奖励、修改资源/成长数值
- 补写过去任务的PASS或Owner验收

## 10. 失败、范围变化和恢复

首先保留最短复现、受测状态和原始证据，再定位到本单或其他负责任务；不能通过清空存档/赠送物资/瞬移/改验收阈值绕过阻塞。回退只针对本单经核对的改动或资产备份，禁止未经授权reset/clean、覆盖他人工作和强制解锁。确需改变共享契约、保存格式、正式配表或包范围时提交具体差异与影响，先获准再施工。


---

<a id="task-085"></a>

# TASK-085｜共享显示契约、动作语义与UI增量接入基础

> 状态：Backlog（任务单已编写，实施未派发）。优先级：P0。阶段：A 基础。日期：2026-10-06。Owner：XLingyyy。Reviewer／Issue：未指派。建议分支：`codex/TASK-085-presentation-contract`（未创建）。

[本批总入口](docs/planning/TASK-084-103/README.md) · [执行约定](docs/planning/TASK-084-103/EXECUTION_GUIDE.md) · [元数据](docs/tasks/TASK-085.json) · [交接模板](docs/handoffs/TASK-085.md)

## 1. 目标与预期结果

为伙伴状态、制作缺口、回营反馈和导航建立最小的只读显示契约与安全动作接入点，后续UI不重复计算或直接改世界。保留现有Slate/Widget架构。

## 2. 当前基础与事实边界

当前界面通过 HearthwardScreenWidget、各 Compose 页面、Gameplay/Companion/Camp 子系统和 Resources/UI 配置构成；已有权威事务与epoch检查，不能另建第二套库存/进度账本。

本单参考基线为 `main@6fcf5c22e965f0f7409438f19bc7b09e96ffb058`；实际开工必须复核届时HEAD与依赖产物。这里不是本轮运行结果，旧报告PASS/FAIL均绑定原受测实现。新行为是本单计划，不能写成当前已经存在。

## 3. 依赖、开工门槛与最小必读

前置：[TASK-084](docs/tasks/TASK-084.md)

本包CT为DRAFT而非已批准接口；085激活时确认采用范围。允许改变的仅是展示适配与必要读取接口，业务规则冲突提交设计差异，不自行覆盖。

共同必读：`AGENTS.md`、`WORKFLOW.md`、`docs/START_HERE.md`、`docs/PROJECT_STATE.md`、`docs/design/CURRENT.md`及本单MD/JSON，然后执行本批指南中的接手/路径核对。

本单额外入口（路径为只读定位，不等于修改授权；前置任务产物需先实际生成）：

- `Source/Hearthward/UI/HearthwardScreenWidget.h`
- `Source/Hearthward/UI/HearthwardScreenActions.cpp`
- `Source/Hearthward/UI/HearthwardScreenLayout.cpp`
- `Source/Hearthward/Companion/HearthwardCompanionFixture.h`
- `Source/Hearthward/Camp/HearthwardCampSubsystem.h`
- `Resources/UI/LAYOUT.md`

## 4. 修改范围与协作边界

除本单MD/JSON、handoff、QA目录、专用规划目录和根`README.md`固定收尾外，候选范围如下；最终以**已获准基线JSON**为准。

- `docs/contracts/CT-TASK-085-presentation-readmodels.md`
- `Source/Hearthward/UI/HearthwardPresentationReadModels.h`
- `Source/Hearthward/UI/HearthwardPresentationReadModels.cpp`
- `Source/Hearthward/UI/HearthwardScreenWidget.h`
- `Source/Hearthward/UI/HearthwardScreenActions.cpp`
- `Source/Hearthward/UI/HearthwardScreenLayout.cpp`
- `Resources/UI/layout.json`
- `Resources/UI/interface.json`
- `Source/Hearthward/Tests/PresentationReadModelTests.cpp`

Content与Save等未授权领域只读；例外仅以JSON实际范围为准，不能用必读清单扩大写权限。

拟新增辅助/测试文件不表示仓库已存在；先搜索可复用类型。共享UI配置、公共头文件、WorldPresentation/CampaignWorld等按[写入调度](docs/planning/TASK-084-103/DISPATCH.md)串行。临时输出使用 `.agent-local/qa/TASK-085/<唯一run>/`，不得写入用户原档。

## 5. Agent具体实施步骤

### 1. 梳理真实读取链

逐项核对 GetPhase/GetRequested/GetAcquired/GetCarried/GetDelivered/BlockReason、库存Available、CraftingStatus、任务/营地数据读取点；找现有同义结构。已有结构能满足就复用，不因本任务名称另造服务层。

### 2. 落实共享契约

以随包 CT 草案列明字段来源、单位、unknown/invalid含义、时间线失效、数量归属、动作权限和更新时机；源定义与实际接口不一致先修草案。Owner激活共享契约后，才添加最小适配结构。

### 3. 实现只读投影

优先纯函数/轻量适配器，输入真实对象或值快照，输出显示数据。只做显示格式与可用性投影，不拥有库存、奖励、任务完成和持久状态；缺对象返回可解释不可用状态。新文件为拟新增，类型名先全仓检索避免重复。

### 4. 定义动作语义

继续/暂停/取消工作、取消提案、取消模型请求、关闭页面分别映射原行为；显示enabled/reason与执行时重新校验分离。按钮不能通过直接写 Phase/State/Inventory 或拼假结果完成动作。

### 5. 整理UI约束

复用现有字体/色系/间距与layout解析；为后续新控件登记稳定LayoutId和Component，明确最小窗口、100%/150%字号、右侧对话区域、长文本滚动。只修改必要布局节点，不翻新整个主题。

### 6. 控制状态刷新

事件可用则失效重算；没有事件则仅对可见页面做有界刷新。地图朝向/屏幕投影保持实时；不得每帧重新遍历全世界生成物品目录，也不得缓存已销毁对象裸指针。

### 7. 建立契约测试

实现 Projection 纯读测试、缺对象/过期epoch/场景销毁测试和动作路由不写状态测试。为各字段提供真实或明确隔离的夹具；展示例子中的14/32不能写成正式逻辑。

### 8. 交接公共写窗口

记录类型、接口、字段来源与后续消费者；将ScreenWidget.h、ScreenActions.cpp和UI两个JSON的后续写入顺序交给调度者。只提交本单获授权范围，不替其他任务改实现。

## 6. 不可破坏的规则

- 不增加存档字段；读模型只缓存可丢弃投影，Load/新游戏/换档销毁。
- DisplayReason不是权限真值；实际事务每次使用现有安全/距离/epoch校验。
- 保持右侧约37%对话、深灰物品格和无黑雾矩形地图；不换框架、不动字体二进制。

所有更改继续遵守只读显示、原事务执行、稳定ID、时间线隔离和不伪填验收规则。规则未明确的新玩法不硬编码；不删测试/吞错误/全仓重构来让当前检查通过。

## 7. 验收用例与执行方式

每条用例在隔离档/明确诊断夹具中建立前置，实际记录操作与断言。夹具、注入输入、真实OS键鼠、真实模型、真人样本分层统计；下面所有结果尚未运行。

|局部编号|场景|前置/具体操作|通过标准|初始结果|
|---|---|---|---|---|
|T085-C01|投影无副作用|读取个人任务/库存/营地快照前后比较权威状态。|所有余额/任务ID/事务记录不变。|NOT_RUN|
|T085-C02|无对象与销毁|无伙伴、角色销毁、世界卸载时构造页面。|无崩溃/野指针，显示不可用且动作禁用。|NOT_RUN|
|T085-C03|时间线|取快照后读档，在旧按钮/回调上触发动作。|旧引用失效，不能跨epoch提交。|NOT_RUN|
|T085-C04|动作分离|依次测试关闭页、丢弃确认卡、取消请求、取消正在执行工作。|四种语义不互相代替，实际任务变化符合原契约。|NOT_RUN|
|T085-C05|布局兼容|默认/大字号与4:3/16:9/超宽窗口检查新控件。|有用信息不重叠、不越界，主动作始终可达。|NOT_RUN|
|T085-C06|刷新成本|连续开关页面与切场景，记录投影调用次数。|无重复订阅增长、后台无限刷新或每帧全表重建。|NOT_RUN|


全局挂接索引：T-002, T-024, T-025, T-011。它们来自现有TEST_MATRIX；正式数值以已批准增量/ACCEPTANCE和本单细化步骤为准，不恢复历史灰盒规则。

本单需新增/复用定向原生测试；新前缀建议 `Hearthward.Iteration.Task085.`。先注册并核验找到用例数量>0，再按公开UEClient运行，不能拿本表局部ID当UE过滤器。正常输入、渲染、真实模型或真人用例另行执行。

公共检查：`python -X utf8 scripts/validate_repo.py`、`python -X utf8 -m unittest discover -s scripts/tests -v`、`git diff --check`；在包含获批本单快照的实际基线上运行 `python -X utf8 scripts/validate_repo.py --task TASK-085 --base <实际完整SHA>`。UE操作/证据要求见执行约定；缺环境则明确NOT_RUN，不能将文件编写完成等同测试通过。

## 8. 交付物与完成定义

- CT-TASK-085-presentation-readmodels.md：从草案到实际采用版及批准记录
- 最小投影/路由代码及相关原生测试
- 字段—权威来源映射、公共文件写入交接、UI对照证据

固定收尾：更新`README.md`当前说明、同名MD/JSON与`docs/handoffs/TASK-085.md`；在`docs/qa/TASK-085/REPORT.md`绑定最终完整SHA、资产/模型指纹、环境、命令、逐用例结果和明确限制。该REPORT为未来执行产物，本包不预填。

工程实现完成、Owner视觉、真人体验、二机、性能与发布各自登记。依赖下游可用产物写入handoff，不把未验项目涂为PASS。没有授权时交付本地改动和证据并写“未提交/未推送/未发布”，不得自增权限。

## 9. 本单明确不做

- 完整MVVM/UMG/CommonUI迁移
- 新增保存格式、资源预留或新的任务执行系统
- 用显示层修正真实数量错误

## 10. 失败、范围变化和恢复

首先保留最短复现、受测状态和原始证据，再定位到本单或其他负责任务；不能通过清空存档/赠送物资/瞬移/改验收阈值绕过阻塞。回退只针对本单经核对的改动或资产备份，禁止未经授权reset/clean、覆盖他人工作和强制解锁。确需改变共享契约、保存格式、正式配表或包范围时提交具体差异与影响，先获准再施工。


---

<a id="task-086"></a>

# TASK-086｜弟弟工作状态卡、停工原因和恢复入口

> 状态：Backlog（任务单已编写，实施未派发）。优先级：P0。阶段：B 玩法与UI。日期：2026-10-06。Owner：XLingyyy。Reviewer／Issue：未指派。建议分支：`codex/TASK-086-companion-status`（未创建）。

[本批总入口](docs/planning/TASK-084-103/README.md) · [执行约定](docs/planning/TASK-084-103/EXECUTION_GUIDE.md) · [元数据](docs/tasks/TASK-086.json) · [交接模板](docs/handoffs/TASK-086.md)

## 1. 目标与预期结果

让个人委托和营地采集队的当前工作、实际入库、携带量、停工原因及可执行操作在对话/HUD中一致可见。

## 2. 当前基础与事实边界

TASK-078—081已有定量委托、查询续接、右侧对话和持续采集队。个人任务仍是指定来源；队伍不是按物资总量停止；普通族人后台生产与弟弟实际到岗必须区分。

本单参考基线为 `main@6fcf5c22e965f0f7409438f19bc7b09e96ffb058`；实际开工必须复核届时HEAD与依赖产物。这里不是本轮运行结果，旧报告PASS/FAIL均绑定原受测实现。新行为是本单计划，不能写成当前已经存在。

## 3. 依赖、开工门槛与最小必读

前置：[TASK-085](docs/tasks/TASK-085.md)

依赖的约定产物与实际实现SHA就绪、Owner派发且共享写窗口空闲后开始。依赖不要求伪改旧任务Done；读取其实际交接并核对采用版本。

共同必读：`AGENTS.md`、`WORKFLOW.md`、`docs/START_HERE.md`、`docs/PROJECT_STATE.md`、`docs/design/CURRENT.md`及本单MD/JSON，然后执行本批指南中的接手/路径核对。

本单额外入口（路径为只读定位，不等于修改授权；前置任务产物需先实际生成）：

- `Source/Hearthward/Companion/HearthwardCompanionFixture.h`
- `Source/Hearthward/UI/HearthwardScreenDialogue.cpp`
- `Source/Hearthward/UI/HearthwardHUDDialogue.cpp`
- `Source/Hearthward/UI/HearthwardScreenCamp.cpp`
- `docs/qa/TASK-081/REPORT.md`
- `docs/tasks/TASK-078.json`

## 4. 修改范围与协作边界

除本单MD/JSON、handoff、QA目录、专用规划目录和根`README.md`固定收尾外，候选范围如下；最终以**已获准基线JSON**为准。

- `Source/Hearthward/UI/HearthwardScreenDialogue.cpp`
- `Source/Hearthward/UI/HearthwardHUDDialogue.cpp`
- `Source/Hearthward/UI/HearthwardScreenContent.cpp`
- `Source/Hearthward/UI/HearthwardScreenCamp.cpp`
- `Source/Hearthward/UI/HearthwardScreenWidget.h`
- `Source/Hearthward/UI/HearthwardScreenActions.cpp`
- `Source/Hearthward/UI/HearthwardPresentationReadModels.cpp`
- `Source/Hearthward/UI/HearthwardPresentationReadModels.h`
- `Resources/UI/layout.json`
- `Resources/UI/interface.json`
- `Source/Hearthward/Tests/CompanionStatusViewTests.cpp`

Content与Save等未授权领域只读；例外仅以JSON实际范围为准，不能用必读清单扩大写权限。

拟新增辅助/测试文件不表示仓库已存在；先搜索可复用类型。共享UI配置、公共头文件、WorldPresentation/CampaignWorld等按[写入调度](docs/planning/TASK-084-103/DISPATCH.md)串行。临时输出使用 `.agent-local/qa/TASK-086/<唯一run>/`，不得写入用户原档。

## 5. Agent具体实施步骤

### 1. 先复验旧路径

重复个人32木材耗尽/14已交付的隔离夹具及真实任务查询/续接。当前版本未复现的问题不先重写状态机；从已有phase、货物和交付数据解释停止。

### 2. 个人任务状态卡

显示目标物品/目标量、已交付、当前携带、当前阶段、目的地可知名称、具体停工原因。剩余量=max(0,requested-delivered)，不得将累计采得量当已交付；状态为已完成/已取消时明确终态。

### 3. 队伍工作状态卡

显示资源岗位、族人人数、弟弟另占名额、弟弟到岗/赶路/受阻、累计入库口径、是否等待刷新。延续最多4族人+弟弟/每岗5人的已批准规则，不显示虚假完成百分比。

### 4. 接通合法处理动作

复用原 ResumeBlocked、个人取消与camp_team_stop/继续入口；无权限时显示具体原因和下一步。个人委托已有未交付货物的处理沿原契约，不悄悄扔货或退还已消耗材料。

### 5. 统一询问与HUD

对话“查看工作”和自然语言task_status展示时重读当前投影，避免推理期间数量已变。HUD只提示重大阶段变化/受阻，不滚屏刷每份产出；详细进度保留在工作页。

### 6. 处理提案生命周期

取消卡片只丢弃提案；返回首页不取消已执行工作；旧确认卡、Load前卡片、请求取消后的迟到回包不得复活。重复点击同一确认不创建第二队或重复入库。

### 7. 补充端到端用例

真实地图分别做个人往返和队伍到岗/暂停/继续；另用隔离夹具覆盖资源0、路径阻断、超重、危险、失去设施和存读档，比较UI与权威状态。

## 6. 不可破坏的规则

- 不开放个人自动换点、队伍定量停产或多任务队列。
- 不能为了显示“工作中”伪造导航到达或族人实体搬运；弟弟到岗由真实劳动力来源判断。
- 禁止把关闭页面、取消模型请求、暂停队伍、取消个人任务统一成一个取消函数。

所有更改继续遵守只读显示、原事务执行、稳定ID、时间线隔离和不伪填验收规则。规则未明确的新玩法不硬编码；不删测试/吞错误/全仓重构来让当前检查通过。

## 7. 验收用例与执行方式

每条用例在隔离档/明确诊断夹具中建立前置，实际记录操作与断言。夹具、注入输入、真实OS键鼠、真实模型、真人样本分层统计；下面所有结果尚未运行。

|局部编号|场景|前置/具体操作|通过标准|初始结果|
|---|---|---|---|---|
|T086-C01|个人进度|制造requested32/delivered14/carried6夹具并展示，再继续实际入库。|显示14/32与携带6分开；入库后按真实交易更新，未到营地不提前完成。|NOT_RUN|
|T086-C02|耗尽|指定源点真实耗尽，之后查询并尝试恢复。|显示资源不足/等待或原支持恢复条件；不补资源、不换来源。|NOT_RUN|
|T086-C03|队伍到岗|组2名族人和弟弟，观察走向岗位前后产量。|人数与到岗状态正确；弟弟未到岗不能计入他的劳动力。|NOT_RUN|
|T086-C04|暂停续接|执行队伍暂停、等待、继续；另测个人续接。|暂停停止相应生产；进度保留，各动作只影响所属工作。|NOT_RUN|
|T086-C05|查询无执行|输入“先不要继续，我只是问还差多少”。|只读回答；任务状态和生产开关不变。|NOT_RUN|
|T086-C06|过期重复|重复确认、换档后确认、返回后旧响应到达。|无重复分工/入库，旧时间线动作被拒绝。|NOT_RUN|
|T086-C07|布局|长中文原因、最大人数、无任务、完成终态，大字号与4:3。|状态及合法操作可读，输入框不被确认卡遮住。|NOT_RUN|
|T086-C08|真实恢复|个人返营携货及队伍暂停状态分别保存后独立重启。|显示由恢复的真实状态产生；无假终态/丢货/重复生产。|NOT_RUN|


全局挂接索引：T-002, T-007, T-009, T-010, T-011, T-025。它们来自现有TEST_MATRIX；正式数值以已批准增量/ACCEPTANCE和本单细化步骤为准，不恢复历史灰盒规则。

本单需新增/复用定向原生测试；新前缀建议 `Hearthward.Iteration.Task086.`。先注册并核验找到用例数量>0，再按公开UEClient运行，不能拿本表局部ID当UE过滤器。正常输入、渲染、真实模型或真人用例另行执行。

公共检查：`python -X utf8 scripts/validate_repo.py`、`python -X utf8 -m unittest discover -s scripts/tests -v`、`git diff --check`；在包含获批本单快照的实际基线上运行 `python -X utf8 scripts/validate_repo.py --task TASK-086 --base <实际完整SHA>`。UE操作/证据要求见执行约定；缺环境则明确NOT_RUN，不能将文件编写完成等同测试通过。

## 8. 交付物与完成定义

- 个人/队伍状态字段与按钮映射表
- 状态卡与HUD修改、定向测试
- 真实入库前后快照、停工/续接/过期卡证据

固定收尾：更新`README.md`当前说明、同名MD/JSON与`docs/handoffs/TASK-086.md`；在`docs/qa/TASK-086/REPORT.md`绑定最终完整SHA、资产/模型指纹、环境、命令、逐用例结果和明确限制。该REPORT为未来执行产物，本包不预填。

工程实现完成、Owner视觉、真人体验、二机、性能与发布各自登记。依赖下游可用产物写入handoff，不把未验项目涂为PASS。没有授权时交付本地改动和证据并写“未提交/未推送/未发布”，不得自增权限。

## 9. 本单明确不做

- 新增委托类型、自动跨来源和定量停队
- 修改营地人数/产量/资源刷新
- 在UI层直接修正货物或任务阶段

## 10. 失败、范围变化和恢复

首先保留最短复现、受测状态和原始证据，再定位到本单或其他负责任务；不能通过清空存档/赠送物资/瞬移/改验收阈值绕过阻塞。回退只针对本单经核对的改动或资产备份，禁止未经授权reset/clean、覆盖他人工作和强制解锁。确需改变共享契约、保存格式、正式配表或包范围时提交具体差异与影响，先获准再施工。


---

<a id="task-087"></a>

# TASK-087｜自然语言委托可靠性、取消回退与真实模型全矩阵

> 状态：Backlog（任务单已编写，实施未派发）。优先级：P0。阶段：B 玩法与UI。日期：2026-10-06。Owner：XLingyyy。Reviewer／Issue：未指派。建议分支：`codex/TASK-087-local-ai-reliability`（未创建）。

[本批总入口](docs/planning/TASK-084-103/README.md) · [执行约定](docs/planning/TASK-084-103/EXECUTION_GUIDE.md) · [元数据](docs/tasks/TASK-087.json) · [交接模板](docs/handoffs/TASK-087.md)

## 1. 目标与预期结果

修复有复现证据的理解/参数/生命周期问题，保留单次本机4B推理架构，并在CPU和Vulkan上分别完成现有语言门槛。模型不可用时手动玩法仍可完成。

## 2. 当前基础与事实边界

历史TASK-068完整理解矩阵未达标，TASK-079—081新增过局部成功，不等于全矩阵通过。完整基准及阈值以已批准ACCEPTANCE为准，禁止选取成功表达替代原集合。

本单参考基线为 `main@6fcf5c22e965f0f7409438f19bc7b09e96ffb058`；实际开工必须复核届时HEAD与依赖产物。这里不是本轮运行结果，旧报告PASS/FAIL均绑定原受测实现。新行为是本单计划，不能写成当前已经存在。

## 3. 依赖、开工门槛与最小必读

前置：[TASK-086](docs/tasks/TASK-086.md)

依赖的约定产物与实际实现SHA就绪、Owner派发且共享写窗口空闲后开始。依赖不要求伪改旧任务Done；读取其实际交接并核对采用版本。

共同必读：`AGENTS.md`、`WORKFLOW.md`、`docs/START_HERE.md`、`docs/PROJECT_STATE.md`、`docs/design/CURRENT.md`及本单MD/JSON，然后执行本批指南中的接手/路径核对。

本单额外入口（路径为只读定位，不等于修改授权；前置任务产物需先实际生成）：

- `Source/Hearthward/AI/HearthwardLocalAISubsystem.cpp`
- `Source/Hearthward/AI/HearthwardNPCContextProjection.cpp`
- `Source/Hearthward/AI/HearthwardAgentContract.cpp`
- `Source/Hearthward/AI/HearthwardAgentInteraction.cpp`
- `config/local-ai.lock.json`
- `docs/planning/TASK-053-074/ACCEPTANCE.md`
- `docs/qa/TASK-079/REPORT.md`

## 4. 修改范围与协作边界

除本单MD/JSON、handoff、QA目录、专用规划目录和根`README.md`固定收尾外，候选范围如下；最终以**已获准基线JSON**为准。

- `Source/Hearthward/AI/HearthwardLocalAISubsystem.cpp`
- `Source/Hearthward/AI/HearthwardLocalAISubsystem.h`
- `Source/Hearthward/AI/HearthwardLocalAIContext.cpp`
- `Source/Hearthward/AI/HearthwardNPCContextProjection.cpp`
- `Source/Hearthward/AI/HearthwardAgentContract.cpp`
- `Source/Hearthward/AI/HearthwardAgentInteraction.cpp`
- `Source/Hearthward/UI/HearthwardScreenDialogue.cpp`
- `Source/Hearthward/UI/HearthwardHUDDialogue.cpp`
- `Source/Hearthward/UI/HearthwardDialogueWidget.cpp`
- `Source/Hearthward/Tests/LocalAIReliabilityTests.cpp`

Content与Save等未授权领域只读；例外仅以JSON实际范围为准，不能用必读清单扩大写权限。

拟新增辅助/测试文件不表示仓库已存在；先搜索可复用类型。共享UI配置、公共头文件、WorldPresentation/CampaignWorld等按[写入调度](docs/planning/TASK-084-103/DISPATCH.md)串行。临时输出使用 `.agent-local/qa/TASK-087/<唯一run>/`，不得写入用户原档。

## 5. Agent具体实施步骤

### 1. 锁定评测集

读取068原60表达、执行/边界用例和原始失败，建立当前版复测副本，保留原文字、类别、预期和来源。新增否定、指代、续接、数量/人数、搬运方向、unsupported案例独立计数，不用调参后的精选集覆盖旧集。

### 2. 先真实复测

CPU/Vulkan各跑60表达，记录原始模型回复、解析结果、UE校验、确认后的实际执行与耗时；模型参数、输入上下文和随机性设置有日志。区分原始理解、澄清拒绝和执行成功，定位最小失败模式。

### 3. 修最小问题

优先修上下文裁剪顺序、可知对象绑定、数量/队伍人数歧义、否定和纯查询的意图边界；有确定性规则时只做现有轻量处理，不能以写死60条答案刷分。安全检查仍在UE，不给模型世界写权限。

### 4. 管理请求生命周期

按请求ID+当前epoch/约束版本验证回调；取消、角色失效、离开允许交流范围、读档或新游戏后废弃旧结果。单并发不变；重复提交有明确忙状态，无法取消底层时也要逻辑失效旧结果。

### 5. 可用的手动回退

请求发起后0.2秒内给等待反馈；允许取消本次请求并回首页/原表单，不取消已运行工作。模型启动失败、超时或不可用时显示原因，玩家能用已有表单完成采集/搬运/工作查询；不伪装成AI回答。

### 6. 保护中文输入

输入法组合期间Enter提交候选而非发送委托，Esc先处理组合/候选；在非组合状态才遵守页面语义。模型回复长文本不抢走输入焦点，编辑中到达回复不清空玩家文本。

### 7. 重跑与保留失败

每次影响模型输入/解析的修改后重跑对应集合，最终同版本重跑完整双后端矩阵；提交全量CSV/JSON与必要失败原文，记录修复前后分母。未达指标本单不得以“安全拒绝了”标全通过。

## 6. 不可破坏的规则

- 沿用锁定模型、3328输入/256输出、单并发、默认16GPU层；不更新GGUF/依赖/后端或扩大上下文。
- 原始理解≥90%、澄清/拒绝≥90%、明确任务端到端≥90%；另≥30行为正确率≥95%、≥20边界响应正确，复制物品和跨旧epoch/约束结算为0。
- CPU/Vulkan各60＝40明确+10歧义+10越权/不支持；精确分母及分类沿原评测，不自行改统计口径。
- 性能完整门槛在102，业务超时120秒不因目标10/30秒而擅自缩短。

所有更改继续遵守只读显示、原事务执行、稳定ID、时间线隔离和不伪填验收规则。规则未明确的新玩法不硬编码；不删测试/吞错误/全仓重构来让当前检查通过。

## 7. 验收用例与执行方式

每条用例在隔离档/明确诊断夹具中建立前置，实际记录操作与断言。夹具、注入输入、真实OS键鼠、真实模型、真人样本分层统计；下面所有结果尚未运行。

|局部编号|场景|前置/具体操作|通过标准|初始结果|
|---|---|---|---|---|
|T087-C01|明确指令|双后端跑40明确表达，包括个人量、人数和搬运方向。|理解与实际动作分开统计，目标/数量/来源目的地一致。|NOT_RUN|
|T087-C02|歧义与拒绝|各跑10歧义、10越权/不支持表达。|该问则澄清、该拒则拒绝；不把不理解的一律拒绝记正确。|NOT_RUN|
|T087-C03|否定查询|验证“别继续”“只是问进度”“刚才完成了吗”等真实表达。|查询不改任务，续接只有确认后执行。|NOT_RUN|
|T087-C04|生命周期|取消后回包、Load期间回包、重复提交、对象销毁。|旧结果不显示为新确认卡、不执行、不重复结算。|NOT_RUN|
|T087-C05|模型不可用|断开本地服务/失败启动/达到原超时；使用手动表单。|界面可退出，既有采集/搬运玩法不被模型故障阻断。|NOT_RUN|
|T087-C06|中文IME|真实中文组合/候选、Enter、Esc、长输入、回复到达。|不误发未完成文字，不丢输入，不双重执行。|NOT_RUN|
|T087-C07|真实行为|执行至少30个可执行行为、20个边界响应。|按批准指标报告，完整交付而非只输出合法JSON。|NOT_RUN|
|T087-C08|最终同版|锁定最终SHA与模型指纹，双后端重跑。|所有原始记录可重算指标；不拼接不同实现的最佳成绩。|NOT_RUN|


全局挂接索引：T-002, T-008, T-010, T-011, T-020, T-025。它们来自现有TEST_MATRIX；正式数值以已批准增量/ACCEPTANCE和本单细化步骤为准，不恢复历史灰盒规则。

本单需新增/复用定向原生测试；新前缀建议 `Hearthward.Iteration.Task087.`。先注册并核验找到用例数量>0，再按公开UEClient运行，不能拿本表局部ID当UE过滤器。正常输入、渲染、真实模型或真人用例另行执行。

公共检查：`python -X utf8 scripts/validate_repo.py`、`python -X utf8 -m unittest discover -s scripts/tests -v`、`git diff --check`；在包含获批本单快照的实际基线上运行 `python -X utf8 scripts/validate_repo.py --task TASK-087 --base <实际完整SHA>`。UE操作/证据要求见执行约定；缺环境则明确NOT_RUN，不能将文件编写完成等同测试通过。

## 8. 交付物与完成定义

- 沿用/增补的评测集合与来源映射
- 双后端原始结果、分项统计与失败分析
- 取消/回退/真实IME及正常地图执行证据

固定收尾：更新`README.md`当前说明、同名MD/JSON与`docs/handoffs/TASK-087.md`；在`docs/qa/TASK-087/REPORT.md`绑定最终完整SHA、资产/模型指纹、环境、命令、逐用例结果和明确限制。该REPORT为未来执行产物，本包不预填。

工程实现完成、Owner视觉、真人体验、二机、性能与发布各自登记。依赖下游可用产物写入handoff，不把未验项目涂为PASS。没有授权时交付本地改动和证据并写“未提交/未推送/未发布”，不得自增权限。

## 9. 本单明确不做

- 替换/训练模型、接云端LLM、增加Embedding或多层生成链
- 扩展危险自主行为/任意任务队列
- 将结构化假输入算作自然语言理解

## 10. 失败、范围变化和恢复

首先保留最短复现、受测状态和原始证据，再定位到本单或其他负责任务；不能通过清空存档/赠送物资/瞬移/改验收阈值绕过阻塞。回退只针对本单经核对的改动或资产备份，禁止未经授权reset/clean、覆盖他人工作和强制解锁。确需改变共享契约、保存格式、正式配表或包范围时提交具体差异与影响，先获准再施工。


---

<a id="task-088"></a>

# TASK-088｜制作配方查找、可制作筛选与缺料追踪

> 状态：Backlog（任务单已编写，实施未派发）。优先级：P1。阶段：B 玩法与UI。日期：2026-10-06。Owner：XLingyyy。Reviewer／Issue：未指派。建议分支：`codex/TASK-088-crafting-tracker`（未创建）。

[本批总入口](docs/planning/TASK-084-103/README.md) · [执行约定](docs/planning/TASK-084-103/EXECUTION_GUIDE.md) · [元数据](docs/tasks/TASK-088.json) · [交接模板](docs/handoffs/TASK-088.md)

## 1. 目标与预期结果

在现有制造逻辑之上增加配方搜索、分类、可制作筛选和一个明确的材料追踪目标，减少准备阶段查表和翻页。

## 2. 当前基础与事实边界

ComposeCrafting现有四行配方、批量、背包/仓储数量、重量预测和CraftingStatus已具备；不能重做制造结算。目标为信息组织，不扩配方或改经济。

本单参考基线为 `main@6fcf5c22e965f0f7409438f19bc7b09e96ffb058`；实际开工必须复核届时HEAD与依赖产物。这里不是本轮运行结果，旧报告PASS/FAIL均绑定原受测实现。新行为是本单计划，不能写成当前已经存在。

## 3. 依赖、开工门槛与最小必读

前置：[TASK-085](docs/tasks/TASK-085.md)

依赖的约定产物与实际实现SHA就绪、Owner派发且共享写窗口空闲后开始。依赖不要求伪改旧任务Done；读取其实际交接并核对采用版本。

共同必读：`AGENTS.md`、`WORKFLOW.md`、`docs/START_HERE.md`、`docs/PROJECT_STATE.md`、`docs/design/CURRENT.md`及本单MD/JSON，然后执行本批指南中的接手/路径核对。

本单额外入口（路径为只读定位，不等于修改授权；前置任务产物需先实际生成）：

- `Source/Hearthward/UI/HearthwardScreenCrafting.cpp`
- `Source/Hearthward/UI/HearthwardScreenActions.cpp`
- `Source/Hearthward/Building/HearthwardBuildingComponent.h`
- `Resources/Data/gameplay.json`
- `Source/Hearthward/UI/HearthwardScreenContent.cpp`

## 4. 修改范围与协作边界

除本单MD/JSON、handoff、QA目录、专用规划目录和根`README.md`固定收尾外，候选范围如下；最终以**已获准基线JSON**为准。

- `Source/Hearthward/UI/HearthwardScreenCrafting.cpp`
- `Source/Hearthward/UI/HearthwardScreenActions.cpp`
- `Source/Hearthward/UI/HearthwardScreenWidget.h`
- `Source/Hearthward/UI/HearthwardScreenContent.cpp`
- `Source/Hearthward/UI/HearthwardCraftingTracker.h`
- `Source/Hearthward/UI/HearthwardCraftingTracker.cpp`
- `Source/Hearthward/UI/HearthwardPresentationReadModels.cpp`
- `Resources/UI/layout.json`
- `Resources/UI/interface.json`
- `Source/Hearthward/Tests/CraftingDiscoveryTests.cpp`

Content与Save等未授权领域只读；例外仅以JSON实际范围为准，不能用必读清单扩大写权限。

拟新增辅助/测试文件不表示仓库已存在；先搜索可复用类型。共享UI配置、公共头文件、WorldPresentation/CampaignWorld等按[写入调度](docs/planning/TASK-084-103/DISPATCH.md)串行。临时输出使用 `.agent-local/qa/TASK-088/<唯一run>/`，不得写入用户原档。

## 5. Agent具体实施步骤

### 1. 保留制造真值

逐项核对配方id、materials、outputs、工作设施、等级和批量限制，记录CraftingStatus调用和实际制造入口；不复制一套“可制造”布尔逻辑。先为现有四行选择/批量制造建回归。

### 2. 搜索和分类

增加中文名称搜索、原数据已有类别筛选；无类别字段则用独立显示映射且保留“全部/其他”，不为搜索修改物品/配方ID。过滤后列表以RecipeId维持选择，滚动合法钳制；空结果提供清空过滤。

### 3. 可制作筛选

“当前可制作”同时考虑材料、现地可用仓储、设施、等阶、状态和输出容量等现有检查。不可制作配方在全部列表仍可查看详细失败原因；过滤状态不能隐藏选择到无法取消。

### 4. 单目标材料追踪

选择一个配方与批数后显式点“追踪材料”，允许替换/取消；只跟踪玩家选定目标，不自动给弟弟派单、不预留物资、不串行展开整棵制作链。目标跨页面保留，本轮为会话暂态。

### 5. 统一缺口计算

每种材料Needed=每批量×批数，Available来自现有可用数；在可用营地范围才计共享仓储。缺口=max(0,Needed-Available)。营地之外可另显示仓储参考量但不得冒充随身可用；输出预览与重量单位沿原API。

### 6. 追踪显示与更新

制作页给完整清单，HUD只给紧凑摘要/打开详细入口；库存、材料消耗、批数、营地进出、任务入库变化后重算。达到材料条件提示“材料齐备”，不等同“设施/等级等全部条件满足”，不自动制造。

### 7. 时间线与输入

清空新游戏/读档/切档后的暂态追踪，页面关闭不清空；在说明中明确不持久化。搜索框IME组合遵守原Enter/Esc规则，输入文字时不触发背包/制造快捷键。

### 8. 完成交易回归

用实际制造按钮测试背包优先、仓储补足、批数变更、材料不足、输出超重、连续双击；统计真实扣料/产出与原规则一致。

## 6. 不可破坏的规则

- 不改配方、成本、产量、等级门槛或即时制造规则。
- 追踪是只读需求，不是资源预留，不影响弟弟/族人能否使用物资。
- 本单不改Save schema；跨重启保持追踪不在本批范围，不能把暂态UI偷偷写入别的角色记录。

所有更改继续遵守只读显示、原事务执行、稳定ID、时间线隔离和不伪填验收规则。规则未明确的新玩法不硬编码；不删测试/吞错误/全仓重构来让当前检查通过。

## 7. 验收用例与执行方式

每条用例在隔离档/明确诊断夹具中建立前置，实际记录操作与断言。夹具、注入输入、真实OS键鼠、真实模型、真人样本分层统计；下面所有结果尚未运行。

|局部编号|场景|前置/具体操作|通过标准|初始结果|
|---|---|---|---|---|
|T088-C01|搜索稳定性|搜索中文、无结果、清空、切分类与换页。|选择/详情属于同一RecipeId；无越界或错配。|NOT_RUN|
|T088-C02|可制作判定|材料充足但缺设施、缺等阶、超容量，逐一过滤。|与正式CraftingStatus一致，不只凭材料放行。|NOT_RUN|
|T088-C03|材料缺口|测试仅背包、营地内背包+仓储、离营、他处库存变化。|显示可用和参考库存有区别，缺口不负数、不双计仓储。|NOT_RUN|
|T088-C04|批量目标|变更批数、替换目标、取消目标、完成材料准备。|追踪准确更新且不发起制造/分工，不预留或扣物品。|NOT_RUN|
|T088-C05|交易一致|真实制造，重复点击，再检查材料/产物/重量。|只按原事务成功次数结算；显示预测与实际单位一致。|NOT_RUN|
|T088-C06|生命周期|开关Tab/J/M/制作页，然后Load和新游戏。|跨页面保留；换时间线清空且无旧对象引用。|NOT_RUN|
|T088-C07|输入布局|中文IME、长配方名、长材料表、大字号和4:3。|不误触快捷键，主动作与全部材料可达可读。|NOT_RUN|


全局挂接索引：T-002, T-004, T-025。它们来自现有TEST_MATRIX；正式数值以已批准增量/ACCEPTANCE和本单细化步骤为准，不恢复历史灰盒规则。

本单需新增/复用定向原生测试；新前缀建议 `Hearthward.Iteration.Task088.`。先注册并核验找到用例数量>0，再按公开UEClient运行，不能拿本表局部ID当UE过滤器。正常输入、渲染、真实模型或真人用例另行执行。

公共检查：`python -X utf8 scripts/validate_repo.py`、`python -X utf8 -m unittest discover -s scripts/tests -v`、`git diff --check`；在包含获批本单快照的实际基线上运行 `python -X utf8 scripts/validate_repo.py --task TASK-088 --base <实际完整SHA>`。UE操作/证据要求见执行约定；缺环境则明确NOT_RUN，不能将文件编写完成等同测试通过。

## 8. 交付物与完成定义

- 搜索/筛选/单目标追踪代码与原生测试
- 配方筛选及缺口口径说明
- 实际制造前后库存/重量对照与界面截图

固定收尾：更新`README.md`当前说明、同名MD/JSON与`docs/handoffs/TASK-088.md`；在`docs/qa/TASK-088/REPORT.md`绑定最终完整SHA、资产/模型指纹、环境、命令、逐用例结果和明确限制。该REPORT为未来执行产物，本包不预填。

工程实现完成、Owner视觉、真人体验、二机、性能与发布各自登记。依赖下游可用产物写入handoff，不把未验项目涂为PASS。没有授权时交付本地改动和证据并写“未提交/未推送/未发布”，不得自增权限。

## 9. 本单明确不做

- 多目标自动制作计划、资源预留、配方解锁改制
- 自动向未知资源点导航或自动委托
- 新增持久化字段

## 10. 失败、范围变化和恢复

首先保留最短复现、受测状态和原始证据，再定位到本单或其他负责任务；不能通过清空存档/赠送物资/瞬移/改验收阈值绕过阻塞。回退只针对本单经核对的改动或资产备份，禁止未经授权reset/clean、覆盖他人工作和强制解锁。确需改变共享契约、保存格式、正式配表或包范围时提交具体差异与影响，先获准再施工。


---

<a id="task-089"></a>

# TASK-089｜背包与仓储操作一致性、实例详情和可读性

> 状态：Backlog（任务单已编写，实施未派发）。优先级：P1。阶段：B 玩法与UI。日期：2026-10-06。Owner：XLingyyy。Reviewer／Issue：未指派。建议分支：`codex/TASK-089-inventory-storage-ux`（未创建）。

[本批总入口](docs/planning/TASK-084-103/README.md) · [执行约定](docs/planning/TASK-084-103/EXECUTION_GUIDE.md) · [元数据](docs/tasks/TASK-089.json) · [交接模板](docs/handoffs/TASK-089.md)

## 1. 目标与预期结果

统一背包/仓储中物品数量、可用量、重量和装备实例表达，改善查找与转移反馈，同时保护拖放、稀疏格子和快捷栏已有成果。

## 2. 当前基础与事实边界

当前仓储已采用深灰格子，支持选择存入/取出、数量调整、分页和失败原因；背包支持拖放、独立装备实例、稀疏位置与四快捷栏。不是重新做一套箱子系统。

本单参考基线为 `main@6fcf5c22e965f0f7409438f19bc7b09e96ffb058`；实际开工必须复核届时HEAD与依赖产物。这里不是本轮运行结果，旧报告PASS/FAIL均绑定原受测实现。新行为是本单计划，不能写成当前已经存在。

## 3. 依赖、开工门槛与最小必读

前置：[TASK-086](docs/tasks/TASK-086.md)、[TASK-088](docs/tasks/TASK-088.md)

依赖的约定产物与实际实现SHA就绪、Owner派发且共享写窗口空闲后开始。依赖不要求伪改旧任务Done；读取其实际交接并核对采用版本。

共同必读：`AGENTS.md`、`WORKFLOW.md`、`docs/START_HERE.md`、`docs/PROJECT_STATE.md`、`docs/design/CURRENT.md`及本单MD/JSON，然后执行本批指南中的接手/路径核对。

本单额外入口（路径为只读定位，不等于修改授权；前置任务产物需先实际生成）：

- `Source/Hearthward/UI/HearthwardScreenStorage.cpp`
- `Source/Hearthward/UI/HearthwardHUDStorage.cpp`
- `Source/Hearthward/UI/HearthwardScreenInventoryDrag.cpp`
- `Source/Hearthward/UI/HearthwardScreenEquipment.cpp`
- `Source/Hearthward/UI/HearthwardScreenRepair.cpp`
- `docs/releases/demo-20261006-2/REPORT.md`

## 4. 修改范围与协作边界

除本单MD/JSON、handoff、QA目录、专用规划目录和根`README.md`固定收尾外，候选范围如下；最终以**已获准基线JSON**为准。

- `Source/Hearthward/UI/HearthwardScreenStorage.cpp`
- `Source/Hearthward/UI/HearthwardHUDStorage.cpp`
- `Source/Hearthward/UI/HearthwardScreenContent.cpp`
- `Source/Hearthward/UI/HearthwardScreenInventoryDrag.cpp`
- `Source/Hearthward/UI/HearthwardScreenEquipment.cpp`
- `Source/Hearthward/UI/HearthwardScreenRepair.cpp`
- `Source/Hearthward/UI/HearthwardScreenWidget.h`
- `Source/Hearthward/UI/HearthwardScreenActions.cpp`
- `Source/Hearthward/UI/HearthwardPresentationReadModels.cpp`
- `Resources/UI/layout.json`
- `Resources/UI/interface.json`
- `Source/Hearthward/Tests/InventoryStorageUXTests.cpp`

Content与Save等未授权领域只读；例外仅以JSON实际范围为准，不能用必读清单扩大写权限。

拟新增辅助/测试文件不表示仓库已存在；先搜索可复用类型。共享UI配置、公共头文件、WorldPresentation/CampaignWorld等按[写入调度](docs/planning/TASK-084-103/DISPATCH.md)串行。临时输出使用 `.agent-local/qa/TASK-089/<唯一run>/`，不得写入用户原档。

## 5. Agent具体实施步骤

### 1. 保存现有交互契约

录制拖拽换位、空格保留、装备穿卸、快捷栏分配、仓储E执行/R丢弃等现行路径。键位文本由语义配置读取，不为了统一外观强制换成同一个键。

### 2. 统一详情

为同一物品在背包/箱子/装备/维修展示一致名称、用途、总数、可用数、重量；实例装备显示唯一实例选择、耐久、装备状态。资料来自真实物品和实例，不以同类ID覆盖实例身份。

### 3. 优化查找

复用制作搜索控件增加可清除的筛选/查找，或在当前布局中提供清晰分类；过滤仅是视图，不改底层槽位，不在搜索结果中暗中压缩稀疏格子。过滤中拖动需映射真实槽位；不能可靠映射时明确禁用拖动而非错换。

### 4. 清楚的转移预览

明确“背包→仓储/仓储→背包”、本次数量、当前可用、转移后个人负重和失败原因。数量有界，负数/零/超源点/超容量不能提交；整组/最大量按钮只有复用原规则可得合法数量才提供。

### 5. 执行复查与幂等

点击时重新取得可用数、距离、设施、动作状态、epoch，调用现有转移事务；处理“预览后库存已变”，成功显示实际MovedCount。双击/重试不能再结算同一OperationId，失败不改变槽位/选择到其他物品。

### 6. 保护危险动作

丢弃、装备实例维修、卸下等保留原有确认与距离/设施要求；明确当前操作的物品/实例/数量。不要在本单增加无事务支持的撤销、远程仓储或一键清空。

### 7. 布局与持久恢复

检查空背包、满箱、长名称、损坏装备、大字号，分页/滚轮/键盘焦点不漂移。用真实保存独立重启核对稀疏布局、装备实例、耐久与四快捷槽，不擅自改变保存格式。

## 6. 不可破坏的规则

- 共享仓储两营地仍是一份，不能为UI复制仓储或添加运输。
- 过滤/分页绝不改变真实物品数量和位置；任何自动整理功能不在本单。
- 继续显示真实失败原因，不把超距/战斗限制仅做成看不见的禁用。

所有更改继续遵守只读显示、原事务执行、稳定ID、时间线隔离和不伪填验收规则。规则未明确的新玩法不硬编码；不删测试/吞错误/全仓重构来让当前检查通过。

## 7. 验收用例与执行方式

每条用例在隔离档/明确诊断夹具中建立前置，实际记录操作与断言。夹具、注入输入、真实OS键鼠、真实模型、真人样本分层统计；下面所有结果尚未运行。

|局部编号|场景|前置/具体操作|通过标准|初始结果|
|---|---|---|---|---|
|T089-C01|实例唯一|同种两件不同耐久装备，选择/穿戴/维修/存取。|始终操作正确实例，详情不串件。|NOT_RUN|
|T089-C02|稀疏拖放|制造空格、换位、筛选后取消，保存继续。|布局和快捷栏不回退，不因过滤重排。|NOT_RUN|
|T089-C03|数量边界|0、负值、超来源、最大可装量及背包容量边缘。|合法数量可执行，非法值安全拒绝且不损物。|NOT_RUN|
|T089-C04|预览失效|预览后由生产/委托/消耗改变库存，再执行。|重新校验，显示实际结果，无负库存。|NOT_RUN|
|T089-C05|距离与epoch|离开设施、进入禁止状态、Load后旧操作。|按原规则拒绝，不远程转移、不跨档执行。|NOT_RUN|
|T089-C06|重复提交|快速双击、重复相同事务与再次合法新操作。|一次事务只结算一次；新操作仍可用。|NOT_RUN|
|T089-C07|可读可操作|空/满/长名称/大字号/4:3与超宽。|无遮挡，分页和数量控件可达，焦点返回正确。|NOT_RUN|
|T089-C08|存档保真|整套装备与快捷栏/槽位/库存快照，保存独立重启。|保持原格式恢复，数量/耐久/实例ID一致。|NOT_RUN|


全局挂接索引：T-002, T-004, T-005, T-011, T-025。它们来自现有TEST_MATRIX；正式数值以已批准增量/ACCEPTANCE和本单细化步骤为准，不恢复历史灰盒规则。

本单需新增/复用定向原生测试；新前缀建议 `Hearthward.Iteration.Task089.`。先注册并核验找到用例数量>0，再按公开UEClient运行，不能拿本表局部ID当UE过滤器。正常输入、渲染、真实模型或真人用例另行执行。

公共检查：`python -X utf8 scripts/validate_repo.py`、`python -X utf8 -m unittest discover -s scripts/tests -v`、`git diff --check`；在包含获批本单快照的实际基线上运行 `python -X utf8 scripts/validate_repo.py --task TASK-089 --base <实际完整SHA>`。UE操作/证据要求见执行约定；缺环境则明确NOT_RUN，不能将文件编写完成等同测试通过。

## 8. 交付物与完成定义

- 共用详情与转移预览实现、定向测试
- 实际转移事务与实例恢复对照
- 布局/字号/键鼠操作证据

固定收尾：更新`README.md`当前说明、同名MD/JSON与`docs/handoffs/TASK-089.md`；在`docs/qa/TASK-089/REPORT.md`绑定最终完整SHA、资产/模型指纹、环境、命令、逐用例结果和明确限制。该REPORT为未来执行产物，本包不预填。

工程实现完成、Owner视觉、真人体验、二机、性能与发布各自登记。依赖下游可用产物写入handoff，不把未验项目涂为PASS。没有授权时交付本地改动和证据并写“未提交/未推送/未发布”，不得自增权限。

## 9. 本单明确不做

- 自动排序、快捷远程存取、共享仓储规则调整
- 新存档schema或复制物品容器
- 恢复旧火红背景/装饰界面

## 10. 失败、范围变化和恢复

首先保留最短复现、受测状态和原始证据，再定位到本单或其他负责任务；不能通过清空存档/赠送物资/瞬移/改验收阈值绕过阻塞。回退只针对本单经核对的改动或资产备份，禁止未经授权reset/clean、覆盖他人工作和强制解锁。确需改变共享契约、保存格式、正式配表或包范围时提交具体差异与影响，先获准再施工。


---

<a id="task-090"></a>

# TASK-090｜HUD与日志目标层级、首次准备引导

> 状态：Backlog（任务单已编写，实施未派发）。优先级：P1。阶段：B 玩法与UI。日期：2026-10-06。Owner：XLingyyy。Reviewer／Issue：未指派。建议分支：`codex/TASK-090-hud-journal-onboarding`（未创建）。

[本批总入口](docs/planning/TASK-084-103/README.md) · [执行约定](docs/planning/TASK-084-103/EXECUTION_GUIDE.md) · [元数据](docs/tasks/TASK-090.json) · [交接模板](docs/handoffs/TASK-090.md)

## 1. 目标与预期结果

让玩家在首次营地准备和外出时知道当前目标、必要补给与弟弟状态，减少全系统菜单堆叠；引导基于真实阶段，不替玩家完成任务。

## 2. 当前基础与事实边界

已有任务日志与追踪、金色方向标记、生存信息、材料追踪和弟弟反馈；本单整合信息层级，不新增剧情目标或奖励机制。

本单参考基线为 `main@6fcf5c22e965f0f7409438f19bc7b09e96ffb058`；实际开工必须复核届时HEAD与依赖产物。这里不是本轮运行结果，旧报告PASS/FAIL均绑定原受测实现。新行为是本单计划，不能写成当前已经存在。

## 3. 依赖、开工门槛与最小必读

前置：[TASK-086](docs/tasks/TASK-086.md)、[TASK-088](docs/tasks/TASK-088.md)、[TASK-089](docs/tasks/TASK-089.md)

依赖的约定产物与实际实现SHA就绪、Owner派发且共享写窗口空闲后开始。依赖不要求伪改旧任务Done；读取其实际交接并核对采用版本。

共同必读：`AGENTS.md`、`WORKFLOW.md`、`docs/START_HERE.md`、`docs/PROJECT_STATE.md`、`docs/design/CURRENT.md`及本单MD/JSON，然后执行本批指南中的接手/路径核对。

本单额外入口（路径为只读定位，不等于修改授权；前置任务产物需先实际生成）：

- `Source/Hearthward/UI/HearthwardScreenContent.cpp`
- `Source/Hearthward/UI/HearthwardHUD.cpp`
- `Source/Hearthward/UI/HearthwardQuestGuidance.cpp`
- `Source/Hearthward/Campaign/HearthwardCampaignQuests.cpp`
- `Source/Hearthward/Gameplay/HearthwardGameplayComponent.h`
- `Resources/Data/gameplay.json`

## 4. 修改范围与协作边界

除本单MD/JSON、handoff、QA目录、专用规划目录和根`README.md`固定收尾外，候选范围如下；最终以**已获准基线JSON**为准。

- `Source/Hearthward/UI/HearthwardScreenContent.cpp`
- `Source/Hearthward/UI/HearthwardHUD.cpp`
- `Source/Hearthward/UI/HearthwardScreenWidget.h`
- `Source/Hearthward/UI/HearthwardScreenActions.cpp`
- `Source/Hearthward/UI/HearthwardScreenLayout.cpp`
- `Source/Hearthward/UI/HearthwardScreenPaint.cpp`
- `Source/Hearthward/UI/HearthwardPreparationView.h`
- `Source/Hearthward/UI/HearthwardPreparationView.cpp`
- `Resources/UI/layout.json`
- `Resources/UI/interface.json`
- `Source/Hearthward/Tests/HUDJournalPreparationTests.cpp`

Content与Save等未授权领域只读；例外仅以JSON实际范围为准，不能用必读清单扩大写权限。

拟新增辅助/测试文件不表示仓库已存在；先搜索可复用类型。共享UI配置、公共头文件、WorldPresentation/CampaignWorld等按[写入调度](docs/planning/TASK-084-103/DISPATCH.md)串行。临时输出使用 `.agent-local/qa/TASK-090/<唯一run>/`，不得写入用户原档。

## 5. Agent具体实施步骤

### 1. 定义三层信息

常驻HUD保留必要生存/当前任务/重要伙伴异常；展开日志显示阶段与下一步；制作/工作页保留详单。标注每条信息的来源、显示条件、隐藏条件和对应操作入口，禁止把全部营地数据塞HUD。

### 2. 准备卡只读推导

依据084路线已有任务阶段，展示本次目标、所需设施/工具/补给与弟弟分工入口。只有已批准且可知的条件才展示；不存在硬性补给要求时用“建议”不加任务门槛。

### 3. 渐进展示

新档到对应阶段再出现取遗物/跟随/查看日志/制作或分工提示；按真实交互完成隐藏。旧档和自由探索不重演已完成提示；不修改Claimed或强制接受/自动提交任务。

### 4. 连接已有页面

目标详情可打开相关制作/营地/工作页并选中对应已有条目；材料追踪和主线追踪用不同图形/文案，取消材料追踪不能取消主线。返回恢复原页与焦点，不遗留暂停状态。

### 5. 任务完成反馈

真实条件已满但需在J领取时明确“回日志提交/领取”，不继续引导玩家跑向旧地点；领取后隐藏旧目标。复用实际奖励与进度回执，不在通知函数发奖。

### 6. 重要性和遮挡

弟弟倒地、受阻、生存危险优先于普通产量提示；使用颜色+图标/文字组合，长原因可展开。地图页、对话页、加载/倒地状态按已有页面约束排版，不能挡住救援/放弃确认。

### 7. 首次流程验证

从正常新游戏和一个已有营地档分别操作，记录玩家卡住时能否在界面找到下一步；本人或自动输入只是工程测试，首次玩家的完成率与时间另在103收集。

## 6. 不可破坏的规则

- 不增加新手强制封路、补给硬门槛、自动接/领奖或新的任务ID。
- HUD“目标完成”只能引用权威已满足条件；显示层不自动使目标满足。
- 遵守对话运行、菜单暂停设置、Esc优先级和快捷键再次关闭，不用引导重置玩家设置。

所有更改继续遵守只读显示、原事务执行、稳定ID、时间线隔离和不伪填验收规则。规则未明确的新玩法不硬编码；不删测试/吞错误/全仓重构来让当前检查通过。

## 7. 验收用例与执行方式

每条用例在隔离档/明确诊断夹具中建立前置，实际记录操作与断言。夹具、注入输入、真实OS键鼠、真实模型、真人样本分层统计；下面所有结果尚未运行。

|局部编号|场景|前置/具体操作|通过标准|初始结果|
|---|---|---|---|---|
|T090-C01|新档阶段|按序完成遗物、跟随、撤离与营地准备。|提示随真实阶段出现/隐藏，不提前剧透。|NOT_RUN|
|T090-C02|旧档恢复|读取营地中期及已完成首救的兼容档副本。|不强制重走新手步骤，不重复奖励。|NOT_RUN|
|T090-C03|两类追踪|同时主线追踪与材料追踪，分别取消。|互不覆盖进度或标记状态。|NOT_RUN|
|T090-C04|待领取目标|满足任务条件但未领奖，之后在J领取。|清楚指向提交动作，领取后旧标记消失。|NOT_RUN|
|T090-C05|页面链路|HUD→日志→制作/工作页→返回；重复快捷键和Load。|焦点与暂停状态正确，无输入丢失或无法退出。|NOT_RUN|
|T090-C06|紧急状态|真实受伤/倒地/弟弟受阻及长中文提示叠加。|重要操作可达，文本不重叠；普通通知不遮挡救援。|NOT_RUN|
|T090-C07|可读性|100%/150%、4:3/16:9/超宽和低对比场景。|完整文字可读，状态不只靠颜色分辨。|NOT_RUN|


全局挂接索引：T-002, T-011, T-013, T-025。它们来自现有TEST_MATRIX；正式数值以已批准增量/ACCEPTANCE和本单细化步骤为准，不恢复历史灰盒规则。

本单需新增/复用定向原生测试；新前缀建议 `Hearthward.Iteration.Task090.`。先注册并核验找到用例数量>0，再按公开UEClient运行，不能拿本表局部ID当UE过滤器。正常输入、渲染、真实模型或真人用例另行执行。

公共检查：`python -X utf8 scripts/validate_repo.py`、`python -X utf8 -m unittest discover -s scripts/tests -v`、`git diff --check`；在包含获批本单快照的实际基线上运行 `python -X utf8 scripts/validate_repo.py --task TASK-090 --base <实际完整SHA>`。UE操作/证据要求见执行约定；缺环境则明确NOT_RUN，不能将文件编写完成等同测试通过。

## 8. 交付物与完成定义

- HUD/日志信息层级表与阶段显示规则
- 首次准备与跨页引导代码/测试
- 新档/旧档/倒地/大字号截图与输入记录

固定收尾：更新`README.md`当前说明、同名MD/JSON与`docs/handoffs/TASK-090.md`；在`docs/qa/TASK-090/REPORT.md`绑定最终完整SHA、资产/模型指纹、环境、命令、逐用例结果和明确限制。该REPORT为未来执行产物，本包不预填。

工程实现完成、Owner视觉、真人体验、二机、性能与发布各自登记。依赖下游可用产物写入handoff，不把未验项目涂为PASS。没有授权时交付本地改动和证据并写“未提交/未推送/未发布”，不得自增权限。

## 9. 本单明确不做

- 改写主线任务、自动领奖或新手强制流程
- 新建全屏百科/技能系统
- 将引导时间冒充真人节奏数据

## 10. 失败、范围变化和恢复

首先保留最短复现、受测状态和原始证据，再定位到本单或其他负责任务；不能通过清空存档/赠送物资/瞬移/改验收阈值绕过阻塞。回退只针对本单经核对的改动或资产备份，禁止未经授权reset/clean、覆盖他人工作和强制解锁。确需改变共享契约、保存格式、正式配表或包范围时提交具体差异与影响，先获准再施工。


---

<a id="task-091"></a>

# TASK-091｜地图可读性与关键入口转折引导

> 状态：Backlog（任务单已编写，实施未派发）。优先级：P1。阶段：C 地图与场景。日期：2026-10-06。Owner：XLingyyy。Reviewer／Issue：未指派。建议分支：`codex/TASK-091-map-route-guidance`（未创建）。

[本批总入口](docs/planning/TASK-084-103/README.md) · [执行约定](docs/planning/TASK-084-103/EXECUTION_GUIDE.md) · [元数据](docs/tasks/TASK-091.json) · [交接模板](docs/handoffs/TASK-091.md)

## 1. 目标与预期结果

保留现有矩形地图与传送规则，解决目标在墙后/楼下而玩家不知道入口的问题；只对关键现有路线增加入口和转折引导，不做全世界逐拐点导航。

## 2. 当前基础与事实边界

当前Map代码分别处理4032米级底层地形和3000×2000米查看区域；任务标记是直线距离和目标投影，不保证可行走。地图自己暂停不应再拦截合法传送。

本单参考基线为 `main@6fcf5c22e965f0f7409438f19bc7b09e96ffb058`；实际开工必须复核届时HEAD与依赖产物。这里不是本轮运行结果，旧报告PASS/FAIL均绑定原受测实现。新行为是本单计划，不能写成当前已经存在。

## 3. 依赖、开工门槛与最小必读

前置：[TASK-090](docs/tasks/TASK-090.md)

依赖的约定产物与实际实现SHA就绪、Owner派发且共享写窗口空闲后开始。依赖不要求伪改旧任务Done；读取其实际交接并核对采用版本。

共同必读：`AGENTS.md`、`WORKFLOW.md`、`docs/START_HERE.md`、`docs/PROJECT_STATE.md`、`docs/design/CURRENT.md`及本单MD/JSON，然后执行本批指南中的接手/路径核对。

本单额外入口（路径为只读定位，不等于修改授权；前置任务产物需先实际生成）：

- `Source/Hearthward/UI/HearthwardScreenMap.cpp`
- `Source/Hearthward/UI/HearthwardQuestGuidance.cpp`
- `Source/Hearthward/UI/HearthwardScreenMapTest.inl`
- `Resources/UI/interface.json`
- `Source/Hearthward/Campaign/HearthwardCampaignWorld.cpp`
- `docs/planning/TASK-084-103/ROUTE_MANIFEST.json`

## 4. 修改范围与协作边界

除本单MD/JSON、handoff、QA目录、专用规划目录和根`README.md`固定收尾外，候选范围如下；最终以**已获准基线JSON**为准。

- `Source/Hearthward/UI/HearthwardScreenMap.cpp`
- `Source/Hearthward/UI/HearthwardQuestGuidance.cpp`
- `Source/Hearthward/UI/HearthwardQuestGuidance.h`
- `Source/Hearthward/UI/HearthwardScreenWidget.h`
- `Source/Hearthward/UI/HearthwardScreenActions.cpp`
- `Source/Hearthward/UI/HearthwardGuidanceRoutes.cpp`
- `Source/Hearthward/UI/HearthwardGuidanceRoutes.h`
- `Resources/Data/quest_guidance.json`
- `Resources/UI/layout.json`
- `Resources/UI/interface.json`
- `Source/Hearthward/Tests/MapGuidanceRouteTests.cpp`

Content与Save等未授权领域只读；例外仅以JSON实际范围为准，不能用必读清单扩大写权限。

拟新增辅助/测试文件不表示仓库已存在；先搜索可复用类型。共享UI配置、公共头文件、WorldPresentation/CampaignWorld等按[写入调度](docs/planning/TASK-084-103/DISPATCH.md)串行。临时输出使用 `.agent-local/qa/TASK-091/<唯一run>/`，不得写入用户原档。

## 5. Agent具体实施步骤

### 1. 核对地图真值

列明地形原点/范围、视窗尺寸、MapPoint/MapWorldAt坐标换算、地图朝向与人物图标；先跑082旧回归，保护等比例、全屏覆盖、拖动边界和传送落点逻辑。

### 2. 配置关键路线

从084清单选择卧室出口、楼梯、庭院侧门及首救路线入口；为各任务阶段配置可知入口/过渡节点，使用稳定任务/地点ID和真实采样坐标。拟新增quest_guidance.json只描述导航表现，不写奖账本/任务状态。

### 3. 区分目标和途经点

主目标继续指向任务终点；在存在有效路线且玩家处在适用区域时显示“先到侧门/楼梯”等下一可达入口，可附高/低处说明。入口标记不得被计作任务完成点，直线距离明确不伪称步行路程。

### 4. 处理节点进退

节点通过依据正式位置/阶段或可验证空间条件；允许玩家走另一合法路，不强制逐点触发。玩家偏离、返走、Load、传送、阶段变化时重新求下一有效节点；找不到适用路线则退回普通目标标记，不盲目画穿墙导航线。

### 5. 优化地点标签

缩放时按重要性裁剪/避让标签，保持关键地点可点；保留红蓝火焰朝向与任务/个人路标区别。无黑雾不意味着显示未知剧情，地点列表继续排除物品/人物。不要恢复边框、黑边和常驻操作说明。

### 6. 保护传送限制

展开地点和现行传送流程只改善失败说明，不跳过激活/剧情/危险/集合/落点准备。地图造成的暂停与其他禁止条件分开，传送后重新定位人物并清除旧途经点。

### 7. 完整路线实走

正式新游戏按实际键鼠由卧室走至侧门，再测营地路线；弟弟真实跟随，不用传送验证可达性。092修改几何后只重测受影响节点与地图映射，保持同一路线清单。

## 6. 不可破坏的规则

- 不新增地图黑雾、室内分层地图、全局自动寻路或未知资源泄漏。
- 导航数据不是任务真值；缺/坏导航数据可安全降级，不能使任务不可完成。
- 3000×2000是查看区域，不据此缩裁实际世界或移动游戏对象。

所有更改继续遵守只读显示、原事务执行、稳定ID、时间线隔离和不伪填验收规则。规则未明确的新玩法不硬编码；不删测试/吞错误/全仓重构来让当前检查通过。

## 7. 验收用例与执行方式

每条用例在隔离档/明确诊断夹具中建立前置，实际记录操作与断言。夹具、注入输入、真实OS键鼠、真实模型、真人样本分层统计；下面所有结果尚未运行。

|局部编号|场景|前置/具体操作|通过标准|初始结果|
|---|---|---|---|---|
|T091-C01|坐标互逆|边界/中心/缩放/平移点做世界到图再反变换。|误差按原精度界限记录，人物与建筑位置对应，无横纵拉伸。|NOT_RUN|
|T091-C02|卧室转折|目标在墙后/楼下，正常实走门口、楼梯、侧门。|提示可解释实际入口，接近终点不指向错误高度层。|NOT_RUN|
|T091-C03|偏离返走|绕另一合法路径、返走、取消追踪再恢复。|不强迫过点，不累积旧节点，不改任务进度。|NOT_RUN|
|T091-C04|时间线|Load/阶段推进/领奖/合法传送后重新开图。|旧标记清除，新阶段目标正确，未知信息不泄漏。|NOT_RUN|
|T091-C05|传送|未激活/危险/伙伴条件不足及全部合法的目的地各测。|拒绝有原因，合法地图暂停不误挡，落点可走。|NOT_RUN|
|T091-C06|布局|最低/最高缩放、4:3/超宽/大字号、密集标签。|保持全屏矩形与比例，关键入口可识别且无深色留边。|NOT_RUN|
|T091-C07|无数据降级|缺失/坏路线条目或定位对象失效。|普通目标标记仍安全工作，无崩溃、不阻断任务。|NOT_RUN|


全局挂接索引：T-002, T-011, T-025。它们来自现有TEST_MATRIX；正式数值以已批准增量/ACCEPTANCE和本单细化步骤为准，不恢复历史灰盒规则。

本单需新增/复用定向原生测试；新前缀建议 `Hearthward.Iteration.Task091.`。先注册并核验找到用例数量>0，再按公开UEClient运行，不能拿本表局部ID当UE过滤器。正常输入、渲染、真实模型或真人用例另行执行。

公共检查：`python -X utf8 scripts/validate_repo.py`、`python -X utf8 -m unittest discover -s scripts/tests -v`、`git diff --check`；在包含获批本单快照的实际基线上运行 `python -X utf8 scripts/validate_repo.py --task TASK-091 --base <实际完整SHA>`。UE操作/证据要求见执行约定；缺环境则明确NOT_RUN，不能将文件编写完成等同测试通过。

## 8. 交付物与完成定义

- 关键入口/节点配置及权威ID映射
- 地图/指引实现与回归
- 正常输入路线录像、坐标/传送/缩放验证

固定收尾：更新`README.md`当前说明、同名MD/JSON与`docs/handoffs/TASK-091.md`；在`docs/qa/TASK-091/REPORT.md`绑定最终完整SHA、资产/模型指纹、环境、命令、逐用例结果和明确限制。该REPORT为未来执行产物，本包不预填。

工程实现完成、Owner视觉、真人体验、二机、性能与发布各自登记。依赖下游可用产物写入handoff，不把未验项目涂为PASS。没有授权时交付本地改动和证据并写“未提交/未推送/未发布”，不得自增权限。

## 9. 本单明确不做

- 改正式地图边界或任务触发/奖励
- 全地图地面导航线和室内分层系统
- 扩大传送权限、揭露未知剧情

## 10. 失败、范围变化和恢复

首先保留最短复现、受测状态和原始证据，再定位到本单或其他负责任务；不能通过清空存档/赠送物资/瞬移/改验收阈值绕过阻塞。回退只针对本单经核对的改动或资产备份，禁止未经授权reset/clean、覆盖他人工作和强制解锁。确需改变共享契约、保存格式、正式配表或包范围时提交具体差异与影响，先获准再施工。


---

<a id="task-092"></a>

# TASK-092｜首次救援往返路线与遭遇灰盒打磨

> 状态：Backlog（任务单已编写，实施未派发）。优先级：P0。阶段：C 地图与场景。日期：2026-10-06。Owner：XLingyyy。Reviewer／Issue：未指派。建议分支：`codex/TASK-092-rescue-route`（未创建）。

[本批总入口](docs/planning/TASK-084-103/README.md) · [执行约定](docs/planning/TASK-084-103/EXECUTION_GUIDE.md) · [元数据](docs/tasks/TASK-092.json) · [交接模板](docs/handoffs/TASK-092.md)

## 1. 目标与预期结果

以084冻结的既有首救路线为唯一对象，修复通行、侦察、撤退和救回后的返营链路，形成供美术替换的可玩的关卡样板。

## 2. 当前基础与事实边界

既有自然世界与049战役含任务、敌人、救援和回营逻辑；本单不重新设计奖励、驻军和人口规则。先查已有路段与遭遇，缺什么补什么。

本单参考基线为 `main@6fcf5c22e965f0f7409438f19bc7b09e96ffb058`；实际开工必须复核届时HEAD与依赖产物。这里不是本轮运行结果，旧报告PASS/FAIL均绑定原受测实现。新行为是本单计划，不能写成当前已经存在。

## 3. 依赖、开工门槛与最小必读

前置：[TASK-091](docs/tasks/TASK-091.md)

本单允许候选C++文件；地图资产和gameplay.json默认不开放。确需修改时以准确包/JSON指针追加范围，取得授权并在基线记录，不用绕改其他文件规避。

共同必读：`AGENTS.md`、`WORKFLOW.md`、`docs/START_HERE.md`、`docs/PROJECT_STATE.md`、`docs/design/CURRENT.md`及本单MD/JSON，然后执行本批指南中的接手/路径核对。

本单额外入口（路径为只读定位，不等于修改授权；前置任务产物需先实际生成）：

- `Source/Hearthward/Campaign/HearthwardCampaignWorld.cpp`
- `Source/Hearthward/Campaign/HearthwardCampaignActor.cpp`
- `Source/Hearthward/Campaign/HearthwardCampaignInteraction.cpp`
- `Source/Hearthward/Campaign/HearthwardCampaignQuests.cpp`
- `Resources/Data/gameplay.json`
- `docs/design/DSGN-003-first-release-slice.md`
- `docs/planning/TASK-084-103/ROUTE_MANIFEST.json`

## 4. 修改范围与协作边界

除本单MD/JSON、handoff、QA目录、专用规划目录和根`README.md`固定收尾外，候选范围如下；最终以**已获准基线JSON**为准。

- `Source/Hearthward/Campaign/HearthwardCampaignWorld.cpp`
- `Source/Hearthward/Campaign/HearthwardCampaignActor.cpp`
- `Source/Hearthward/Campaign/HearthwardCampaignInteraction.cpp`
- `Source/Hearthward/Campaign/HearthwardCampaignQuests.cpp`
- `docs/planning/TASK-084-103/ROUTE_MANIFEST.json`
- `Resources/Data/quest_guidance.json`
- `Source/Hearthward/Tests/RescueRouteTests.cpp`

本单不授予整个Content目录。准确包名单由现场引用/094清单确定，先补入已批准快照并核验LFS锁，才可编辑。未批准包仅可只读检查。

拟新增辅助/测试文件不表示仓库已存在；先搜索可复用类型。共享UI配置、公共头文件、WorldPresentation/CampaignWorld等按[写入调度](docs/planning/TASK-084-103/DISPATCH.md)串行。临时输出使用 `.agent-local/qa/TASK-092/<唯一run>/`，不得写入用户原档。

## 5. Agent具体实施步骤

### 1. 复现路线断点

重走084记录路线，列碰撞/坡度/门洞/导航/流送/触发五类缺口，标明受影响现有对象。检查实现是否运行时生成；可在生成代码修正的，不随意改主umap。

### 2. 画功能灰盒

在规划文件中标出营地出口、过渡段、可观察敌情位置、救援交互区、退出与返营接续；使用既有地形，不新开大地图。为每一处调整说明“为什么玩家要经过/能作何选择”。

### 3. 修实际通行

按当前角色与弟弟/获救对象胶囊、移动/攀越规则调整障碍、台阶和门洞；确保地面连续、落脚稳定、导航可达，避免只对玩家跳跃可过而跟随者过不去。更改碰撞包前完成逐包路径授权和LFS锁。

### 4. 打磨单处遭遇

在既有敌数、职业、警戒和增援规则内调整视线/掩体/入口；玩家能在被迫近战前观察，存在符合原规则的绕行/处理/撤退方式。不能为造选择关闭敌人AI、使其永不发现或直接减少正式驻军。

### 5. 救援与安全承接

验证解救、跟随/等待/再次接续、回到营地的人口增长与奖励；危险条件不允许解救/承接时原样提示。不能通过UI点击把被救人直接入库或空降营地。

### 6. 路标与非奖励内容

仅补充有用途的地标、观察点和环境线索，灰盒模型可复用；不凭空添加资源奖励/随机箱子/新敌人。与091同步入口节点坐标及任务图示，物理位置是唯一依据。

### 7. 反向与恢复测试

往返均实走，测试弟弟留营和同行两种已允许分工选择；分别在出发、救援后返营、到营前保存继续。测试区域卸载重载与旧任务进度，不复制敌人/族人或重复奖账本。

### 8. 冻结美术接入边界

输出通行走廊、角色净空、不可侵入触发区、碰撞/导航要求与测量截图，交给095—098；后续美术只替换表现不能偷改灰盒路线功能。

## 6. 不可破坏的规则

- 真实普通输入的完整路线是验收对象；注入位置/强制阶段只能另列诊断夹具。
- 若需要移动正式资源/敌人/任务坐标，先列gameplay.json精确JSON指针、规则影响和Owner批准，再扩allowed_paths；当前JSON未给整表写权。
- 新增/修改.umap和ExternalActors都需列准确包并取得锁，不授权整个Content/。

所有更改继续遵守只读显示、原事务执行、稳定ID、时间线隔离和不伪填验收规则。规则未明确的新玩法不硬编码；不删测试/吞错误/全仓重构来让当前检查通过。

## 7. 验收用例与执行方式

每条用例在隔离档/明确诊断夹具中建立前置，实际记录操作与断言。夹具、注入输入、真实OS键鼠、真实模型、真人样本分层统计；下面所有结果尚未运行。

|局部编号|场景|前置/具体操作|通过标准|初始结果|
|---|---|---|---|---|
|T092-C01|正向通行|玩家正常走营地到首救点，弟弟跟随。|无卡死/掉地/必须调试传送；弟弟自然到达。|NOT_RUN|
|T092-C02|观察与撤退|到观察点辨敌，再绕行或处理；进入风险后合法撤退。|存在可解释选择，警戒/追击遵守原规则。|NOT_RUN|
|T092-C03|救援对象|解救、等待、继续跟随、返营接续。|对象真实移动，安全条件正确，人口最多加一次。|NOT_RUN|
|T092-C04|伙伴留营|弟弟留在安全营地工作，玩家走允许的路线。|不强迫互斥任务并发；是否可完成按正式条件给反馈。|NOT_RUN|
|T092-C05|双向碰撞|按正常步行/冲刺/原有攀越回走门槛、坡与狭口。|无单向卡路，角色/弟弟/被救者均按其能力通过。|NOT_RUN|
|T092-C06|存读档流送|出发/救援后/到营前三节点保存继续，卸载重载区域。|任务/敌人/族人/掉落不重置套利，路线仍可走。|NOT_RUN|
|T092-C07|资料交接|核对美术净空图与导航配置。|所有受影响包/坐标/触发区可追溯，091显示对应实际位置。|NOT_RUN|


全局挂接索引：T-002, T-003, T-007, T-011, T-014。它们来自现有TEST_MATRIX；正式数值以已批准增量/ACCEPTANCE和本单细化步骤为准，不恢复历史灰盒规则。

本单需新增/复用定向原生测试；新前缀建议 `Hearthward.Iteration.Task092.`。先注册并核验找到用例数量>0，再按公开UEClient运行，不能拿本表局部ID当UE过滤器。正常输入、渲染、真实模型或真人用例另行执行。

公共检查：`python -X utf8 scripts/validate_repo.py`、`python -X utf8 -m unittest discover -s scripts/tests -v`、`git diff --check`；在包含获批本单快照的实际基线上运行 `python -X utf8 scripts/validate_repo.py --task TASK-092 --base <实际完整SHA>`。UE操作/证据要求见执行约定；缺环境则明确NOT_RUN，不能将文件编写完成等同测试通过。

## 8. 交付物与完成定义

- 路线灰盒方案、净空/碰撞/导航保护清单
- 限定范围的路线修复及测试
- 真实往返与救援成长证据、供美术接入的冻结清单

固定收尾：更新`README.md`当前说明、同名MD/JSON与`docs/handoffs/TASK-092.md`；在`docs/qa/TASK-092/REPORT.md`绑定最终完整SHA、资产/模型指纹、环境、命令、逐用例结果和明确限制。该REPORT为未来执行产物，本包不预填。

工程实现完成、Owner视觉、真人体验、二机、性能与发布各自登记。依赖下游可用产物写入handoff，不把未验项目涂为PASS。没有授权时交付本地改动和证据并写“未提交/未推送/未发布”，不得自增权限。

## 9. 本单明确不做

- 扩大世界面积、重配敌军总数/奖励/任务规则
- 新建营救剧情或随机事件系统
- 把一处路线测试当完整四区通关

## 10. 失败、范围变化和恢复

首先保留最短复现、受测状态和原始证据，再定位到本单或其他负责任务；不能通过清空存档/赠送物资/瞬移/改验收阈值绕过阻塞。回退只针对本单经核对的改动或资产备份，禁止未经授权reset/clean、覆盖他人工作和强制解锁。确需改变共享契约、保存格式、正式配表或包范围时提交具体差异与影响，先获准再施工。


---

<a id="task-093"></a>

# TASK-093｜营地准备、回营成长反馈与岗位可视状态

> 状态：Backlog（任务单已编写，实施未派发）。优先级：P1。阶段：C 地图与场景。日期：2026-10-06。Owner：XLingyyy。Reviewer／Issue：未指派。建议分支：`codex/TASK-093-camp-loop-feedback`（未创建）。

[本批总入口](docs/planning/TASK-084-103/README.md) · [执行约定](docs/planning/TASK-084-103/EXECUTION_GUIDE.md) · [元数据](docs/tasks/TASK-093.json) · [交接模板](docs/handoffs/TASK-093.md)

## 1. 目标与预期结果

把营地已有岗位、获救人口、真实入库和一次升阶串成可理解的准备/回营反馈；展示来自现有事件和状态，不增加经济模拟。

## 2. 当前基础与事实边界

营地两地共享仓储和等阶、建筑独立；族人后台生产，弟弟实际到岗。首切片需人口20→21、工作台I、营地S2，数值与解锁效果沿已批准数据。

本单参考基线为 `main@6fcf5c22e965f0f7409438f19bc7b09e96ffb058`；实际开工必须复核届时HEAD与依赖产物。这里不是本轮运行结果，旧报告PASS/FAIL均绑定原受测实现。新行为是本单计划，不能写成当前已经存在。

## 3. 依赖、开工门槛与最小必读

前置：[TASK-090](docs/tasks/TASK-090.md)、[TASK-092](docs/tasks/TASK-092.md)

CampSubsystem只允许增补缺失的只读/事件展示出口，不改生产计算；运行资产如需替换交由098，本单可先复用既有模型。

共同必读：`AGENTS.md`、`WORKFLOW.md`、`docs/START_HERE.md`、`docs/PROJECT_STATE.md`、`docs/design/CURRENT.md`及本单MD/JSON，然后执行本批指南中的接手/路径核对。

本单额外入口（路径为只读定位，不等于修改授权；前置任务产物需先实际生成）：

- `Source/Hearthward/Camp/HearthwardCampState.h`
- `Source/Hearthward/Camp/HearthwardCampSubsystem.h`
- `Source/Hearthward/Camp/HearthwardCampSubsystem.cpp`
- `Source/Hearthward/Campaign/HearthwardCampaignState.h`
- `Source/Hearthward/UI/HearthwardScreenCamp.cpp`
- `Source/Hearthward/Gameplay/HearthwardWorldPresentation.cpp`

## 4. 修改范围与协作边界

除本单MD/JSON、handoff、QA目录、专用规划目录和根`README.md`固定收尾外，候选范围如下；最终以**已获准基线JSON**为准。

- `Source/Hearthward/UI/HearthwardScreenCamp.cpp`
- `Source/Hearthward/UI/HearthwardScreenContent.cpp`
- `Source/Hearthward/UI/HearthwardScreenActions.cpp`
- `Source/Hearthward/UI/HearthwardScreenWidget.h`
- `Source/Hearthward/Camp/HearthwardCampSubsystem.h`
- `Source/Hearthward/Camp/HearthwardCampSubsystem.cpp`
- `Source/Hearthward/Gameplay/HearthwardWorldPresentation.cpp`
- `Source/Hearthward/Gameplay/HearthwardWorldPresentation.h`
- `Source/Hearthward/UI/HearthwardCampFeedback.cpp`
- `Source/Hearthward/UI/HearthwardCampFeedback.h`
- `Resources/UI/layout.json`
- `Resources/UI/interface.json`
- `Source/Hearthward/Tests/CampLoopFeedbackTests.cpp`

Content与Save等未授权领域只读；例外仅以JSON实际范围为准，不能用必读清单扩大写权限。

拟新增辅助/测试文件不表示仓库已存在；先搜索可复用类型。共享UI配置、公共头文件、WorldPresentation/CampaignWorld等按[写入调度](docs/planning/TASK-084-103/DISPATCH.md)串行。临时输出使用 `.agent-local/qa/TASK-093/<唯一run>/`，不得写入用户原档。

## 5. Agent具体实施步骤

### 1. 列明营地真值

映射人口、空闲/占岗人数、弟弟到岗、共享仓储、营地等阶、当地设施、升级材料及效果的读取与实际提交入口；两地共享字段不可在UI汇总时加两遍。

### 2. 升级缺口与收益

营地页清楚显示当前等阶、下一阶原有条件、缺口和已批准解锁效果。可跳到相关制作/分工页但不自动造设施/调人，升级执行仍由原事务检验并扣料一次。

### 3. 回营事实反馈

基于现有救援/入库/升阶成功回执展示“本次带回的人/实际入库/已经开放的能力”。没有可用回执时显示当前状态而非臆测本次变化；不能用库存净差冒充弟弟交付，因为同期有生产/消耗。

### 4. 去重与时间线

通知以当前epoch+已有事务/事件标识去重，Load后不重放奖励庆祝；不在本单加Save字段。无既有可持久回执支持时，本次行程摘要仅在当前会话有效，恢复后明确显示当前营地状态。

### 5. 岗位可视表现

为选定木材/石材岗位展示工作/暂停/待资源/无人状态；附近族人可复用已有模型做与岗位绑定的工作表现，身份/岗位计数来自现状。可见动画不参与产量计时，也不宣称独立搬运寻路。

### 6. 体现成长

升级后实际可用的设施/岗位/配方在页面清楚标识；与098将替换的设施外观对齐。新建装饰只做表现，不能成为未经批准的生产设施或增加劳动力。

### 7. 切片与两营地回归

验证第一次救援回营和第一次升级，之后去第二营地再开页；普通生产、暂停、睡眠8小时、刷新和保存继续前后各有快照，确保展示没有改变原结算。

## 6. 不可破坏的规则

- 不新增普通族人死亡/床位门槛、个人运输模拟、资源预留或两营地运输。
- 动画停播/区域卸载不得停止既有合法后台生产；视觉帧率不决定产量。
- 任意一条“新增人口/完成升级”通知必须有真实成功回执或已验证状态转移，不在通知中发奖励。

所有更改继续遵守只读显示、原事务执行、稳定ID、时间线隔离和不伪填验收规则。规则未明确的新玩法不硬编码；不删测试/吞错误/全仓重构来让当前检查通过。

## 7. 验收用例与执行方式

每条用例在隔离档/明确诊断夹具中建立前置，实际记录操作与断言。夹具、注入输入、真实OS键鼠、真实模型、真人样本分层统计；下面所有结果尚未运行。

|局部编号|场景|前置/具体操作|通过标准|初始结果|
|---|---|---|---|---|
|T093-C01|首救变化|真实带回一人后查看人口与岗位。|20→21一次；已救对象不能重复增加。|NOT_RUN|
|T093-C02|升级条件|材料不足、缺设施和全部满足，分别点击升级。|缺口/原因正确；满足时只扣一次并展示真实解锁。|NOT_RUN|
|T093-C03|交付口径|同时队伍生产、个人入库与玩家制造消耗。|摘要不把净库存差当某一行动成果，不重复显示归属。|NOT_RUN|
|T093-C04|岗位表现|有工人/暂停/缺资源/弟弟赶路与到岗。|工作表现与权威状态相符，不增减产量。|NOT_RUN|
|T093-C05|共享状态|两营地分别查看仓储/等阶/当地设施。|共享数据不双计，建筑保持各自状态。|NOT_RUN|
|T093-C06|休息卸载|睡眠/暂停/区域流送再返回岗位。|按既有世界时钟结算，表现不触发第二次生产。|NOT_RUN|
|T093-C07|Load反馈|保存后继续并重开营地页。|不重复庆祝或发奖；无会话来源的行程摘要不伪造。|NOT_RUN|


全局挂接索引：T-002, T-004, T-005, T-007, T-011, T-025。它们来自现有TEST_MATRIX；正式数值以已批准增量/ACCEPTANCE和本单细化步骤为准，不恢复历史灰盒规则。

本单需新增/复用定向原生测试；新前缀建议 `Hearthward.Iteration.Task093.`。先注册并核验找到用例数量>0，再按公开UEClient运行，不能拿本表局部ID当UE过滤器。正常输入、渲染、真实模型或真人用例另行执行。

公共检查：`python -X utf8 scripts/validate_repo.py`、`python -X utf8 -m unittest discover -s scripts/tests -v`、`git diff --check`；在包含获批本单快照的实际基线上运行 `python -X utf8 scripts/validate_repo.py --task TASK-093 --base <实际完整SHA>`。UE操作/证据要求见执行约定；缺环境则明确NOT_RUN，不能将文件编写完成等同测试通过。

## 8. 交付物与完成定义

- 营地状态/回执字段映射和显示规则
- 升级缺口、回营反馈与有限岗位表现代码
- 人口/材料/劳动力/两营地状态回归证据

固定收尾：更新`README.md`当前说明、同名MD/JSON与`docs/handoffs/TASK-093.md`；在`docs/qa/TASK-093/REPORT.md`绑定最终完整SHA、资产/模型指纹、环境、命令、逐用例结果和明确限制。该REPORT为未来执行产物，本包不预填。

工程实现完成、Owner视觉、真人体验、二机、性能与发布各自登记。依赖下游可用产物写入handoff，不把未验项目涂为PASS。没有授权时交付本地改动和证据并写“未提交/未推送/未发布”，不得自增权限。

## 9. 本单明确不做

- 改变八阶经济、劳动力上限、共享仓储或睡眠规则
- 全体族人独立AI与真实搬运
- 新增持久化行程账本

## 10. 失败、范围变化和恢复

首先保留最短复现、受测状态和原始证据，再定位到本单或其他负责任务；不能通过清空存档/赠送物资/瞬移/改验收阈值绕过阻塞。回退只针对本单经核对的改动或资产备份，禁止未经授权reset/clean、覆盖他人工作和强制解锁。确需改变共享契约、保存格式、正式配表或包范围时提交具体差异与影响，先获准再施工。


---

<a id="task-094"></a>

# TASK-094｜关键资产盘点、风格样板与逐包制作清单

> 状态：Backlog（任务单已编写，实施未派发）。优先级：P1。阶段：D 美术准备。日期：2026-10-06。Owner：XLingyyy。Reviewer／Issue：未指派。建议分支：`codex/TASK-094-asset-baseline`（未创建）。

[本批总入口](docs/planning/TASK-084-103/README.md) · [执行约定](docs/planning/TASK-084-103/EXECUTION_GUIDE.md) · [元数据](docs/tasks/TASK-094.json) · [交接模板](docs/handoffs/TASK-094.md)

## 1. 目标与预期结果

将已有070/077/051素材与本轮路线实际引用对齐，形成可直接交给资产Agent的逐件清单、可复用源、验收视角和精确包路径；先评审样板，再批量制作。

## 2. 当前基础与事实边界

已有070三组概念/样品、051动物模型动作、077石堡立面和大量制作源；资料入库不等于运行时使用或风格验收。070总体风格与077特定石堡方向同时保留，不用较早木构方向推翻石堡。

本单参考基线为 `main@6fcf5c22e965f0f7409438f19bc7b09e96ffb058`；实际开工必须复核届时HEAD与依赖产物。这里不是本轮运行结果，旧报告PASS/FAIL均绑定原受测实现。新行为是本单计划，不能写成当前已经存在。

## 3. 依赖、开工门槛与最小必读

前置：[TASK-084](docs/tasks/TASK-084.md)

可在085—093进行时并行盘点/制作源准备；修改Content和确认碰撞前必须等相关路线净空就绪。094完成表示清单和已批准样板可用，不代表全部资产已制作。

共同必读：`AGENTS.md`、`WORKFLOW.md`、`docs/START_HERE.md`、`docs/PROJECT_STATE.md`、`docs/design/CURRENT.md`及本单MD/JSON，然后执行本批指南中的接手/路径核对。

本单额外入口（路径为只读定位，不等于修改授权；前置任务产物需先实际生成）：

- `docs/planning/TASK-053-074/TASK-070.md`
- `docs/qa/TASK-077/REPORT.md`
- `art_source/README.md`
- `resourceSummary.md`
- `docs/REPOSITORY_LAYOUT.md`
- `docs/planning/TASK-084-103/ROUTE_MANIFEST.json`

## 4. 修改范围与协作边界

除本单MD/JSON、handoff、QA目录、专用规划目录和根`README.md`固定收尾外，候选范围如下；最终以**已获准基线JSON**为准。

- `art_source/TASK-094/`
- `docs/assets/TASK-094/`

本单不授予整个Content目录。准确包名单由现场引用/094清单确定，先补入已批准快照并核验LFS锁，才可编辑。未批准包仅可只读检查。

拟新增辅助/测试文件不表示仓库已存在；先搜索可复用类型。共享UI配置、公共头文件、WorldPresentation/CampaignWorld等按[写入调度](docs/planning/TASK-084-103/DISPATCH.md)串行。临时输出使用 `.agent-local/qa/TASK-094/<唯一run>/`，不得写入用户原档。

## 5. Agent具体实施步骤

### 1. 盘点真实引用

从角色/设施生成与Content引用查首线实际出现资产，和art_source制作源逐一配对；记录现有模型、材质、动作、图标、音效为正式/临时/未生产/未验收。不要按文件数推算完成率。

### 2. 建立逐件登记

每条列稳定资产ID、玩家用途、对应物品/角色/设施ID、P0/P1、当前运行包、可编辑源、拟替换/复用、新包路径、骨架或厘米尺度、材质/贴图、碰撞、LOD/Nanite策略、许可证据和负责人任务。资产尚未选定时明确UNRESOLVED，不假造存在的包路径。

### 3. 复用既有样板

先检查070已有角色/营地/地形三组样图与077石堡结果，不自动付费重生成。给出近/中/远景、昼/夜一致性对照，列保留/修改理由，未有Owner视觉结论的保持待审，不自批。

### 4. 明确风格与功能

继承木棕、石灰、草绿、朴素织物/皮革与局部工具金属；石堡按077。明确兄弟/族人/普通敌/射手/重型轮廓差异和交互设施用途。颜色是辅助，不靠名字标签区分全部角色。

### 5. 冻结路线保护区

使用092可得成果；092尚未完成时只做084路线资产盘点，净空项保持未锁定。关键门洞/台阶/工作位/战斗接触区必须在对应资产导入前补齐测量，禁止未实测就批量套模型。

### 6. 分配技术预算

依据084测量和已批准目标平台，提出每类贴图、材质槽、实例密度、骨架/动画开销及近景/远景策略；注明预算是制作约束提案，不是已测性能。高模或Unlit源单独验证，不能一律启用Nanite或一律降面。

### 7. 来源与恢复性

检查原作者/来源/许可文件/生成记录/使用限制和打包许可状态，UNKNOWN项不得默认可发行。已有源必须保留，重名不同版本记录指纹；不要把字体、模型权重、密钥和本机路径散入制作包。

### 8. 给后续精确工作单

将095角色动作、096石堡、097自然路线、098营地道具、099声音逐包分配；为每单提供拟变更包清单和LFS锁责任。写STYLE_REVIEW.md与ASSET_REGISTER.csv，Owner确认样板后只冻结已确认项。

## 6. 不可破坏的规则

- 不移动/删除既有素材，不从Resource旧目录恢复重复副本。
- 本单默认只读Content；新建样板源放art_source/TASK-094，未获许可/费用授权不调用付费服务。
- 资产Agent开工前必须把准确.uasset/.umap/ExternalActors路径追加到所属任务JSON，不能用整Content目录作默认许可。

所有更改继续遵守只读显示、原事务执行、稳定ID、时间线隔离和不伪填验收规则。规则未明确的新玩法不硬编码；不删测试/吞错误/全仓重构来让当前检查通过。

## 7. 验收用例与执行方式

每条用例在隔离档/明确诊断夹具中建立前置，实际记录操作与断言。夹具、注入输入、真实OS键鼠、真实模型、真人样本分层统计；下面所有结果尚未运行。

|局部编号|场景|前置/具体操作|通过标准|初始结果|
|---|---|---|---|---|
|T094-C01|引用配对|抽查兄弟、石堡、仓储、自然植被、动作各一条。|能从运行引用定位制作源和来源记录，反向也能说明是否实际使用。|NOT_RUN|
|T094-C02|来源状态|逐件检查许可与生成证据字段。|缺证据标UNKNOWN和发行阻塞，不写“已授权”空结论。|NOT_RUN|
|T094-C03|样板评审|对照现有070/077图与新比较页。|三类风格边界清楚，Owner结论与Agent技术结论分开。|NOT_RUN|
|T094-C04|包范围|检查095—099计划包与不同Agent责任。|无同包双写、无整Content许可，待确定项明确不能先改。|NOT_RUN|
|T094-C05|净空关联|核对关键通行资产与路线保护区。|未冻结净空的项目禁止进入碰撞替换；不阻挡无关源制作。|NOT_RUN|
|T094-C06|存储规范|检查原源/运行图/数据/临时输出路径。|符合art_source/Content/Resources分工，无重复根目录。|NOT_RUN|


全局挂接索引：T-002, T-003。它们来自现有TEST_MATRIX；正式数值以已批准增量/ACCEPTANCE和本单细化步骤为准，不恢复历史灰盒规则。

本单以调查/资料、实际运行或真人验证为主，不为凑测试数量编造原生用例；使用现有有效回归并注明执行层。

公共检查：`python -X utf8 scripts/validate_repo.py`、`python -X utf8 -m unittest discover -s scripts/tests -v`、`git diff --check`；在包含获批本单快照的实际基线上运行 `python -X utf8 scripts/validate_repo.py --task TASK-094 --base <实际完整SHA>`。UE操作/证据要求见执行约定；缺环境则明确NOT_RUN，不能将文件编写完成等同测试通过。

## 8. 交付物与完成定义

- ASSET_REGISTER.csv：逐件来源/运行包/技术与验收状态
- STYLE_REVIEW.md：沿用/修改的三组样板及Owner待审栏
- 每个资产任务的PACKAGES.json候选与锁/碰撞范围
- 技术预算、真实视角清单及未决问题

固定收尾：更新`README.md`当前说明、同名MD/JSON与`docs/handoffs/TASK-094.md`；在`docs/qa/TASK-094/REPORT.md`绑定最终完整SHA、资产/模型指纹、环境、命令、逐用例结果和明确限制。该REPORT为未来执行产物，本包不预填。

工程实现完成、Owner视觉、真人体验、二机、性能与发布各自登记。依赖下游可用产物写入handoff，不把未验项目涂为PASS。没有授权时交付本地改动和证据并写“未提交/未推送/未发布”，不得自增权限。

## 9. 本单明确不做

- 整仓素材搬家或清理历史失败证据
- 未经批准批量新生成/采购
- 以概念图、源预览冒充UE运行效果

## 10. 失败、范围变化和恢复

首先保留最短复现、受测状态和原始证据，再定位到本单或其他负责任务；不能通过清空存档/赠送物资/瞬移/改验收阈值绕过阻塞。回退只针对本单经核对的改动或资产备份，禁止未经授权reset/clean、覆盖他人工作和强制解锁。确需改变共享契约、保存格式、正式配表或包范围时提交具体差异与影响，先获准再施工。


---

<a id="task-095"></a>

# TASK-095｜兄弟与敌人辨识、武器持握和关键动作收尾

> 状态：Backlog（任务单已编写，实施未派发）。优先级：P1。阶段：D 关键资产。日期：2026-10-06。Owner：XLingyyy。Reviewer／Issue：未指派。建议分支：`codex/TASK-095-character-action-polish`（未创建）。

[本批总入口](docs/planning/TASK-084-103/README.md) · [执行约定](docs/planning/TASK-084-103/EXECUTION_GUIDE.md) · [元数据](docs/tasks/TASK-095.json) · [交接模板](docs/handoffs/TASK-095.md)

## 1. 目标与预期结果

提升首线近景角色、三类敌人轮廓和关键动作的可信度，重点修手持穿插、脚滑、动作与实际接触错位；不重做战斗数值或整个角色控制器。

## 2. 当前基础与事实边界

已有兄弟骨架、角色素材、055持握/动作改进和战斗有效时序；动作源存在不等于正常连续动作验收通过。正式ActionId与准备/有效/恢复规则继续沿用。

本单参考基线为 `main@6fcf5c22e965f0f7409438f19bc7b09e96ffb058`；实际开工必须复核届时HEAD与依赖产物。这里不是本轮运行结果，旧报告PASS/FAIL均绑定原受测实现。新行为是本单计划，不能写成当前已经存在。

## 3. 依赖、开工门槛与最小必读

前置：[TASK-092](docs/tasks/TASK-092.md)、[TASK-094](docs/tasks/TASK-094.md)

依赖的约定产物与实际实现SHA就绪、Owner派发且共享写窗口空闲后开始。依赖不要求伪改旧任务Done；读取其实际交接并核对采用版本。

共同必读：`AGENTS.md`、`WORKFLOW.md`、`docs/START_HERE.md`、`docs/PROJECT_STATE.md`、`docs/design/CURRENT.md`及本单MD/JSON，然后执行本批指南中的接手/路径核对。

本单额外入口（路径为只读定位，不等于修改授权；前置任务产物需先实际生成）：

- `docs/planning/TASK-053-074/TASK-070.md`
- `Source/Hearthward/Animation/`
- `Source/Hearthward/Experience/HearthwardPresentationComponent.cpp`
- `Source/Hearthward/Combat/`
- `Source/Hearthward/Gameplay/HearthwardEquipment.cpp`
- `art_source/TASK-055/`
- `docs/assets/TASK-094/ASSET_REGISTER.csv`

## 4. 修改范围与协作边界

除本单MD/JSON、handoff、QA目录、专用规划目录和根`README.md`固定收尾外，候选范围如下；最终以**已获准基线JSON**为准。

- `art_source/TASK-095/`
- `docs/assets/TASK-095/`
- `Source/Hearthward/Animation/`
- `Source/Hearthward/Experience/HearthwardPresentationComponent.cpp`
- `Source/Hearthward/Experience/HearthwardPresentationComponent.h`
- `Source/Hearthward/Gameplay/HearthwardEquipment.cpp`
- `Source/Hearthward/Tests/CharacterPresentationTests.cpp`

本单不授予整个Content目录。准确包名单由现场引用/094清单确定，先补入已批准快照并核验LFS锁，才可编辑。未批准包仅可只读检查。

拟新增辅助/测试文件不表示仓库已存在；先搜索可复用类型。共享UI配置、公共头文件、WorldPresentation/CampaignWorld等按[写入调度](docs/planning/TASK-084-103/DISPATCH.md)串行。临时输出使用 `.agent-local/qa/TASK-095/<唯一run>/`，不得写入用户原档。

## 5. Agent具体实施步骤

### 1. 锁定动作列表

按真实ActionId列首线必需：待机/行走/疾跑/转向、跳跃与已支持攀越、轻重攻击/拉弓射箭、受击/倒地/扶起、采集。保留已有动画绑定，缺陷逐条定位到具体骨骼、挂点、片段和时段。

### 2. 修轮廓与装备区分

保留兄弟已有身高/骨架比例，通过批准的服装/护具/色块区分主角、弟弟和族人；普通兵、射手弓箭袋、重型护具在中距离可辨。不得把保护对象换成敌军造型。

### 3. 逐武器校准

核对武器真实尺寸、握把枢轴、Socket、左右手关系，修穿手/漂浮/反握。先做一种实际首线武器样板，通过后按清单覆盖常用武器；保存原绑定和可回退源，不粗暴缩小所有武器掩盖持握问题。

### 4. 同步动作与权威事件

绘制准备—有效接触—恢复时间轴，动画/音效接口读取实际攻击/采集/扶起事件；不通过动画Notify另扣血/加物品。不能为观感擅改重击命中窗口、处决3秒、体力或救援规则。

### 5. 连续运动收尾

在原移动/负重/重伤速度下检查步幅、脚滑、坡地、停止转向和装卸武器过渡；不要用改变游戏速度匹配动画。骨架与动画兼容性先验，再批量重定向。

### 6. 回归战斗可读性

在普通/射手/重型的既有遭遇中观察起手、命中与受击区分；受击反馈只表现真实受击，未命中不得播放命中成功。玩家/弟弟倒地与动物死亡不能互串动作。

### 7. 逐包导入和运行

依094清单授权准确包、核验LFS锁，再通过公开UE工具导入；检查材质/骨架/绑定和Cook引用，录连续近/远景昼夜片段，不以静帧验动作。

### 8. 记录遗留与交接

仅对实际替换/修复动作标完成；其余临时动作保持登记。将稳定事件时序、接触点与音效挂接需求交099，供100/103正常战斗实玩验证。

## 6. 不可破坏的规则

- 不改Combat伤害/命中扇区/耐力/敌人血量；发现逻辑bug先登记并协调精确修复范围。
- 不自动重做303段动物动作，动物路线观感归097。
- Content包未追加到本单allowed_paths并取得锁前，只做源和技术调查；不能把写权限借给别的资产任务。

所有更改继续遵守只读显示、原事务执行、稳定ID、时间线隔离和不伪填验收规则。规则未明确的新玩法不硬编码；不删测试/吞错误/全仓重构来让当前检查通过。

## 7. 验收用例与执行方式

每条用例在隔离档/明确诊断夹具中建立前置，实际记录操作与断言。夹具、注入输入、真实OS键鼠、真实模型、真人样本分层统计；下面所有结果尚未运行。

|局部编号|场景|前置/具体操作|通过标准|初始结果|
|---|---|---|---|---|
|T095-C01|识别|同光照下近/中/远距离观察兄弟、族人与三类敌人。|轮廓/装备能区分角色职能，Owner视觉结论单列。|NOT_RUN|
|T095-C02|持握|逐件实际装备首线武器，移动、攻击、受击与卸下。|无明显浮握/反握/穿掌，武器挂点稳定。|NOT_RUN|
|T095-C03|时序|逐帧对比有效攻击/采集事件与可见接触。|动作和实际判定一致，不新增第二次伤害/奖励。|NOT_RUN|
|T095-C04|移动|空载/负重/重伤、坡面、疾跑停止、攀越。|不修改速度规则来过验收，脚滑/穿插缺陷逐条关闭或保留。|NOT_RUN|
|T095-C05|倒地救援|双方分别倒地、扶起、中断扶起。|播放正确动作，时长/血量结果沿原逻辑，无重复复活。|NOT_RUN|
|T095-C06|Cook绑定|独立开发/Shipping候选中加载角色与装备。|无丢材质/骨架/动画/编辑器专用引用。|NOT_RUN|
|T095-C07|连续观感|录制连续战斗而非摆拍，昼/夜和不同视距。|源预览与实机分开，尚未Owner验收的保持未验收。|NOT_RUN|


全局挂接索引：T-002, T-003, T-013。它们来自现有TEST_MATRIX；正式数值以已批准增量/ACCEPTANCE和本单细化步骤为准，不恢复历史灰盒规则。

本单需新增/复用定向原生测试；新前缀建议 `Hearthward.Iteration.Task095.`。先注册并核验找到用例数量>0，再按公开UEClient运行，不能拿本表局部ID当UE过滤器。正常输入、渲染、真实模型或真人用例另行执行。

公共检查：`python -X utf8 scripts/validate_repo.py`、`python -X utf8 -m unittest discover -s scripts/tests -v`、`git diff --check`；在包含获批本单快照的实际基线上运行 `python -X utf8 scripts/validate_repo.py --task TASK-095 --base <实际完整SHA>`。UE操作/证据要求见执行约定；缺环境则明确NOT_RUN，不能将文件编写完成等同测试通过。

## 8. 交付物与完成定义

- 角色/武器/动作逐件变更与绑定表、可编辑源
- 精确导入包/骨架/材质与LFS记录
- 关键ActionId时序对照、原生与实机连续录像

固定收尾：更新`README.md`当前说明、同名MD/JSON与`docs/handoffs/TASK-095.md`；在`docs/qa/TASK-095/REPORT.md`绑定最终完整SHA、资产/模型指纹、环境、命令、逐用例结果和明确限制。该REPORT为未来执行产物，本包不预填。

工程实现完成、Owner视觉、真人体验、二机、性能与发布各自登记。依赖下游可用产物写入handoff，不把未验项目涂为PASS。没有授权时交付本地改动和证据并写“未提交/未推送/未发布”，不得自增权限。

## 9. 本单明确不做

- 重做移动控制、修改战斗平衡或一对三指标
- 新敌种或新战斗招式系统
- 仅凭静态模型预览签动作验收

## 10. 失败、范围变化和恢复

首先保留最短复现、受测状态和原始证据，再定位到本单或其他负责任务；不能通过清空存档/赠送物资/瞬移/改验收阈值绕过阻塞。回退只针对本单经核对的改动或资产备份，禁止未经授权reset/clean、覆盖他人工作和强制解锁。确需改变共享契约、保存格式、正式配表或包范围时提交具体差异与影响，先获准再施工。


---

<a id="task-096"></a>

# TASK-096｜石堡开场必经区域、近景材质与夜袭氛围

> 状态：Backlog（任务单已编写，实施未派发）。优先级：P1。阶段：D 关键资产。日期：2026-10-06。Owner：XLingyyy。Reviewer／Issue：未指派。建议分支：`codex/TASK-096-stonehold-opening`（未创建）。

[本批总入口](docs/planning/TASK-084-103/README.md) · [执行约定](docs/planning/TASK-084-103/EXECUTION_GUIDE.md) · [元数据](docs/tasks/TASK-096.json) · [交接模板](docs/handoffs/TASK-096.md)

## 1. 目标与预期结果

把已经可走的石堡开场收尾为完整可读的卧室—回廊—楼梯—庭院—侧门体验，优先近景结构与有限夜袭表现，不做全城室内。

## 2. 当前基础与事实边界

TASK-077保留可运行布局与裁分立面，主体多为外观体；已有无光照源与世界时钟调色不能直接当作最终近景/火光验收。必须保留原可走路线和撤离规则。

本单参考基线为 `main@6fcf5c22e965f0f7409438f19bc7b09e96ffb058`；实际开工必须复核届时HEAD与依赖产物。这里不是本轮运行结果，旧报告PASS/FAIL均绑定原受测实现。新行为是本单计划，不能写成当前已经存在。

## 3. 依赖、开工门槛与最小必读

前置：[TASK-091](docs/tasks/TASK-091.md)、[TASK-094](docs/tasks/TASK-094.md)、[TASK-095](docs/tasks/TASK-095.md)

依赖的约定产物与实际实现SHA就绪、Owner派发且共享写窗口空闲后开始。依赖不要求伪改旧任务Done；读取其实际交接并核对采用版本。

共同必读：`AGENTS.md`、`WORKFLOW.md`、`docs/START_HERE.md`、`docs/PROJECT_STATE.md`、`docs/design/CURRENT.md`及本单MD/JSON，然后执行本批指南中的接手/路径核对。

本单额外入口（路径为只读定位，不等于修改授权；前置任务产物需先实际生成）：

- `docs/qa/TASK-077/REPORT.md`
- `art_source/TASK-077/README.md`
- `Source/Hearthward/Campaign/HearthwardCampaignWorld.cpp`
- `Source/Hearthward/Gameplay/HearthwardWorldPresentation.cpp`
- `Source/Hearthward/Experience/HearthwardPresentationComponent.cpp`
- `Resources/Data/quest_guidance.json`

## 4. 修改范围与协作边界

除本单MD/JSON、handoff、QA目录、专用规划目录和根`README.md`固定收尾外，候选范围如下；最终以**已获准基线JSON**为准。

- `art_source/TASK-096/`
- `docs/assets/TASK-096/`
- `Source/Hearthward/Campaign/HearthwardCampaignWorld.cpp`
- `Source/Hearthward/Gameplay/HearthwardWorldPresentation.cpp`
- `Source/Hearthward/Gameplay/HearthwardWorldPresentation.h`
- `Source/Hearthward/Experience/HearthwardPresentationComponent.cpp`
- `Source/Hearthward/Experience/HearthwardPresentationComponent.h`
- `Resources/Data/quest_guidance.json`
- `Source/Hearthward/Tests/StoneholdOpeningPolishTests.cpp`

本单不授予整个Content目录。准确包名单由现场引用/094清单确定，先补入已批准快照并核验LFS锁，才可编辑。未批准包仅可只读检查。

拟新增辅助/测试文件不表示仓库已存在；先搜索可复用类型。共享UI配置、公共头文件、WorldPresentation/CampaignWorld等按[写入调度](docs/planning/TASK-084-103/DISPATCH.md)串行。临时输出使用 `.agent-local/qa/TASK-096/<唯一run>/`，不得写入用户原档。

## 5. Agent具体实施步骤

### 1. 保护已有通路

记录077净空/门槛/台阶/碰撞和091关键节点，存好当前源与包指纹。判断每个面是否玩家可触达，区分近景游戏结构、远景外观和诊断试验网格，不把被拒整网格重新接回。

### 2. 优先结构收边

修卧室地墙交界、门框、回廊栏杆、楼梯、庭院基座和侧门等近景缺面/穿插/浮空；用清晰可控模块补背面/收边，简化碰撞继续独立于杂乱生成网格。

### 3. 处理材质差异

对无光照源先保留诊断对照，测试白天/20:00开局/火光下与原生结构的接缝。需要受光的可达近景按批准方向重建材质，不能只换shader就宣称PBR完成；远景保留源也需避免夜间不合逻辑发亮。

### 4. 补有限生活痕迹

仅在必经视线补床边、灯笼、木箱、织物等已批准风格道具，保证遗物包、门口和可交互对象区别清楚；摆件不遮路、不抢交互、不增加奖励容器。

### 5. 有限夜袭氛围

在已发生夜袭的正确阶段接入局部火光、烟尘和远处动静的表现；沿既有阶段重建/清理，Load不能重复叠加。不可用烟雾完全遮住入口/敌人，不新增伤害火区、强制镜头或新剧情目标。

### 6. 输入与时钟不变

新档仍按当前第1日20:00；无光照对照/临时日光只用于诊断，不写正式环境。对话和暂停仍按既有规则，天气/灯光表现不得改变权威感知亮度与生产难度规则。

### 7. 实走与Cook验证

通过逐包授权与锁后导入并在正式地图新档连续撤离，检查弟弟跟随、遗物拾取、日夜、存读档与独立Cook。补图注明诊断视角或真实游戏视角。

### 8. 保留未完成边界

未制作的主堡/石屋内室保持外观体并给合理视觉边界，不用假门诱导不存在的玩法。报告列已收尾的必经范围，不宣称整堡或整个夜袭演出完成。

## 6. 不可破坏的规则

- 不移动主线触发/撤离条件，不新增火焰伤害、强制过场或整城可探索内室。
- 任何新碰撞替换后重跑玩家/弟弟路线；不可用瞬移证明通行。
- 主umap/ExternalActors默认只读，确需编辑按准确包授权，不覆盖未保存编辑器。

所有更改继续遵守只读显示、原事务执行、稳定ID、时间线隔离和不伪填验收规则。规则未明确的新玩法不硬编码；不删测试/吞错误/全仓重构来让当前检查通过。

## 7. 验收用例与执行方式

每条用例在隔离档/明确诊断夹具中建立前置，实际记录操作与断言。夹具、注入输入、真实OS键鼠、真实模型、真人样本分层统计；下面所有结果尚未运行。

|局部编号|场景|前置/具体操作|通过标准|初始结果|
|---|---|---|---|---|
|T096-C01|新档撤离|真实键鼠从卧室取物、叫弟弟、下楼到侧门撤离。|全程自然通行和推进，导航不中断。|NOT_RUN|
|T096-C02|近景几何|沿玩家正常视点检查地墙、门框、栏杆、楼梯、基座。|无阻断通路的穿插/浮空/缺面，关键对象可识别。|NOT_RUN|
|T096-C03|昼夜材质|同视角对照白天、正式开局夜间和局部火光。|源与补面协调；临时Unlit/日光诊断不冒充正式效果。|NOT_RUN|
|T096-C04|表现生命周期|夜袭阶段进入/退出，Load、重开新游戏。|火烟正确重建和清理，不无限叠加音光/VFX。|NOT_RUN|
|T096-C05|可读与舒适|正常亮度/大字号/对话开启时观察出口和任务提示。|路面与入口可辨，烟火不遮关键动作/救援。|NOT_RUN|
|T096-C06|原规则|对比时钟、遗物数量、任务阶段及撤离前后状态。|无免费物资或新增危险判定，游戏规则不变。|NOT_RUN|
|T096-C07|独立包|Cook后正常启动并走同路线。|所有新模块/材质/效果可加载，无编辑器依赖。|NOT_RUN|


全局挂接索引：T-002, T-003, T-011, T-021。它们来自现有TEST_MATRIX；正式数值以已批准增量/ACCEPTANCE和本单细化步骤为准，不恢复历史灰盒规则。

本单需新增/复用定向原生测试；新前缀建议 `Hearthward.Iteration.Task096.`。先注册并核验找到用例数量>0，再按公开UEClient运行，不能拿本表局部ID当UE过滤器。正常输入、渲染、真实模型或真人用例另行执行。

公共检查：`python -X utf8 scripts/validate_repo.py`、`python -X utf8 -m unittest discover -s scripts/tests -v`、`git diff --check`；在包含获批本单快照的实际基线上运行 `python -X utf8 scripts/validate_repo.py --task TASK-096 --base <实际完整SHA>`。UE操作/证据要求见执行约定；缺环境则明确NOT_RUN，不能将文件编写完成等同测试通过。

## 8. 交付物与完成定义

- 近景模块/材质/有限夜袭表现清单与源
- 逐包范围/LFS/碰撞净空记录
- 连续撤离、昼夜与独立运行证据

固定收尾：更新`README.md`当前说明、同名MD/JSON与`docs/handoffs/TASK-096.md`；在`docs/qa/TASK-096/REPORT.md`绑定最终完整SHA、资产/模型指纹、环境、命令、逐用例结果和明确限制。该REPORT为未来执行产物，本包不预填。

工程实现完成、Owner视觉、真人体验、二机、性能与发布各自登记。依赖下游可用产物写入handoff，不把未验项目涂为PASS。没有授权时交付本地改动和证据并写“未提交/未推送/未发布”，不得自增权限。

## 9. 本单明确不做

- 整城内室、完整新电影过场或战斗剧情重写
- 整份Marble试验网格直接作为碰撞
- 把诊断Unlit视口当正式日夜材质

## 10. 失败、范围变化和恢复

首先保留最短复现、受测状态和原始证据，再定位到本单或其他负责任务；不能通过清空存档/赠送物资/瞬移/改验收阈值绕过阻塞。回退只针对本单经核对的改动或资产备份，禁止未经授权reset/clean、覆盖他人工作和强制解锁。确需改变共享契约、保存格式、正式配表或包范围时提交具体差异与影响，先获准再施工。


---

<a id="task-097"></a>

# TASK-097｜救援路线自然环境、生态外观与远近景收尾

> 状态：Backlog（任务单已编写，实施未派发）。优先级：P1。阶段：D 关键资产。日期：2026-10-06。Owner：XLingyyy。Reviewer／Issue：未指派。建议分支：`codex/TASK-097-nature-route-art`（未创建）。

[本批总入口](docs/planning/TASK-084-103/README.md) · [执行约定](docs/planning/TASK-084-103/EXECUTION_GUIDE.md) · [元数据](docs/tasks/TASK-097.json) · [交接模板](docs/handoffs/TASK-097.md)

## 1. 目标与预期结果

在冻结的可玩路线周边形成统一、可辨路的自然环境，收尾实际会看到的植被/地表/岩石及生态外观，控制实例与材质开销。

## 2. 当前基础与事实边界

自然地形与资源规则已有，来源素材高低模混杂；14套动物资产不是对正式15类物种规则的替代。按批准清单映射模型，不凭资产数量擅自增删物种。

本单参考基线为 `main@6fcf5c22e965f0f7409438f19bc7b09e96ffb058`；实际开工必须复核届时HEAD与依赖产物。这里不是本轮运行结果，旧报告PASS/FAIL均绑定原受测实现。新行为是本单计划，不能写成当前已经存在。

## 3. 依赖、开工门槛与最小必读

前置：[TASK-092](docs/tasks/TASK-092.md)、[TASK-094](docs/tasks/TASK-094.md)、[TASK-096](docs/tasks/TASK-096.md)

依赖的约定产物与实际实现SHA就绪、Owner派发且共享写窗口空闲后开始。依赖不要求伪改旧任务Done；读取其实际交接并核对采用版本。

共同必读：`AGENTS.md`、`WORKFLOW.md`、`docs/START_HERE.md`、`docs/PROJECT_STATE.md`、`docs/design/CURRENT.md`及本单MD/JSON，然后执行本批指南中的接手/路径核对。

本单额外入口（路径为只读定位，不等于修改授权；前置任务产物需先实际生成）：

- `docs/planning/TASK-053-074/TASK-070.md`
- `docs/design/DSGN-R15-nature-production.md`
- `art_source/TASK-051/制作说明.md`
- `Source/Hearthward/Gameplay/HearthwardWorldPresentation.cpp`
- `Resources/Data/animal_motion.json`
- `docs/assets/TASK-094/ASSET_REGISTER.csv`

## 4. 修改范围与协作边界

除本单MD/JSON、handoff、QA目录、专用规划目录和根`README.md`固定收尾外，候选范围如下；最终以**已获准基线JSON**为准。

- `art_source/TASK-097/`
- `docs/assets/TASK-097/`
- `Source/Hearthward/Gameplay/HearthwardWorldPresentation.cpp`
- `Source/Hearthward/Gameplay/HearthwardWorldPresentation.h`
- `Resources/Data/animal_motion.json`
- `Source/Hearthward/Tests/NatureRoutePresentationTests.cpp`

本单不授予整个Content目录。准确包名单由现场引用/094清单确定，先补入已批准快照并核验LFS锁，才可编辑。未批准包仅可只读检查。

拟新增辅助/测试文件不表示仓库已存在；先搜索可复用类型。共享UI配置、公共头文件、WorldPresentation/CampaignWorld等按[写入调度](docs/planning/TASK-084-103/DISPATCH.md)串行。临时输出使用 `.agent-local/qa/TASK-097/<唯一run>/`，不得写入用户原档。

## 5. Agent具体实施步骤

### 1. 限定自然样板范围

以092通路和视线包络为边界列林道、河岸、坡面、观察点和返程地标；先利用已有地形/植被，禁止为了“更丰富”全图随机散布。不同段落用地表/树群/岩石形态帮助辨路。

### 2. 整理小型复用套件

从已许可来源选择有限树/灌木/草/岩石/地表，统一尺度、枢轴、材质与平铺尺寸，修缺失贴图、法线和不合理高光；分离近景与远景用途，保留可编辑源与导出参数。

### 3. 沿路布景

主路留清楚脚下轮廓，分叉用地标区分，观察点保留敌人可见窗口；野外资源点/任务对象周围不种阻挡交互的草。不得移动资源刷新ID或新增奖励密度掩盖空旷。

### 4. 生态外观映射

只修路线实际出现物种/作物的TEMP外观和材质：野生/家畜可辨，已批准作物各生长阶段不只靠颜色。沿已有骨架和动作做必要修补，不新建物种/生成/逃跑速度/成熟或掉落规则。

### 5. LOD与碰撞

按094预算为高模、Masked植被和地表逐项选择LOD/Nanite/实例策略；草/普通灌木不增加无意义阻挡，树干岩石使用合理简化碰撞。原模型可作源，不等于适合高密度实例。

### 6. 流送与地表连续

测试路口/河岸/山坡进出分区、远景切换、材质尺度变化、水面接缝和导航烘焙。拟改分区包/HLOD须登记确切路径和锁；不一次性重建整个世界。

### 7. 局部性能与回退

同084/092路线相同设置记录替换前后帧时/内存/显存和镜头，先移除造成明显退化的本单新改动，不降全局画质。将必要最终热点交102；有可回退包指纹。

### 8. 实机交接

昼/夜实际往返、采集、观察动物和敌情，核对所有互动/逃离/任务点不被美术改变；将环境音源位置与材质接触类别交099。

## 6. 不可破坏的规则

- 不改动物概率、物种、资源/作物产量、刷新或采集点稳定ID。
- animal_motion.json只允许批准的缺失/错误表现绑定，不改行为速度或规则参数；准确JSON指针在激活范围记录。
- 主地图和分区包依然逐包授权；本单不是全图美术重建许可证。

所有更改继续遵守只读显示、原事务执行、稳定ID、时间线隔离和不伪填验收规则。规则未明确的新玩法不硬编码；不删测试/吞错误/全仓重构来让当前检查通过。

## 7. 验收用例与执行方式

每条用例在隔离档/明确诊断夹具中建立前置，实际记录操作与断言。夹具、注入输入、真实OS键鼠、真实模型、真人样本分层统计；下面所有结果尚未运行。

|局部编号|场景|前置/具体操作|通过标准|初始结果|
|---|---|---|---|---|
|T097-C01|路线辨识|无调试飞行走主路与分叉，昼/夜各一次。|地标辅助方向，关键入口/观察视线未被植被堵住。|NOT_RUN|
|T097-C02|交互通行|采集、接近救援对象、角色/弟弟往返窄段。|原交互/导航范围可用，装饰不抢目标。|NOT_RUN|
|T097-C03|自然阶段|观察路线物种与作物各真实阶段。|外形符合现有物种ID，成熟可辨，不改变成长/掉落。|NOT_RUN|
|T097-C04|LOD材质|正常移动从近到远、转镜、跨分区。|无严重跳变/贴图缺失/破面，问题逐件记录。|NOT_RUN|
|T097-C05|性能对照|同场景/输入/画质测替换前后。|提交原始帧时和资源占用，不以单截图或Editor帧率宣称全局达标。|NOT_RUN|
|T097-C06|持久恢复|采过资源后离区/读档再返回。|采集/刷新/动物状态沿原规则，布景不复制资源。|NOT_RUN|
|T097-C07|Cook与来源|独立包查看关键植被/岩石/生态模型，反查登记。|运行引用完整，制作源/许可/包路径可追溯。|NOT_RUN|


全局挂接索引：T-002, T-003, T-011。它们来自现有TEST_MATRIX；正式数值以已批准增量/ACCEPTANCE和本单细化步骤为准，不恢复历史灰盒规则。

本单需新增/复用定向原生测试；新前缀建议 `Hearthward.Iteration.Task097.`。先注册并核验找到用例数量>0，再按公开UEClient运行，不能拿本表局部ID当UE过滤器。正常输入、渲染、真实模型或真人用例另行执行。

公共检查：`python -X utf8 scripts/validate_repo.py`、`python -X utf8 -m unittest discover -s scripts/tests -v`、`git diff --check`；在包含获批本单快照的实际基线上运行 `python -X utf8 scripts/validate_repo.py --task TASK-097 --base <实际完整SHA>`。UE操作/证据要求见执行约定；缺环境则明确NOT_RUN，不能将文件编写完成等同测试通过。

## 8. 交付物与完成定义

- 自然套件、逐物种/作物表现映射及制作源
- 限定路线布景包、碰撞/LOD/材质记录
- 路线昼夜、流送、互动与局部性能对照

固定收尾：更新`README.md`当前说明、同名MD/JSON与`docs/handoffs/TASK-097.md`；在`docs/qa/TASK-097/REPORT.md`绑定最终完整SHA、资产/模型指纹、环境、命令、逐用例结果和明确限制。该REPORT为未来执行产物，本包不预填。

工程实现完成、Owner视觉、真人体验、二机、性能与发布各自登记。依赖下游可用产物写入handoff，不把未验项目涂为PASS。没有授权时交付本地改动和证据并写“未提交/未推送/未发布”，不得自增权限。

## 9. 本单明确不做

- 扩大地图面积、随机散布新资源和怪物
- 全量重做动物动作或新生态模拟
- 降低全局目标画质来掩盖资产开销

## 10. 失败、范围变化和恢复

首先保留最短复现、受测状态和原始证据，再定位到本单或其他负责任务；不能通过清空存档/赠送物资/瞬移/改验收阈值绕过阻塞。回退只针对本单经核对的改动或资产备份，禁止未经授权reset/clean、覆盖他人工作和强制解锁。确需改变共享契约、保存格式、正式配表或包范围时提交具体差异与影响，先获准再施工。


---

<a id="task-098"></a>

# TASK-098｜营地核心设施、道具与关键图标统一

> 状态：Backlog（任务单已编写，实施未派发）。优先级：P1。阶段：D 关键资产。日期：2026-10-06。Owner：XLingyyy。Reviewer／Issue：未指派。建议分支：`codex/TASK-098-camp-props-icons`（未创建）。

[本批总入口](docs/planning/TASK-084-103/README.md) · [执行约定](docs/planning/TASK-084-103/EXECUTION_GUIDE.md) · [元数据](docs/tasks/TASK-098.json) · [交接模板](docs/handoffs/TASK-098.md)

## 1. 目标与预期结果

让玩家一眼理解营地仓储、制作、休息和生产岗位的用途，收尾首线所需设施与图标；设施模型替换不改变建造、生产和保存逻辑。

## 2. 当前基础与事实边界

营地设施/仓储/制造/维修已经存在；本单沿用真实设施ID、等阶、配方、交互范围与升级效果，配合093显示状态。

本单参考基线为 `main@6fcf5c22e965f0f7409438f19bc7b09e96ffb058`；实际开工必须复核届时HEAD与依赖产物。这里不是本轮运行结果，旧报告PASS/FAIL均绑定原受测实现。新行为是本单计划，不能写成当前已经存在。

## 3. 依赖、开工门槛与最小必读

前置：[TASK-093](docs/tasks/TASK-093.md)、[TASK-094](docs/tasks/TASK-094.md)、[TASK-097](docs/tasks/TASK-097.md)

依赖的约定产物与实际实现SHA就绪、Owner派发且共享写窗口空闲后开始。依赖不要求伪改旧任务Done；读取其实际交接并核对采用版本。

共同必读：`AGENTS.md`、`WORKFLOW.md`、`docs/START_HERE.md`、`docs/PROJECT_STATE.md`、`docs/design/CURRENT.md`及本单MD/JSON，然后执行本批指南中的接手/路径核对。

本单额外入口（路径为只读定位，不等于修改授权；前置任务产物需先实际生成）：

- `docs/assets/TASK-094/ASSET_REGISTER.csv`
- `Source/Hearthward/Gameplay/HearthwardWorldPresentation.cpp`
- `Source/Hearthward/UI/HearthwardScreenCamp.cpp`
- `Resources/UI/interface.json`
- `Resources/UI/art-provenance.json`
- `Source/Hearthward/Building/`

## 4. 修改范围与协作边界

除本单MD/JSON、handoff、QA目录、专用规划目录和根`README.md`固定收尾外，候选范围如下；最终以**已获准基线JSON**为准。

- `art_source/TASK-098/`
- `docs/assets/TASK-098/`
- `Source/Hearthward/Gameplay/HearthwardWorldPresentation.cpp`
- `Source/Hearthward/Gameplay/HearthwardWorldPresentation.h`
- `Resources/UI/Art/TASK-098/`
- `Resources/UI/interface.json`
- `Resources/UI/art-provenance.json`
- `Source/Hearthward/Tests/CampPropsPresentationTests.cpp`

本单不授予整个Content目录。准确包名单由现场引用/094清单确定，先补入已批准快照并核验LFS锁，才可编辑。未批准包仅可只读检查。

拟新增辅助/测试文件不表示仓库已存在；先搜索可复用类型。共享UI配置、公共头文件、WorldPresentation/CampaignWorld等按[写入调度](docs/planning/TASK-084-103/DISPATCH.md)串行。临时输出使用 `.agent-local/qa/TASK-098/<唯一run>/`，不得写入用户原档。

## 5. Agent具体实施步骤

### 1. 冻结设施清单

首批列火堆、共享仓储入口、工作台、床/休息点、木材/石材岗位和首救成长实际用到的设施；再根据当前解锁覆盖炉窑/锻台/炊具。每件绑定已有设施ID，不把新外观当新功能。

### 2. 制作用途可辨的外形

用已批准石木、织物、工具和朴素结构表现用途，容器开口/工作面/床面/炉口方向清楚；工作中/暂停/缺材料的差别与093真值对应，避免所有设施只是一张桌子换标签。

### 3. 尺寸和工作位

测玩家/弟弟胶囊、交互中心、工具接触点和脚下净空；模型不能挡原E/R交互、出入口或工作位。替换碰撞后重跑建造摆放、到岗、存取、制作/维修与休息。

### 4. 保留升级语义

按原等阶状态选用允许的外观差异；未建造不能只因升级就显示可用设备，另一营地也不能凭共享等阶自动复制建筑。没有批准的新资产阶段则先用可辨的既有表现。

### 5. 统一关键图标

覆盖本批相关物品/配方/设施/状态/地图符号，复用已有图标ID和图集布局，新增映射仅指向许可清晰的图像。常用尺寸清楚，透明边正确，不能靠文字/颜色独自区分。不得共享或重新打包字体文件。

### 6. 图标接入与回退

修改interface/provenance精确键，测试高DPI和大字号下图标与文字，缺图给现有合理fallback。替换图集注意UV/裁切，不能重排导致所有旧图标串位。

### 7. 逐包实机验证

按094名单授权包/LFS，导入后在正常营地执行真实操作和一次升级，再保存独立重启；采样昼夜/近远景及当地设施差别。与099交接合法音源/接触点。

## 6. 不可破坏的规则

- 建造成本、设施权限/产量、碰撞净空和实际ID不被美术重定义。
- 两营地共享等阶/仓储，建筑各自存在；外观生成不能造成复制建筑或劳动力。
- 不恢复旧全屏装饰、火红底或宣传Logo；只补支持当前简约风格的资产。

所有更改继续遵守只读显示、原事务执行、稳定ID、时间线隔离和不伪填验收规则。规则未明确的新玩法不硬编码；不删测试/吞错误/全仓重构来让当前检查通过。

## 7. 验收用例与执行方式

每条用例在隔离档/明确诊断夹具中建立前置，实际记录操作与断言。夹具、注入输入、真实OS键鼠、真实模型、真人样本分层统计；下面所有结果尚未运行。

|局部编号|场景|前置/具体操作|通过标准|初始结果|
|---|---|---|---|---|
|T098-C01|用途辨识|正常营地看火/箱/工作台/床/岗位及已解锁设施。|形态与用途明确，关键交互点易找到。|NOT_RUN|
|T098-C02|实际交互|存取、制作、维修、休息、弟弟到岗各执行一次。|不因模型/碰撞替换失效；结果沿原事务。|NOT_RUN|
|T098-C03|升级外观|升级前后、未建造设施、第二营地分别检查。|只表现真实状态，不生成无成本建筑或重复岗位。|NOT_RUN|
|T098-C04|图标映射|逐个打开背包/制作/营地/地图相关图标。|正确物品/设施不串图；缺图fallback不崩溃。|NOT_RUN|
|T098-C05|UI缩放|默认/150%、小窗口/超宽与长文本。|图标边缘/透明/裁切正确，不挤压关键信息。|NOT_RUN|
|T098-C06|保存与Cook|保存重启并在独立包操作同设施。|引用无缺失，建筑/材料/布局保持，源资产可追溯。|NOT_RUN|


全局挂接索引：T-002, T-003, T-004, T-005, T-011。它们来自现有TEST_MATRIX；正式数值以已批准增量/ACCEPTANCE和本单细化步骤为准，不恢复历史灰盒规则。

本单需新增/复用定向原生测试；新前缀建议 `Hearthward.Iteration.Task098.`。先注册并核验找到用例数量>0，再按公开UEClient运行，不能拿本表局部ID当UE过滤器。正常输入、渲染、真实模型或真人用例另行执行。

公共检查：`python -X utf8 scripts/validate_repo.py`、`python -X utf8 -m unittest discover -s scripts/tests -v`、`git diff --check`；在包含获批本单快照的实际基线上运行 `python -X utf8 scripts/validate_repo.py --task TASK-098 --base <实际完整SHA>`。UE操作/证据要求见执行约定；缺环境则明确NOT_RUN，不能将文件编写完成等同测试通过。

## 8. 交付物与完成定义

- 设施/道具/图标制作源、逐包与provenance清单
- 交互尺寸/碰撞/工作位测量
- 真实操作、升级/两营地、UI缩放和Cook证据

固定收尾：更新`README.md`当前说明、同名MD/JSON与`docs/handoffs/TASK-098.md`；在`docs/qa/TASK-098/REPORT.md`绑定最终完整SHA、资产/模型指纹、环境、命令、逐用例结果和明确限制。该REPORT为未来执行产物，本包不预填。

工程实现完成、Owner视觉、真人体验、二机、性能与发布各自登记。依赖下游可用产物写入handoff，不把未验项目涂为PASS。没有授权时交付本地改动和证据并写“未提交/未推送/未发布”，不得自增权限。

## 9. 本单明确不做

- 新增设施规则、经济平衡和全体族人独立行为
- 重新设计整套UI或更换字体
- 以纯渲染图替代实际设施交互验收

## 10. 失败、范围变化和恢复

首先保留最短复现、受测状态和原始证据，再定位到本单或其他负责任务；不能通过清空存档/赠送物资/瞬移/改验收阈值绕过阻塞。回退只针对本单经核对的改动或资产备份，禁止未经授权reset/clean、覆盖他人工作和强制解锁。确需改变共享契约、保存格式、正式配表或包范围时提交具体差异与影响，先获准再施工。


---

<a id="task-099"></a>

# TASK-099｜动作音效、环境声与事件反馈同步

> 状态：Backlog（任务单已编写，实施未派发）。优先级：P1。阶段：D 关键资产。日期：2026-10-06。Owner：XLingyyy。Reviewer／Issue：未指派。建议分支：`codex/TASK-099-audio-feedback`（未创建）。

[本批总入口](docs/planning/TASK-084-103/README.md) · [执行约定](docs/planning/TASK-084-103/EXECUTION_GUIDE.md) · [元数据](docs/tasks/TASK-099.json) · [交接模板](docs/handoffs/TASK-099.md)

## 1. 目标与预期结果

补齐首线与已打磨场景的脚步/接触/命中/采集/环境声，使声音来自真实事件并可正确暂停、清理和混音；固定录音继续保持原暂缓决定。

## 2. 当前基础与事实边界

固定对白有已批准清单但录音未生产，动态弟弟回复仅文字。070明确本轮不录制、不用AI代替固定人声；不能用静音文件完成覆盖率。

本单参考基线为 `main@6fcf5c22e965f0f7409438f19bc7b09e96ffb058`；实际开工必须复核届时HEAD与依赖产物。这里不是本轮运行结果，旧报告PASS/FAIL均绑定原受测实现。新行为是本单计划，不能写成当前已经存在。

## 3. 依赖、开工门槛与最小必读

前置：[TASK-095](docs/tasks/TASK-095.md)、[TASK-096](docs/tasks/TASK-096.md)、[TASK-097](docs/tasks/TASK-097.md)、[TASK-098](docs/tasks/TASK-098.md)

依赖的约定产物与实际实现SHA就绪、Owner派发且共享写窗口空闲后开始。依赖不要求伪改旧任务Done；读取其实际交接并核对采用版本。

共同必读：`AGENTS.md`、`WORKFLOW.md`、`docs/START_HERE.md`、`docs/PROJECT_STATE.md`、`docs/design/CURRENT.md`及本单MD/JSON，然后执行本批指南中的接手/路径核对。

本单额外入口（路径为只读定位，不等于修改授权；前置任务产物需先实际生成）：

- `Source/Hearthward/Experience/HearthwardPresentationComponent.cpp`
- `Source/Hearthward/Experience/HearthwardPresentationComponent.h`
- `Source/Hearthward/Gameplay/HearthwardWorldPresentation.cpp`
- `Resources/Data/experience.json`
- `Resources/Audio/`
- `docs/planning/TASK-053-074/TASK-070.md`

## 4. 修改范围与协作边界

除本单MD/JSON、handoff、QA目录、专用规划目录和根`README.md`固定收尾外，候选范围如下；最终以**已获准基线JSON**为准。

- `art_source/TASK-099/`
- `docs/assets/TASK-099/`
- `Resources/Audio/TASK-099/`
- `Source/Hearthward/Experience/HearthwardPresentationComponent.cpp`
- `Source/Hearthward/Experience/HearthwardPresentationComponent.h`
- `Source/Hearthward/Gameplay/HearthwardWorldPresentation.cpp`
- `Resources/Data/experience.json`
- `Source/Hearthward/Tests/AudioFeedbackLifecycleTests.cpp`

本单不授予整个Content目录。准确包名单由现场引用/094清单确定，先补入已批准快照并核验LFS锁，才可编辑。未批准包仅可只读检查。

拟新增辅助/测试文件不表示仓库已存在；先搜索可复用类型。共享UI配置、公共头文件、WorldPresentation/CampaignWorld等按[写入调度](docs/planning/TASK-084-103/DISPATCH.md)串行。临时输出使用 `.agent-local/qa/TASK-099/<唯一run>/`，不得写入用户原档。

## 5. Agent具体实施步骤

### 1. 建立事件声音表

从现有Presentation入口及095动作时间轴列脚步材质、挥动、真实命中、受击、格挡、采集接触/完成、救援/升级提示、火/水/风等环境事件；每条记录触发源、位置、去重键、停止条件和素材状态。

### 2. 准备合法音源

优先复用已有来源可查素材，必要编辑裁切/降噪/循环点并保留源。未知许可或付费来源先登记阻塞，不下载未授权素材。固定voice条目继续UNPRODUCED，动态回复不合成配音。

### 3. 接触与结算分离

挥动/脚步可在实际动画接触点触发，命中/采集成功/升级成功必须来自真实成功事件。播放声音不能再执行伤害、消耗或发奖；取消动作不能补播“已完成”。

### 4. 去重不吞合法声音

用现有事务/事件ID或动作实例区分，同一成功回执重复处理只播一次；两次不同合法攻击可各播一次。不能用全局一秒节流掩盖重复回调、吞掉所有快速动作。

### 5. 环境生命周期

营地火/河水等声源按当前位置和已加载场景建销，离区、Load、结束夜袭和退出游戏清理。睡眠跳时不连播经过八小时的历史脚步/生产提示；新阶段只重建当前循环声。

### 6. 混音与设置

遵守现有主音量/分类设置/暂停例外，测试近远衰减和循环接缝。环境声不长期盖住关键命中/警戒反馈；不新增未设计的听声AI感知半径或天气难度。

### 7. 字幕与无声可玩

已确认固定台词仍有文字，未产出配音的状态如实保留；重要拒绝/完成反馈不能只有声音。可访问文本与真实事件一致，不写新剧情。

### 8. 实听与独立包

正常输入走序章、营地、采集、遭遇、救援返回，实际录带音轨片段并试听；仅波形/文件存在检查不能算声音体验通过。验证Cook能找到非UE运行音频及许可证随包需要。

## 6. 不可破坏的规则

- 不录固定人声、不用AI声替代、不给动态弟弟加TTS，除非Owner另有明确新决定。
- experience.json仅允许本单音效/环境事件映射；不得修改固定文本、voice生产状态或玩法参数来凑覆盖率。
- 音效触发读取事实，不产生事实；暂停、读档、区域卸载不能留下重复循环声源。

所有更改继续遵守只读显示、原事务执行、稳定ID、时间线隔离和不伪填验收规则。规则未明确的新玩法不硬编码；不删测试/吞错误/全仓重构来让当前检查通过。

## 7. 验收用例与执行方式

每条用例在隔离档/明确诊断夹具中建立前置，实际记录操作与断言。夹具、注入输入、真实OS键鼠、真实模型、真人样本分层统计；下面所有结果尚未运行。

|局部编号|场景|前置/具体操作|通过标准|初始结果|
|---|---|---|---|---|
|T099-C01|事件同步|真实挥击空中、命中、受击、采集成功/中断分别操作。|只有正确事件播对应声，不未命中播成功、不取消播完成。|NOT_RUN|
|T099-C02|去重|重复同一事务回执和连续两次不同合法动作。|前者一次，后者各一次，不以粗暴节流隐藏错误。|NOT_RUN|
|T099-C03|材质位置|走石/木/土等已支持表面，靠近/远离火水。|来源位置、接触类别与衰减合理，无明显循环接缝。|NOT_RUN|
|T099-C04|音量暂停|调主音量/既有分类，暂停/对话/恢复。|遵守设置及现有世界运行例外，恢复无音量突变叠声。|NOT_RUN|
|T099-C05|生命周期|区域卸载、睡眠、Load、重开新游戏。|当前音源数量有界，不重放历史事件或残留旧循环。|NOT_RUN|
|T099-C06|无配音状态|固定台词与动态回复在无录音时查看。|文字可用，voice仍未生产，不以静音文件冒充完成。|NOT_RUN|
|T099-C07|实听Cook|独立包录制带音轨正常路线并实际试听。|音效资源完整，主观试听与技术检查分别记录。|NOT_RUN|


全局挂接索引：T-002, T-003, T-011, T-025。它们来自现有TEST_MATRIX；正式数值以已批准增量/ACCEPTANCE和本单细化步骤为准，不恢复历史灰盒规则。

本单需新增/复用定向原生测试；新前缀建议 `Hearthward.Iteration.Task099.`。先注册并核验找到用例数量>0，再按公开UEClient运行，不能拿本表局部ID当UE过滤器。正常输入、渲染、真实模型或真人用例另行执行。

公共检查：`python -X utf8 scripts/validate_repo.py`、`python -X utf8 -m unittest discover -s scripts/tests -v`、`git diff --check`；在包含获批本单快照的实际基线上运行 `python -X utf8 scripts/validate_repo.py --task TASK-099 --base <实际完整SHA>`。UE操作/证据要求见执行约定；缺环境则明确NOT_RUN，不能将文件编写完成等同测试通过。

## 8. 交付物与完成定义

- 事件—声音映射、来源许可及可编辑音频源
- 音效生命周期/去重测试
- 带音轨的正常流程证据、实际试听记录与未生产录音清单

固定收尾：更新`README.md`当前说明、同名MD/JSON与`docs/handoffs/TASK-099.md`；在`docs/qa/TASK-099/REPORT.md`绑定最终完整SHA、资产/模型指纹、环境、命令、逐用例结果和明确限制。该REPORT为未来执行产物，本包不预填。

工程实现完成、Owner视觉、真人体验、二机、性能与发布各自登记。依赖下游可用产物写入handoff，不把未验项目涂为PASS。没有授权时交付本地改动和证据并写“未提交/未推送/未发布”，不得自增权限。

## 9. 本单明确不做

- 固定人声录制/TTS、重写剧情和音乐系统大改
- 声音触发伤害/奖励或改变AI听觉规则
- 用空音频/静帧冒充完整音频验收

## 10. 失败、范围变化和恢复

首先保留最短复现、受测状态和原始证据，再定位到本单或其他负责任务；不能通过清空存档/赠送物资/瞬移/改验收阈值绕过阻塞。回退只针对本单经核对的改动或资产备份，禁止未经授权reset/clean、覆盖他人工作和强制解锁。确需改变共享契约、保存格式、正式配表或包范围时提交具体差异与影响，先获准再施工。


---

<a id="task-100"></a>

# TASK-100｜四区差异化内容与首版全流程接续

> 状态：Backlog（任务单已编写，实施未派发）。优先级：P0。阶段：E 完整流程。日期：2026-10-06。Owner：XLingyyy。Reviewer／Issue：未指派。建议分支：`codex/TASK-100-campaign-content-completion`（未创建）。

[本批总入口](docs/planning/TASK-084-103/README.md) · [执行约定](docs/planning/TASK-084-103/EXECUTION_GUIDE.md) · [元数据](docs/tasks/TASK-100.json) · [交接模板](docs/handoffs/TASK-100.md)

## 1. 目标与预期结果

在现有主支线、驻军、控制区和第二营地规则内，把已验证首线延伸到永久夺回与通关后继续，补实际内容和空间表现缺口；不另起战争系统。

## 2. 当前基础与事实边界

首版边界约十小时、四区、80基础驻军、最多救回10人、第二营地使用已批准；主支线/增援/胜利规则以当前gameplay.json和049设计为准。旧脚本逐点传送测试不等于正常全通关。

本单参考基线为 `main@6fcf5c22e965f0f7409438f19bc7b09e96ffb058`；实际开工必须复核届时HEAD与依赖产物。这里不是本轮运行结果，旧报告PASS/FAIL均绑定原受测实现。新行为是本单计划，不能写成当前已经存在。

## 3. 依赖、开工门槛与最小必读

前置：[TASK-087](docs/tasks/TASK-087.md)、[TASK-090](docs/tasks/TASK-090.md)、[TASK-091](docs/tasks/TASK-091.md)、[TASK-092](docs/tasks/TASK-092.md)、[TASK-093](docs/tasks/TASK-093.md)、[TASK-095](docs/tasks/TASK-095.md)、[TASK-096](docs/tasks/TASK-096.md)、[TASK-097](docs/tasks/TASK-097.md)、[TASK-098](docs/tasks/TASK-098.md)、[TASK-099](docs/tasks/TASK-099.md)

依赖的约定产物与实际实现SHA就绪、Owner派发且共享写窗口空闲后开始。依赖不要求伪改旧任务Done；读取其实际交接并核对采用版本。

共同必读：`AGENTS.md`、`WORKFLOW.md`、`docs/START_HERE.md`、`docs/PROJECT_STATE.md`、`docs/design/CURRENT.md`及本单MD/JSON，然后执行本批指南中的接手/路径核对。

本单额外入口（路径为只读定位，不等于修改授权；前置任务产物需先实际生成）：

- `Source/Hearthward/Campaign/HearthwardCampaignState.cpp`
- `Source/Hearthward/Campaign/HearthwardCampaignWorld.cpp`
- `Source/Hearthward/Campaign/HearthwardCampaignQuests.cpp`
- `Source/Hearthward/Campaign/HearthwardCampaignInteraction.cpp`
- `Resources/Data/gameplay.json`
- `docs/design/CURRENT.md`
- `docs/planning/TASK-053-074/ACCEPTANCE.md`

## 4. 修改范围与协作边界

除本单MD/JSON、handoff、QA目录、专用规划目录和根`README.md`固定收尾外，候选范围如下；最终以**已获准基线JSON**为准。

- `Source/Hearthward/Campaign/HearthwardCampaignWorld.cpp`
- `Source/Hearthward/Campaign/HearthwardCampaignActor.cpp`
- `Source/Hearthward/Campaign/HearthwardCampaignQuests.cpp`
- `Source/Hearthward/Campaign/HearthwardCampaignInteraction.cpp`
- `Source/Hearthward/UI/HearthwardScreenContent.cpp`
- `Resources/Data/quest_guidance.json`
- `docs/assets/TASK-100/`
- `Source/Hearthward/Tests/CampaignCompletionRouteTests.cpp`

本单不授予整个Content目录。准确包名单由现场引用/094清单确定，先补入已批准快照并核验LFS锁，才可编辑。未批准包仅可只读检查。

拟新增辅助/测试文件不表示仓库已存在；先搜索可复用类型。共享UI配置、公共头文件、WorldPresentation/CampaignWorld等按[写入调度](docs/planning/TASK-084-103/DISPATCH.md)串行。临时输出使用 `.agent-local/qa/TASK-100/<唯一run>/`，不得写入用户原档。

## 5. Agent具体实施步骤

### 1. 建立完整覆盖表

读取现有全部主线、15一次性支线、4区、80基础驻军、固定增援和10救援对象，列稳定ID、前置、可知条件、触发位置、胜利/奖励条件、对应源码与缺口。以真实配置核对数量，若与批准设计不一致先记录差异，不能直接重写数据。

### 2. 选一控制区作样板

在既有地形/驻军职责内整理入口、可侦察视线、掩体、撤退路径、旗帜交互区与回程；用普通/射手/重型布局表现差别，不增血量当差异。样板通过后才处理其他三区。

### 3. 完善其余三区

为每区写“空间特征—已有敌人职能—玩家现有选择—清敌/占旗反馈—夺回后变化”；只用现有能力与获批内容，避免四次同走廊复刻。新增资源/敌人/奖项/任务门槛需要单独设计决定，不在本任务暗增。

### 4. 接续主支线

从第一救援成长自然推进侦察、各区控制和夺回；日志/地图目标随真实阶段变化。支线确保可接受、可到达、条件可满足、只领奖一次；缺少线索优先改善可知提示，不直接解锁未知地点。

### 5. 验证占领与清敌

按已有规则清敌、E交互及站定5秒等条件逐一实测；击晕等效击杀、故乡永久清除、固定增援只按既定触发结算。未清敌/未满足阶段时明确拒绝，不强改胜利Flag。

### 6. 救援与第二营地

检查已获救人数上限/唯一性、安全承接、永久夺回后第二营地实际可用；共享仓储/等阶而建筑独立，获胜不复制资源/建筑/劳动力。通关后继续可保存和正常生活。

### 7. 先全工程通路

用正常输入从新游戏完成至少一条全主线，允许多次自然保存继续并记录连续时间线；不得通过给物资/清敌控制台/逐点传送跳过缺口。支线可用独立合法存档覆盖，但不能把它伪称同一角色一次全收集。

### 8. 把体验指标交真人

记录阶段实际耗时、停滞和资源流作调表依据，不用强制计时凑8—12小时。103按3名新玩家与支线合计覆盖做真人验收；未闭合的旧049/065/073验收继续留原来源。

## 6. 不可破坏的规则

- 不增加反击敌方腹地、部族战斗小队、最终投降或长期自适应敌军。
- gameplay.json和CampaignState的规则/存档字段默认只读；确需修差异先列准确字段、契约与授权，不绕过配置在C++硬编码替代。
- 所有改地图/分区包仍需逐包登记；本任务优先复用已收尾095—099资产，不临时引入风格不一致的批量新素材。

所有更改继续遵守只读显示、原事务执行、稳定ID、时间线隔离和不伪填验收规则。规则未明确的新玩法不硬编码；不删测试/吞错误/全仓重构来让当前检查通过。

## 7. 验收用例与执行方式

每条用例在隔离档/明确诊断夹具中建立前置，实际记录操作与断言。夹具、注入输入、真实OS键鼠、真实模型、真人样本分层统计；下面所有结果尚未运行。

|局部编号|场景|前置/具体操作|通过标准|初始结果|
|---|---|---|---|---|
|T100-C01|覆盖表一致|配置与设计逐项核对主支线、驻军、救援和控制区。|实际ID全部可追溯，缺口有归属，无编造完成项。|NOT_RUN|
|T100-C02|四区可走|正常步行/战斗进入退出各区、接近旗帜。|每区有可读空间与合法退路，无必须调试的阻断。|NOT_RUN|
|T100-C03|清敌占旗|不满足/满足清敌、击晕与占旗条件分别交互。|严格沿原胜利规则，一生命代次只结算一次。|NOT_RUN|
|T100-C04|主线全程|新档完成夜袭、营地、救援、成长、夺回和第二营地。|连续进度无强制跳阶段，必要资源真实取得。|NOT_RUN|
|T100-C05|支线唯一|逐条完成可获得的15一次性支线并尝试重复领取。|奖励/人口不重复，不可获得条目明确原因而非假PASS。|NOT_RUN|
|T100-C06|第二营地|永久夺回后使用其真实设施、共享仓储并再回第一营地。|共享/独立字段正确，不新增两地搬运或复制建筑。|NOT_RUN|
|T100-C07|通关继续|胜利前后分别保存、自然退出、继续和离区重载。|永久清敌/奖励/救援/第二营地状态保留，游戏可继续。|NOT_RUN|


全局挂接索引：T-002, T-003, T-005, T-011, T-015, T-022。它们来自现有TEST_MATRIX；正式数值以已批准增量/ACCEPTANCE和本单细化步骤为准，不恢复历史灰盒规则。

本单需新增/复用定向原生测试；新前缀建议 `Hearthward.Iteration.Task100.`。先注册并核验找到用例数量>0，再按公开UEClient运行，不能拿本表局部ID当UE过滤器。正常输入、渲染、真实模型或真人用例另行执行。

公共检查：`python -X utf8 scripts/validate_repo.py`、`python -X utf8 -m unittest discover -s scripts/tests -v`、`git diff --check`；在包含获批本单快照的实际基线上运行 `python -X utf8 scripts/validate_repo.py --task TASK-100 --base <实际完整SHA>`。UE操作/证据要求见执行约定；缺环境则明确NOT_RUN，不能将文件编写完成等同测试通过。

## 8. 交付物与完成定义

- 全主支线/救援/驻军/控制区覆盖表
- 四区样板与差异化空间说明、限定代码/包改动
- 合法全主线时间线与支线/第二营地证据
- 供103真人试玩使用的阶段检查表，不含伪填样本

固定收尾：更新`README.md`当前说明、同名MD/JSON与`docs/handoffs/TASK-100.md`；在`docs/qa/TASK-100/REPORT.md`绑定最终完整SHA、资产/模型指纹、环境、命令、逐用例结果和明确限制。该REPORT为未来执行产物，本包不预填。

工程实现完成、Owner视觉、真人体验、二机、性能与发布各自登记。依赖下游可用产物写入handoff，不把未验项目涂为PASS。没有授权时交付本地改动和证据并写“未提交/未推送/未发布”，不得自增权限。

## 9. 本单明确不做

- 首版边界外剧情与战争系统
- 未经实玩依据任意调资源/经验/时长
- 用旧049传送脚本冒充完整游玩

## 10. 失败、范围变化和恢复

首先保留最短复现、受测状态和原始证据，再定位到本单或其他负责任务；不能通过清空存档/赠送物资/瞬移/改验收阈值绕过阻塞。回退只针对本单经核对的改动或资产备份，禁止未经授权reset/clean、覆盖他人工作和强制解锁。确需改变共享契约、保存格式、正式配表或包范围时提交具体差异与影响，先获准再施工。


---

<a id="task-101"></a>

# TASK-101｜存读档、时间线和跨页面集成回归

> 状态：Backlog（任务单已编写，实施未派发）。优先级：P0。阶段：F 集成门槛。日期：2026-10-06。Owner：XLingyyy。Reviewer／Issue：未指派。建议分支：`codex/TASK-101-save-input-regression`（未创建）。

[本批总入口](docs/planning/TASK-084-103/README.md) · [执行约定](docs/planning/TASK-084-103/EXECUTION_GUIDE.md) · [元数据](docs/tasks/TASK-101.json) · [交接模板](docs/handoffs/TASK-101.md)

## 1. 目标与预期结果

在同一整合版本回归所有本批改动涉及的存档、输入、事务和世界时间边界，修实际复现的兼容/生命周期问题，确保没有坏档、复制物品或旧响应越界。

## 2. 当前基础与事实边界

项目已有多期存档迁移、统一世界时钟、独立档池、输入恢复和旧epoch保护。本批默认不新增Save字段；先读取实际格式与兼容策略，不把旧schema编号写成永远最新。

本单参考基线为 `main@6fcf5c22e965f0f7409438f19bc7b09e96ffb058`；实际开工必须复核届时HEAD与依赖产物。这里不是本轮运行结果，旧报告PASS/FAIL均绑定原受测实现。新行为是本单计划，不能写成当前已经存在。

## 3. 依赖、开工门槛与最小必读

前置：[TASK-100](docs/tasks/TASK-100.md)

依赖的约定产物与实际实现SHA就绪、Owner派发且共享写窗口空闲后开始。依赖不要求伪改旧任务Done；读取其实际交接并核对采用版本。

共同必读：`AGENTS.md`、`WORKFLOW.md`、`docs/START_HERE.md`、`docs/PROJECT_STATE.md`、`docs/design/CURRENT.md`及本单MD/JSON，然后执行本批指南中的接手/路径核对。

本单额外入口（路径为只读定位，不等于修改授权；前置任务产物需先实际生成）：

- `Source/Hearthward/Save/`
- `Source/Hearthward/UI/HearthwardLoadingSubsystem.cpp`
- `Source/Hearthward/UI/HearthwardSaveWidget.cpp`
- `Source/Hearthward/UI/HearthwardScreenActions.cpp`
- `docs/qa/update-compatibility-20260930.md`
- `docs/planning/TASK-053-074/ACCEPTANCE.md`

## 4. 修改范围与协作边界

除本单MD/JSON、handoff、QA目录、专用规划目录和根`README.md`固定收尾外，候选范围如下；最终以**已获准基线JSON**为准。

- `Source/Hearthward/Save/`
- `Source/Hearthward/UI/HearthwardLoadingSubsystem.cpp`
- `Source/Hearthward/UI/HearthwardSaveWidget.cpp`
- `Source/Hearthward/UI/HearthwardHUDSave.cpp`
- `Source/Hearthward/UI/HearthwardScreenActions.cpp`
- `Source/Hearthward/UI/HearthwardScreenWidget.h`
- `Source/Hearthward/UI/HearthwardPresentationReadModels.cpp`
- `Source/Hearthward/UI/HearthwardCraftingTracker.cpp`
- `Source/Hearthward/UI/HearthwardCampFeedback.cpp`
- `Source/Hearthward/AI/HearthwardLocalAISubsystem.cpp`
- `Source/Hearthward/Tests/IterationSaveInputRegressionTests.cpp`

Content与Save等未授权领域只读；例外仅以JSON实际范围为准，不能用必读清单扩大写权限。

拟新增辅助/测试文件不表示仓库已存在；先搜索可复用类型。共享UI配置、公共头文件、WorldPresentation/CampaignWorld等按[写入调度](docs/planning/TASK-084-103/DISPATCH.md)串行。临时输出使用 `.agent-local/qa/TASK-101/<唯一run>/`，不得写入用户原档。

## 5. Agent具体实施步骤

### 1. 冻结整合基线

确认084—100实际接入的SHA、Content/LFS与模型指纹，建立只读回归档副本清单；新档、当前档、实际仍支持的旧档、未来/损坏档各自隔离。原件计算哈希并禁止覆盖。

### 2. 制定字段对照

逐项比较物品数量/可用量、装备实例/耐久、稀疏背包与四快捷栏、弟弟任务和携货、队伍岗位/暂停/产出、任务/奖账本/救援、控制区/敌生命代次、两营地/设施和世界时钟。显示暂态如材料追踪按本批约定清空。

### 3. 回归暂态失效

制造模型进行中、待确认、个人返营携货、队伍暂停、地图传送准备、消费/救援进行中等状态后存读档；旧提案/回包/投影必须失效，恢复按原持久化规则，不恢复不存在的待处理事务。

### 4. 真实菜单焦点

用实际OS键鼠走设置Apply→继续加载、Esc/Tab/J/M/T/F6、重复打开关闭、读档确认/取消、IME候选状态。输入恢复以当前页面与设置为准，不恢复旧快照把玩家锁死。

### 5. 时间与幂等

检查暂停冻结、对话运行、8小时睡眠、生产与刷新、危险状态拒绝保存/传送等既有规则；读取前后只结算允许的一次操作。击晕/击杀/奖励/救援/升级去重回归为零违规。

### 6. 兼容与失败保护

旧档在副本上迁移，记录原格式/迁移结果/备份与拒绝原因；未来或损坏档保留原件，不“修复”为新游戏覆盖。中途失败不删除用户Profile，不用清空档池消除异常。

### 7. 最小修复和复跑

先复现RED，再只修相关已登记文件；修改序列化字段/版本、迁移政策或时钟规则不是本单默认权限，必须单列设计/兼容决定。修生命周期后重跑所有受影响消费者，不删失败测试。

### 8. 独立重启确认

至少以首救前、回营成长后、四区中途、永久夺回后四个合法节点自然退出再启动，核对完整字段快照；测试当前开发与Shipping候选分开记录，不能只在同一PIE重置世界。

## 6. 不可破坏的规则

- 原用户档只读不改；所有测试副本独立目录、哈希与清理范围可追溯。
- 禁止新增schema/强制升级存档、缩小兼容范围、隐藏坏档；需要时先有明确批准。
- 复制物品、重复奖励/救援/升级、旧epoch越权结算、损坏用户原档任一出现均为阻塞。

所有更改继续遵守只读显示、原事务执行、稳定ID、时间线隔离和不伪填验收规则。规则未明确的新玩法不硬编码；不删测试/吞错误/全仓重构来让当前检查通过。

## 7. 验收用例与执行方式

每条用例在隔离档/明确诊断夹具中建立前置，实际记录操作与断言。夹具、注入输入、真实OS键鼠、真实模型、真人样本分层统计；下面所有结果尚未运行。

|局部编号|场景|前置/具体操作|通过标准|初始结果|
|---|---|---|---|---|
|T101-C01|字段恢复|四阶段合法档逐一独立重启并逐字段对照。|数量/实例/任务/人口/区域/营地/时钟一致或有现行规则解释。|NOT_RUN|
|T101-C02|旧响应|模型请求和确认卡生成后Load/新游戏再收到响应。|旧回包不执行或进入新档UI；无跨时间线写入。|NOT_RUN|
|T101-C03|货物生产|个人返营携货、队伍暂停/等待刷新分别恢复。|不丢货、不重复入库、不误多算弟弟劳动力。|NOT_RUN|
|T101-C04|输入恢复|真实设置Apply/Continue、各菜单、Esc/Enter和IME。|可移动、可打开关闭菜单，无焦点/暂停残留。|NOT_RUN|
|T101-C05|时间推进|暂停、对话、睡眠和跨日刷新各测。|继续原时间顺序，不因视觉/通知/保存复算。|NOT_RUN|
|T101-C06|重复事务|重复领奖/救援/升级/同敌代次击晕击杀。|每次合法事务仅结算一次，重复操作无收益。|NOT_RUN|
|T101-C07|兼容拒绝|受支持旧档、未来档、损坏档及模拟加载失败。|合法迁移备份，未知/坏档不覆盖，原件哈希不变。|NOT_RUN|
|T101-C08|暂态清理|跨页保留追踪，跨Load清空追踪和无来源行程摘要。|符合本批约定，不残留旧世界引用。|NOT_RUN|


全局挂接索引：T-002, T-004, T-005, T-007, T-010, T-011, T-012, T-013, T-016, T-025。它们来自现有TEST_MATRIX；正式数值以已批准增量/ACCEPTANCE和本单细化步骤为准，不恢复历史灰盒规则。

本单需新增/复用定向原生测试；新前缀建议 `Hearthward.Iteration.Task101.`。先注册并核验找到用例数量>0，再按公开UEClient运行，不能拿本表局部ID当UE过滤器。正常输入、渲染、真实模型或真人用例另行执行。

公共检查：`python -X utf8 scripts/validate_repo.py`、`python -X utf8 -m unittest discover -s scripts/tests -v`、`git diff --check`；在包含获批本单快照的实际基线上运行 `python -X utf8 scripts/validate_repo.py --task TASK-101 --base <实际完整SHA>`。UE操作/证据要求见执行约定；缺环境则明确NOT_RUN，不能将文件编写完成等同测试通过。

## 8. 交付物与完成定义

- 档副本来源/哈希/支持版本与字段对照矩阵
- RED→GREEN最小修复与相关原生/输入回归
- 当前整合SHA的独立重启/旧epoch/时间/事务证据

固定收尾：更新`README.md`当前说明、同名MD/JSON与`docs/handoffs/TASK-101.md`；在`docs/qa/TASK-101/REPORT.md`绑定最终完整SHA、资产/模型指纹、环境、命令、逐用例结果和明确限制。该REPORT为未来执行产物，本包不预填。

工程实现完成、Owner视觉、真人体验、二机、性能与发布各自登记。依赖下游可用产物写入handoff，不把未验项目涂为PASS。没有授权时交付本地改动和证据并写“未提交/未推送/未发布”，不得自增权限。

## 9. 本单明确不做

- 重写Save系统、增加云存档或新存档schema
- 清空用户档池解决测试失败
- 复用不同版本PASS拼接集成通过

## 10. 失败、范围变化和恢复

首先保留最短复现、受测状态和原始证据，再定位到本单或其他负责任务；不能通过清空存档/赠送物资/瞬移/改验收阈值绕过阻塞。回退只针对本单经核对的改动或资产备份，禁止未经授权reset/clean、覆盖他人工作和强制解锁。确需改变共享契约、保存格式、正式配表或包范围时提交具体差异与影响，先获准再施工。


---

<a id="task-102"></a>

# TASK-102｜游戏与本地模型联合性能、内存及运行优化

> 状态：Backlog（任务单已编写，实施未派发）。优先级：P0。阶段：F 集成门槛。日期：2026-10-06。Owner：XLingyyy。Reviewer／Issue：未指派。建议分支：`codex/TASK-102-joint-performance`（未创建）。

[本批总入口](docs/planning/TASK-084-103/README.md) · [执行约定](docs/planning/TASK-084-103/EXECUTION_GUIDE.md) · [元数据](docs/tasks/TASK-102.json) · [交接模板](docs/handoffs/TASK-102.md)

## 1. 目标与预期结果

按已批准目标机/画质/双后端标准测量同一整合版本，定位并修复可复现的UI、流送、资产或模型并发热点；报告各场景而非平均帧率。

## 2. 当前基础与事实边界

历史072存在CPU超时和联合内存/帧时风险，局部成品成绩不能外推。当前正式门槛在ACCEPTANCE，本单不得降低画质、换模型或改变统计口径获得PASS。

本单参考基线为 `main@6fcf5c22e965f0f7409438f19bc7b09e96ffb058`；实际开工必须复核届时HEAD与依赖产物。这里不是本轮运行结果，旧报告PASS/FAIL均绑定原受测实现。新行为是本单计划，不能写成当前已经存在。

## 3. 依赖、开工门槛与最小必读

前置：[TASK-101](docs/tasks/TASK-101.md)

依赖的约定产物与实际实现SHA就绪、Owner派发且共享写窗口空闲后开始。依赖不要求伪改旧任务Done；读取其实际交接并核对采用版本。

共同必读：`AGENTS.md`、`WORKFLOW.md`、`docs/START_HERE.md`、`docs/PROJECT_STATE.md`、`docs/design/CURRENT.md`及本单MD/JSON，然后执行本批指南中的接手/路径核对。

本单额外入口（路径为只读定位，不等于修改授权；前置任务产物需先实际生成）：

- `docs/planning/TASK-053-074/ACCEPTANCE.md`
- `docs/qa/TASK-072/normal-new-cpu-vulkan-runtime-review.md`
- `docs/qa/TASK-072/game-development-runtime-review.md`
- `config/local-ai.lock.json`
- `Source/Hearthward/AI/HearthwardLocalAIRuntime.cpp`
- `Source/Hearthward/UI/HearthwardScreenPaint.cpp`
- `Source/Hearthward/UI/HearthwardScreenLayout.cpp`

## 4. 修改范围与协作边界

除本单MD/JSON、handoff、QA目录、专用规划目录和根`README.md`固定收尾外，候选范围如下；最终以**已获准基线JSON**为准。

- `Source/Hearthward/UI/HearthwardScreenPaint.cpp`
- `Source/Hearthward/UI/HearthwardScreenLayout.cpp`
- `Source/Hearthward/UI/HearthwardScreenWidget.h`
- `Source/Hearthward/UI/HearthwardScreenMap.cpp`
- `Source/Hearthward/UI/HearthwardPresentationReadModels.cpp`
- `Source/Hearthward/UI/HearthwardScreenContent.cpp`
- `Source/Hearthward/AI/HearthwardLocalAIRuntime.cpp`
- `Source/Hearthward/AI/HearthwardLocalAISubsystem.cpp`
- `Source/Hearthward/Gameplay/HearthwardWorldPresentation.cpp`
- `docs/assets/TASK-102/`
- `Source/Hearthward/Tests/PerformanceLifecycleRegressionTests.cpp`

本单不授予整个Content目录。准确包名单由现场引用/094清单确定，先补入已批准快照并核验LFS锁，才可编辑。未批准包仅可只读检查。

拟新增辅助/测试文件不表示仓库已存在；先搜索可复用类型。共享UI配置、公共头文件、WorldPresentation/CampaignWorld等按[写入调度](docs/planning/TASK-084-103/DISPATCH.md)串行。临时输出使用 `.agent-local/qa/TASK-102/<唯一run>/`，不得写入用户原档。

## 5. Agent具体实施步骤

### 1. 冻结测试条件

目标Windows11x64/i7-13650HX/RTX4060 Laptop 8GB/16GB RAM/SSD，通电记录驱动/功耗/温度。1080p、100%渲染、sg3、DX12SM6/Nanite/VSM/原生TAA，无动态分辨率/帧生成/VSync/限帧。非目标机器结果单列，不能冒充。

### 2. 复用既有采样

读072实际脚本与CSV统计定义，固定场景、路线输入、采样段长、暖机与加载剔除规则；场景含营地昼夜、序章、三人战斗、路线流送、经营面板，分别与真实CPU和Vulkan模型请求同时运行。084仅作历史对照，不替本单最终样本。

### 3. 完整记录模型耗时

冷启动到ready，暖请求从玩家提交到完整UE校验后显示；记录排队/上下文/HTTP/显示和后端配置。非流式TTFT保持NOT_RUN，不把prompt时间或模型token时间当用户等待时间。

### 4. 记录资源和尾延迟

采UE/模型进程RAM、显存、页文件和可用内存，保留逐帧数据；报告p99、1%Low、>50ms占比及公式。可用RAM<1GB登记容量风险，任何OOM/进程崩溃为阻塞，不能从均值抹去。

### 5. 先定位再优化

用现有UEClient/Insights路径找GameThread/RenderThread/GPU/Slate/资源流送/模型竞争热点。优先消除每帧全表重建和重复订阅，限制不可见UI刷新；地图朝向/投影仍实时。模型进程资源调度只能在现有批准配置内，改锁定模型参数须另批。

### 6. 资产热点定向处理

确认某材质/实例/贴图/HLOD导致问题后，列准确包/参数及视觉影响，与对应资产任务单写者窗口修复；不遍历降级全部材质/地形，保持目标画质和路线可见性。未授权包不动。

### 7. 复测全矩阵

每次优化保留前后trace，最终同一SHA/包/硬件条件完成所有场景双后端；同时重跑受影响UI输入、伙伴实际执行、保存和路线测试。达不到的指标保留FAIL与瓶颈，不自改门槛。

### 8. 形成平台结论

目标机完整结果、可实际测得的次级配置和容量限制分别记录，不能凭GPU名称估算最低配置。将正式包与第二独立机器的安装/离线验证交103；没有机器就明确NOT_RUN。

## 6. 不可破坏的规则

- 每场p99帧时≤16.67ms，1%Low≥60FPS，>50ms帧占比≤0.1%；原采样统计口径不改。
- 暖启动完整回复p95：Vulkan≤10秒、CPU≤30秒；冷启动ready：Vulkan≤60秒、CPU≤90秒；等待反馈≤0.2秒。业务超时120秒保持。
- 保留当前4B/3328输入/256输出/单并发/默认16GPU层和锁版本；不靠扩大上下文、换云模型、降低分辨率或sg等级过关。
- 最低配置只能由实际测试得出；无独立硬件结果不宣称两机验证完成。

所有更改继续遵守只读显示、原事务执行、稳定ID、时间线隔离和不伪填验收规则。规则未明确的新玩法不硬编码；不删测试/吞错误/全仓重构来让当前检查通过。

## 7. 验收用例与执行方式

每条用例在隔离档/明确诊断夹具中建立前置，实际记录操作与断言。夹具、注入输入、真实OS键鼠、真实模型、真人样本分层统计；下面所有结果尚未运行。

|局部编号|场景|前置/具体操作|通过标准|初始结果|
|---|---|---|---|---|
|T102-C01|目标配置|采样前后导出实际渲染和模型设置。|符合批准配置，覆盖项明确，无隐藏降画质/限帧。|NOT_RUN|
|T102-C02|双后端场景|每个指定场景与CPU/Vulkan真实请求并行。|逐场独立报告全部帧时指标，不以最佳一场代表全局。|NOT_RUN|
|T102-C03|端到端延迟|冷/暖、排队、UI等待提示全链计时。|按批准p95/ready/0.2秒门槛，缺失阶段不算成功。|NOT_RUN|
|T102-C04|内存|长路线/重复菜单/模型并行监控资源。|无持续增长、OOM或崩溃，<1GB风险清楚登记。|NOT_RUN|
|T102-C05|优化正确性|热点修复后重跑对应输入/任务/地图/存档路径。|性能改善不改变玩法规则、显示实时性或保存结果。|NOT_RUN|
|T102-C06|视觉回归|所有资产优化包做同镜头昼夜近远景对照。|无不可接受跳变/遮挡/交互破坏，Owner视觉结论单列。|NOT_RUN|
|T102-C07|最终可重算|检查同SHA全矩阵CSV、统计脚本/公式、环境和trace。|第三方可重新计算指标；未测/失败场景未被隐藏。|NOT_RUN|


全局挂接索引：T-002, T-020, T-021, T-025。它们来自现有TEST_MATRIX；正式数值以已批准增量/ACCEPTANCE和本单细化步骤为准，不恢复历史灰盒规则。

本单需新增/复用定向原生测试；新前缀建议 `Hearthward.Iteration.Task102.`。先注册并核验找到用例数量>0，再按公开UEClient运行，不能拿本表局部ID当UE过滤器。正常输入、渲染、真实模型或真人用例另行执行。

公共检查：`python -X utf8 scripts/validate_repo.py`、`python -X utf8 -m unittest discover -s scripts/tests -v`、`git diff --check`；在包含获批本单快照的实际基线上运行 `python -X utf8 scripts/validate_repo.py --task TASK-102 --base <实际完整SHA>`。UE操作/证据要求见执行约定；缺环境则明确NOT_RUN，不能将文件编写完成等同测试通过。

## 8. 交付物与完成定义

- 固定测试配置与命令、逐帧/请求/内存原始数据
- CPU/Vulkan逐场矩阵、耗时拆分与热点证据
- 最小优化改动、准确资产包与前后对照
- 真实最低/次级配置证据或NOT_RUN说明

固定收尾：更新`README.md`当前说明、同名MD/JSON与`docs/handoffs/TASK-102.md`；在`docs/qa/TASK-102/REPORT.md`绑定最终完整SHA、资产/模型指纹、环境、命令、逐用例结果和明确限制。该REPORT为未来执行产物，本包不预填。

工程实现完成、Owner视觉、真人体验、二机、性能与发布各自登记。依赖下游可用产物写入handoff，不把未验项目涂为PASS。没有授权时交付本地改动和证据并写“未提交/未推送/未发布”，不得自增权限。

## 9. 本单明确不做

- 改验收阈值/降目标画质/换模型/虚构硬件数据
- 泛化全仓性能重构
- 用平均FPS、纯Editor或固定空镜宣称全局性能达标

## 10. 失败、范围变化和恢复

首先保留最短复现、受测状态和原始证据，再定位到本单或其他负责任务；不能通过清空存档/赠送物资/瞬移/改验收阈值绕过阻塞。回退只针对本单经核对的改动或资产备份，禁止未经授权reset/clean、覆盖他人工作和强制解锁。确需改变共享契约、保存格式、正式配表或包范围时提交具体差异与影响，先获准再施工。


---

<a id="task-103"></a>

# TASK-103｜候选包、真人验收与有条件发行交接

> 状态：Backlog（任务单已编写，实施未派发）。优先级：P0。阶段：F 集成门槛。日期：2026-10-06。Owner：XLingyyy。Reviewer／Issue：未指派。建议分支：`codex/TASK-103-release-acceptance`（未创建）。

[本批总入口](docs/planning/TASK-084-103/README.md) · [执行约定](docs/planning/TASK-084-103/EXECUTION_GUIDE.md) · [元数据](docs/tasks/TASK-103.json) · [交接模板](docs/handoffs/TASK-103.md)

## 1. 目标与预期结果

将同一整合版本构建为独立Windows候选包，完成真实使用与已有真人门槛，交付可审查发行材料；只有得到本次明确发布授权才上传新Release。

## 2. 当前基础与事实边界

083已发布的预览与旧QA保留；本任务是本批新候选，不覆盖旧包。工程可运行、自动测试、真人体验、独立机器与实际发布是五个分开的状态。

本单参考基线为 `main@6fcf5c22e965f0f7409438f19bc7b09e96ffb058`；实际开工必须复核届时HEAD与依赖产物。这里不是本轮运行结果，旧报告PASS/FAIL均绑定原受测实现。新行为是本单计划，不能写成当前已经存在。

## 3. 依赖、开工门槛与最小必读

前置：[TASK-102](docs/tasks/TASK-102.md)

依赖的约定产物与实际实现SHA就绪、Owner派发且共享写窗口空闲后开始。依赖不要求伪改旧任务Done；读取其实际交接并核对采用版本。

共同必读：`AGENTS.md`、`WORKFLOW.md`、`docs/START_HERE.md`、`docs/PROJECT_STATE.md`、`docs/design/CURRENT.md`及本单MD/JSON，然后执行本批指南中的接手/路径核对。

本单额外入口（路径为只读定位，不等于修改授权；前置任务产物需先实际生成）：

- `docs/releases/demo-20261006-2/REPORT.md`
- `docs/tasks/TASK-083.json`
- `scripts/release/`
- `docs/planning/TASK-053-074/ACCEPTANCE.md`
- `Source/Hearthward/Hearthward.Build.cs`
- `TestClient/README.md`

## 4. 修改范围与协作边界

除本单MD/JSON、handoff、QA目录、专用规划目录和根`README.md`固定收尾外，候选范围如下；最终以**已获准基线JSON**为准。

- `docs/releases/iteration-084-103-rc/`
- `scripts/release/`
- `docs/PROJECT_STATE.md`
- `docs/START_HERE.md`
- `docs/qa/README.md`
- `TestClient/README.md`
- `Source/Hearthward/Update/HearthwardUpdateSubsystem.h`

Content与Save等未授权领域只读；例外仅以JSON实际范围为准，不能用必读清单扩大写权限。

拟新增辅助/测试文件不表示仓库已存在；先搜索可复用类型。共享UI配置、公共头文件、WorldPresentation/CampaignWorld等按[写入调度](docs/planning/TASK-084-103/DISPATCH.md)串行。临时输出使用 `.agent-local/qa/TASK-103/<唯一run>/`，不得写入用户原档。

## 5. Agent具体实施步骤

### 1. 发行前证据审计

列084—102每单实现SHA、必要资产与LFS对象、原生/实机/语言/性能/兼容结果及未决项。旧报告仍绑定旧版本；不得把本包草拟状态或历史统计写成当前PASS。检查新增资产来源与打包依赖。

### 2. 准备版本与隔离路径

在现有发行工具中选择未占用候选版本/标签和输出目录，先记录建议，不复用0.2.0-preview.20261006.2。区分版本字段更新与GitHub发布权限，保留旧包和测试档原件。

### 3. 构建独立候选

按现有公开UEClient及scripts/release真实入口执行Build/Cook/Stage/Archive，绑定完整源码与资产指纹。打包本地AI与必要运行资源/许可，不带QA用户档、缓存、制作工具、密钥或本机环境文件；不改模型来源锁。

### 4. 独立包正常操作

不用UE/Python启动，完成新游戏、设置、背包/仓储/制作、地图/传送、伙伴实际任务、救援回营、保存退出继续及正常退出。开发包与Shipping结果分开；修改源码后受影响项目必须重新构建验证。

### 5. 第二机器验证

在真实独立Windows机器以非管理员、中文/空格路径干净解压，断网运行随包模型和游戏，测试设置/存档/升级兼容。记录硬件与包哈希；另一进程、另一档或同机虚拟重复不等于第二机器。

### 6. 执行已批准真人门槛

沿065：5名初次玩家无作者提示，从夜袭后营地控制开始，至少4人完成救回1人/工作台I/S2/保存继续，完成者有效实玩中位数30—60分钟；夜袭另测。沿073：3名新玩家全员无阻塞到永久夺回并实际用第二营地，有效通关中位数8—12小时，15支线由全样本合计覆盖。只扣加载、真实休息、暂停，不能改时钟凑指标。

### 7. 战斗与视听验收

依既有规则玩家1v3/弟弟1v3各阶段I/II，每阶段5名真人×3次=15次、至少12次成功，玩家测试时弟弟不攻击/不救援、提供常规食物1和药1；弟弟测试时玩家不攻击、不吃药代救，其余条件沿原ACCEPTANCE；自动输入不能冒充。Owner另看昼夜近远景、连续动作与声音；固定录音未生产如实保留，是否字幕预览交付由Owner明确决定。

### 8. 处理失败而非掩盖

真人不足/环境缺失记NOT_RUN，未达标记FAIL并附路径和证据，反馈到负责任务。P0坏档/复制物品/主线阻断/崩溃未解决不得推荐可发布；仍可交付内部候选及问题清单，不冒充最终完成。

### 9. 获批后才发布

取得本次独立commit/push/merge/release授权后，按现有流程提交需要的版本/文档并上传新Release；明确预览标签、附件名称/大小/校验、重组步骤（如分片）与下载说明。无权限就交付本地候选与发布步骤，不把附件“准备好”写成“已上线”。

### 10. 收尾状态与交接

更新本单README/项目状态/发行报告，使版本、功能、操作、已知限制和实际验证一致。记录真实作者/Reviewer；没有独立Reviewer不造人、不修改校验器躲过规则，必要流程冲突交Owner处理。

## 6. 不可破坏的规则

- 本次写任务单不继承083对旧版的提交/合并/发布授权；所有远端写权限当前not_granted。
- 真人数据必须是实际样本与原始计时，不能虚拟五个人或用Agent循环补齐。
- 固定录音保持暂缓，不能声称完整配音；性能/理解门槛未过不能用小范围烟雾测试替代。
- 发布前任何文件变动都应追溯到受测包；版本号、附件和实际程序一致。

所有更改继续遵守只读显示、原事务执行、稳定ID、时间线隔离和不伪填验收规则。规则未明确的新玩法不硬编码；不删测试/吞错误/全仓重构来让当前检查通过。

## 7. 验收用例与执行方式

每条用例在隔离档/明确诊断夹具中建立前置，实际记录操作与断言。夹具、注入输入、真实OS键鼠、真实模型、真人样本分层统计；下面所有结果尚未运行。

|局部编号|场景|前置/具体操作|通过标准|初始结果|
|---|---|---|---|---|
|T103-C01|证据完整|核对本批所有依赖的SHA/资产/结果与未测项。|能定位每项证据，旧PASS不移植到新包。|NOT_RUN|
|T103-C02|独立运行|Shipping解压后不装UE/Python/API密钥完成正常路径。|游戏/随包模型可用，关键页面和实际委托有效。|NOT_RUN|
|T103-C03|二机离线|第二实体环境、非管理员中文/空格路径、断网运行。|硬件与包哈希明确，存档/设置/模型/继续可验证。|NOT_RUN|
|T103-C04|首切片真人|真实5名首次玩家，按规定记录完成与有效时长。|≥4完成且中位30—60分钟；未达到如实记录，不强制截时。|NOT_RUN|
|T103-C05|全流程真人|真实3名新玩家完成永久夺回、第二营地，合计覆盖15支线。|全员无阻塞且中位8—12小时；未测不宣称完成。|NOT_RUN|
|T103-C06|战斗真人|按原阶段/人数/尝试与隔离补给条件测试双方1v3。|每阶段15次≥12成功，原始记录完整，非真人不计入。|NOT_RUN|
|T103-C07|视听边界|Owner看近远景/昼夜/连续动作并实际听音效。|技术PASS与人工结论分开，未产固定录音保留。|NOT_RUN|
|T103-C08|发布安全|核验授权、标签唯一、附件大小/哈希及解压重组。|只有实际上传并核对才标PUBLISHED；未授权止于候选交付。|NOT_RUN|


全局挂接索引：T-001, T-002, T-020, T-021, T-022。它们来自现有TEST_MATRIX；正式数值以已批准增量/ACCEPTANCE和本单细化步骤为准，不恢复历史灰盒规则。

本单以调查/资料、实际运行或真人验证为主，不为凑测试数量编造原生用例；使用现有有效回归并注明执行层。

公共检查：`python -X utf8 scripts/validate_repo.py`、`python -X utf8 -m unittest discover -s scripts/tests -v`、`git diff --check`；在包含获批本单快照的实际基线上运行 `python -X utf8 scripts/validate_repo.py --task TASK-103 --base <实际完整SHA>`。UE操作/证据要求见执行约定；缺环境则明确NOT_RUN，不能将文件编写完成等同测试通过。

## 8. 交付物与完成定义

- 新候选包与BUILD-INFO/资产和文件指纹
- Shipping/第二机器/真人切片/通关/战斗/视听验收记录
- 发行报告、已知限制、升级与回退说明
- 已获授权时的新Release与附件核对；否则明确NOT_PUBLISHED

固定收尾：更新`README.md`当前说明、同名MD/JSON与`docs/handoffs/TASK-103.md`；在`docs/qa/TASK-103/REPORT.md`绑定最终完整SHA、资产/模型指纹、环境、命令、逐用例结果和明确限制。该REPORT为未来执行产物，本包不预填。

工程实现完成、Owner视觉、真人体验、二机、性能与发布各自登记。依赖下游可用产物写入handoff，不把未验项目涂为PASS。没有授权时交付本地改动和证据并写“未提交/未推送/未发布”，不得自增权限。

## 9. 本单明确不做

- 覆盖旧预览包、擅自发布或伪造真人验收
- 更改玩法/存档/模型以赶发版
- 把候选构建成功写成首版体验全部完成

## 10. 失败、范围变化和恢复

首先保留最短复现、受测状态和原始证据，再定位到本单或其他负责任务；不能通过清空存档/赠送物资/瞬移/改验收阈值绕过阻塞。回退只针对本单经核对的改动或资产备份，禁止未经授权reset/clean、覆盖他人工作和强制解锁。确需改变共享契约、保存格式、正式配表或包范围时提交具体差异与影响，先获准再施工。
