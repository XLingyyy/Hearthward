# TASK-072 / 074 继续工程的真实条件

2026-10-05，只读审阅。权威树：`G:/GameFactory/Hearthward/.agent-local/task051`；本记录写入隔离树 QA。没有启动 UE、模型、UAT、Build、Cook、Git，没有复制、移动、删除文件，也没有计算 hash。下列采样方案尚未执行。

## 已确认的边界

- 当前五场景与模型联动帧时长为 **NOT_RUN**。已批准072 / ACCEPTANCE要求每后端分别采营地昼夜、序章、三人战斗、路线流送、营地管理；1080p、100%渲染、sg3、DX12/SM6、原生TAA、VSM，关闭动态分辨率、垂直同步及限帧。目标p99≤16.67ms、1%Low≥60FPS、>50ms帧≤0.1%。固定视角、单后端和纯Editor采样不能取得全部联动信用。
- 当前 **Cook为NOT_RUN**。Root明确没有执行本次Cook，没有磁盘不足失败日志或已触发门槛。G空闲约4.72GiB是容量风险；后文源文件大小与磁盘容量不能推导实际Cook峰值。
- Root当前UI修复/真渲染需结束后才能使用其串行UE窗口。本agent没有取得UE运行权。
- 074已批准设计仍依赖073验收；当前未冻结RC。065/073真人、070视觉/资产材料、第二机器、正常菜单与完整发行门槛按各自真实证据处理，不能由本记录补齐。

## 当前可复用的真实实现

| 实物 | 已确认能力 | 当前缺口 |
|---|---|---|
| [旧固定视角采样脚本](G:/GameFactory/Hearthward/.agent-local/task051/docs/qa/fix2/run_performance.py:27) | 公开UEClient启停自有PID、`UnrealEditor.exe -game`独立游戏实例、自然正式图、真实UE CSV Profiler、原生1080p/sg3/无帧限制；解析CSV `FrameTime`并计算p99及1%Low | 未启动模型；没有动态五场景切换或实际请求重叠记录。使用启动后固定删1800帧；无加载事件区间、>50ms比例、模型阶段/进程资源统计；`ok=true`表示成功解析，旧target_pass只检查两项。不能直接获得新072门槛信用 |
| [历史camp JSON](G:/GameFactory/Hearthward/.agent-local/task051/docs/qa/fix2/performance-camp.json) / [prologue JSON](G:/GameFactory/Hearthward/.agent-local/task051/docs/qa/fix2/performance-prologue.json) | 各6000帧，测4200帧；camp54.63s/p99 14.3025ms/1%Low66.34，prologue44.63s/p99 12.7703ms/1%Low69.85。明确static camera/no input | 历史Hearthward-ui-fix结果，未绑定当前施工源码与模型；不能继承为当前联动PASS |
| 同目录两份 `.csv.gz` | 实际头分别434/391列，含FrameTime、Game/Render/Slate、加载及GPU统计；已有raw格式可复用 | 不能由截图或Python回调间隔构造帧时长 |
| [正常Bootstrap→自然图QA流](G:/GameFactory/Hearthward/.agent-local/task051/docs/qa/fix2/verify_pie.py:40) | 实际title/new、等待Loading、正式World/HUD/Actor、公开存读档及正常Campaign.Interact | 后续存在显式定位，适合隔离诊断；整份脚本不适合作联动benchmark直接复用，不能宣称正常路线实玩 |
| [068模型runner](G:/GameFactory/Hearthward/.agent-local/task051/docs/qa/TASK-068/run_pie.py:29) / [矩阵](G:/GameFactory/Hearthward/.agent-local/task051/docs/qa/TASK-068/verify_model_matrix_pie.py:283) | 真实SubmitPlayerText、现模型/预算/单并发、公API计数/UE复核/monotonic时延 | Graybox、巨型临时floor、移走战斗Actor、显式摆人物/source/camp。复制此setup会移除要测的真实场景负载。冻结60/20不修改 |
| [072当前硬件facts](G:/GameFactory/Hearthward/.agent-local/task051/docs/qa/TASK-072/target-machine-reboot-facts.json) | i7-13650HX、16780480512B RAM、RTX4060Laptop8188MiB、driver577.00 | 仅瞬时功耗/温度；joint与second-machine均NOTRUN，AC/持续温度条件尚未固定 |

