# TASK-099：三项移动声音、活火与局部风声本地补丁

> 2026-10-10本机整合更新：099声音与104斧柄/分地面脚步已接入；实际斧头资产已保存并重开核验。Editor构建通过，099原生30项、104原生9项均已有通过结果。当前26事件／31个引用WAV。设备实听、完整动作/路线及新Shipping仍未验收。见[当前整合报告](../../TASK-104/integration-20261010/REPORT.md)。以下保留原交付及历史验证记录，其中NOT_RUN、云端403、旧事件数量仅适用于各段原始快照。

## 受测范围与状态

- 日期：2026-10-10（UTC）。基线：`1bdc01b642dcf6e322ddca6cd4d7ad1a3fcc6351`，已实际 fetch 并核验为 PR #67 合并后的 main。
- 独立本地分支：`task099-audio-completion`。本报告绑定基线加本次未提交补丁；没有虚构实现提交 SHA。交付包 manifest 记录每个变更文件的基线与当前 SHA256。
- 本轮交付源代码、5份运行WAV、重制工程、试听拼接与测试。**未提交、未推送、未发布；TASK-099 保持 Active。**
- 当前环境：Linux，Python 3.12，约9.7 GiB RAM／29 GiB可用磁盘，ffmpeg与Git可用；未找到 UE Editor、UE 5.8.2构建工具链或GPU运行环境。

## 2026-10-10 第二版声音候选及第三版入水试听（入水已接受）

Owner反馈首版风偏大、入水／出水不自然。当前修订将风源文件降低6.000dB，保留同一16秒波形与0.24运行增益；落地和火源文件逐字节不变。两个水声重做为Peludo公开CC0录音的不同剪辑，不使用原来的气泡振荡器、合成噪声、水声反转或变调。入水1.08秒／-7dBTP，出水0.88秒／-11dBTP；新的出水更短、更轻。来源是作者水桶实验录音，未冒称人体入湖实录。

新旧A/B为20.30秒，每项旧→新：风0.50–6.50／7.00–13.00，入水13.65–15.03／15.53–16.61，出水17.26–18.42／18.92–19.80。风两版均乘0.24，水均乘1.0；未归一化掩盖音量差。新版完整试听亦使用火0.35／风0.24的运行比例，所以它与首版原始文件音量拼接不同。所有试听仍是素材展示，非实际引擎录音。

104项技术检查包含隔离重制、A/B逐样本一致、6dB降幅与落地／火源不变；不能替代主观自然度验收。两个水声相对首版RMS还分别降低约8.9／11.1dB，比较同时体现音色、包络和响度变化。目前只交付A/B待Owner听，不把修订标为听感已接受。新源原文件、许可与作者要求的 https://rnan.itch.io/ 署名保留在制作源及运行时说明。

第三版仅修改入水瞬态：针对录音内部突出爆点做频谱与峰均比整形，峰值约-16dBTP，峰均比26.96→13.69dB。平均RMS比第二版增加4.26dB，因此不称为整体降音量；已发送3.80秒入水A/B供Owner判断，Owner已答复“可以”，接受该入水版本；不扩大为风／出水逐项接受。其余四WAV与第二版逐字节一致。

## 实现内容

### 真实移动事件

`movement.landed` 绑定0.78秒落地复合声，`movement.swim.enter` 绑定1.08秒真实水花录音剪辑，`movement.swim.exit` 绑定0.88秒较轻的湿动／排水录音剪辑。继续使用实际胶囊 Landed 与 MovementModeChanged 回调，不按定时器猜动作。落地在 PostPhysics 等待伤害结算，使用保存的 Hit.ImpactPoint；泳态声使用当次角色位置。使用 effects 类别与既有空间衰减，不新增伤害、奖励或 AI 感知。移动声音不累积每次新GUID的整段时间线账本。

### 活火

读取当前已加载、BeginPlay、非隐藏的设施／石堡 Actor 的当前 Niagara 组件。组件必须注册、可见、未HiddenInGame、活跃，具有现有 `Hearthward.FacilityFire` 或 `HearthwardRaidVFX` 标签，并精确使用 `NS_HearthFire`，因此烟不会另开一份声音。

设施的点火／暂停／生产事实仍由现有 BuildingPresentation 决定；石堡另检查当前 prologue 阶段。晚创建或重新创建的火焰组件会在缓存的现有 Owner 上重新读取。每个真实火焰位置对应一个独立声音和 PCM 游标，按距离取最多4份，18米线性衰减，单源混音为 environment 音量的0.35。灭火、隐藏、注销、销毁、离距、卸载、暂停、死亡、读档及退出均移除声源。

### 经Owner确认的局部风

只使用实际注册、可见并匹配 `SM_RouteLookout` 的两类路标 Actor：`CampaignNode:route_ridge`、`CampaignNode:route_watch`。不依赖硬编码世界坐标、不在缺失锚点时全地图兜底。

声音参数集中在 experience.json 的这条 sound_event.source 中：内圈18米，外圈60米，中间用 smoothstep 空间增益；重叠区域取较强者，始终最多1份风声，environment 类别系数0.24。首次创建另有0.4秒淡入。正常走入／走出边界连续渐变；传送、锚点卸载、暂停、Load、死亡等生命周期失效立即停止，避免旧区残音。

角色视点向上25米的实际 Visibility 碰撞遮挡会停止局部风，游泳状态也停止。这个实现判定的是当前实体遮顶，不是所有建筑的抽象“室内”标签；无碰撞装饰屋顶不构成遮挡，需正式地图实测。这些是声音范围／混音参数，不是天气、风向、生产或战斗机制。

