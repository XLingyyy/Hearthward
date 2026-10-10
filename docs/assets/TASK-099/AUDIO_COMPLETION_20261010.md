# TASK-099：音源交付与技术检查（revision 3）

日期：2026-10-10。基线 `1bdc01b642dcf6e322ddca6cd4d7ad1a3fcc6351`，分支 `task099-audio-completion`。用户试听 revision 1 后认为风偏大、入出水不自然；revision 2 三声已实质修订。用户再反馈入水太爆，revision 3只修改入水，其他四声字节不变；用户随后对确切已交付v3入水对比回答“可以”，已接受此入水版本。风／出水按要求修订，未分别明确批准。两旧版均已先备份。本报告以revision3为当前，旧79／96项结果不沿用。

## 当前交付

全部为 mono 48,000 Hz PCM16。五个正式 ID／运行路径不变。以下电平由实际输出测量，true-peak 是 4 倍过采样估计。

|正式事件|Resources/Audio/TASK-099/ 文件|秒|源 true peak|源 RMS|试听运行 gain|
|---|---|---:|---:|---:|---:|
|movement.landed|movement-land.wav|0.78|-4.500 dBTP|-25.362 dBFS|1.0|
|movement.swim.enter|movement-water-enter.wav|1.08|-16.001 dBTP|-29.770 dBFS|1.0|
|movement.swim.exit|movement-water-exit.wav|0.88|-11.000 dBTP|-36.345 dBFS|1.0|
|environment.fire.active|environment-fire-loop.wav|16.00|-7.000 dBTP|-29.617 dBFS|0.35|
|environment.wind.lookouts|environment-wind-loop.wav|16.00|-16.000 dBTP|-30.421 dBFS|0.24|

落地 SHA `7664890ca057e7a2a24ebe3b5fb2f5fd9c2f2f73e5f16f71ce7d03dfdd485c92`、火 SHA `7d4833b73af34f84728b81ff2254f4c2743161ce566a5d7c668b98bd6ca20805` 与 revision 1 **逐字节相同**。风保持原种子／周期／相位，源降低 6 dB，配置 gain 0.24 不变；绝非只把预览调小而运行文件未改。

## 两个水 cue 的实质变化