当前worktree的旧性能脚本还存在确定的入口缺陷：只运行其`--help`即在import失败，退出1：`ModuleNotFoundError: No module named 'engine_adapters'`，位置14。`parents[4]`在原仓层级是GameFactory，在task051隔离层级是`.agent-local`。根074已经采用的 `HEARTHWARD_FACTORY_ROOT` + ancestor `engine_adapters/ue5/__init__.py` marker可局部复用；当前072登记范围没有旧`docs/qa/fix2/`，本agent未修改它。

## 模型与正常UI的联动约束

[ScreenWidget:174](G:/GameFactory/Hearthward/.agent-local/task051/Source/Hearthward/UI/HearthwardScreenWidget.cpp:174) 明确离开dialogue时调用`CancelPending()`；[Subsystem:108](G:/GameFactory/Hearthward/.agent-local/task051/Source/Hearthward/AI/HearthwardLocalAISubsystem.cpp:108)会推进serial、取消HTTP、弃ticket。通过交流页发送后Esc回HUD进行战斗/行走，会取消请求。

[ScreenWidget:214](G:/GameFactory/Hearthward/.agent-local/task051/Source/Hearthward/UI/HearthwardScreenWidget.cpp:214) 在MenuPause开启时暂停营地等菜单；[Subsystem:706](G:/GameFactory/Hearthward/.agent-local/task051/Source/Hearthward/AI/HearthwardLocalAISubsystem.cpp:706)在World暂停时不ApplyProposal；Clock也暂停。UI联动必须记录实际MenuPause与World.IsPaused；若场景目标为管理界面加真实生产，使用既有普通菜单暂停设置关闭并记录，完成后按正常设置恢复。更换页面应发生在提交前。

最小有效诊断应先进入目标真实场景/页面，再通过已有公开 [SubmitPlayerText](G:/GameFactory/Hearthward/.agent-local/task051/Source/Hearthward/AI/HearthwardLocalAISubsystem.h:28)提交。它保留30m实际交流、人物Alive、World/epoch/ticket、真实count/schema/一次生成/确认链。不得同时取消或确认无关委托。模型生成期间真实Game/Render/AI/Slate继续运行；返候选不等于执行。每次样本保存GetGenerationCalls、serverPID、tokens、tier、status/reason、start/end与IsBusy/IsModelReady。

当前读取的[Runtime:78](G:/GameFactory/Hearthward/.agent-local/task051/Source/Hearthward/AI/HearthwardLocalAIRuntime.cpp:78)是4096服务端context、4/4线程、并发1、CPU0层/Vulkan16层、cache-prompt开启；policy输入3328、输出256仍保持。本方案固定读到的生产参数并登记，不能改模型/层数/线程/Schema/阈值来取得性能信用；072旧latency报告里cache关闭属于早期诊断版本。

## 五场景最小实际采样提案

这是初次容量诊断，建议每个场景条件先取**至少60秒有效真实帧**、且至少有一次完整实际generation区间在窗口内；若推理尚未结束或有效场景不足60秒，延长同一窗口或标INCOMPLETE，不能把空闲模型进程当正在推理。营地昼/夜分别成窗，共6种条件×CPU/Vulkan；每后端可复用一个自有进程顺序运行，保留epoch/load/场景边界。首次cold单独记录，后续已ready为warm；一两个暖回复只能给组件诊断，不能给暖p95门槛。

