# TASK-051｜联合性能、试玩协议与报告模板

状态：APPROVED，量化门槛统一取[设计D5／D6](../../design/DSGN-R23-input-traversal-acceptance.md)，本文件定义测法。本单局部运行覆盖见REPORT；本协议的联合性能与真人矩阵尚未执行。CPU与Vulkan各保存一份报告；没有覆盖的场景填NOT_RUN，缺少环境或前置填BLOCKED，并附原因。

## 1. 版本与环境

报告绑定完整受测源码SHA；若有未提交游戏改动，列出实际diff和资产状态，不只绑定HEAD。另记录内容数据版本、存档schema、地图、模型文件／模型修订、llama.cpp b10964或实际锁定版本、提示和检索版本、并发、上下文预算、输入／输出token、GPU layers／CPU线程。普通测试不重复计算checksum。

机器记录：Windows版本、CPU、内存、GPU型号及是否Laptop、显存、驱动、SSD、接电／电源及散热模式、实际GPU功率、后台负载。报告区分Development独立游戏、PIE及Shipping；最终性能门槛以实际发行包复测，PIE仅定位问题。UEClient指定当前工程，使用独立QA存档池；不覆盖玩家常用进度、不关闭其他已有UE进程。

画面记录：实际输出1920×1080、100%渲染比例、当前“极高”各sg与相关cvar、RHI／SM6、TAA、Nanite／VSM、VSync、帧率限制、动态分辨率／帧生成、日夜与截图。改过画质的结果独立成组，不能仍称同档。

## 2. 场景和采样

| 场景ID | 正常入口与路线 | 有效测量 |
|---|---|---|
| P01 | 夜袭可控后屋内、后巷至撤离 | 3分钟或完整可控段（较短则记录实际时长），包括转镜头与实际移动 |
| P02 | 营地日间，仓储、建造、制作／设施活动 | 3分钟，世界保持运行；不以暂停页面替代背景负载 |
| P03 | 同营地夜间及建筑／篝火照明 | 3分钟，固定同一路径、相同物件和岗位数量 |
| P04 | 近郊真实三敌遭遇、弟弟同行 | 完整战斗并记录至少3分钟场景负载，死亡／撤离与重试单列 |
| P05 | 049南浅滩归路至故乡入口再返回 | 全程连续采样，保留World Partition跨区与折返，按实际路径记录长度和时长 |
| P06 | 已夺回故乡、两营地访问及经营 | 至少3分钟；没有该阶段合法存档则NOT_RUN，不靠控制台制造胜利 |

每场景先记录模型关闭对照，再分别CPU和Vulkan联合运行；从等价合法起始节点恢复，消除时段／敌人／设施数量差异。复测首次先记录冷启动，不混入暖阶段。开始采样前单列预热30秒，仅排除第一次初始化；正式移动路线中的流送、GC、后台任务和AI计算全部保留，不按“异常”删帧。

优先复用[fix2 CSV采样入口](../fix2/run_performance.py)与[报告](../fix2/REPORT.md)，当前脚本硬编码旧工作区的部分须在正式测量任务中显式协调，不能无检查运行到另一个检出。用UE CSV Profiler采帧时，Unreal Insights记录Game／Render／RHI／GPU、流送和GC定位；最终Shipping可用的采样能力先核实，不能假定开发控制台存在。截图和录屏另取，不用截图编码录像的帧率当游戏FPS。

请求在有效测量窗口内发起并保持完整同期帧数据；不能将整个30分钟路线的高帧率稀释10秒推理卡顿。每次请求从UI接受到校验结果显示标记区间；汇总全部活动请求帧，再单列每次请求最慢帧／最长停顿。报告显示无请求阶段和活动请求阶段两组门槛，取消／超时窗口也保留。

暖请求每后端至少60例，低／中／高上下文档各20，最大输入不得超实际锁定预算；记录每例实测token，不能只按字符数分档。60例可复用D6语料但必须覆盖相应上下文档，按后台空闲／战斗／流送分类；固定相同请求集与温度，不调用云API。冷启动每后端3次独立启动，确认只退出该轮UEClient所创建进程。连续2小时真人使用另记内存趋势、循环委托、保存／退出及迟到回复，不用加速跳时替代持续运行。

