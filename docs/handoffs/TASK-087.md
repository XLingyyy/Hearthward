# TASK-087｜本地实现交接

[任务单](../tasks/TASK-087.md) · [元数据](../tasks/TASK-087.json) · [报告](../qa/TASK-087/REPORT.md) · [矩阵命令](../qa/TASK-087/RUN.md)

## 当前状态

2026-10-07，根目录`G:/GameFactory/Hearthward`，共享分支`codex/TASK-084-103-iteration`，基线HEAD `6fcf5c22e965f0f7409438f19bc7b09e96ffb058` 加本批次dirty实现。用户本轮已授权逐项本地执行084—103，主代理委派task_audit实施087；Owner为XLingyyy，Reviewer未指派。未提交/未推送/未合并/未发布。

## 最小生产修复

主代理首轮Development Editor构建成功，实际087原生1/3成功、2/3失败：自由文字重复提交改写原输入并释放pending；Fail未改变Serial。证据为`docs/qa/TASK-087/native-red-20261007.json`及其原联合index。

随后LocalAISubsystem.cpp仅增加忙保护和故障时Serial失效/Request取消释放，共3行；继续使用既有请求序号、提案票据、epoch、StillCurrent和原执行器。LocalAISubsystem.h只有开发期friend，新增3项有意义原生测试用于可控请求状态及真实Companion票据/无模型手动查询。AI源码窗口已释放，主代理repair build成功并实际运行`Hearthward.Iteration.Task087.`保存3/3 Success、0警告/0错误的GREEN：docs/qa/TASK-087/native-green-20261007.json。

## 实施与证据

原始60中文集合完整复制，原文/分类/预期和40＋10＋10/30行为/20边界分母不变。新矩阵脚本使用唯一run_id输出原回复、解析、物品前后状态和JSON/JSONL/CSV，CPU/Vulkan由主代理串行在PIE运行。额外10条表达独立集合均NOT_RUN，不混原基准。Python语法检查、原始JSON对象相等检查及相关git diff --check通过。

模型Qwen3.5-4B-Q4_K_M和b10964CPU/Vulkan执行文件可见；未修改模型/依赖/lock或重算大模型SHA。没有改Save、Content、UI、验收阈值、上下文、默认GPU层或120秒超时。CPU原C01单条PIE诊断已实际FAIL（空原回复、MODEL_UNAVAILABLE，generation1/input3206，最终状态129.797秒）；Vulkan原60完整真实RED已完成（33/60原始、2/20受限、34/40明确E2E、26/30行为、20/20边界、白得物品0），语言门槛未达标；随后Source.2最终Vulkan原60实际32/60语言FAIL，另有独立暖组件窗口PASS；最终CPU完整60已实际33/60原始、25/30行为FAIL，独立60暖p95=22.656秒≤30秒组件PASS；IME/正常输入仍NOT_RUN。原始诊断已导出docs/qa/TASK-087/cpu-c01-diagnostic-20261007.json。ScreenWidget.cpp的Esc预览处理顺序为待真实IME复现风险，文件不在087写范围，已交主代理协调。

## 新投影RED与受限输入修复

原完整Vulkan语言门槛失败后，4项新投影Native真实0Success/4Fail，原报告已导出docs/qa/TASK-087/projection-red-20261007-01.json。真实RED后Subsystem.cpp补齐5字段公开快照：弟弟Bag真实数量/availability、局部自然动作intent:item候选数、已接触且当前可观察护送人物ID；没有玩家背包、未见人物位置、GUID预选或新增Save字段。栏舍产物数量敏感，候选按至少1份，实际请求量继续原UE复核。System仅角色/代词/目录事实边界、库存问报与旧状态语义的替换前移，总字面+4字符，原Schema、两正例和锁定参数保持。投影由mcp_setup独占实现，主代理统一重建实际SUCCESS97.97秒，新投影真实RED4→GREEN4、生命周期同次3/3、兼容079两项及Bounded一项另3/3，087分母为7不是10；均0警告/错误。联合16项13P3F完整保留，不能把文件夹名green当全部成功。原test对象/devices/report时间见docs/qa/TASK-087/native-green-20261007-01.json及native-compatibility-green-20261007-01.json。087源已冻结，后续Vulkan定向12条实际token预算3169—3312≤3328已测，但仅2/12原始正确；Source.2最终Vulkan原60及固定暖辅助已实际执行，语言FAIL/组件窗口PASS分别登记；CPU完整60/辅助已实测；102联合性能完整验收与OS IME尚未完成；启动日志frame0 Smoke13 ERROR独立保留，不改具体Native分母。

## 交付与权限

