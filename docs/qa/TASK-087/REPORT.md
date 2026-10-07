# TASK-087 本地执行记录

当前状态：Active，已完成实际原生RED复现及最小生产修复，生命周期及新投影同版原生GREEN 7/7已完成，另复用兼容3/3单列。分支 `codex/TASK-084-103-iteration`，基线完整HEAD `6fcf5c22e965f0f7409438f19bc7b09e96ffb058` 加本批次本地未提交改动。首次Development Editor构建成功；087原生修复前1/3成功、2/3失败。Vulkan原60完整真实复测未达语言门槛；CPU C01单条诊断已实际失败并保留原始回复；新投影Source.2 Vulkan定向12条实际2/12原始正确，全部输入3169—3312 tokens且generation1，语义仍FAIL。Source.2最终Vulkan原60已实际32/60原始、2/20受限、33/40明确E2E、24/30行为，语言FAIL；原59暖不足保留，唯一辅助后60暖p95=8.672秒组件窗口PASS。Source.2最终CPU原60实际33/60原始、3/20受限、33/40明确E2E、25/30行为FAIL，边界20/20、白得0；原59暖不足保留，唯一辅助后60暖p95=22.656秒≤30秒组件窗口PASS；UIpaint/joint、真实IME仍未验收。当前源码版本0.2.0-preview.20261007.2，不将旧包结果迁移为最终同版成绩；未提交、未推送、未发布。

## 已确认的源代码缺口

- 自由文字 `SubmitPlayerTextInternal` 进入后直接 `CancelPending`，没有 `bPending` 忙保护；重复提交会替换原请求。快捷建议路径已单独检查忙状态。
- `Fail` 清除busy/card但未递增请求Serial、未取消/释放HTTP请求。故障后晚到的原序号回调仍能进入，覆写原故障原因。已有取消和快照重置路径递增Serial，可复用其逻辑失效方式。
- 主UI及legacy预览键事件先处理Esc，缺少组合态判断。真实Windows中文IME尚未复现；当前记录为待验证风险。主UI `ScreenWidget.cpp` 不在087允许路径内，已交主代理协调。

主代理已实际保存RED，见[修复前原生结果](native-red-20261007.json)，源联合报告`.agent-local/qa/TASK-085-088/native-baseline-20261007/index.json`。BusyKeepsRequest记录原输入变为第二条、pending释放、Serial41→42、原票据失效；FailureInvalidatesCallback记录Fail后序号不变。CancelKeepsManualFallback成功，无警告/错误。联合轮共发现12项、9成功、3失败，其中另1项属于088，不混入087分母。

随后仅在LocalAISubsystem.cpp修3行：忙时明确拒绝重复自由文字并保留原请求；Fail先递增Serial并取消/释放Request，各HTTP回调继续复用既有ExpectedSerial及StillCurrent检查。未修改UI、模型、lock、Save或执行器。相关`git diff --check`通过；主代理实际Development Editor repair build成功；修复后3/3原生Success、0警告/0错误，见 [原生GREEN](native-green-20261007.json)，原联合报告为 .agent-local/qa/TASK-085-099/native-first-20261007/index.json。此结论仅含开发夹具的忙请求、故障序号失效和取消手动回退，不等于真实模型矩阵或HTTP端到端通过。

## 评测集合与运行环境

`cases.json` 与068原始JSON对象完全相同：60＝40明确＋10歧义＋10越权；30行为及20边界的原分母、原文、预期和阈值均保留。`supplemental-cases.json` 单列10条否定、查询、指代续接、人数/份数、搬运方向和unsupported表达，状态NOT_RUN，不进入原60统计。

锁定模型和CPU/Vulkan b10964执行文件在 `Runtime/LocalAI` 可见；未更新模型、依赖、lock、上下文、并发、GPU层默认值或业务超时。模型完整指纹使用原lock记录，未因普通准备工作重算SHA。

原历史完整结果仅作定位参考，绑定其原实现：CPU原始34/60、澄清拒绝1/20、明确端到端36/40、行为28/30；Vulkan原始36/60、澄清拒绝1/20、明确端到端38/40、行为30/30。两者均未通过原理解门槛。这些数字不能代填本轮结果。

