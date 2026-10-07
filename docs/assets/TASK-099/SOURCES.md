# TASK-099 音源与使用记录

已保留原仓储候选，并在2026-10-07按root本地候选授权增加8个真实Kenney原OGG与13条正式成功事件映射。Owner首件/语义/混音尚未试听；此记录不表示最终设计或实际设备声音通过。

- 官方作品页：https://kenney.nl/assets/rpg-audio
- 官方免费包：https://kenney.nl/media/pages/assets/rpg-audio/8e99002d76-1677590336/kenney_rpg-audio.zip
- 作者：Kenney Vleugels / Kenney.nl；许可证：Creative Commons Zero（CC0）。官网与压缩包 `License.txt` 均确认允许个人及商业使用。
- 取源日期：2026-10-07；压缩包保留于 `art_source/TASK-099/kenney-rpg-audio/kenney_rpg-audio.zip`（964837 bytes）。选择的原始 OGG 和原许可亦在同目录保留。
- 运行文件：`Resources/Audio/TASK-099/storage-transfer.wav`；随包许可：`Resources/Audio/TASK-099/License-Kenney-RPG-Audio.txt`。
- 本机现有 `imageio_ffmpeg` 自带 ffmpeg 7.1，将原始 OGG 解码为单声道 44100 Hz PCM16；没有新增依赖、合成、重采样后的增益或静音占位。
- WAV 数据：14915 frames，0.3382086 秒，29908 bytes，最大绝对 PCM16 采样值 6080。格式和非零采样只证明可解码，实际 UE 播放及试听为 NOT_RUN。

转换调用（制作阶段使用，玩家运行不需要 ffmpeg/Python）：

```text
G:/GameFactory/.venv/Lib/site-packages/imageio_ffmpeg/binaries/ffmpeg-win-x86_64-v7.1.exe -hide_banner -loglevel error -y -i art_source/TASK-099/kenney-rpg-audio/handleSmallLeather.ogg -map_metadata -1 -ac 1 -ar 44100 -c:a pcm_s16le Resources/Audio/TASK-099/storage-transfer.wav
```

本候选仅为无空间化的近仓储操作提示。它没有替代脚步材质、命中、制作完成或火水循环声。现有 Build.cs 已将 Resources 下全部文件登记为 NonUFS RuntimeDependencies，因此 WAV 和许可沿用既有部署路径；本轮 Cook 文件实际存在和独立包有声仍需验证。

固定对白54个 cue / 28个 audio_group 继续为 UNPRODUCED。动态弟弟回复没有 TTS。本次未写 Content 包，也未借此取得任何资产包锁或远端权限。

## 2026-10-07 新增候选制作与逐事件映射

完整逐cue/源/格式/数量见 [CUE_AUDIO_CANDIDATES.json](CUE_AUDIO_CANDIDATES.json)，可重复转换参数保存在 `art_source/TASK-099/kenney-rpg-audio/candidate-conversion.json`。原OGG直接从已保存官方CC0包对应成员提取；8份新增PCM16使用本机已有ffmpeg解码为mono 44100Hz，无合成、人声、增益或静音占位。玩家运行无需ffmpeg/Python。

|真实提交事件|原成员|运行PCM16|候选语义与限制|
|---|---|---|---|
|player.craft / npc.craft / facility.upgrade|Audio/metalLatch.ogg|TASK-099/candidate-metalLatch.wav|工具/装配完成确认；具体木石材质未认定|
|player.repair / npc.repair|Audio/metalClick.ogg|TASK-099/candidate-metalClick.wav|工具调整完成候选|
|player.harvest / npc.acquired / combat.hit|Audio/chop.ogg|TASK-099/candidate-chop.wav|离散工具工作或实体接触候选；不宣称人体/资源表面类型|
|npc.delivered|Audio/handleSmallLeather2.ogg|TASK-099/candidate-handleSmallLeather2.wav|携物交付候选；正式仓储仍原独立WAV，同操作GUID避免双播|
|combat.block|Audio/metalPot2.ogg|TASK-099/candidate-metalPot2.wav|金属挡击候选；实际听感未验|
|combat.damage|Audio/dropLeather.ogg|TASK-099/candidate-dropLeather.wav|实体碰撞候选，无人声；受击语义首件待试听|
|camp.rescue|Audio/bookOpen.ogg|TASK-099/candidate-bookOpen.wav|救援登记提示候选；书本语义首件待试听，不新增剧情|
|quest.claimed|Audio/bookClose.ogg|TASK-099/candidate-bookClose.wav|任务登记收尾候选；书本语义首件待试听，不修改奖励|

