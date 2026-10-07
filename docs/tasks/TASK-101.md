# TASK-101｜存读档、时间线和跨页面集成回归

> 状态：Active（既有7项原生结果保留；candidate4/version.3最新一个旧副本节点加载、隔离保存与第二独立进程显式Continue已部分实测，四个新自然路线节点/完整字段等值/二机待验收）。优先级：P0。阶段：F 集成门槛。日期：2026-10-07。Owner：XLingyyy。Reviewer／Issue：未指派。当前共享分支：`codex/TASK-084-103-iteration`（本地未提交）。

[本批总入口](../planning/TASK-084-103/README.md) · [执行约定](../planning/TASK-084-103/EXECUTION_GUIDE.md) · [元数据](TASK-101.json) · [交接模板](../handoffs/TASK-101.md)

## 1. 目标与预期结果

在同一整合版本回归所有本批改动涉及的存档、输入、事务和世界时间边界，修实际复现的兼容/生命周期问题，确保没有坏档、复制物品或旧响应越界。

2026-10-07：root已派发独立技术子范围，并只读复制/一次SHA登记原用户档。现有Save/Time/Input源码已核对，选出7条最窄相关过滤器供root同一整合版本运行；没有改变Save格式、时钟规则或共享UI源。字段及暂态矩阵、运行边界见 [FIELD_MATRIX](../qa/TASK-101/FIELD_MATRIX.md)、[RUN](../qa/TASK-101/RUN.md)、[REPORT](../qa/TASK-101/REPORT.md)。既有整合build和7/7原生Success、0错误继续绑定原报告（Loading夹具1条EnhancedInput警告保留）。candidate-2/version.1旧副本/第三进程Continue及Return误选New game撤回声明保留为历史。

当前candidate4/version.3真实Shipping以中文空格独立UserDir读取readonlycopy的再复制件：列原4节点、正常Load确认最新2026-09-27 15:07，室外夜main02/100生命/空快捷栏恢复；manualSave4→5，隔离Compatible-v8 pool394263B，第二独立public API进程明确鼠标Continue恢复同可见状态，F6仍5，两轮正常退出。首launch已成功但dict.to_dict回执处理错误保留，以真实进程/OS补证且未重复启动。原件未写、只读copy大小/mtime未变、共享launcher profile未触，无新hash；仅归档[安全summary](../qa/TASK-101/OS_CANDIDATE4_ORIGINAL_COPY_COMPATIBILITY.json)，当前实存6图，不复制个人save/raw profile。只加载最新旧node1，四旧节点全加载/四新正常节点/完整字段等值/二机及其余OS/IME未验，不宣称整单通过。

## 2. 当前基础与事实边界

项目已有多期存档迁移、统一世界时钟、独立档池、输入恢复和旧epoch保护。本批默认不新增Save字段；先读取实际格式与兼容策略，不把旧schema编号写成永远最新。

本单参考基线为 `main@6fcf5c22e965f0f7409438f19bc7b09e96ffb058`；实际开工必须复核届时HEAD与依赖产物。这里不是本轮运行结果，旧报告PASS/FAIL均绑定原受测实现。新行为是本单计划，不能写成当前已经存在。

## 3. 依赖、开工门槛与最小必读

前置：[TASK-100](TASK-100.md)

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

拟新增辅助/测试文件不表示仓库已存在；先搜索可复用类型。共享UI配置、公共头文件、WorldPresentation/CampaignWorld等按[写入调度](../planning/TASK-084-103/DISPATCH.md)串行。临时输出使用 `.agent-local/qa/TASK-101/<唯一run>/`，不得写入用户原档。

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

每条用例在隔离档/明确诊断夹具中建立前置，实际记录操作与断言。夹具、注入输入、真实OS键鼠、真实模型、真人样本分层统计；下表保留规划初始结果，最新原生和Shipping OS子范围见REPORT，部分证据不代填完整用例通过。

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
