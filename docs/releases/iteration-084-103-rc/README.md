# TASK-084—103 内部候选准备

日期：2026-10-07。当前计划为 candidate-3，CandidateID `iteration-084-103-20261007-3`，源码版本 `0.2.0-preview.20261007.2`，独立预期输出 `F:/HearthwardDemo/iteration-084-103-20261007-3`。状态 **NOT_FROZEN / NOT_BUILT / NOT_RUN / NOT_PUBLISHED**。099战斗成功回执当前只有声明与待RED定向测试，消费者及声音映射仍在实施；此截点不代表最终源码冻结。最终Shipping、正常输入、运行树清单、zip和hash均待root实际记录。

当前候选骨架见 [CANDIDATE3_BUILD_INFO.json](CANDIDATE3_BUILD_INFO.json)。root完成最终核验后将该候选的准确资料复制到新Windows树，作为运行树BUILD-INFO；仓库现有 [BUILD-INFO.json](BUILD-INFO.json) 与 [CANDIDATE_BUILD_INFO.json](CANDIDATE_BUILD_INFO.json) 原样保留candidate-2历史证据，不能作为candidate-3元数据。没有实施提交SHA，参考HEAD `6fcf5c22e965f0f7409438f19bc7b09e96ffb058` 加本批次dirty实现。

本轮源码范围包括共享只读投影与个人/队伍状态卡、请求生命周期及各档真实身份/容器/候选快照、制作搜索与材料目标、具体装备实例与转移预览、HUD/日志准备与紧急救援优先、堡垒节点及waiting目标修复、营地升级预览与会话反馈。095—098完成来源、绑定和既有行为调查，待审风格资产没有批量替换。099已测仓储事务SFX、真实玩家/NPC成功观察与落地/入出水入口；新战斗回执/新素材绑定的完成与测试状态由后续实际结果更新，固定录音保持UNPRODUCED。

087工程原生生命周期3项＋投影4项同版7/7成功，兼容3项单列；原联合轮16项13成功/3失败继续保留。099最新选中7项成功（6无warning、1缺固定voice文件warning），测试错误0；wrapper启动frame0的13条processError另留，不隐藏或加进7项测试错误。以上结果绑定各自Development Editor快照，未迁移为candidate-3包通过。

固定4B与参数的真实语言质量未达门槛：修复前Vulkan完整原60原始33/60、受限2/20、明确E2E34/40、行为26/30；修复后Source.2定向12为2/12原始，实际token预算全部符合。CPU单C01空回复超时失败保留。最终同版CPU/Vulkan原60及固定performance-only暖辅助待实际；定位12条不代原60，原生GREEN不代模型理解。需要Owner决定模型/预算/部署策略的契约方向，不能硬编码答案或放宽阈值。

历史candidate-2/version.1保持 **BUILD_SUCCESS / FINALIZATION_PENDING / NOT_PUBLISHED**，独立输出 `F:/HearthwardDemo/iteration-084-103-20261007-2`。第二次公开UEClient ok=true、ready、Exit0，Cook0errors/1warning（MCP EULA提示保留），BuildCookRun124.78秒；初始运行树150文件/4,885,690,146 bytes。首尝试UAT25/GameFeatureData规则缺失失败、FIRST_*文件和旧输出均保留。最小空扫描目录/AlwaysCook配置规则已纳入当时冻结，旧包不包含后续version.2差异。

历史OS部分证据和更正保持原样：[OS103](../../qa/TASK-103/OS_PARTIAL_NORMAL_INPUT.json) 仅直接exe标题版本、新游戏卧室/J/F6手动保存1→2及退出后独立启动列表；Return误选默认新游戏，其Continue声明已撤回，见 [erratum](../../qa/TASK-103/OS_CONTINUE_ERRATUM.json)。[101原档副本](../../qa/TASK-101/OS_ORIGINAL_COPY_COMPATIBILITY.json) 第三独立进程明确鼠标Continue恢复营地准备且仍7节点。两者绑定candidate-2/version.1，不能补candidate-3兼容或四个新路线节点。Shipping档隔离依靠UserDir，第二进程compositor截图限制继续保留。

[CPU启动器](Start-Hearthward-CPU.cmd) 与 [Vulkan启动器](Start-Hearthward-Vulkan.cmd) 已准备candidate-3专用 `%LOCALAPPDATA%/Hearthward/Candidates/iteration-084-103-20261007-3` profile，共享该候选自己的档，后端参数分别cpu/vulkan、GpuLayers16；CPU实际使用0层。二选一直接启动随包exe，无Python，不复制旧档或QA档。cmd仅静态核对，尚未执行；root只在candidate-3真实构建成功并核验后复制到Windows根。

[19单矩阵](../../qa/TASK-103/DELIVERY_MATRIX.json)、[技术审计](../../qa/TASK-103/REPORT.md)、[历史二次构建](../../qa/TASK-103/PACKAGE_BUILD_SUCCESS.json)、[首Cook失败](FIRST_COOK_FAIL.md) 与 [候选计划](CANDIDATE_PLAN.md) 各自注明范围。旧 [验收空表](ACCEPTANCE_RECORDS.json) 绑定candidate-2，当前candidate-3尚无真人/二机/Owner样本。连续首切片、完整主支线、正常库存制作、最终模型/联合性能、Owner视听、来源闭合和二机均未完成；固定录音UNPRODUCED，外部字幕预览Owner未决。

未提交、未推送、未合并、未发布。最终分发资料只能依据实际candidate-3冻结、构建和独立操作更新，不预填finalPass。
