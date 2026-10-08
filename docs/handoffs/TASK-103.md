# TASK-103｜内部候选交接

## 2026-10-08 候选6启动回归

2026-10-08 候选6 Shipping Build/Cook/Stage/Archive成功，Cook 0错误/1条MCP许可提示；97个新增资产中96个进入IoStore，未引用的独立箭袋网格正常剔除。实际启动失败：轻量Niagara在石堡CDO构造期间反序列化异常，进入ReportCrash后黑屏无响应；已通过原生线程栈定位，原候选保留并标记不可交付，公开UEClient关闭自有进程。 证据见docs/qa/TASK-103/CANDIDATE6_STARTUP_FAILURE.json及CANDIDATE6_GAME_THREAD.txt。后续修复与重建单独记录，以下历史候选记录不代表当前资产版本。

[任务](../tasks/TASK-103.md) · [元数据](../tasks/TASK-103.json) · [报告](../qa/TASK-103/REPORT.md) · [准确元数据](../releases/iteration-084-103-rc/CANDIDATE5_BUILD_INFO.json)

2026-10-07，Active/partial；参考HEAD 6fcf5c22e965f0f7409438f19bc7b09e96ffb058加共享分支codex/TASK-084-103-iteration本地dirty差异，未提交/推送/合并/发布。root负责Source/UE/实际package/OS/finalizer，本Agent只写获准QA/docs，Reviewer未指定。

候选5/version.4在准确冻结Source上已实际公开Shipping Build/Cook/Stage/Archive成功：UAT0/dry_run=false，总235.52s，Build71.70/Cook63.43/Stage39.73/Archive58.81s，Cook0error/1 MCP EULA warning保留。当前BUILD_SUCCESS_INTERNAL_CANDIDATE_ZIP_CREATED。双CMD脚本启动＋真实OS开局/保存/明确Continue局部验证通过；最终ZIP为F:/HearthwardDemo/iteration-084-103-20261007-5.zip，4,645,167,974 bytes、166运行文件。finalizer运行树/ZIP文件名与大小一致、有界文本扫描BOUNDED_TEXT_CLEAN、17event/12WAV/9许可/water98顶点96三角形配对通过；最终ZIP SHA256恰计算1次，sidecar存在，无重算。正式验收false，NOT_PUBLISHED。

Archive F:/HearthwardDemo/iteration-084-103-20261007-5/Windows、Stage E:/HearthwardQA/TASK-103/package-20261007-5/Staged、ZIP目标F同名.zip；证据.agent-local/qa/TASK-103/package-20261007-5/，准确freeze patch/untracked与结果绑定。5版本.4新增两项已有指引的窄修复，固定AI Source.2契约未改。候选5两CMD实际SCRIPT_LAUNCH_AND_OS_GAME_INPUT_VERIFIED，Title.4双见，共9张真实截图；fresh profile且未复制旧/QA档。CPU新游戏卧室/main01、Tneutral、F6自动1→鼠标手动保存2；Vulkan明确鼠标Continue恢复同卧室/main01，F6仍02:26manual/02:25auto共2、Tneutral。正常退出后全自有bootstrap/game进程gone且无候选模型残留。本轮无模型请求/中文输入/IME/完整路线/5旧档兼容信用。release四资料已实际随包冻结为BUILD_SUCCESS/OS_PARTIAL_VERIFIED/ZIP_EXTERNAL_RECORD_PENDING、profile5，属于pre-ZIP快照；ZIP完成后的最终结果在[CANDIDATE5_FINALIZATION](../qa/TASK-103/CANDIDATE5_FINALIZATION.json)、源Info5、外部record/sidecar。禁止刷新已压缩Runtime或重hash。

091顶与侧门各独立单case RED1Fail0W1E→GREEN1Success0W0E，四报告与两独立public Editor build原件保留，不合为两条。Nav12的四个既有指引节点实际通过；只用实际确认地形+Z100、固定XY250/Z220作QA查询，首段完整query/controller路径普通PathFollowing移动1976.913883cm。第二段双端投影true、FindPath valid=true/partial=true，严格停在SimpleMove前，自有Editor退出true。完整撤离/营地/首救/新保存和正常OS路线未完成，地图/碰撞根因未定。 [12安全摘要](../qa/TASK-103/NAV12_GROUNDED_FINAL_FIRST_BLOCKER.json)与[09—11历史](../qa/TASK-103/NAV09_11_PROJECTION_HISTORY.json)保存精确事实，不把QA插值Z问题写成地图/碰撞根因。

Native13本单09923/23、完整28/28、0test warning/error；wrapper13启动errors与1 EULA warning单列，NoSound/NullRHI不代实听。资源17event/12WAV/9必需许可、水几何98vertices96tris及source报告保持；候选5actual install/execute独立已P、最终BOUNDED_TEXT_CLEAN；未知Content许可和Owner听感不因此通过。5实际一INI提取的目标节/字段均剔除，inspection.json安全结果保留，不能证明未知Content来源闭合。

固定AI contract Source.2双后端原60均完成：CPU原始33/60、受限3/20、明确E2E33/40、执行25/30；Vulkan32/60、2/20、33/40、24/30。各边界20/20、白得0，语言质量均FAIL。原59暖不足单列；预声明辅助1条补成各60样本后，CPU组件p95 22.656s≤30、Vulkan8.672s≤10，只代表暖组件门槛。Prompt/模型/Schema/参数/预算/阈值未改，候选5两项路线修复不宣称重跑语言矩阵。

102独立Development/API单一稳定新游戏固定视角：Vulkan6183帧/61.1867968s，p99 16.7755ms、1%Low49.0051591fps、>50ms0；CPU11603帧/129.8656161s，p99 16.5324ms、1%Low51.3488456fps、>50ms2。两者联合帧门槛均FAIL。完整单请求均在CSV内：VulkanHTTP24.407s正常完成后TARGET_REQUIRED→clarify/无candidate语义FAIL；CPUHTTP120.007s超时→MODEL_UNAVAILABLE/raw空。两次单场均实际1920×1080/D3D12/SM6/Epic3/100/Vsync0；余五场、Shipping/OS联合性能未验，未把CPU超时与GPU语义错误混算。

历史候选1Cook UAT25、2构建/原档副本明确Continue更正、3CPU模型失败与新游戏提示RED、4双CMD脚本启动+OS局部/单一Vulkan跟随确认全部只绑定自身版本。4ZIP暂停、全部Stage/archive/五文档snapshot保留；本次不覆盖或删除旧包。完整历史表与原文件链接见报告。

root已完成install5→双CMD脚本启动与真实OS输入→源资料刷新→finalizer执行→ZIP单次SHA和sidecar。audit最终一次生成19任务/62 Resources元数据，142逐case验收矩阵已生成，formal false/Done0。候选4原档副本4旧节点→实际最新户外营地准备任务旧节点 Load→手工5→第二进程鼠标Continue仍5/6图是新增历史，只覆盖可见字段，5原档/四新阶段仍未验。正常完整路线、六场Shipping性能、整体模型质量、Owner、真人与二机分别未满足，NOT_PUBLISHED。正式scope基线/独立Reviewer/提交未发生，不改验证器或伪造实施SHA。

Root最终默认仓库校验已实际通过：104份任务快照、0错误；git diff --check退出0。首次18处错误仅来自私有临时旧草稿的相对链接，草稿内容保留并归档为文本快照，原失败记录保留。此次检查不替代UE、玩法、模型或正式task/base验收，原生与模型测试未重复。[实际检查记录](../qa/TASK-103/FINAL_REPOSITORY_VALIDATION.json)。
