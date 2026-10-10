# TASK-099｜动作音效、环境声与事件反馈同步

> 2026-10-10本机整合更新：099声音与104斧柄/分地面脚步已接入；实际斧头资产已保存并重开核验。Editor构建通过，099原生30项、104原生9项均已有通过结果。当前26事件／31个引用WAV。设备实听、完整动作/路线及新Shipping仍未验收。见[当前整合报告](../qa/TASK-104/integration-20261010/REPORT.md)。以下保留原交付及历史验证记录，其中NOT_RUN、云端403、旧事件数量仅适用于各段原始快照。

> 2026-10-10 最终声音修订：Owner已接受第三版入水的爆点软化试听，交付保持该段精确WAV；风采用降低6dB版本、出水采用Peludo CC0录音第二版，二者按先前反馈修订，未冒称逐项明确验收。落地／火源及场景触发不变。104项素材检查、6项配置检查和33项工具测试通过；UE编译／Native／引擎实听／Cook仍NOT_RUN。

> 2026-10-10 声音第二版候选：按Owner反馈降低风源6dB，并用Peludo CC0真实录音重做1.08秒入水／0.88秒出水；落地、火源和场景触发不变。20.30秒新旧A/B已供试听，Owner接受仍待确认。当前素材技术96项通过，UE与设备听感仍未运行。下面首轮43.07秒／合成水声制作描述属于首版历史，最新来源与结果见本轮报告。

> 状态：Active（2026-10-10 新增三项移动声音和火／风循环本地补丁；素材／静态检查完成，当前 UE 编译、Native、实听、Cook 待验证，不沿用历史23/23）。优先级：P1。Owner：XLingyyy。当前分支：`task099-audio-completion`。

[本批总入口](../planning/TASK-084-103/README.md) · [执行约定](../planning/TASK-084-103/EXECUTION_GUIDE.md) · [元数据](TASK-099.json) · [交接模板](../handoffs/TASK-099.md)

## 1. 目标与预期结果

补齐首线与已打磨场景的脚步/接触/命中/采集/环境声，使声音来自真实事件并可正确暂停、清理和混音；固定录音继续保持原暂缓决定。

## 2. 当前基础与事实边界

固定对白有已批准清单但录音未生产，动态弟弟回复仅文字。070明确本轮不录制、不用AI代替固定人声；不能用静音文件完成覆盖率。

本单参考基线为 `main@6fcf5c22e965f0f7409438f19bc7b09e96ffb058`；实际开工必须复核届时HEAD与依赖产物。这里不是本轮运行结果，旧报告PASS/FAIL均绑定原受测实现。新行为是本单计划，不能写成当前已经存在。

## 3. 依赖、开工门槛与最小必读

前置：[TASK-095](TASK-095.md)、[TASK-096](TASK-096.md)、[TASK-097](TASK-097.md)、[TASK-098](TASK-098.md)

依赖的约定产物与实际实现SHA就绪、Owner派发且共享写窗口空闲后开始。依赖不要求伪改旧任务Done；读取其实际交接并核对采用版本。

共同必读：`AGENTS.md`、`WORKFLOW.md`、`docs/START_HERE.md`、`docs/PROJECT_STATE.md`、`docs/design/CURRENT.md`及本单MD/JSON，然后执行本批指南中的接手/路径核对。

本单额外入口（路径为只读定位，不等于修改授权；前置任务产物需先实际生成）：

- `Source/Hearthward/Experience/HearthwardPresentationComponent.cpp`
- `Source/Hearthward/Experience/HearthwardPresentationComponent.h`
- `Source/Hearthward/Gameplay/HearthwardWorldPresentation.cpp`
- `Resources/Data/experience.json`
- `Resources/Audio/`
- `docs/planning/TASK-053-074/TASK-070.md`

## 4. 修改范围与协作边界

除本单MD/JSON、handoff、QA目录、专用规划目录和根`README.md`固定收尾外，候选范围如下；最终以**已获准基线JSON**为准。