真实矩阵脚本、参数、独立输出及耗时估计见 [RUN.md](RUN.md)。每后端使用新的run_id，原回复、解析、预期、epoch、参数及物品前后状态落盘JSON/JSONL/CSV。部分集合只用于定位，不改变完整门槛。

## 本轮真实CPU单条诊断

主代理实际运行 `actual-c01-20261007`，只选原集合C01“请新采一份木材并带回仓库”。[原始单条诊断结果](cpu-c01-diagnostic-20261007.json) 原样导出自 `.agent-local/qa/TASK-087/actual-c01-20261007/cpu/results.json`：`diagnostic_subset=true`，generation=1、input_tokens=3206，原回复为空，最终原因 `MODEL_UNAVAILABLE`，raw/e2e/execution均false；未确认世界变化、无白得物品。40明确/20受限/60全部/30行为/20边界的原分母仍保留；不把单条结果重算成完整矩阵成绩。

实际CPU运行日志记录模型start→ready为9.472秒；Generation日志至HTTP失败为120.021秒，HTTP自身提交以来elapsed=129.73秒。脚本真实单调时钟submit→`!busy`并读到最终proposal/status为129.797秒，包含启动等待，不包含确认、执行或Slate绘制。旧raw-only `latency_seconds=0.0` 字段在本次无原回复场景中不能解释成瞬时成功。业务120秒未改；失败/空回复真实保留，不补造模型输出。此证据是PIE公开API诊断，正常OS输入/画面反馈未验收；后续最终CPU原60已另行实测归档；后续Source.2最终Vulkan60及独立暖组件p95已实际，分别登记，不由此旧CPU单条代填。

## 本轮真实Vulkan完整RED

主代理实际完成 `full-vulkan-20261007-01`，60条原表达全部执行，`diagnostic_subset=false`，受测版本 `6fcf5c22e965f0f7409438f19bc7b09e96ffb058-dirty`。完整原报告原样导出为 [Vulkan完整RED JSON](vulkan-full-red-20261007.json)，逐条表为 [CSV](vulkan-full-red-20261007.csv)；本地原始 `cases.jsonl`、边界与运行日志保留在 `.agent-local/qa/TASK-087/full-vulkan-20261007-01/`。模型/参数锁保持原样：3328输入、256输出、温度0、单并发、4096上下文、16GPU层、120秒HTTP超时、非流式、thinking=false。

|层级|实际结果|批准门槛/含义|
|---|---|---|
|原始理解|33/60，55%|≥54/60；FAIL|
|歧义/越权原始澄清拒绝|2/20，10%|≥18/20；FAIL|
|明确表达端到端|34/40，85%|≥36/40；FAIL|
|确认后实际行为|26/30，86.67%|≥29/30；FAIL|
|确定性边界|20/20|公开API边界通过，独立于模型理解|
|白得物品|0例|本轮夹具未观察到白得物品|
|暖请求时延|59样本，INSUFFICIENT_SAMPLES|首条冷启动排除后未到60，p95=null；不计作102性能通过|

已确认的错误包括：C14伙伴协助被解析为原地等待；C18中转交付丢失最终入库方向；C21仓库至弟弟被解析为仓库至玩家；C24弟弟至玩家方向反转。歧义组会为缺数量/次数/具体对象补1或32，维修两把同类装备仍选未指明实例；越权组会把负数/小数/33改成合法正整数或上限，把玩家装备改作弟弟自有装备，或以目录里的其他物品替代未知物品。UE多处挡住了这些写入，安全挡住不能计作原始理解正确。历史同类失败只用于定位，不豁免本轮失败，也不能由不同上下文的成绩差异推断因果。

