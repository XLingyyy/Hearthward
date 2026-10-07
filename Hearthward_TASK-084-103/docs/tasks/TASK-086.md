# TASK-086｜弟弟工作状态卡、停工原因和恢复入口

> 状态：Backlog（任务单已编写，实施未派发）。优先级：P0。阶段：B 玩法与UI。日期：2026-10-06。Owner：XLingyyy。Reviewer／Issue：未指派。建议分支：`codex/TASK-086-companion-status`（未创建）。

[本批总入口](../planning/TASK-084-103/README.md) · [执行约定](../planning/TASK-084-103/EXECUTION_GUIDE.md) · [元数据](TASK-086.json) · [交接模板](../handoffs/TASK-086.md)

## 1. 目标与预期结果

让个人委托和营地采集队的当前工作、实际入库、携带量、停工原因及可执行操作在对话/HUD中一致可见。

## 2. 当前基础与事实边界

TASK-078—081已有定量委托、查询续接、右侧对话和持续采集队。个人任务仍是指定来源；队伍不是按物资总量停止；普通族人后台生产与弟弟实际到岗必须区分。

本单参考基线为 `main@6fcf5c22e965f0f7409438f19bc7b09e96ffb058`；实际开工必须复核届时HEAD与依赖产物。这里不是本轮运行结果，旧报告PASS/FAIL均绑定原受测实现。新行为是本单计划，不能写成当前已经存在。

## 3. 依赖、开工门槛与最小必读

前置：[TASK-085](TASK-085.md)

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

拟新增辅助/测试文件不表示仓库已存在；先搜索可复用类型。共享UI配置、公共头文件、WorldPresentation/CampaignWorld等按[写入调度](../planning/TASK-084-103/DISPATCH.md)串行。临时输出使用 `.agent-local/qa/TASK-086/<唯一run>/`，不得写入用户原档。

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
