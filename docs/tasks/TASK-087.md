# TASK-087｜自然语言委托可靠性、取消回退与真实模型全矩阵

> 状态：Active（已获本批次本地实施授权；两项原生RED已复现并修复，生命周期3/3及新投影RED4→GREEN4同版7/7，另兼容3/3单列；CPU单C01 FAIL；旧Vulkan33/60和定向2/12保留；Source.2最终Vulkan原60为32/60 FAIL、24/30行为FAIL；原暖59不足，固定辅助后60暖p95=8.672秒组件窗口PASS；最终CPU33/60语言、25/30行为FAIL，60暖p95=22.656秒≤30组件PASS；IME/Owner仍待）。优先级：P0。阶段：B 玩法与UI。日期：2026-10-07。Owner：XLingyyy。Reviewer／Issue：未指派。实际共享分支：`codex/TASK-084-103-iteration`。

[本批总入口](../planning/TASK-084-103/README.md) · [执行约定](../planning/TASK-084-103/EXECUTION_GUIDE.md) · [元数据](TASK-087.json) · [交接模板](../handoffs/TASK-087.md)

## 1. 目标与预期结果

修复有复现证据的理解/参数/生命周期问题，保留单次本机4B推理架构，并在CPU和Vulkan上分别完成现有语言门槛。模型不可用时手动玩法仍可完成。

2026-10-07执行记录：原60条集合及逐条原始记录脚本已准备；主代理首次构建成功，3项定向原生回归实际1成功/2失败。两项已复现生命周期缺口已最小修复，修复后3/3原生GREEN（0警告/0错误）；CPU原C01单条真实诊断因MODEL_UNAVAILABLE/空回复失败；修复前Vulkan原60完整复测实际33/60原始理解、2/20澄清拒绝、34/40明确端到端、26/30行为、20/20边界、白得物品0，语言门槛未达标。该轮暖请求59样本不足。随后新投影4项实际全Fail保存，受限Capture/System/Projection补丁已同版实际build SUCCESS97.97秒，新投影4/4 GREEN、生命周期3/3、兼容079两项＋Bounded一项3/3单列；联合16项13 Success/3 Fail不能记全绿。Source.2后续Vulkan定向12条实际原始2/12、明确E2E2/6、行为2/6、受限0/6；全部input3169—3312≤3328、generation1，预算符合仍未达到语言门槛。随后final-vulkan-20261007-02实际Source.2完整原60：原始32/60、受限2/20、明确E2E33/40、行为24/30，语言门槛FAIL；边界20/20、白得0独立保留。原59暖INSUFFICIENT/null保持；唯一WARM-C01-01不确认、不计语言/执行，合并60暖p95=8.672秒≤10秒组件窗口PASS，UIpaint/joint仍NOT_RUN。Source.2最终CPU原60实际33/60原始、3/20受限、33/40明确E2E、25/30行为FAIL，边界20/20、白得0；原59暖不足保留，唯一辅助后60暖p95=22.656秒≤30秒组件窗口PASS；UIpaint/joint、真实IME仍NOT_RUN。固定4B/冻结参数下语义门槛失败，尚未确认可修工程缺陷；更换模型、预算或部署策略须由Owner决定契约变化，不能硬编码评测答案或放宽阈值。当前事实和不同范围证据见[本单QA](../qa/TASK-087/REPORT.md)及[复测与决策记录](../qa/TASK-087/MODEL_RETEST_DECISION.md)。

## 2. 当前基础与事实边界

历史TASK-068完整理解矩阵未达标，TASK-079—081新增过局部成功，不等于全矩阵通过。完整基准及阈值以已批准ACCEPTANCE为准，禁止选取成功表达替代原集合。

本单参考基线为 `main@6fcf5c22e965f0f7409438f19bc7b09e96ffb058`；实际开工必须复核届时HEAD与依赖产物。这里不是本轮运行结果，旧报告PASS/FAIL均绑定原受测实现。新行为是本单计划，不能写成当前已经存在。

## 3. 依赖、开工门槛与最小必读

前置：[TASK-086](TASK-086.md)

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
- `Source/Hearthward/AI/HearthwardNPCContextProjection.h`
- `Source/Hearthward/AI/HearthwardNPCContextProjection.cpp`
- `Source/Hearthward/AI/HearthwardAgentContract.cpp`
- `Source/Hearthward/AI/HearthwardAgentInteraction.cpp`
- `Source/Hearthward/UI/HearthwardScreenDialogue.cpp`
- `Source/Hearthward/UI/HearthwardHUDDialogue.cpp`
- `Source/Hearthward/UI/HearthwardDialogueWidget.cpp`
- `Source/Hearthward/Tests/LocalAIReliabilityTests.cpp`
- `Source/Hearthward/Tests/LocalAIContextProjectionTests.cpp`

本轮主代理明确委派的Projection头文件及定向测试增补已同步JSON，修复以实际4项RED为前置；Source.2冻结后仅做证据和待决策记录。