依据本单允许的上下文裁剪/可知对象绑定范围，主代理已实施受限AI工程修复：保留每个投影档的角色/背包归属与实际候选，补齐公开权威快照，并澄清目录/上限与本次实际对象之间的关系。新投影测试已真实4/4 Fail保存，受限Capture/System补丁已落盘；投影已真实重建/GREEN 4/4，生命周期同次3/3、兼容3/3分开登记；随后新投影Vulkan定向12条实际2/12原始正确，语义仍FAIL；最终Source.2完整Vulkan随后实际32/60原始及24/30行为FAIL，本段不宣称语言已通过。最终CPU完整矩阵已另行实测33/60语言FAIL；额外10条、正常OS输入/IME、真人/二机仍NOT_RUN。

## 权威快照与投影修复阶段

主代理真实 `build-repaired-syntax` 构建成功后执行新过滤器 `Hearthward.Iteration.Task087.Projection.`，实际4项全部Fail、0项NOT_RUN，见 [投影原生RED原报告](projection-red-20261007-01.json)，源为 `.agent-local/qa/TASK-087/projection-red-20261007-01/native/index.json`。缺口分别为：各降级档角色/容器归属未保留；“斧头”等口语别名下两件自有维修候选被裁剪；已观察候选及未知视图区分未投影；旧伙伴/当前任务状态与新指令混杂。此RED与先前生命周期3/3 GREEN属于不同定向集合，分母分开。

真实RED后受限生产补丁已落盘并由主代理实际重建、定向原生GREEN：`CaptureContextSnapshot` 增加弟弟公开Bag可用标记及真实数量；按StageCandidate局部边界一次统计自然动作的 `intent:item` 候选数，不选择Station/GUID或公开坐标；已检查类别保留零；Nature未完成EnsureWorld初始化（Seed为0）或视图不存在时保留unknown。鱼点及活动物要求当前Actor、3千厘米内；资源/作物/栏舍3百厘米内，照料按未浇/未施肥/成熟判断；栏舍产物候选按至少1份，正式请求数量仍由原执行器复核。已接触护送候选只包含能力范围的rescued人物，非未接触/已到营状态，当前Actor存在且位于弟弟3百厘米内、玩家与弟弟3千厘米内。它们是可观察绑定候选，不表示Safe/Busy/成本/库存等执行资格已经通过。没有读取玩家背包或全局未见人物位置。

System将最短角色/原话代词说明前移到目录之前，说明目录枚举不等于持有/发现/接触/唯一/已指定，以及上限不补缺量；库存询问与确数报告分开，旧状态不替本次指令。共享System字面总字符1444→1448（+4字符，未当作实际token数），原Schema、模型、3328/256预算、单生成、业务超时及两条正例不变。其余投影实现由主代理协调mcp_setup，保持同一快照用于各档；Native同版投影4/4、生命周期3/3与独立兼容3/3已Success；随后定向12条实际输入3169—3312 tokens全部符合3328预算，但只2/12原始正确；最终Vulkan完整原60随后已实际语言FAIL、独立暖组件窗口PASS；最终CPU完整矩阵随后已实际33/60语言FAIL、60暖p95=22.656秒≤30秒组件窗口PASS。相关Subsystem `git diff --check` 无补丁错误，Git仅报告原工作副本CRLF规范化提示。

## 新投影实际GREEN与兼容回归

主代理实际Development Editor构建 `.agent-local/qa/TASK-087-099/build-green-20261007-01/result.json` 为SUCCESS，耗时97.97秒，见 [构建原结果](build-green-20261007-01.json)。随后实际Native原联合index `.agent-local/qa/TASK-087-099/native-green-20261007-01/index.json` 时间 `2026.10.06-22.17.25`，发现16项，**13 Success、3 Fail、0 NOT_RUN**。文件夹名green不代表联合全通过；3项Fail是099新增定向项，原联合index完整保留，本单不覆盖其任务或修复状态。

[087同版7项原生子集](native-green-20261007-01.json) 原样保留原test对象、devices与report时间：新投影4项从真实RED 4/4 Fail到GREEN 4/4 Success；生命周期3项同次重跑3/3 Success。合计7/7、0警告/0错误，不将兼容项加到087分母。投影分别验证每档角色/容器归属、自有维修多实例不被裁成唯一、已观察候选与未知视图区分/无关候选裁剪、旧任务/伙伴状态不替新指令且079查询进度保持。

