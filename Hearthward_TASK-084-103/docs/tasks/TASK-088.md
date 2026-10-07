# TASK-088｜制作配方查找、可制作筛选与缺料追踪

> 状态：Backlog（任务单已编写，实施未派发）。优先级：P1。阶段：B 玩法与UI。日期：2026-10-06。Owner：XLingyyy。Reviewer／Issue：未指派。建议分支：`codex/TASK-088-crafting-tracker`（未创建）。

[本批总入口](../planning/TASK-084-103/README.md) · [执行约定](../planning/TASK-084-103/EXECUTION_GUIDE.md) · [元数据](TASK-088.json) · [交接模板](../handoffs/TASK-088.md)

## 1. 目标与预期结果

在现有制造逻辑之上增加配方搜索、分类、可制作筛选和一个明确的材料追踪目标，减少准备阶段查表和翻页。

## 2. 当前基础与事实边界

ComposeCrafting现有四行配方、批量、背包/仓储数量、重量预测和CraftingStatus已具备；不能重做制造结算。目标为信息组织，不扩配方或改经济。

本单参考基线为 `main@6fcf5c22e965f0f7409438f19bc7b09e96ffb058`；实际开工必须复核届时HEAD与依赖产物。这里不是本轮运行结果，旧报告PASS/FAIL均绑定原受测实现。新行为是本单计划，不能写成当前已经存在。

## 3. 依赖、开工门槛与最小必读

前置：[TASK-085](TASK-085.md)

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

拟新增辅助/测试文件不表示仓库已存在；先搜索可复用类型。共享UI配置、公共头文件、WorldPresentation/CampaignWorld等按[写入调度](../planning/TASK-084-103/DISPATCH.md)串行。临时输出使用 `.agent-local/qa/TASK-088/<唯一run>/`，不得写入用户原档。

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