Content与Save等未授权领域只读；例外仅以JSON实际范围为准，不能用必读清单扩大写权限。

拟新增辅助/测试文件不表示仓库已存在；先搜索可复用类型。共享UI配置、公共头文件、WorldPresentation/CampaignWorld等按[写入调度](../planning/TASK-084-103/DISPATCH.md)串行。临时输出使用 `.agent-local/qa/TASK-087/<唯一run>/`，不得写入用户原档。

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

每条用例在隔离档/明确诊断夹具中建立前置，实际记录操作与断言。夹具、注入输入、真实OS键鼠、真实模型、真人样本分层统计；此表保存验收定义及当前分层登记，逐条原始结果见QA REPORT。

|局部编号|场景|前置/具体操作|通过标准|当前结果|
|---|---|---|---|---|
|T087-C01|明确指令|双后端跑40明确表达，包括个人量、人数和搬运方向。|理解与实际动作分开统计，目标/数量/来源目的地一致。|最终CPU/Vulkan明确E2E各33/40 FAIL|
|T087-C02|歧义与拒绝|各跑10歧义、10越权/不支持表达。|该问则澄清、该拒则拒绝；不把不理解的一律拒绝记正确。|最终CPU3/20、Vulkan2/20原始受限FAIL|
|T087-C03|否定查询|验证“别继续”“只是问进度”“刚才完成了吗”等真实表达。|查询不改任务，续接只有确认后执行。|NOT_RUN|
|T087-C04|生命周期|取消后回包、Load期间回包、重复提交、对象销毁。|旧结果不显示为新确认卡、不执行、不重复结算。|生命周期原生3/3 PASS；Load/销毁真实HTTP另未验|
|T087-C05|模型不可用|断开本地服务/失败启动/达到原超时；使用手动表单。|界面可退出，既有采集/搬运玩法不被模型故障阻断。|原生回退PASS；旧PIE及Standalone故障保留，正常OS完整回退未验|
|T087-C06|中文IME|真实中文组合/候选、Enter、Esc、长输入、回复到达。|不误发未完成文字，不丢输入，不双重执行。|NOT_RUN|
|T087-C07|真实行为|执行至少30个可执行行为、20个边界响应。|按批准指标报告，完整交付而非只输出合法JSON。|最终CPU25/30、Vulkan24/30行为FAIL；各边界20/20/白得0|
|T087-C08|最终同版|锁定最终SHA与模型指纹，双后端重跑。|所有原始记录可重算指标；不拼接不同实现的最佳成绩。|Source.2双后端原60均语言FAIL；各固定辅助60暖组件PASS，UIpaint/joint/Owner未验|


全局挂接索引：T-002, T-008, T-010, T-011, T-020, T-025。它们来自现有TEST_MATRIX；正式数值以已批准增量/ACCEPTANCE和本单细化步骤为准，不恢复历史灰盒规则。

本单需新增/复用定向原生测试；新前缀建议 `Hearthward.Iteration.Task087.`。先注册并核验找到用例数量>0，再按公开UEClient运行，不能拿本表局部ID当UE过滤器。正常输入、渲染、真实模型或真人用例另行执行。

公共检查：`python -X utf8 scripts/validate_repo.py`、`python -X utf8 -m unittest discover -s scripts/tests -v`、`git diff --check`；在包含获批本单快照的实际基线上运行 `python -X utf8 scripts/validate_repo.py --task TASK-087 --base <实际完整SHA>`。UE操作/证据要求见执行约定；缺环境则明确NOT_RUN，不能将文件编写完成等同测试通过。

## 8. 交付物与完成定义

- 沿用/增补的评测集合与来源映射
- 双后端原始结果、分项统计与失败分析
- 取消/回退/真实IME及正常地图执行证据

固定收尾：更新`README.md`当前说明、同名MD/JSON与`docs/handoffs/TASK-087.md`；在`docs/qa/TASK-087/REPORT.md`绑定最终完整SHA、资产/模型指纹、环境、命令、逐用例结果和明确限制。该REPORT已绑定本轮真实原生及双后端模型分层结果；未运行的渲染/OS/Owner/二机项继续保留未验状态。

工程实现完成、Owner视觉、真人体验、二机、性能与发布各自登记。依赖下游可用产物写入handoff，不把未验项目涂为PASS。没有授权时交付本地改动和证据并写“未提交/未推送/未发布”，不得自增权限。

## 9. 本单明确不做

- 替换/训练模型、接云端LLM、增加Embedding或多层生成链
- 扩展危险自主行为/任意任务队列
- 将结构化假输入算作自然语言理解

## 10. 失败、范围变化和恢复

首先保留最短复现、受测状态和原始证据，再定位到本单或其他负责任务；不能通过清空存档/赠送物资/瞬移/改验收阈值绕过阻塞。回退只针对本单经核对的改动或资产备份，禁止未经授权reset/clean、覆盖他人工作和强制解锁。确需改变共享契约、保存格式、正式配表或包范围时提交具体差异与影响，先获准再施工。