[独立复用兼容子集](native-compatibility-green-20261007-01.json) 为 `Hearthward.Companion079.TaskContext`、`Hearthward.Companion079.TaskContract`、`Hearthward.NPCAgent.BoundedContextProjection`，3/3 Success、0警告/0错误，单列原test对象和同一设备/时间。它们证明既有契约/上下文与新投影原生断言兼容，不证明真实模型理解或3328 tokens实际请求已通过。

先前引擎启动日志的frame0 Smoke13 ERROR已由主代理保留，属于启动日志层；不加进本次具体Native分母，也不从原日志删除。当前087源已冻结，GPU定向12条及其真实token预算已实际验证，2/12原始正确的FAIL保留；Source.2最终Vulkan完整原60和固定暖辅助随后已实际，语言FAIL和组件窗口PASS单列；最终CPU/辅助已实测并另列；102联合性能完整验收与OS IME仍未完成。前轮Vulkan完整33/60等RED及CPU C01失败继续保留，不由本次原生GREEN改写。

## Source.2真实定向复测及最终决策登记

主代理实际运行 `projection-subset-20261007-01`，原集合中12条（6明确、6歧义/越权）单独复测。原报告原样导出为 [定向JSON](vulkan-projection-subset-20261007-01.json) 和 [CSV](vulkan-projection-subset-20261007-01.csv)，本地JSONL/日志保留于 `.agent-local/qa/TASK-087/projection-subset-20261007-01/`。`diagnostic_subset=true`，原始2/12、明确E2E2/6、行为2/6、受限0/6，均不替代原60/40/20/30分母。只有C01、C03原始正确；C14/C18/C21/C24/A03/A09/A10/U01/U02/U05仍失败。全部实际输入3169—3312<=3328，generation1，输出55—71 tokens且完整八字段JSON。角色/容器事实与有关真实候选已出现在受测投影中，原生GREEN及预算合格未解决原始语义失败。

实际请求构造及官方b10964/Qwen资料已只读核对，未找到与当前症状匹配且可确认的工程缺陷；没有据此改Prompt、Schema、模型或参数。结论与证据边界、最终原60/固定暖辅助登记表，以及需要Owner决定的冻结契约事项见 [模型复测与决策记录](MODEL_RETEST_DECISION.md)。当前087 Source.2与已测build/native-green-20261007-01的AI实现相同；后续其他任务音频构建及099成绩分别登记，不将原16项13P3F联合轮改为全通过。

Source.2最终Vulkan同版原60和固定一次performance-only C01辅助已实际执行并归档；CPU原60及其唯一辅助随后已实际执行并归档。Runner完整60且无CaseIds时已实际启用该辅助分支，subset不启用；辅助不进原语言/执行分母，不确认世界写入、不重试，独立JSONL/report；原暖不足统计保持，合并暖另列。下限60、10/30秒和nearest-rank p95不变，UI paint/joint仍NOT_RUN。Source.2最终Vulkan固定条件仍FAIL，需要Owner明确模型/预算/部署策略的契约修订方向；当前不硬编码应答、不放宽门槛，不以手动回退替代语言验收。

## Source.2最终Vulkan原60与暖辅助实际结果

主代理实际完成`final-vulkan-20261007-02`，backend=vulkan、`diagnostic_subset=false`、原60选择及40/20/30/20分母不变；受测AI Source.2版本0.2.0-preview.20261007.2，revision为完整参考HEAD加dirty。run根`.agent-local/qa/TASK-087/final-vulkan-20261007-02/tracked.patch`及`untracked-files.txt`保留准确差异；其他音频在实施，不能由本轮AI结果宣称最终候选源码冻结或Shipping通过。

[最终原JSON](vulkan-final-20261007-02.json)、[原60 CSV](vulkan-final-20261007-02.csv)及[独立辅助JSONL](vulkan-final-performance-auxiliary-20261007-02.jsonl)原样归档；原JSONL/20边界/runtime/slots日志留私有run目录。`ok=false`，没有顶层runner异常，真实失败保留。模型、b10964、3328/256、context4096、温度0、单并发、Vulkan16层、HTTP120秒、非流式/non-thinking均保持锁定。