| 条件 | 最小真实场景与公共进入路径 | 有效窗口要确认的事实 |
|---|---|---|
| 营地昼 / 夜 | 正式自然图营地，保留真实兄弟、族人、设施/生产和光照；正常Campaign流程取得营地存档。昼夜用真实床Sleep或Camp.WaitAtCampfire(60/240/480)付费推进后重新保存，保留Receipt、食物/饱食与设施安全条件 | Clock.GetSnapshot实际minute/daylight，记录开始结束。昼/夜各自够60s，等待/读档加载不混入稳态；不直接设置Clock/太阳、不关世界tick |
| 序章 | L_Bootstrap正常new→自然图prologue_relic附近正式房屋/夜袭，Loading结束后HUD；弟弟活着且≤30m | Campaign实际phase=prologue、正式房屋/敌人/照明可见；不误采title或Loading画面，不将历史静态图移植为当前证据 |
| 三人战斗 | 正式自然图真实玩家+弟弟+至少1真实敌方战斗Actor，采用公开战斗输入/真实感知与伤害；保存/准备过程在窗口外 | 三个参与者持续存在、实际Combat行为和场景可见；瞬间杀光敌人后不能继续给战斗窗信用。弟弟alive/交流≤30m；无法稳定持续即INCOMPLETE，不造永生/假攻击/伤害状态 |
| 路线流送 | 从真实营地沿批准camp(-980,-750)m→route_fork(-630,-890)m向外移动，真实角色/相机/流送source；使用正常输入。需要时往返保留路径 | 移动实际越过WorldPartition加载范围，记录位置/实际streaming完成状态与可见地形。路径长度本身不能证明新cell加载。普通步行的流送hitch计入分布；显式Travel加载单独记录，不删掉路线中所有streaming帧 |
| 营地管理 | 安全营地公API/UI打开camp，在workers/facilities页显示真实族人/生产/队列；现有MenuPause关闭以保留真实生产 | 实际GetPage/Category/DescribeLayout及生产账本更新；模型在页面稳定后提交，避免离开dialogue取消。不得用离屏Widget Render截图作为整机帧 |

场景存档以正常真实进度取得；明确允许的定位只用于隔离性能场景准备，窗口内不teleport、不禁tick、不移走敌人/植被，不给065/073正常路线/真人信用。现有071两营地Native存档来自手建fixture/临时floor，缺正式自然世界包与正常角色路径前置，不能直接拿GUID传旧性能脚本冒充五场景checkpoint。最终RC/第二机器仍用正式正常入口。

## 最小接入，不新增生产helper