README由主代理串行收尾。原生生命周期3项及新投影4项当前7/7通过，兼容3项单列；CPU单条真实模型诊断失败、修复前Vulkan原60完整RED及修复后Source.2定向12条FAIL已分别保留。Source.2最终Vulkan final-vulkan-20261007-02已原样归档JSON/CSV/独立aux JSONL：原始32/60、受限2/20、明确E2E33/40、行为24/30语言FAIL，边界20/20、白得0。原59暖INSUFFICIENT/p95=null保留；固定唯一WARM-C01-01无确认/无世界写入/不加语言分母，合并60暖p95=8.672000000020489秒≤10秒组件窗口PASS；UIpaint/joint尚NOT_RUN。最终CPU原60及唯一辅助现已实际归档（cpu-final-20261007-01.json/.csv及cpu-final-performance-auxiliary-20261007-01.jsonl）：33/60、3/20、33/40、25/30语言FAIL，20/20边界/白得0；原59暖不足保留，60暖p95=22.65600000001723秒≤CPU30秒组件PASS。渲染/真实输入/真人/二机/发行仍NOT_RUN。范围基线检查NOT_RUN，旧HEAD不含任务快照且没有提交权限；没有修改验证器。

## 下一位Agent

保留修复前Vulkan完整33/60 RED、新投影实际RED→GREEN，以及Source.2定向12条2/12 FAIL；12条定位集合与原60完整分母分别登记。当前请求构造、non-thinking模板和Schema路径未确认可修工程缺陷，固定模型/参数的语义门槛失败保持。最终同版Vulkan原60已真实32/60 FAIL保留，不与旧33/60或定向2/12混分母。CPU最终同版原60及固定一次performance-only辅助已实际完成，语言FAIL保持，辅助不进入原语言/执行/边界分母，无重试筛选。C01 cold_first107.672秒完成、无cold_restart；这与旧PIE单C01129.797秒失败及102 Development/API Standalone单场HTTP120.007秒失败不同，各留原证据，不跨场景迁移时延或质量。CPU私有run根tracked.patch/untracked-files、launch/runtime/slots和profile保留。Owner对模型、预算或部署策略的契约变化尚未决定，详见[复测与决策记录](../qa/TASK-087/MODEL_RETEST_DECISION.md)。禁止硬编码答案、换模型、调参或放宽原阈值。对显示和IME的改动需先协调UI写窗口及任务范围，真实OS输入证据单独记录。

## candidate3 .2 新游戏提示 OS RED → candidate4 .3 新游戏/继续 GREEN

Root实际Shipping CPU launcher在无旧档副本的fresh profile中执行NewGame进入卧室、J(0/3)、F6手动保存1/2、Esc、T，未Continue或LoadPoint，对话底部仍显示“已恢复存档，请重新交流”。原始RED截图为 `.agent-local/qa/TASK-103/os-launchers-candidate3-20261007/cpu-dialogue-before.jpg`。源码确认为NewGame复用 `Restore(S,true)` 后无条件 `ResetForSnapshot` 写入该限定文案；手动保存不触发Restore，也没有OnTimelineChanged回调造成该提示。

按Root唯一Source授权，已用apply_patch仅将 `HearthwardLocalAISubsystem.cpp::ResetForSnapshot` 的Status行改为“请输入委托，或选择任务卡”；其他控制流、Save、测试、Prompt、锁定模型参数和ReleaseInfo未改。Source冻结、局部diff检查通过。Root实际candidate4 `.20261007.3` Shipping构建完成，CPU.cmd正常新游戏、F6保存1→2及退出已验证；交流页新提示取得实际OS GREEN，原图为 `.agent-local/qa/TASK-103/os-launchers-candidate4-20261007/cpu-new-game-reset-green.jpg`。

Root随后实际执行Vulkan.cmd并显式鼠标Continue，恢复同一卧室，F6仍为01:19手动＋01:10自动共2节点，T交流再次显示中性提示，实际Continue提示GREEN；原图为同目录`vulkan-restore-reset-green.jpg`，节点图`vulkan-shared-save-still-2.jpg`。两启动脚本经隐藏命令进程运行，Explorer双击仍NOT_RUN。真实Unicode“跟随我”由随包Vulkan16层模型生成确认卡，实际鼠标确认后回复“好，我跟着你”，Esc回HUD为“跟随中”；本单条Shipping CASE成功。39.122秒pending及60.985秒首次候选是OS观察值，精确HTTP/UE Paint时延NOT_READ，不补完整60、行为或性能分母。CPU4未重复请求，没有新CPU4信用；真实IME和完整路线未验。

保留candidate3 .2原RED；新提示GREEN只绑定上述Shipping输入路径，不迁移旧Native或模型结果，不提供模型提交/理解成功信用。087原Source.2/version.2模型60统计、质量FAIL和Owner契约决策PENDING保持；详见 [REPORT](../qa/TASK-087/REPORT.md)。