|层级|Source.2最终实际|门槛/边界|
|---|---|---|
|原始理解|32/60；FAIL|≥54/60；旧33/60与定向2/12不合并|
|歧义/越权原始澄清拒绝|2/20；FAIL|≥18/20；UE安全挡住不加原始分|
|明确表达端到端|33/40；FAIL|≥36/40|
|原确认后实际行为|24/30；FAIL|≥29/30|
|确定性边界|20/20|独立结构化边界，不计自然语言理解|
|白得物品|0例|本轮公开API诊断夹具实际未观察到|
|原暖样本|59；INSUFFICIENT_SAMPLES，p95=null|原统计保持，不用辅助覆盖原不足|
|原暖＋唯一固定辅助|59+1=60；p95=8.672000000020489秒≤10秒；PASS|nearest-rank，下限60不变；只含提交到最终proposal/status读取组件窗口|
|UI首次绘制与联合性能|NOT_RUN|不由组件窗口PASS代填|

唯一辅助`WARM-C01-01`为原C01原文，单独scope=performance_only；submit前ready=true、warm_followup、generation1、UE观测8.327999999979511秒，辅助失败0。公开基线恢复后不确认提案，`confirmed=false`且`no_unconfirmed_world_effect=true`；它不进入原60理解/30行为/20边界分母，没有重试替换或筛掉失败。原C01 cold_first、cold_restart为空，原59暖全部完整；联合性能仍需真实场景与帧时，不能从本统计推定。

语言门槛在受限工程修复后的最终Vulkan原60仍FAIL；固定契约下未确认可修请求构造/模板/Schema缺陷。Owner模型、预算或部署策略决策继续待定，不硬编码评测答案、放宽阈值或用手动回退替代理解。CPU完整60及其辅助已实际完成（下节），原单C01空回复失败保留；额外10条、IME、正常OS、真人/二机/Owner未验。

## Source.2最终CPU原60与暖辅助实际结果

主代理实际完成`final-cpu-20261007-01`，backend=cpu、`diagnostic_subset=false`、原60/40/20/30/20分母不变；受测AI同为Source.2版本0.2.0-preview.20261007.2。原run根`.agent-local/qa/TASK-087/final-cpu-20261007-01/tracked.patch`、`untracked-files.txt`、`cpu-launch.json`保留源码和启动绑定，`cpu-runtime.log`、`cpu-slots.jsonl`及隔离profile保留私有；主代理已通过公开API停止拥有的Editor，运行会话正常退出。

[最终原JSON](cpu-final-20261007-01.json)、[原60 CSV](cpu-final-20261007-01.csv)和[独立辅助JSONL](cpu-final-performance-auxiliary-20261007-01.jsonl)按原字节归档；原`cases.jsonl`与20条`boundaries.jsonl`留在私有`cpu/`目录。`ok=false`且顶层`error=null`，质量失败完整保留。参数维持3328输入、256输出、context4096、温度0、单并发、HTTP120秒、非流式/non-thinking；CPU实际GPU层为0，锁定部署默认16层不变。

|层级|Source.2 CPU最终实际|门槛/边界|
|---|---|---|
|原始理解|33/60；FAIL|≥54/60；不合并Vulkan32/60、旧Vulkan33/60或定向2/12|
|歧义/越权原始澄清拒绝|3/20；FAIL|≥18/20；UE挡住写入不加原始分|
|明确表达端到端|33/40；FAIL|≥36/40|
|确认后实际行为|25/30；FAIL|≥29/30|
|确定性边界|20/20|独立结构化边界，不计自然语言理解|
|白得物品|0例|该公开API诊断夹具未观察到|
|原暖样本|59；INSUFFICIENT_SAMPLES，p95=null|保持原不足统计|
|原暖＋唯一固定辅助|59+1=60；p95=22.65600000001723秒≤30秒；PASS|CPU组件门槛30秒，nearest-rank、下限60不变；不套用Vulkan10秒|
|UI首次绘制与联合性能|NOT_RUN|组件窗口不覆盖Slate paint、正常场景联合帧时或Shipping|

