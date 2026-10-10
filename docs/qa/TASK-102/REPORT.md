# TASK-102｜联合性能执行记录（Partial）

2026-10-07。实际根目录 G:/GameFactory/Hearthward；分支 codex/TASK-084-103-iteration，HEAD 6fcf5c22e965f0f7409438f19bc7b09e96ffb058＋本地未提交批次改动。用户已授权本地实现及验证；Root统一 UE/模型/构建生命周期。报告不宣称当前源码与最终包完整绑定，未提交/推送/合并/发布。CPU与Vulkan稳定单场均已实际捕获，各自1%Low未达、CPU请求超时／Vulkan语义FAIL，均有容量警告；完整六条件×CPU/Vulkan、暖p95、首次Paint、第二机器仍未执行。

## 已有真实证据

[EVIDENCE_INDEX](EVIDENCE_INDEX.json)保留实际历史入口，[新runner运行边界](RUN.md)列出命令及缺口。

首轮 Vulkan `prologue-vulkan-20261007-01` 是启动即 capture 的 boot 诊断。原 parser 在 metadata 的小写 `[commandline]` 和未转义命令逗号处失败；Root仅修QA解析大小写，并对同一原 CSV 重解析，保留原异常及 `reparsed-result.json`，没有删帧或重跑来替换旧失败。完整9000帧累计116.4529097秒，p99=11.7866ms，1%Low=2.0998256263FPS，>50ms共7帧（7/9000=0.0777778%）。统计口径维持原072：p99索引floor((N−1)×.99)，最慢max(1,N//100)帧均值倒数。帧样本包含启动、加载和frame60—77画质命令前的区间，因此不给稳定场景门槛信用。

同份 CSV `systemresolution.resx=1920/resy=1080`，RHI D3D12/SM6、VSync=0；同次日志已执行sg3/ScreenPercentage100/动态分辨率0/TAA2/VSM1/Nanite1。Sky的2560×1600捕获图只表示截图尺寸。代码核查未发现ScreenWidget、PlayerSettings或WorldPresentation初始化自动覆盖CLI分辨率的入口；显示配置应用与全屏切换来自显式用户动作，UE原生CLI处理windowed/ResX/ResY/ForceRes。无需据截图追加修改产品显示配置。

首轮实际 model generations=0。正常输入发生于CaptureExit之后，未产生本窗联合请求证据；冷ready、暖完整UE回复p95、联合采样均不能由这9000帧补齐。

第二次 `prologue-joint-vulkan-20261007-02` 计划24000帧，实际Frame3启动失败：Renderer.dll读取访问冲突、Background Worker，breadcrumb含Frame1 Nanite NodeAndClusterCull/MainPass/VisBuffer。原 `failure.json/runtime.log/resources.jsonl` 保留；没有完整CSV、模型未启动、frame60画质命令尚未执行。结果为 FAILED_STARTUP_RENDERER_ACCESS_VIOLATION，不能构造正常性能结果。CrashContext.AvailablePhysical=0字段没有证明OOM；无Hearthward callstack、安装缺Renderer PDB/cdb，目前首因仍未定位。

官方 [UE-230827](https://issues.unrealengine.com/issue/UE-230827)记录含5.8.2、首帧偶发NodeAndClusterCull GPU崩溃并处于未解决状态；其复现为Hillside/Nanite VSM shadow/GPU crash。本次为CPU worker Renderer AV/MainPass，现有证据不足认定同一缺陷。本批WorldPresentation.cpp无diff；新PresentationComponent改动为音频/事务/角色移动订阅与暂停tick，没有Nanite/材质/渲染资源创建变更，未找到可证明其导致本次Renderer生命周期错误的调用链。保留批准配置，未关Nanite/降画质/改驱动。

## Vulkan稳定单场实际结果

根Agent实际执行 `settled-dialogue-vulkan-20261007-01`；[原结果](settled-dialogue-vulkan-20261007-01/results.json)、[完整原帧CSV](settled-dialogue-vulkan-20261007-01/frames.csv)、[frame-report](settled-dialogue-vulkan-20261007-01/frame-report.json)及[汇总](settled-dialogue-vulkan-20261007-01/summary.json)已归档。原CSV13,342,673字节，未改帧／统计公式／画质／模型参数，未计算hash。输入为public API注入原话“采集1份木材送入营地仓库”；场景仅 `normal_new_initial_scene_dialogue_fixed_view`，未进行确认或执行，Development uncooked standalone `-game`，没有正常OS／Shipping／六场完整验收信用。

10秒稳定暖机在CSV窗外；全部6183数字帧累计61.1867968秒，坏帧／剔除帧0。采样前后实际读回全部sg3、ScreenPercentage100、动态分辨率0、TAA2、VSM1、Nanite1、VSync0、MaxFPS0；完成CSV元数据为1920×1080、D3D12／SM6、Development。该次正常完成采样并验证自有UE进程退出，旧Frame3 Renderer AV历史不覆盖；首因仍未定位。

| 原批准帧门槛 | 实际Vulkan单场 | 局部比较 |
|---|---|---|
| p99≤16.67ms | 16.7755ms | FAIL |
| 1%Low≥60FPS | 49.0051591185FPS | FAIL |
| >50ms≤0.1% | 0／6183＝0% | PASS |

同一UE日志确认实际generation1、required_minimal、3221输入／59输出tokens。model启动→ready10.617秒，generation→HTTP终态13.542秒，提交→HTTP终态24.407秒；公共getter提交→响应24.4068347秒、1秒轮询观察完整UE终态24.8588829秒，各自时基保留。实际runtime startup和完整请求均位于CSV窗内，但整个61.19秒窗没有全程推理负载，推理重叠仅13.542秒。HTTP正常返回raw，原模型输出 `nature_collect / wood / additional_acquired / known_target` 被真实 `TARGET_REQUIRED` 校验转为 `clarify`，无candidate；单条语义为FAIL。结果保留 `CAPTURE_COMPLETE_MODEL_FAILED` 和原 `model_reply_received:false`，该布尔代表runner未得到预期采集候选，不能解释为没有HTTP返回。源木材16→16，未确认事务没有执行。

关键原始模型证据为 local-ai.log（仅本机留存，未随仓库公开；路径：settled-dialogue-vulkan-20261007-01/local-ai.log）、[model-progress.jsonl](settled-dialogue-vulkan-20261007-01/model-progress.jsonl)和原results的raw／reason／same-log区间；原runtime.log／http-events.jsonl留在private run。公开归档没有HTTP headers、密钥或完整模型启动命令。一次含cold-start的请求不产生暖p95、TTFT、≤0.2秒首次反馈／Paint或087矩阵信用。

[resources.jsonl](settled-dialogue-vulkan-20261007-01/resources.jsonl)保留12次低频CIM样本，读取失败0；可用物理RAM最低 **0.5588264465GiB，3次低于1GiB**，按原门槛记录容量风险。UE/model WorkingSet观察峰值4.640266／2.646286GiB（12／6样本），GPU总显存观察1496—5632MiB；总GPU量不能归因模型单进程。没有OOM因果结论，低频短窗不能证明无持续增长；每进程VRAM和长路线／重复菜单仍NOT_RUN。

## CPU同场独立实际结果

随后根Agent独立执行 `settled-dialogue-cpu-20261007-01`，[原结果](settled-dialogue-cpu-20261007-01/results.json)、[全部帧CSV](settled-dialogue-cpu-20261007-01/frames.csv)、[frame-report](settled-dialogue-cpu-20261007-01/frame-report.json)及[汇总](settled-dialogue-cpu-20261007-01/summary.json)已归档。原CSV24,924,094字节未改；同一场景／原话／10秒窗外稳定，独立新pool、UserDir及UE／模型进程，不沿用Vulkan进程。实际backend由startup日志确认为cpu。每次独立全部帧报告，不合并两个后端的帧或延迟分母。

11603数字帧累计129.8656161秒，剔除0；p99=16.5324ms（≤16.67，PASS），1%Low=51.3488456425FPS（<60，FAIL），>50ms=2／11603＝0.0172369%（≤0.1%，PASS）。实际1920×1080／D3D12／SM6／Development，sg3／100%等前后cvar保持原值。延长采样来自等待该次请求真实终态，不是删除120秒失败区间。

CPU startup→ready8.496秒；实际generation1、required_minimal、3221输入tokens，generation→HTTP失败120.007秒，提交→HTTP失败128.69秒，全部区间位于该CSV；公共getter终态观察129.7950766秒。业务HTTP120秒期限保持，HTTP_FAILED／code0／success0→MODEL_UNAVAILABLE，raw为空、output0、无candidate／执行。CPU模型语义无可评估响应，原runner `single_semantic_expectation:FAIL` 与 `CAPTURE_COMPLETE_MODEL_FAILED`保留；分类为transport timeout，不能与Vulkan有raw的TARGET_REQUIRED语义错误合并。失败路径getter latency=0未代表零等待时间。源木材16→16；未确认动作。

CPU local-ai.log（仅本机留存，未随仓库公开；路径：settled-dialogue-cpu-20261007-01/local-ai.log）／[progress](settled-dialogue-cpu-20261007-01/model-progress.jsonl)与[resources](settled-dialogue-cpu-20261007-01/resources.jsonl)均已归档，原runtime.log/http-events留private。17资源样本读错0，最低可用物理RAM **0.4245300293GiB，9次低于1GiB**；UE/model WorkingSet峰4.639442／4.139626GiB（17／12样本），GPU总量1505—3932MiB。独立记录容量风险，没有证据把HTTP超时归因为内存；未证明OOM或长期增长趋势。自有UE退出与stop结果已验证，模型终态PID0；根Agent确认自有模型已退出。单条cold请求仍不提供暖p95、Paint或完整六场门槛信用。

## 稳定诊断入口与当前覆盖

新增 `run_standalone_joint.py` 复用072真实RC正常new链；Loading=false/真实交流页/未暂停/可交流/画质读回后，以10秒窗外稳定再START，单条真实请求，至少60秒完整数字帧，STOP后核对实际resolution/RHI与同log完整generation区间。Fatal/进程退出检测保留失败。独立Source、旧072、模型锁与Save未修改。

[STATIC_CHECK](STATIC_CHECK.json)已完成7项静态/只读检查，含同一真实9000帧精确统计复算、坏帧失败/有效0保留/小写metadata、Win32存活检查、历史fatal检测、一次Submit与固定new控制流。均为静态或历史数据复算；新runner已分别归档上述CPU／Vulkan实际公开API、模型与完整CSV；完整六场与正式验收仍NOT_RUN。

|实际范围|CPU|Vulkan|六条件信用|
|---|---|---|---|
|normal_new_initial_scene_dialogue_fixed_view 稳定注入输入诊断|CAPTURE_COMPLETE_MODEL_FAILED：11603帧／129.87秒；1%Low FAIL，HTTP120秒timeout|CAPTURE_COMPLETE_MODEL_FAILED：6183帧／61.19秒；帧目标FAIL，语义FAIL|NOT_EVALUATED；单场Development/API层|
|营地昼/夜、三人战斗、路线流送、营地管理|NOT_RUN|NOT_RUN|真实前置缺口见RUN.md|
|暖完整UE回复p95/反馈≤0.2秒/TTFT|NOT_RUN|NOT_RUN|单请求/1秒getter不替代Paint或样本分布|

## 本单验收状态

|用例|当前结果|
|---|---|
|T102-C01目标配置|PARTIAL：CPU／Vulkan单场前后cvar与完成CSV确认1080p Epic100/D3D12/SM6；完整六场／最终包仍NOT_RUN|
|T102-C02双后端场景|PARTIAL／已测门槛FAIL：Vulkan6183帧p99与1%Low FAIL；CPU11603帧1%Low FAIL，p99单项PASS。六条件NOT_RUN；boot generations0／旧CRASH保留|
|T102-C03端到端延迟|PARTIAL／已测请求FAIL：两个cold请求完整落在各自CSV；Vulkan HTTP24.407秒后语义FAIL，CPU提交128.69秒后HTTPtimeout。暖p95／Paint／反馈门槛NOT_RUN|
|T102-C04内存|PARTIAL／容量警告：Vulkan最低0.559GiB（3／12次<1GiB），CPU最低0.425GiB（9／17次<1GiB）；长路线／重复菜单／每进程VRAM仍NOT_RUN|
|T102-C05优化正确性|NOT_RUN：当前只QA入口，未作性能生产修复|
|T102-C06视觉回归|NOT_RUN：无本单资产优化包，未取得Owner视觉结果|
|T102-C07最终可重算|PARTIAL：9000boot重解析及Vulkan6183／CPU11603稳定原CSV／统计／模型／资源已保留；最终同版本双后端全矩阵与构建绑定NOT_RUN|

目标帧门槛仍每场p99≤16.67ms、1%Low≥60FPS、>50ms≤0.1%；Vulkan/CPU暖p95≤10/30秒、coldready≤60/90秒、反馈≤0.2秒、业务HTTP120秒。保持原4B/3328输入/256输出、单并发、默认16GPU层与锁版本。模型语义、实际执行、FrameTime、资源和现场输入分别统计，不删异常或改公式通过。

当前限制含一次尚未定位Renderer启动AV历史、双后端1%Low失败、Vulkan模型语义失败／CPU HTTP超时、低于1GiB容量警告及未执行完整验收，不归为需要Owner统一确认。其余五条件需要真实场景checkpoint/持续战斗/实际流送/经营生产；不通过写进度、赠物或瞬移制造门槛。正常OS boot、稳定API诊断、Shipping、真人和二机各层单列。Master README/PROJECT_STATE由Root收尾；范围基线含当前任务快照的真实提交检查、全仓公共检查及最终完整包绑定由Root统一执行，当前不伪填PASS。