- `art_source/TASK-099/`
- `docs/assets/TASK-099/`
- `Resources/Audio/TASK-099/`
- `Source/Hearthward/Experience/HearthwardPresentationComponent.cpp`
- `Source/Hearthward/Experience/HearthwardPresentationComponent.h`
- `Source/Hearthward/Gameplay/HearthwardWorldPresentation.cpp`
- `Resources/Data/experience.json`
- `Source/Hearthward/Tests/AudioFeedbackLifecycleTests.cpp`

本单不授予整个Content目录。准确包名单由现场引用/094清单确定，先补入已批准快照并核验LFS锁，才可编辑。未批准包仅可只读检查。

拟新增辅助/测试文件不表示仓库已存在；先搜索可复用类型。共享UI配置、公共头文件、WorldPresentation/CampaignWorld等按[写入调度](../planning/TASK-084-103/DISPATCH.md)串行。临时输出使用 `.agent-local/qa/TASK-099/<唯一run>/`，不得写入用户原档。

## 5. Agent具体实施步骤

### 1. 建立事件声音表

从现有Presentation入口及095动作时间轴列脚步材质、挥动、真实命中、受击、格挡、采集接触/完成、救援/升级提示、火/水/风等环境事件；每条记录触发源、位置、去重键、停止条件和素材状态。

### 2. 准备合法音源

优先复用已有来源可查素材，必要编辑裁切/降噪/循环点并保留源。未知许可或付费来源先登记阻塞，不下载未授权素材。固定voice条目继续UNPRODUCED，动态回复不合成配音。

### 3. 接触与结算分离

挥动/脚步可在实际动画接触点触发，命中/采集成功/升级成功必须来自真实成功事件。播放声音不能再执行伤害、消耗或发奖；取消动作不能补播“已完成”。

### 4. 去重不吞合法声音

用现有事务/事件ID或动作实例区分，同一成功回执重复处理只播一次；两次不同合法攻击可各播一次。不能用全局一秒节流掩盖重复回调、吞掉所有快速动作。

### 5. 环境生命周期

营地火/河水等声源按当前位置和已加载场景建销，离区、Load、结束夜袭和退出游戏清理。睡眠跳时不连播经过八小时的历史脚步/生产提示；新阶段只重建当前循环声。

### 6. 混音与设置

遵守现有主音量/分类设置/暂停例外，测试近远衰减和循环接缝。环境声不长期盖住关键命中/警戒反馈；不新增未设计的听声AI感知半径或天气难度。

### 7. 字幕与无声可玩

已确认固定台词仍有文字，未产出配音的状态如实保留；重要拒绝/完成反馈不能只有声音。可访问文本与真实事件一致，不写新剧情。

### 8. 实听与独立包

正常输入走序章、营地、采集、遭遇、救援返回，实际录带音轨片段并试听；仅波形/文件存在检查不能算声音体验通过。验证Cook能找到非UE运行音频及许可证随包需要。

## 6. 不可破坏的规则

- 不录固定人声、不用AI声替代、不给动态弟弟加TTS，除非Owner另有明确新决定。
- experience.json仅允许本单音效/环境事件映射；不得修改固定文本、voice生产状态或玩法参数来凑覆盖率。
- 音效触发读取事实，不产生事实；暂停、读档、区域卸载不能留下重复循环声源。

所有更改继续遵守只读显示、原事务执行、稳定ID、时间线隔离和不伪填验收规则。规则未明确的新玩法不硬编码；不删测试/吞错误/全仓重构来让当前检查通过。

## 7. 验收用例与执行方式

每条用例在隔离档/明确诊断夹具中建立前置，实际记录操作与断言。夹具、注入输入、真实OS键鼠、真实模型、真人样本分层统计；下面所有结果尚未运行。