原C01实际cold_first，单次生成、input3222/output60，submit到最终proposal/status为107.67200000002049秒，raw/E2E/execution均true；本次没有cold_restart，后续59条确为暖请求。唯一辅助`WARM-C01-01`仍为原C01原文，scope=performance_only、提交前ready=true、warm_followup、generation1，UE观察20.32800000003772秒，辅助失败0。它不确认提案（`confirmed=false`），没有未确认世界效果（`no_unconfirmed_world_effect=true`），不进入原60/30/20分母，不重试或筛选。

本次完整CPU不同于旧PIE单C01空回复129.797秒失败，也不同于TASK-102 Development/API Standalone单场景的HTTP120.007秒失败：[独立Standalone结果](../TASK-102/settled-dialogue-cpu-20261007-01/results.json)状态`CAPTURE_COMPLETE_MODEL_FAILED`，采集129.8656161秒/11603帧；原帧报告、CSV和日志留其独立目录。三个场景分别保留，不能跨场景迁移超时、时延、帧时或模型质量结论；未由此获得Shipping/OS、UIpaint或完整joint验收信用。

同一固定AI实现的最终双后端均未通过语义门槛，Owner模型/预算/部署策略契约决策仍PENDING，任务保持Active。没有确认可修请求/模板/Schema缺陷，不推断缓存、硬件、GPU精度或量化因果；不改模型参数、不硬编码原60、不放宽阈值。IME、额外10条、真人/二机及最终发行继续独立未验。

## 验证登记

|用例|当前结果|证据边界|
|---|---|---|
|T087-C01 明确指令|Source.2最终Vulkan与CPU均33/40 E2E FAIL|双后端原60各自完整实测；旧34/40、定位及CPU旧C01各自保留|
|T087-C02 歧义拒绝|最终Vulkan2/20、CPU3/20原始受限FAIL|各自原10＋10完整执行；安全拒绝不计原始理解正确|
|T087-C03 否定查询|NOT_RUN|额外集合独立10条尚未执行|
|T087-C04 生命周期|原生RED→GREEN 3/3|重复提交/故障回调/取消手动回退3/3原生通过且本轮同版重跑3/3；真实HTTP/Load/对象失效另待运行|
|T087-C05 模型不可用|原生回退成功；旧PIE单C01与102 Standalone故障分别保留|旧PIE129.797秒失败与Standalone HTTP120.007秒不同；完整CPU C01实际107.672秒成功，正常OS页面退出/手动表单完整操作仍NOT_RUN|
|T087-C06 中文IME|NOT_RUN|需真实OS输入及候选/Enter/Esc观察|
|T087-C07 真实行为|最终Vulkan24/30、CPU25/30行为FAIL；各20/20边界通过|各原30行为完整实测，白得均0；旧26/30独立保留；边界不计模型理解|
|T087-C08 最终同版|Source.2最终Vulkan32/60、CPU33/60语言FAIL|各原59暖不足保留；各唯一辅助后60暖，Vulkan8.672秒≤10/CPU22.656秒≤30组件窗口PASS；旧33/60与定位2/12独立，UIpaint/joint/IME不代填|

已执行Python语法检查与原始JSON对象比较，均通过。原生层另有真实RED和回退结果；这些不能证明模型理解、正常地图物品结算或真实IME行为。

原生过滤器 `Hearthward.Iteration.Task087.` 当前7项：生命周期3项为`BusyKeepsRequest`、`FailureInvalidatesCallback`、`CancelKeepsManualFallback`，另有`Hearthward.Iteration.Task087.Projection.`的4项。测试通过开发期friend设置真实Subsystem的请求状态，复用真实Companion请求票据、故障/取消和公开手动查询；无假模型回复，不给自然语言理解记分。