## 3. 统计定义

- 平均FPS＝有效帧数／有效帧时总和；不能对每帧FPS直接平均。
- 帧时分位数用nearest-rank：排序后取第ceil(p×n)项。分别报告p50／p95／p99／最大值，不把GPU时间直接当完整Frame。
- 1% Low FPS＝1000／最慢ceil(0.01×n)帧的平均ms；>16.67、>33.33、>50和>100ms帧数及比例一并保存。
- 完整回复延迟＝UI显示已校验完整回复时刻−UI接受提交时刻，使用单调墙钟；与A/W分开，暂停不会把真实等待从指标中抹掉。服务器prompt_ms／predicted_ms作诊断拆分，不能冒充端到端。
- 成功延迟分位数与失败率并列；超时／退出没有成功延迟，保留其真实等待、原因和分母。缺少测量入口填NOT_RUN，不填0。TTFT仅真实流式首token可测时填写。
- 每场景／后端独立判定。加载、冷启动、稳定游戏与模型请求阶段分别记录；合计报告不能掩盖某个场景FAIL。
- 单次有效短场景不足预定样本时写实际样本和未覆盖范围；没有运行P06不代表发布质量通过。只在出现失败或新变化时扩测相关组，避免无依据反复全量执行。

## 4. 真语言和真实执行

每后端60例分40明确可支持、10必须澄清、10超范围／危险拒绝。由人预先标注目标、物品、数量、来源、限制、预期反馈及当时权限／世界条件；不能在看完模型输出后改答案。包含同义／省略、改数量、撤销／覆盖、稀有授权、未知事实及不可达，不把手动卡当模型理解成功。

原始理解分母40：结构化候选在UE规范化前已正确保留目标和全部限制。澄清分母10：未盲猜并指出实际缺槽；拒绝分母10：未触发世界动作并解释边界。UE护栏修正正确另列，不加回原始理解分子。

40明确指令端到端完成从真实输入起计，包括解析、卡片、玩家一次确认、执行和真实领域回执；理解／超时／计划／导航失败都留在分母。取消类不纳入“明确可完成”组，另测其正确取消。至少30次前置实际合法的已确认执行评估执行器成功率；数量、稀有权限、任务对象或资源在执行中改变的结果注明原因，禁止事后把系统失败归为“测试不合法”删除。

另20例异常矩阵覆盖缺料、满载、已耗尽、路被堵、目标失效、模型退出、任务替换、取消全部、授权撤销、读档后旧回复。分别检查正确解释、已提交真实物资保全和副作用；这些用例可正确拒绝，但不算产出成功。安全越权／重复奖励／旧时间线写入出现一次即FAIL。

## 5. 真人战斗和体验

玩家一对三：5人中3人首次接触；先10分钟统一移动／防御教学，不讲最佳打法。I／II阶段配装、技能、普通难度及三名同阶段守卫按047真实表保存快照，每人每档3局共30。弟弟另两档各15局由真人给已支持指令，主角不介入。起点无遮挡且三敌距离10—12m，没有隐形排队；同档费用、恢复和起始库存一致，每局实际付出材料。三敌全清且角色未真死亡为胜；逃跑、读档及中途退出计未胜，日志保留原因。分别列倒地、普通药、食物、格挡／闪避及失败，不靠只剪成功录像证明可靠性。

玩家组弟弟等待，不攻击或施救；弟弟组主角不攻击、施救或提供药物。出现跨角色帮助的结果保留但单列，不计独立成功。

切片5名首次体验者从夜袭后初次营地控制开始，到人口21／营地2阶并保存退出继续；包括迷路、委托误解、失败和回档，加载／现实中断／真暂停另扣。主线入口和作者解释需求记录，人工直接指路后的完成单列为辅助完成，不算无帮助成功。初始物品只能来自正常新游戏及实际已批准奖励。