|局部编号|场景|前置/具体操作|通过标准|初始结果|
|---|---|---|---|---|
|T099-C01|事件同步|真实挥击空中、命中、受击、采集成功/中断分别操作。|只有正确事件播对应声，不未命中播成功、不取消播完成。|NOT_RUN|
|T099-C02|去重|重复同一事务回执和连续两次不同合法动作。|前者一次，后者各一次，不以粗暴节流隐藏错误。|NOT_RUN|
|T099-C03|材质位置|走石/木/土等已支持表面，靠近/远离火水。|来源位置、接触类别与衰减合理，无明显循环接缝。|NOT_RUN|
|T099-C04|音量暂停|调主音量/既有分类，暂停/对话/恢复。|遵守设置及现有世界运行例外，恢复无音量突变叠声。|NOT_RUN|
|T099-C05|生命周期|区域卸载、睡眠、Load、重开新游戏。|当前音源数量有界，不重放历史事件或残留旧循环。|NOT_RUN|
|T099-C06|无配音状态|固定台词与动态回复在无录音时查看。|文字可用，voice仍未生产，不以静音文件冒充完成。|NOT_RUN|
|T099-C07|实听Cook|独立包录制带音轨正常路线并实际试听。|音效资源完整，主观试听与技术检查分别记录。|NOT_RUN|


全局挂接索引：T-002, T-003, T-011, T-025。它们来自现有TEST_MATRIX；正式数值以已批准增量/ACCEPTANCE和本单细化步骤为准，不恢复历史灰盒规则。

本单需新增/复用定向原生测试；新前缀建议 `Hearthward.Iteration.Task099.`。先注册并核验找到用例数量>0，再按公开UEClient运行，不能拿本表局部ID当UE过滤器。正常输入、渲染、真实模型或真人用例另行执行。

公共检查：`python -X utf8 scripts/validate_repo.py`、`python -X utf8 -m unittest discover -s scripts/tests -v`、`git diff --check`；在包含获批本单快照的实际基线上运行 `python -X utf8 scripts/validate_repo.py --task TASK-099 --base <实际完整SHA>`。UE操作/证据要求见执行约定；缺环境则明确NOT_RUN，不能将文件编写完成等同测试通过。

## 8. 交付物与完成定义

- 事件—声音映射、来源许可及可编辑音频源
- 音效生命周期/去重测试
- 带音轨的正常流程证据、实际试听记录与未生产录音清单

固定收尾：更新`README.md`当前说明、同名MD/JSON与`docs/handoffs/TASK-099.md`；在`docs/qa/TASK-099/REPORT.md`绑定最终完整SHA、资产/模型指纹、环境、命令、逐用例结果和明确限制。该REPORT为未来执行产物，本包不预填。

工程实现完成、Owner视觉、真人体验、二机、性能与发布各自登记。依赖下游可用产物写入handoff，不把未验项目涂为PASS。没有授权时交付本地改动和证据并写“未提交/未推送/未发布”，不得自增权限。

## 9. 本单明确不做

- 固定人声录制/TTS、重写剧情和音乐系统大改
- 声音触发伤害/奖励或改变AI听觉规则
- 用空音频/静帧冒充完整音频验收

## 10. 失败、范围变化和恢复

首先保留最短复现、受测状态和原始证据，再定位到本单或其他负责任务；不能通过清空存档/赠送物资/瞬移/改验收阈值绕过阻塞。回退只针对本单经核对的改动或资产备份，禁止未经授权reset/clean、覆盖他人工作和强制解锁。确需改变共享契约、保存格式、正式配表或包范围时提交具体差异与影响，先获准再施工。

## 2026-10-07 本地执行记录

用户授权当前Codex逐单本地实施，root批准独立SFX技术范围。已准备Kenney RPG Audio官方CC0原包、选定皮革袋操作声与PCM16运行文件；Development Editor repair build成功，3/3原生RED（0警告）确认无声音消费者后，已接入Storage正式已提交回执和播放/生命周期。最终整合build成功，修复后3/3原生GREEN（0警告/错误）；实际有声/Owner试听和完整覆盖由root后续验证。

音源/事件边界见 [SOURCES](../assets/TASK-099/SOURCES.md)、[EVENTS](../assets/TASK-099/EVENTS.md)，当前证据见 [REPORT](../qa/TASK-099/REPORT.md)。Owner试听、动作接触与环境全覆盖、独立包有声仍为NOT_RUN；固定配音保持UNPRODUCED。不编辑Content、WorldPresentation、共享Screen或Save，不新增依赖。

## 2026-10-07 成功事件与移动回调整合

