# TASK-089｜背包与仓储操作一致性、实例详情和可读性

> 状态：Backlog（任务单已编写，实施未派发）。优先级：P1。阶段：B 玩法与UI。日期：2026-10-06。Owner：XLingyyy。Reviewer／Issue：未指派。建议分支：`codex/TASK-089-inventory-storage-ux`（未创建）。

[本批总入口](../planning/TASK-084-103/README.md) · [执行约定](../planning/TASK-084-103/EXECUTION_GUIDE.md) · [元数据](TASK-089.json) · [交接模板](../handoffs/TASK-089.md)

## 1. 目标与预期结果

统一背包/仓储中物品数量、可用量、重量和装备实例表达，改善查找与转移反馈，同时保护拖放、稀疏格子和快捷栏已有成果。

## 2. 当前基础与事实边界

当前仓储已采用深灰格子，支持选择存入/取出、数量调整、分页和失败原因；背包支持拖放、独立装备实例、稀疏位置与四快捷栏。不是重新做一套箱子系统。

本单参考基线为 `main@6fcf5c22e965f0f7409438f19bc7b09e96ffb058`；实际开工必须复核届时HEAD与依赖产物。这里不是本轮运行结果，旧报告PASS/FAIL均绑定原受测实现。新行为是本单计划，不能写成当前已经存在。

## 3. 依赖、开工门槛与最小必读

前置：[TASK-086](TASK-086.md)、[TASK-088](TASK-088.md)

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

拟新增辅助/测试文件不表示仓库已存在；先搜索可复用类型。共享UI配置、公共头文件、WorldPresentation/CampaignWorld等按[写入调度](../planning/TASK-084-103/DISPATCH.md)串行。临时输出使用 `.agent-local/qa/TASK-089/<唯一run>/`，不得写入用户原档。

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