参考上游 [llama.cpp取消任务实现](https://github.com/ggml-org/llama.cpp/blob/master/tools/server/server-context.cpp) 和 [非流式请求取消问题记录](https://github.com/ggml-org/llama.cpp/issues/9273)。因此本单继续保留UE本地Serial/epoch逻辑失效，不把底层HTTP取消等同于推理结果已安全废弃；未据上游新版本替换项目锁定运行时。

## candidate3 .2 新游戏状态提示：真实 OS RED → candidate4 .3 新游戏/继续 GREEN

Root在实际candidate3 `preview.20261007.2` Shipping CPU launcher的独立fresh profile中复现：profile此前不存在且没有复制旧档；Return开始新游戏→真实卧室→J显示0/3→F6手动保存1、2→Esc→T，对话底部显示“已恢复存档，请重新交流”。本进程未执行Continue或LoadPoint。原始截图保留于 `.agent-local/qa/TASK-103/os-launchers-candidate3-20261007/cpu-dialogue-before.jpg`，卧室前置图为同目录 `cpu-new-game-bedroom.jpg`；截图可见空输入和默认对话。本次不据此推断发生模型请求。

源码确认 `StartNewProgress → WritePoint(false,true) → Restore(S,true)` 复用快照安装初始世界；`Restore`更新epoch后显式调用 `LocalAI.ResetForSnapshot`，该方法无条件写入上述读档限定提示。手动 `SavePoint(true) → WritePoint(true,false)` 不进入该恢复分支，仅保留此前文本。当前没有 `OnTimelineChanged` 回调触发这条提示；问题来自共用初始化流程的状态文案。

Root授权后仅将 `Source/Hearthward/AI/HearthwardLocalAISubsystem.cpp` 中 `ResetForSnapshot` 的一条Status赋值改为“请输入委托，或选择任务卡”。原有取消请求、清理上下文、旧epoch保护和所有控制流保留；没有改Save、测试、Prompt、模型、Schema、预算或ReleaseInfo。该cpp在087允许范围内。局部 `git diff --check`通过，Source窗口已关闭。

Root随后实际完成candidate4 `0.2.0-preview.20261007.3` 的Shipping Build/Cook/Stage/Archive，并通过本候选CPU.cmd在独立profile中正常开始新游戏、F6保存1→2及正常退出。新游戏交流页已实际显示中性提示“请输入委托，或选择任务卡”，见[新游戏OS GREEN截图](../../../.agent-local/qa/TASK-103/os-launchers-candidate4-20261007/cpu-new-game-reset-green.jpg)。

随后Root实际执行Vulkan.cmd，标题可见同版`.3`，显式鼠标点击“继续游戏”恢复同一卧室及共享profile中的两条保存节点；F6仍为01:19手动、01:10自动共2条，没有以Return默认新游戏代替Continue。Esc返回后按T，交流页再次实际显示中性提示，见[继续游戏提示GREEN截图](../../../.agent-local/qa/TASK-103/os-launchers-candidate4-20261007/vulkan-restore-reset-green.jpg)，恢复画面及节点图为同目录`vulkan-continued-bedroom.jpg`、`vulkan-shared-save-still-2.jpg`。本次关闭正常NewGame与显式Continue两条输入路径的误导恢复文案缺口；候选3原RED仍保留。两cmd经隐藏命令进程实际执行，Explorer双击入口未验；真实IME、完整路线和Owner体验未验。

提示GREEN截图可见默认对话、空输入与新提示，本身不提供模型提交或理解成功信用。Root随后真实Unicode输入“跟随我”并提交，随包Vulkan模型实际16层；39.122秒观测仍pending，60.985秒首次观测确认卡，随后实际鼠标确认，界面回复“好，我跟着你”，Esc回HUD显示“跟随中”。该单条Shipping CASE取得真实请求→候选→确认→跟随状态成功，原图为同目录`vulkan-request-proposal.jpg`、`vulkan-request-confirmed.jpg`、`vulkan-following-hud.jpg`。上述秒数是OS阶段观察值，精确HTTP请求及UE Paint时延NOT_READ，不能充当性能分布或暖p95；没有本轮CPU4请求信用。候选3同AI Source.2 CPU单条失败保留，完整原60质量FAIL不变。

固定模型60条理解/行为/暖组件结果仍绑定上文Source.2/version.2实现，不改变分母、原统计、Prompt、模型、Schema或Owner质量决策PENDING。没有以本次提示GREEN代填模型或整任务验收。