Root实际Editor build及本单7条Native Success：6clean、NPC完成固定voice预期缺文件1warning、0用例错误。真实玩家Workshop/Repair/Harvest、NPC wood v2/craft/repair/Storage receipt、实际SavePoint/LoadPoint、实际Launch碰撞与fixture water Traversal已验证。新增事件均无cue且静默；仓储原3条持续GREEN。历史新增夹具错误、无LocalPlayer运动前置、World.EndPlay警告及第二轮非UUID runner拒绝均保留，详见本单REPORT/JSON。启动期process diagnostics单独保留，不声称整个进程无Error。

Root另授权最窄progress范围（Presentation三Source和本单测试）：新增RecordRescue、Facility GUID+Level、Quest.Claimed真实成功/拒绝/Load/暂停死亡夹具，总9条；先跑实际RED，消费者尚未实现。当前工作树UNPRODUCED缺文件gate和这2条新夹具NOT_RUN，不套用之前7项GREEN。只读动画Notify脚本已AST检查，UE执行待root；无资产写入。战斗producer由独立agent持有；fire burning/wind契约与环境loop素材缺口明确记录，技术工作不泛归为Owner音色阻塞。

2026-10-07 音源绑定窗口：root确认8真实CC0原OGG→8非零PCM16、13新event候选映射，保留原仓储映射与固定voice不变；逐项见SOURCES/CUE_AUDIO_CANDIDATES。明确Unbound fixture仅保留storage cue，新增2条Bound真实producer/位置/线性球形0→3000uu衰减/暂停/ActualLoad夹具；AudioLifecycle当前14+Combat producer3=预计17条，本轮未运行Native。三个源PCM16满刻度采样计数已登记，具体书本/dropLeather语义、响度与实际听感首件待试听，不记最终设计PASS。

2026-10-07 Foot追加：generic footstep真实CC0源及Presentation只读消费者已实现，累计9新源/14新event+原仓储15映射。Root实际LFS verify四精准Animation包ours/XLingyyy，权限已在Task JSON登记；Skeleton/Mesh不扩围，当前包未写入，mcp准备精确producer/fixture/writer，root唯一UE执行。正常片段接触、真实表面分类、实际听感与Cook不由权限/文件存在证明。

2026-10-07 当前冻源计数：15个独立event ID对应10个独立运行WAV路径，包含原仓储1个文件与新增9个文件；新增9个真实源OGG/PCM16、14个新event。AudioLifecycle14 + Combat producer3 + FootContact producer2 = 预计19条，尚未运行本轮Native。动画writer准备与Source冻结不表示四包已写入或正常路线已通过。

2026-10-07 内审帧契约修复：FootReceipt.Frame与GFrameCounter同步；消费者仅接受当前帧，帧变化清观察账，同帧独立GUID可分别播放，Frame0合法。foot不进入全epoch PlayedEvents，其它事务/战斗身份契约保留；两Presentation源再次冻结，静态PASS/本轮Native未运行。新增water/inspect_water_source.py仅QA只读，Bootstrap保存actor与live活动分开，UE执行NOT_RUN。

2026-10-07 西湖环境工程扩围：root已实际第二次UE只读READ98 vertices/96 triangles，保存actor与live实例分开。批准新增EnvironmentLoopWave .h/.cpp、独立EnvironmentAudioLifecycleTests.cpp和Resources/Data/TASK-099-water-audio.json；精确Source/JSON范围已登记同名任务JSON，不编辑Content或fire/wind玩法。真实RandomMind Vistula CC0源已保存/编辑mono PCM16内部候选，当前16 events/11独立运行WAV；实际Owner试听另NOT_RUN。独立环境3测试包含真实活动mesh+tag/current transform、far/hidden/unregister/destroy、真实Save/Load/epoch/pause/dead/EndPlay、实际procedural PCM连续/有界。当前无生产loop占位，静态资源PASS，3项Native/编译NOT_RUN，由root先跑真正RED后闭合生产。详见[water证据](../qa/TASK-099/water/README.md)。

