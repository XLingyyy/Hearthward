# TASK-093｜营地准备、回营成长反馈与岗位可视状态

> 状态：Active（原生3项有成功证据；首轮2夹具FAIL纠正后实跑PASS，渲染/正常游戏待验）。优先级：P1。阶段：C 地图与场景。日期：2026-10-06。Owner：XLingyyy。Reviewer／Issue：未指派。实际分支：`codex/TASK-084-103-iteration`。

[本批总入口](../planning/TASK-084-103/README.md) · [执行约定](../planning/TASK-084-103/EXECUTION_GUIDE.md) · [元数据](TASK-093.json) · [交接模板](../handoffs/TASK-093.md)

## 1. 目标与预期结果

把营地已有岗位、获救人口、真实入库和一次升阶串成可理解的准备/回营反馈；展示来自现有事件和状态，不增加经济模拟。

## 2. 当前基础与事实边界

营地两地共享仓储和等阶、建筑独立；族人后台生产，弟弟实际到岗。首切片需人口20→21、工作台I、营地S2，数值与解锁效果沿已批准数据。

本单参考基线为 `main@6fcf5c22e965f0f7409438f19bc7b09e96ffb058`；实际开工必须复核届时HEAD与依赖产物。这里不是本轮运行结果，旧报告PASS/FAIL均绑定原受测实现。新行为是本单计划，不能写成当前已经存在。

## 3. 依赖、开工门槛与最小必读

前置：[TASK-090](TASK-090.md)、[TASK-092](TASK-092.md)

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

拟新增辅助/测试文件不表示仓库已存在；先搜索可复用类型。共享UI配置、公共头文件、WorldPresentation/CampaignWorld等按[写入调度](../planning/TASK-084-103/DISPATCH.md)串行。临时输出使用 `.agent-local/qa/TASK-093/<唯一run>/`，不得写入用户原档。

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

2026-10-07实施记录：mcp_setup完成本单允许范围的营地发展/真实缺口/本营设施/回营会话反馈；升阶卡绑定当前tier+epoch并走原事务；现有模型/经济/Save不改。Native源3项、diff --check PASS，实际构建及原生已有成功证据，渲染与正常输入待root。所有源码窗口已释放，详情与分层结果见docs/qa/TASK-093/REPORT.md。

093首轮实际Native（2026-10-07）：1 Success、2 Fail；Fail已确认为测试夹具漏SetIsFocusable导致真实UIOnly焦点日志Error，生产InitializeScreen已设置。仅夹具已获授权纠正，保留原RED证据，不屏蔽ExpectedError。根Agent随后于`.agent-local/qa/TASK-090-100/native-red-20261007/index.json`实际重跑纠正后的两项，2 Success、2 warnings/0 errors；ReceiptTimeline复用前轮PASS，见`docs/qa/TASK-093/native-correctedfixture-20261007.json`。