新素材为 [Peludo / RNAn：Water Splash and sand footsteps](https://opengameart.org/content/water-splash-and-sand-footsteps)，原页面明确 CC0；[作者主页](https://opengameart.org/users/peludo) 链接 [RNAn itch.io](https://rnan.itch.io/)，已遵循作者 credit 请求。作者记录的是实际 bucket 水实验，不是现场人物出入湖录音。

- 入水使用 splash1 的0.525–1.455秒，保留真实水花／泄水；revision3额外柔化内部瞬态（下节）。
- 出水使用 splash2 的0.883–1.670秒泄水与0.639–0.774秒轻微湿动作，避开其最大中心冲击；较短、较小，不反放入水。
- 两者删除旧版全部气泡振荡器／合成噪声／Vistula河浪层，只保留真实录音的带通、边缘淡化与电平编辑；revision3入水另外平滑衰减局部尖峰并暖化前段频谱。没有 pitch shift、反转、生成滴落、噪声门或重度频谱降噪。
- 两源原文件均无 PCM 满刻度采样；前置底噪窗口 RMS 约 -84.02／-76.37 dBFS。桶体／麦克风颜色是否可接受仍须人耳确认，不因数值安静就假称专业现场录音质量。

原始 WAV、源链接、SHA、license、窗口与检查见 [peludo-water-splash](../../../art_source/TASK-099/audio-completion-v1/peludo-water-splash/PROVENANCE.json)。运行许可见 [新水来源声明](../../../Resources/Audio/TASK-099/License-Peludo-Water-Splash.txt) 和 [本批总声明](../../../Resources/Audio/TASK-099/License-Audio-Completion-v1.txt)。独立既有湖环境和其 RandomMind 来源不变。

## 当前入水 revision 2 → 3

[仅入水对比](../../../art_source/TASK-099/audio-completion-v1/comparison-entry-v2-v3-runtime-gain.wav)，**3.80秒**。00.40–01.48旧入水，02.13–03.21柔化入水；两者gain1.0，未归一化，无人声标签。SHA `c8660caa77723f19313dbb88ec8a803231eea40eb408e6e7da4f2bcb5b74ac90`。只有入水源改变；落地、火、风和出水均与revision2字节相同。

v3不是仅降低开头几毫秒或整段音量：针对原素材内部约0.14–0.225秒尖锐水花进行前段频谱暖化和平滑局部峰值衰减，保留真实水流主体／尾音，无新增合成层。源true-peak上限-16dBTP避免削峰后再归一化到旧爆点。

- 峰值0.4432→0.1570，crest26.96→13.69dB
- 前0.3秒最大滑动1ms RMS：-13.02→-19.32dBFS
- 最大滑动10ms RMS：-19.84→-21.18dBFS；20ms：-20.59→-22.73dBFS
- 水流主体更均匀，整段RMS -34.03→-29.77dBFS；前0.3秒总体RMS也更饱满，不声称整体水声所有部分都变小

[瞬态测量](../../../art_source/TASK-099/audio-completion-v1/entry-revision3-transient-check.json) 只证明波形改动，不证明主观自然／满意。用户对本次已交付比较的后半段回答“可以”，已接受精确v3入水：源SHA `045787014fabe08a434399c0b2776a66396f6feacd04311f05c240d4ef09368c`。不换用私下v3b，不将其接受扩大到风／出水、正常游戏或Shipping。

## 历史三声 A/B，保持原字节

[comparison-v1-v2-runtime-gain.wav](../../../art_source/TASK-099/audio-completion-v1/comparison-v1-v2-runtime-gain.wav)，**20.30秒**；[精确章节 JSON](../../../art_source/TASK-099/audio-completion-v1/comparison-v1-v2-runtime-gain.json)。无旁白／TTS、无分段归一化。两版风都使用相同0–6秒源窗口和 gain 0.24，两版水都是1.0。实测风同窗口 RMS 差 **-6.000 dB**。

- 00.50–06.50：旧风；07.00–13.00：新风
- 13.65–15.03：旧入水；15.53–16.61：新入水
- 17.26–18.42：旧出水；18.92–19.80：新出水

A/B SHA-256：`bd92084e1bd235dbf607167fe3c6ddee11242d5757a34bf50dead434d6ec52be`。此文件已冻结；后续元数据收尾没有重调音或改写该比较。

[完整试听](../../../art_source/TASK-099/audio-completion-v1/audition-task099-completion.wav) 当前 **42.49秒**。也改为实际运行 gain：落地／水1.0、火0.35、风0.24，不把原始风电平当作游戏混音。主音量／分类音量以1.0展示，游戏实际设置可继续缩放。两环境各16秒加跨wrap2秒；火接点21.34、风接点39.99秒。火运行文件未改，只有预览按已有运行 gain 正确呈现。

## 独立技术验证

实际命令：

```sh
python art_source/TASK-099/audio-completion-v1/render_audio.py
python art_source/TASK-099/audio-completion-v1/verify_audio.py
python art_source/TASK-099/audio-completion-v1/make_revision2_comparison.py --baseline-root <revision1根目录>
git diff --check
```

**104/104 PASS**，逐项见 [validation.json](../../../art_source/TASK-099/audio-completion-v1/validation.json)。覆盖 PCM／长度／非零／0削波／余量／DC／频带、当前源 SHA、运行映射、源 notice、落地火不变、风降低6dB、历史A/B保持完整、新入水A/B源与当前文件吻合、试听增益，以及隔离临时目录重建后五个运行 WAV 和完整合辑 SHA 完全一致。

火／风仍周期连续，无插入静音缝；两源相邻边界样本差分别 +1／-1 PCM16单位。新水的性质来自真实素材与编辑方案，不由这些数值测试证明主观“自然”。

精确 cue、源与编辑 seed／工具版本见 [cue-manifest.json](../../../art_source/TASK-099/audio-completion-v1/cue-manifest.json)；可编辑 Python、设计 JSON、A/B生成器及说明都在 [制作目录](../../../art_source/TASK-099/audio-completion-v1/README.md)。A/B生成器接收旧版本根目录参数，可指向旧交付包的 changes 目录，无唯一机器路径依赖。

## 验收边界

**已接受：**上列精确v3入水用户试听。**未分别明确接受／仍待集成证据：**风与出水最终音色、其他设备听感与完整混音、正常游戏动作同步、空间衰减与暂停生命周期、正常游戏带声录制、当前 Shipping/Cook。已记录用户对旧版的反馈及对确切v3入水的接受；不假称制作者替用户或实际声音设备执行了未做的试听。资源104项不替代UE或听感验收。

本资产子任务未提交／推送。固定voice仍UNPRODUCED，无TTS／静音替代；既有落地／火源文件之外的旧音源也未改。父级负责配置状态、最终整合文档与获授权提交。