2026-10-07 水声真实RED闭合施工：Root build09 PASS，Foot2P、Environment3F、0warnings、12errors仅缺Source，真实Save/Load前置P，后续PCM未达到。Presentation与新增LoopWave生产已实现/静态冻结，环境3测试保留真实三角内部远离actor中心的nearest位置和无限loop生命周期断言，生产编译/GREEN尚NOT_RUN。Root另授权真实empty swing active-window回执候选，准确Kenney knifeSlice已存在并保存/转换PCM16，consumer当前未提前接入Kind swing，待真RED。当前17events/12独立WAV/11新增保留源；全部Owner/设备试听/水声接缝/混音/最新Cook另NOT_RUN。Root唯一UE执行，不编辑水Content或新fire/wind规则。

2026-10-07 当前实测13收尾：Root Development/HearthwardEditor buildSUCCESS；本单23＝AudioLifecycle14+Combat4(含actual empty swing声源)+Foot2+Environment3，23/23P、联合28/28P、0选中用例warning/error。原07/08/09/12各快照与真实RED信用不合并。NoSound/NullRHI命令、独立UUID/Profile、13frame0 Smoke errors+1MCP启动warning和完整private stdout均如实保留。[当前REPORT](../qa/TASK-099/REPORT.md)已更新。

当前17event/12运行WAV/11新增保留源(10OGG+1MP3)，普通材料generic footsteps、真实BrotherSource及两Run右近起点P(9.999999747378752e-05s，非精确0/非wrap)，实际湖mesh当前96tri最近点/有界无限PCM生命周期NativeP；Source/Resources四动画包冻结。正常Hero/AnimGraph与自然地图流送、audible seam/混音/Owner试听、最新Shipping音频/许可/完整路线仍NOT_RUN。落地/泳模式实际callbacks已观察但独立cue未绑定，activefire/wind区域无确认契约；不泛称完整音效体验PASS。本单Active，未提交/推送/发布，scope baseline validatorNOT_RUN。

## 2026-10-10：落地／进出水、真实火源与局部风声补齐（本地补丁）

本轮基于已核验 `main@1bdc01b642dcf6e322ddca6cd4d7ad1a3fcc6351`，工作分支 `task099-audio-completion`，只交付本地补丁，未提交、未推送、未发布。Owner 本轮要求补 TASK-099 截图中的落地、入水、出水、火与风声音缺口；并明确同意山脊与瞭望点附近轻风、进出区域渐变、室内／入水停声、不增加天气或玩法影响。

- 3 个独立单次 WAV 已绑定 `movement.landed`、`movement.swim.enter`、`movement.swim.exit`，仍由真实 Landed／MovementModeChanged 回调触发；落地在 PostPhysics 等待跌落伤害结算后播放，使用实际 Hit.ImpactPoint。位置化移动声不积累永久 GUID 播放账本。
- 2 个 16 秒环境循环已绑定。火声只读取当前世界、已注册／可见／活跃的已知 Niagara 火焰资产与标签，排除烟；晚创建组件也能被读取，最多四个独立声源与独立 PCM 游标，距离衰减到 18 米。
- 风使用实际 `CampaignNode:route_ridge`／`CampaignNode:route_watch` 路标网格锚点；18 米内稳定、18—60 米平滑衰减，重叠区域只有一个环境底声。头顶 25 米内的真实 Visibility 遮挡、游泳、离区／失去锚点、暂停、死亡、Load／epoch、EndPlay 会清理声源。自然行走跨区使用空间渐变；传送或生命周期失效立即停止，不拖留旧区域尾声。
- 现有西湖映射、几何、固定对白和其它 sound_events 均保留；新环境初始化不再依赖水几何文件成功读取。新声音共 5 个，现配置合计 22 个事件／17 个独立运行 WAV。无固定人声或 TTS，未改地图、动画二进制或存档。
- 素材源／许可、可重制参数、43.07 秒试听拼接见 [音频制作说明](../assets/TASK-099/AUDIO_COMPLETION_20261010.md)。火／风是原创程序化声音设计；落地与进出水是 CC0 录音纹理和原创合成的组合，不冒称真实人物动作录音。
- 当前新版本的 UE 编译、Native、渲染 PIE、实机／Owner 试听、Cook／Shipping 全部 **NOT_RUN**。当前云环境无 UE 5.8.2 与 GPU；不能沿用下方 2026-10-07 的 23/23。

当前检查和后续验收入口见 [本轮报告](../qa/TASK-099/audio-completion-v1/REPORT.md)。本单保持 Active。
