# TASK-102｜游戏与本地模型联合性能、内存及运行优化

> 状态：Active（CPU／Vulkan单场实测FAIL／容量警告，六场未验）。优先级：P0。阶段：F 集成门槛。日期：2026-10-07。Owner：XLingyyy。Reviewer／Issue：未指派。实际分支：`codex/TASK-084-103-iteration`（本地未提交改动）。

[本批总入口](../planning/TASK-084-103/README.md) · [执行约定](../planning/TASK-084-103/EXECUTION_GUIDE.md) · [元数据](TASK-102.json) · [交接模板](../handoffs/TASK-102.md)

## 1. 目标与预期结果

按已批准目标机/画质/双后端标准测量同一整合版本，定位并修复可复现的UI、流送、资产或模型并发热点；报告各场景而非平均帧率。

## 2. 当前基础与事实边界

历史072存在CPU超时和联合内存/帧时风险，局部成品成绩不能外推。当前正式门槛在ACCEPTANCE，本单不得降低画质、换模型或改变统计口径获得PASS。

本单参考基线为 `main@6fcf5c22e965f0f7409438f19bc7b09e96ffb058`；实际开工必须复核届时HEAD与依赖产物。这里不是本轮运行结果，旧报告PASS/FAIL均绑定原受测实现。新行为是本单计划，不能写成当前已经存在。

## 3. 依赖、开工门槛与最小必读

前置：[TASK-101](TASK-101.md)

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

拟新增辅助/测试文件不表示仓库已存在；先搜索可复用类型。共享UI配置、公共头文件、WorldPresentation/CampaignWorld等按[写入调度](../planning/TASK-084-103/DISPATCH.md)串行。临时输出使用 `.agent-local/qa/TASK-102/<唯一run>/`，不得写入用户原档。

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

每条用例在隔离档/明确诊断夹具中建立前置，实际记录操作与断言。夹具、注入输入、真实OS键鼠、真实模型、真人样本分层统计；下表保留完整验收边界；本轮boot诊断、解析失败/重解析和启动CRASH分层记录在[REPORT](../qa/TASK-102/REPORT.md)。新稳定runner已实际执行CPU／Vulkan独立单场；当前帧目标FAIL、Vulkan语义错误／CPU HTTP超时、容量风险及六场未验边界见REPORT。

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

固定收尾：更新`README.md`当前说明、同名MD/JSON与`docs/handoffs/TASK-102.md`；在`docs/qa/TASK-102/REPORT.md`绑定最终完整SHA、资产/模型指纹、环境、命令、逐用例结果和明确限制。该REPORT已记录本轮真实Partial证据及未执行层级；未达完整门槛前不填最终PASS。

工程实现完成、Owner视觉、真人体验、二机、性能与发布各自登记。依赖下游可用产物写入handoff，不把未验项目涂为PASS。没有授权时交付本地改动和证据并写“未提交/未推送/未发布”，不得自增权限。

## 9. 本单明确不做

- 改验收阈值/降目标画质/换模型/虚构硬件数据
- 泛化全仓性能重构
- 用平均FPS、纯Editor或固定空镜宣称全局性能达标

## 10. 失败、范围变化和恢复

首先保留最短复现、受测状态和原始证据，再定位到本单或其他负责任务；不能通过清空存档/赠送物资/瞬移/改验收阈值绕过阻塞。回退只针对本单经核对的改动或资产备份，禁止未经授权reset/clean、覆盖他人工作和强制解锁。确需改变共享契约、保存格式、正式配表或包范围时提交具体差异与影响，先获准再施工。

## 11. 2026-10-07 当前执行增量

用户已授权本批本地施工，Root统一UE/模型生命周期。新增稳定单场QA入口与[RUN说明](../qa/TASK-102/RUN.md)，复用072正式自然World/正常new公开链，不改Source/旧072/Save/模型参数。本次STATIC_CHECK 7项通过仅指静态与历史数据复算。Vulkan稳定单场实际6183帧／61.1867968秒、p99 16.7755ms、1%Low49.0051591FPS、>50ms0；帧目标未达。完整单条请求在CSV内，HTTP24.407秒／ready10.617秒，nature_collect/known_target被TARGET_REQUIRED转clarify无candidate，单条语义FAIL。12资源样本RAM最低0.558826GiB／3次低于1GiB，容量风险保留。CPU另已实测11603帧／129.8656161秒、p99 16.5324ms单项PASS、1%Low51.3488456FAIL、>50ms2/11603；HTTP120.007秒timeout→MODEL_UNAVAILABLE/raw空，提交128.69秒，完整失败区间在CSV。CPU最低RAM0.424530GiB／9/17次低于1GiB。六条件、p95／首次Paint／正常OS／Shipping／真人／二机仍NOT_RUN，未宣称Source／最终包绑定。

9000帧首轮保留原parser失败，同CSV重解析保持9000帧/116.4529097秒、p99 11.7866ms、1%Low 2.0998256263FPS、>50ms 7帧；model generations=0，无联合请求。第二次计划24000帧在Frame3 Renderer AV启动崩溃，无完整CSV/无模型/未到画质命令，首因未解决。原始证据位置见[索引](../qa/TASK-102/EVIDENCE_INDEX.json)。五个其他场景的真实前置缺口在RUN列清楚，属于工程验收未执行；不以伪造进度或改画质绕过。Master README由Root串行收尾；未提交、未推送、未发布。