### 共存与资源

现有西湖资源、精确几何、其它17条声音事件、固定对白／voice状态均未改变。新火／风的注册与生命周期不依赖湖面JSON成功读取。新增环境波形使用 PlayWhenSilent 虚拟化策略，避免类别静音后有效组件长期静默；真实设备静音恢复仍须验证。

当前22个事件对应17个不同WAV。新WAV均为48kHz、mono、PCM16，火／风各16秒循环。落地使用现有Kenney CC0 foley；进出水在第二版改用Peludo CC0真实水花录音剪辑，不再使用Vistula纹理或合成气泡；火／风为原创程序化声音。没有固定人声或TTS，不冒称真实人物入水录音。完整源、许可、哈希和试听时间轴见[素材报告](../../../assets/TASK-099/AUDIO_COMPLETION_20261010.md)。

## 已执行检查

- 音频独立技术验证：104/104 PASS；包含全部5 WAV及42.49秒试听拼接的隔离重制逐字节一致、源哈希、真实峰值、无削波与接缝度量。见制作源 `validation.json`。
- `python -X utf8 docs/qa/TASK-099/audio-completion-v1/validate_audio_completion.py`：6/6 PASS，检查绑定、格式、头尾、局部规则、旧JSON全部其它字段保留及许可文件。数学边界检查不是UE运行模拟。
- `python -X utf8 -m unittest discover -s scripts/tests -v`：33/33 PASS，仓库工具自测，不是游戏Native。
- `git diff --check`：PASS。
- 额外频谱合理性检查：火／风循环接缝跳变量为1／2个PCM16单位；高于10kHz的能量占比约0.40%／0.006%。这些仅是波形证据，不是“不刺耳”或自然听感保证；结果见本目录 `spectral-check.json`。
- `python -X utf8 scripts/validate_repo.py`：FAIL，4个原有缺失链接；对独立干净基线工作树运行同一验证得到同样4项，未修改无关任务来掩盖它们。
- 带 `--task TASK-099 --base 1bdc01b642dcf6e322ddca6cd4d7ad1a3fcc6351` 的范围检查必须与最终包一起核验；整体退出仍受上面4项影响；独立调用同一检查器的 check_scope 返回0个范围错误。最终命令原始结果保留在本目录日志。

主干已有缺失链接：TASK-087 的两个旧局部截图；TASK-102 的 CPU/Vulkan 两份 local-ai.log。源代码／素材都未新增这些链接。

## 未执行，不能算通过

- UE 5.8.2 编译、UHT、实际发现Native测试数及执行结果：**NOT_RUN**。
- 火焰真实Niagara活性、正式地图流送／区域卸载、设备静音恢复／混音压力：**NOT_RUN**。
- 自然路线落地／涉水、身体与表面匹配、火近远定位、室内遮顶与风区域调音：**NOT_RUN**。
- 实际设备与长循环检查：**NOT_RUN**；Owner已提供首版和第二版反馈，第三版入水已获Owner接受，风／出水未逐项明确接受，引擎实听未执行。波形接缝／非静音检查不等同听感通过。
- Cook、Shipping、Windows独立包：**NOT_RUN**。现有Build.cs会收集Resources作NonUFS，但代码中的收集规则不证明新包已成功。

历史2026-10-07的23/23或旧Shipping证据不抵扣这些项目。

## 新增测试源码（尚未运行）

本轮新增7个原生测试定义：2个真实移动绑定测试、3个风域测试、2个需要渲染的活火测试；保留原3个湖面测试与原无绑定移动测试。测试源码检查不是实际发现／执行数量。活火测试强引用精确火／烟系统，在引擎帧之间最多30秒轮询真实就绪状态；缺失或超时报告 READINESS_BLOCK，不跳过后声称PASS。新增名称：

- `Hearthward.Iteration.Task099.BoundActualLandingAndLifecycle`
- `Hearthward.Iteration.Task099.BoundActualWaterTransitionsAndLifecycle`
- `Hearthward.Iteration.Task099.Environment.Wind.LoadedLookoutDistanceAndMix`
- `Hearthward.Iteration.Task099.Environment.Wind.ShelterPauseSwimmingAndExit`
- `Hearthward.Iteration.Task099.Environment.Wind.ActualLoadAndDeadListener`
- `Hearthward.Iteration.Task099.Environment.Fire.RenderedActualFacilityFlameLifecycle`
- `Hearthward.Iteration.Task099.Environment.Fire.RenderedActualLoadAndBoundedSources`

## 具备UE环境后的验证顺序

1. 在专用任务分支按交付包预检应用，保留最新main和用户未提交修改；取齐原仓库LFS资产与项目既有工具链。
2. 按 `docs/qa/BUILD_AND_TEST.md` 使用 UEClient 对本游戏工程构建 Development Editor。
3. 使用新的隔离 UUID 存档池，运行 `Hearthward.Iteration.Task099`，确认非零实际发现数量、全部当前结果及0意外warnings/errors。火焰测试必须使用有渲染的Niagara运行环境，不能用NullRHI替代活火验收。
4. 在正式地图正常输入完成：跳下→落地、入水→游泳→出水；营火暂停／恢复、真实生产火启停、夜袭结束；两个风锚点近远移动／遮顶／游泳；静音→恢复、暂停、Load、传送、卸载、新游戏／退出。记录声音组件数量和带音轨片段。
5. 实际听42.49秒素材试听，再听引擎内混音与两个循环跨接缝；Owner可按听感调源参数和配表，不先标Done。
6. Cook／Shipping验证新5 WAV、旧西湖文件及对应许可随包，并用独立包重复关键路线。