首版3名首次体验者从夜袭到真实四区永久胜利及第二营地使用；不要求刷到人物60级／营地8阶。15支线及通关后安全入口安排总覆盖，至少一次保存／退出／继续及跨区返回。三类活动采用互斥标签：营地经营管理；资源获取／加工／种养；探索／任务／潜入战斗。混合行为按主操作和时间区间分段，战斗时弟弟后台采集不双计玩家时间。菜单整理物资计对应活动，真实暂停闲置不计。

每人记录实际时间、资源净流、首次显著成长、目标误解、弟弟完成的独立成果、失败恢复及主观评价。样本不足明确报告，Owner体验签收独立填写；成功率和阈值批准不能代签最终满意度。

## 6. 每后端报告模板

复制本节到正式执行任务的报告，替换空字段并附原始证据。null表示未测，结论仅PASS／FAIL／BLOCKED／NOT_RUN；模板本身不能成为测试结果。

```json
{
  "task": "TASK-051",
  "criteria_status": "DRAFT_PENDING_OWNER",
  "result": "NOT_RUN",
  "tested_commit": null,
  "tested_working_changes": [],
  "date": null,
  "operator": null,
  "build_configuration": null,
  "ue_version": null,
  "environment": {
    "windows": null, "cpu": null, "ram_gib": null,
    "gpu": null, "vram_gib": null, "driver": null,
    "power_mode": null, "gpu_power_w": null, "background_load": null
  },
  "render": {
    "output_resolution": null, "screen_percentage": null,
    "quality_and_cvars": {}, "rhi": null, "shader_model": null,
    "antialiasing": null, "vsync": null, "fps_limit": null,
    "dynamic_resolution": null, "frame_generation": null
  },
  "inference": {
    "backend": null, "gpu_layers": null, "threads": null,
    "model_revision": null, "runtime_revision": null,
    "prompt_version": null, "retrieval_version": null,
    "context_size": null, "parallel": null
  },
  "scenes": [{
    "id": "P01", "result": "NOT_RUN", "reason": "尚未执行",
    "save_pool": null, "save_node": null, "time_of_day": null,
    "measured_seconds": null, "frame_count": null,
    "average_fps": null, "p50_frame_ms": null,
    "p95_frame_ms": null, "p99_frame_ms": null,
    "one_percent_low_fps": null, "max_frame_ms": null,
    "frames_over_50ms": null, "active_request_frames": null,
    "active_request_p99_frame_ms": null,
    "active_request_one_percent_low_fps": null,
    "vram_peak_mib": null, "ue_memory_mib": null,
    "model_memory_mib": null, "available_ram_min_mib": null,
    "raw_csv": null, "trace": null
  }],
  "requests": [{
    "case_id": null, "tier": null, "scene_id": null,
    "result": "NOT_RUN", "prompt_tokens": null, "output_tokens": null,
    "queue_ms": null, "prompt_ms": null, "predicted_ms": null,
    "ttft_ms": null, "e2e_ms": null, "wait_until_failure_ms": null,
    "raw_understanding_correct": null, "guard_result": null,
    "confirmed": null, "executor_result": null, "receipt": null
  }],
  "language_summary": {
    "clear_total": null, "raw_correct": null, "e2e_completed": null,
    "ambiguous_total": null, "clarified_correctly": null,
    "unsupported_total": null, "refused_correctly": null,
    "confirmed_legal_total": null, "executor_completed": null,
    "abnormal_total": null, "abnormal_handled_correctly": null,
    "unauthorized_or_stale_effects": null,
    "successful_e2e_p95_ms": null, "timeouts": null
  },
  "cold_starts": [],
  "playtest_participants": [],
  "owner_experience_signoff": "NOT_RUN",
  "limitations": [],
  "evidence": []
}
```

P02—P06各复制一条scene，CPU／Vulkan分文件，不只留下P01模板行。真人记录表最少列参与者匿名ID／首次体验、配装／难度、步骤时长、结果、物资变化、帮助、回档与问题；一对三列每局胜负和对应原始记录。最终报告解释任何FAIL／BLOCKED和剩余范围，Owner签收保持单列。