1. 复用旧run_performance的UEClient launch/poll/stop、CSV读法和画质设置，Root登记准确QA文件后再做局部修订。现有`runtime.launch_editor(extra_args=...)`与`runtime.launch_packaged`已公开支持参数，无需改Adapter。独立label/UUID/savepool保证当前证据不覆盖旧结果。
2. 首先只做一个正式自然图Standalone场景 + 一次真实模型请求的联动诊断，确认注入路径、真实Busy/gen1、CSV、输入返回与正常UI。若接入失败，在这个点停止扩成全矩阵。
3. Standalone可评估Epic现成RemoteControl `PUT /remote/object/call`调用上述public UFUNCTION；以`-RCWebControlEnable`启动，当前UEClient启动已有WebControl.StartServer。5.8本机[WebRemoteControl.cpp:230](G:/UnrealEngine/UE_5.8/Engine/Plugins/VirtualProduction/RemoteControl/Source/WebRemoteControl/Private/WebRemoteControl.cpp:230)确认`-game`默认关闭，flag可启用。标准库HTTP按[官方HTTP文档](https://dev.epicgames.com/documentation/en-us/unreal-engine/remote-control-api-http-reference-for-unreal-engine)调用，不导入Adapter私有transport、不加公共生产API。World/Actor/Subsystem引用取公开GameplayStatics/SubsystemBlueprintLibrary返回的实际路径，先验证当前World；不访问private字段/函数、不固定猜运行对象名。
4. 此Standalone接入**仅源码可行性，尚未实测**。本机SearchActor/SearchObject handlers分别[1864](G:/UnrealEngine/UE_5.8/Engine/Plugins/VirtualProduction/RemoteControl/Source/WebRemoteControl/Private/WebRemoteControl.cpp:1864)/[2002](G:/UnrealEngine/UE_5.8/Engine/Plugins/VirtualProduction/RemoteControl/Source/WebRemoteControl/Private/WebRemoteControl.cpp:2002)仍返回NotSupported；不能编写虚构`/remote/search/objects`路径。先验证公开函数调用和实际对象引用。若此限定接入不可用，现有PIE公API可先诊断，但只保留PIE evidence，不冒称完整Standalone/Shipping验收；无需现在造长期QA服务。
5. 采样用引擎已有`CsvProfile START`/`STOP`（本机命令参数为大写；[CsvProfiler.cpp:1050](G:/UnrealEngine/UE_5.8/Engine/Source/Runtime/Core/Private/ProfilingDebugging/CsvProfiler.cpp:1050)）。稳定场景后启动、有效时间到且当次model结束后停止。CSV包含原始序号/FrameTime；boot capture也可保留全部raw，但应按可证实Loading区间筛选，不机械删1800帧。
6. 初次优先CSV真实分布。失败或解释具体尖峰时，再用现成[Timing Insights](https://dev.epicgames.com/documentation/unreal-engine/timing-insights-in-unreal-engine)的cpu/frame/gpu/bookmark轨道定位；必要时[Memory Insights](https://dev.epicgames.com/documentation/en-us/unreal-engine/memory-insights-in-unreal-engine)分析分配。不默认启用全部Memory trace/Slate tracing，每个测量run记profiler开关与开销条件。Epic也说明[附加profiler会影响性能](https://dev.epicgames.com/documentation/en-us/unreal-engine/introduction-to-performance-profiling-and-configuration-in-unreal-engine)。

## 统计口径和确切缺口

- 帧来自引擎CSV `FrameTime`，保持原序列、有效窗口和排除reason/index；p99使用nearest-rank `sorted[ceil(.99*N)-1]`。1%Low延续旧定义：`1000 / mean(slowest ceil(.01*N) ms)`并明示；>50ms fraction=`count(FrameTime>50)/N`。原样保存各场有效时长、N、max、p99、1%Low、>50ms数量/比例及raw路径。目标三项全部检查，缺数据不给PASS。
- 只剔除确认的boot/显式Loading/场景准备/截图采集区间；模型prompt/decode、HTTP完成、UE复核、正常移动中的streaming、GC、真实UI刷新属于联动负载，不能按“慢”剔除。
- 模型20–30s是一次回复跨越很多帧的壁钟延迟，不是某一UI帧20–30s。单独保留server prompt/predicted/cache timings、提交→ready→HTTP→UE复核；UI首次可见Paint终点当前getter/IsBusy未覆盖，不能声称0.2s反馈或完整可见回复p95。
- 原CSV已含MemoryFreeMB/PhysicalUsedMB/VirtualUsedMB（本机CsvProfiler.cpp4188）；它们不足以代表模型进程、VRAM或页文件。可复用已用公开CIM进程、GPUProcessMemory、nvidia-smi按低频采样UE/model PID及系统RAM/commit/pagefile/显存；无需新依赖。freeRAM<1GiB按风险记录，OOM/crash按实际证据，不推断换页或降频。
- 5.8 [CsvProfilerConfig.h:8](G:/UnrealEngine/UE_5.8/Engine/Source/Runtime/Core/Public/ProfilingDebugging/CsvProfilerConfig.h:8)默认`CSV_PROFILER_ENABLE_IN_SHIPPING=0`、Shipping debug switches关闭；当前[Hearthward.Target.cs](G:/GameFactory/Hearthward/.agent-local/task051/Source/Hearthward.Target.cs)无覆盖。现旧Editor -game CSV适合组件/场景诊断；当前Shipping无法直接复用同参数。正式Shipping采样工具/具体Target窗口需要Root依据批准设计登记并验证，不能现在越过允许路径加宏，也不能把Editor build数据等价为发行包数据。
- CPU/Vulkan组件已有真实报告，质量/执行成功与帧门槛分开；072正式暖p95、coldready、UI反馈、五场景联动与独立第二机器都依各自实际样本。本次初次最小诊断不会因为一条请求完成就给这些总体门槛信用。

## 磁盘 / 074打包路径实物

子审阅一次公开disk_usage：C free28,931,342,336B≈26.94GiB，F90,376,437,760B≈84.17GiB，G5,063,753,728B≈4.72GiB。metadata stat统计Root Content2481文件/3,714,747,913B≈3.459629GiB；Runtime4文件/15,795B，仅knowledge、两个LICENSE、README，没有GGUF/server。此处未读模型bytes、未计算hash。原真实bundle可复用的事实不等于当前worktree已部署。

- [package_demo.py:22](G:/GameFactory/Hearthward/.agent-local/task051/scripts/release/package_demo.py:22)已有archive/log→output、stage→output/Staged；默认旧output为`F:/HearthwardDemo/20260924`。它未外置cook；旧F目录不能被推导成当前可覆盖的输出。
- [UEClient build.package:190](G:/GameFactory/engine_adapters/ue5/build/client.py:190)已有archive_dir、log_path、extra_args。技术上可直接传`-CookOutputDir=<独立批准F根>/Cooked/Windows`、`-stagingdirectory=<F根>/Staged`；无需修改Adapter。引擎单平台cook用原override，stage若末尾非Windows则再追加Windows，路径应明确以Windows结尾，见[CookOnTheFlyServer.cpp:7309](G:/UnrealEngine/UE_5.8/Engine/Source/Editor/UnrealEd/Private/CookOnTheFlyServer.cpp:7309)及[CopyBuildToStagingDirectory.Automation.cs:1438](G:/UnrealEngine/UE_5.8/Engine/Source/Programs/AutomationTool/Scripts/CopyBuildToStagingDirectory.Automation.cs:1438)。
- 普通temp可只为后续启动进程配置TMP/TEMP，API当前无env参数、subprocess继承host环境；不改系统持久环境。Shader可用`-AdditionalCookerOptions=-ShaderWorkingDir=<F根>/Shaders`，本机[Paths.cpp:1976](G:/UnrealEngine/UE_5.8/Engine/Source/Runtime/Core/Private/Misc/Paths.cpp:1976)有该参数。BuildGraph TempStorage不能代替BuildCookRun所有临时路径。
- F:/uagent-task-temp当前为空，可作为Root选址参考。本轮TASK074.allowed_paths没有F外部temp/cook/stage/archive具体根、Runtime bundle部署窗口，也没有Source/Target窗口；技术支持与执行授权分开记录。Root登记本轮独立子目录及必要Runtime路径后，才可按会话授权实施。当前子代理只可写QA072。
- 即使archive/stage/cook/temp外置，工程G上的Build/Intermediate仍可能写入。Content逻辑3.46GiB不能推导压缩cook/stage/build/DDC空间，84.17GiB F空闲也不能代替峰值实测。Epic的[打包说明](https://dev.epicgames.com/documentation/en-us/unreal-engine/packaging-your-project)明确Build/Cook/Stage/Archive分阶段；尚没有当前Cook容量不足或成功证据。

## 可继续与外部条件

现在可继续：Root完成UI相关验证后，用现成UEClient/CSV和正式自然场景做一次联动接入诊断；登记最小QA采样窗口后再扩6种条件×2后端。准备F独立目录参数及实际runtime部署清单也可继续，尚不执行搬运/Cook。

尚需真实条件：场景checkpoint/战斗与流送持续区间、Standalone公开调用Smoke、供电/温度与目标渲染条件、磁盘写范围+真实峰值、当前模型bundle部署、当前Game/Shipping产物、073/RC冻结与独立第二机器。不用当前CPU响应壁钟时间推导UI帧失败，也不把文档status Active当作这些实测门槛已经完成。

## 旧采样入口的精确局部提案

Root要求后追加：`run-performance-factory-discovery-proposal.patch`仅旧run_performance.py，复用当前package_demo.py11–18的环境变量/祖先marker方式，无新helper/依赖。删除parse_args前的固定层级import，增加os；root计算仍为parents[3]，之后按HEARTHWARD_FACTORY_ROOT及root.parents找到实际GameFactory再导入UEClient。`--help`在Engine导入/输出mkdir前退出；原根与隔离树的root/Saved输出逻辑、所有采样参数/统计均不改变。

补丁仅写ownQA；Root尚未应用。原Root脚本--help为实际exit1/import RED；候选在内存编译执行--help为exit0，namespace未出现factory/out/ue，确认未导入Engine或创建目录。AST通过，真实原仓/隔离树祖先marker均解析G:/GameFactory。静态检查不等于真实UE采样通过；完整check在`run-performance-factory-discovery-proposal-check.json`。Root登记精确旧QA路径后才能写入Root。