当前完整映射14条，原storage.transfer条目逐字段保留。新增8个wav均实际非零PCM16；格式只证明可解码。metalLatch/metalPot2/bookClose分别4/189/2个采样达到PCM16满刻度，原源与计数保留，不据此宣布听感、响度或削波质量已通过。无试听证据时不擅自掩盖峰值或称最终声音合理。

带TargetPosition的战斗SFX使用真实回执目标坐标，新增明确UE线性球形衰减：内半径0、FalloffDistance3000uu；原二参数UI确认/仓储不启用空间化。未增加听声AI规则。近中远实际参数Evaluate和真实producer创建组件由最窄Bound Native夹具验证，声音设备/听感、表面材质、环境循环、正常路线及独立Cook仍NOT_RUN。

Unbound原有夹具现在显式只保留storage.transfer cue，保护其“未绑定先观察、合法仓储仍可播放”的诊断前置；未删除成功、拒绝、GUID、多目标、Load或epoch断言。BoundSuccessAndSpatialAttenuation / BoundPauseAndActualLoad使用实际完整运行映射和真实Craft/HitTarget/ReceiveDamage/SavePoint/LoadPoint，不假Broadcast，不再次结算已成功事务来补音效。当前新增资源和这轮Source尚未运行Native。

## 2026-10-07 通用脚步候选与精准动画范围

Root进一步授权实际FootContact工程消费及通用脚步候选：追加官方同包 `Audio/footstep00.ogg`（9475 bytes）与 `Resources/Audio/TASK-099/candidate-footstep00.wav`（22028 bytes，10975 frames，0.24886621315192745s，mono44100Hz PCM16，max30749、0满刻度采样）。当前总9个新增OGG/PCM16、14新增events+原storage共15映射。脚步来自真实Notify足下blocking+walkable接触回执，Source仅实际玩家或当前弟弟，实际Hit.ImpactPoint定位及线性0→3000uu衰减；未读取到physicalMaterial分类契约，采用generic movement.footstep，不声称石/木/土已区分。声音首件尚未试听，当前四包仍0Notify，生产clip标记尚未写入。

实际LFS核实 [四包锁证据](../../qa/TASK-099/foot-locks-20261007.json)：四精准Animation包ours/XLingyyy，已获root本地技术写锁；Skeleton/Mesh范围保持不变。mcp准备精确writer/备份，最终由root统一UE执行。此权限事实不代替角色原始输入权利、Owner风格或旧基线scopeValidator验收。

2026-10-07 当前冻源计数：15个独立event ID对应10个独立运行WAV路径，包含原仓储1个文件与新增9个文件；新增9个真实源OGG/PCM16、14个新event。AudioLifecycle14 + Combat producer3 + FootContact producer2 = 预计19条，尚未运行本轮Native。动画writer准备与Source冻结不表示四包已写入或正常路线已通过。

## 2026-10-07 实际水声候选与RED准备

西湖新增独立RandomMind CC0 Vistula真实河岸水波原MP3与PCM16候选，准确来源/许可/保留源/编辑argv/原UE几何/验证边界见 [水声工程记录](../../qa/TASK-099/water/README.md)。未复用仓储或脚步WAV冒充连续环境声。原源12004407bytes；runtime candidate-water-west.wav 1367144bytes / mono44100Hz PCM16 / 15.5s。0.5s真实尾头交叉淡化，首尾采样差313仅文件事实，实际设备/接缝/Owner试听NOT_RUN。新增environment.water.lake_west环境channel，当前16独立events / 11独立运行WAV；前述Kenney阶段15/10为当时事实。生产循环尚未接入，新3条Native先取得真实RED，旧19GREEN不移植。许可证与Resources仍沿既有NonUFS staging约定，水候选新增后的Cook NOT_RUN。

## 2026-10-07 空挥候选与当前准确计数

Root授权既有正式Attack validated active-window空挥回执工程。官方已保存Kenney包真实成员Audio/knifeSlice.ogg存在（15532bytes），已保留原源并解码mono44100Hz PCM16为candidate-knifeSlice.wav：52958bytes、26440frames、0.599546s、max23602、满刻度0。完整元数据/argv见 [SWING_CANDIDATE](SWING_CANDIDATE.json)。候选用于真实有效挥动开始，无命中仍可有挥声，不声称命中成功；生产仍须真实Swing RED后接入，不在资源准备阶段假填通过。Owner首件/实际设备试听NOT_RUN。当前CUE_AUDIO_CANDIDATES已同步17 events / 12独立WAV /11新增保留源文件（10 Kenney原OGG与1 RandomMind原MP3），包含原仓储独立1份。声音调色不由工程格式/非零PCM推定。
