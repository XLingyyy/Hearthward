# TASK-099：音源制作（当前 revision 3）

2026-10-10，按“风声偏大、入水和出水不自然”的实际试听反馈修订三份音源。落地与火的运行 WAV 与 revision 1 逐字节相同。随后用户认为 revision 2 的入水仍太爆，因此 revision 3 只修订入水。用户随后对已交付的revision3入水对比回答“可以”，因此只将该确切入水版本记为已接受。风／出水按要求修订，但未分别明确批准；不扩大为全音效、UE或Shipping验收。

## 当前变化

- 风：原风设计和循环相位保留，源电平降低 **6 dB**；运行配置 gain 继续 0.24。
- 入水：**1.08 秒**，改用 Peludo 实际水实验 splash1 录音。保留自然不规则水花和收尾；去掉旧版所有气泡振荡器／噪声冲击层。
- 出水：**0.88 秒**，改用独立 splash2 的较轻湿动作和泄水部分，避开原素材最大的冲击尖峰；不反放入水，也不增加有音高的滴落。源 true-peak 目标 -11 dBTP，当前 revision 3 入水为 -16 dBTP。入水内部前0.3秒尖锐水花经暖化与平滑局部削峰，原始水流尾音仍在；不是只降低整段音量或给开头5ms淡入。
- 新水 cue 只有经编辑的真实录音；旧 Vistula 河浪纹理与程序化水层不再使用。独立既有湖环境声不变。

## 复现与编辑

已有 Python、NumPy、SciPy、ffmpeg 环境下运行：

```sh
python art_source/TASK-099/audio-completion-v1/render_audio.py
python art_source/TASK-099/audio-completion-v1/verify_audio.py
```

玩家运行不需要这些工具。制作器不会下载、安装或调用外部服务。`design.json` 保存时长、seed、true-peak 目标、试听运行 gain；分层与编辑窗口在 `render_audio.py`。`--output-root <独立目录>` 可在不覆盖运行文件的情况下复现，原始源仍从本仓库读。脚本不会覆盖本批以外的声音文件。

`cue-manifest.json` 保存当前源／输出 SHA-256、工具版本、seed、PCM、时长与音频测量。相同版本和源字节已在独立临时目录实际复现；五份运行 WAV 与完整试听 WAV 均逐字节一致。`verify_audio.py` 的检查不代表设备试听或 UE 验收。

## 真实来源与许可

- 落地仍为 Kenney RPG Audio 官方 CC0 包四段 foley：footstep03、footstep07、dropLeather、cloth2，加原创重量／碎屑。原样保留；只表示通用落地，未引入实体材质检测。
- 新水素材：[Peludo / RNAn，Water Splash and sand footsteps](https://opengameart.org/content/water-splash-and-sand-footsteps)，页面明确标注 CC0。作者描述为 webcam／bucket 的实际水实验；不能说成“现场人物入湖／出湖录音”。作者请求的 [RNAn itch.io](https://rnan.itch.io/) credit 已随运行文件保存。
- 原始 splash1_0.wav／splash2_0.wav、链接、SHA、取源日期及许可证据见 `peludo-water-splash/`。原素材无满刻度采样；前置底噪窗口 RMS 约 -84.02／-76.37 dBFS。除revision3入水的平滑局部峰值／频谱暖化外，只做轻度高／低通、短边缘淡化和电平调整，没有噪声门／重度频谱降噪、移调或倒放。技术记录不证明麦克风或桶体共振在主观上不可闻。
- 火／风仍是确定性原创程序化多层设计，不冒称真实现场采样。火完全不变；风仅减小源增益。
- 运行声明为 `Resources/Audio/TASK-099/License-Audio-Completion-v1.txt` 与 `License-Peludo-Water-Splash.txt`；保留原 Kenney／RandomMind notice。没有给整个项目添加新开源许可证。

## 当前只试听入水 revision 2 → 3

`comparison-entry-v2-v3-runtime-gain.wav`，**3.80秒**；00.40–01.48旧入水，02.13–03.21更柔和入水。两段gain1.0，不归一化。其他四个运行源与revision2逐字节相同。用户已明确接受此对比的revision3后半段，运行入水SHA为 `045787014fabe08a434399c0b2776a66396f6feacd04311f05c240d4ef09368c`；未采用任何私下更低增益备选。

入水峰值0.4432→0.1570；crest26.96→13.69dB；前0.3秒最大滑动1ms/10ms/20ms RMS均下降。原水流主体更均匀保留，所以整段RMS约-34.03→-29.77dBFS，并非把所有内容一起压小。精确比较见 `entry-revision3-transient-check.json`，不将指标当作听感签收。

```sh
python art_source/TASK-099/audio-completion-v1/make_entry_revision3_comparison.py --baseline-root <revision2根目录> --candidate-root <revision3根目录>
```

两个比较生成器都校验指定版本源SHA，错误版本会拒绝写出，以免覆盖用户已听过的比较。旧根可以是对应交付包的changes目录，或已保留的版本根，不依赖特定机器路径。

## 历史三声 A/B（保持冻结）

`comparison-v1-v2-runtime-gain.wav`，20.30 秒。无旁白／TTS，无分段归一化。旧风和新风都用运行 gain 0.24 和相同 0–6 秒窗口；水都用 gain 1.0。文件冻结供用户比较。

- 00.50–06.50 旧风
- 07.00–13.00 新风（实测降低约 6.000 dB）
- 13.65–15.03 旧入水
- 15.53–16.61 新入水
- 17.26–18.42 旧出水
- 18.92–19.80 新出水

精确章节、gain、输入／输出哈希见 `comparison-v1-v2-runtime-gain.json`。生成器使用可传入的旧版本根目录，不依赖唯一机器路径：

```sh
python art_source/TASK-099/audio-completion-v1/make_revision2_comparison.py --baseline-root <revision1根目录> --candidate-root <保留的revision2根目录>
```

该目录须有 `Resources/Audio/TASK-099/` 下三份旧版 WAV；可用既有 v1 交付包的 changes 目录。工具仅读所指定的两个版本根；不自动把当前revision3入水混入已冻结的v1/v2对比。新旧源 SHA 都写在比较 JSON 中，源版本不正确时应先核对，不用另一版本冒充同一 A/B。

## 完整试听与循环

`audition-task099-completion.wav` 当前 **42.49 秒**，按落地／入水／出水／火／风顺序。现在使用真实文件级运行增益：落地与水 1.0、火 0.35、风 0.24；不再把原始风文件电平当运行混音。用户主音量／分类音量暂以 1.0 展示，游戏实际设置可继续缩放。

两环境循环仍各16秒，并在合辑中播放下一周期开头2秒。火的真 wrap 在21.34秒，风在39.99秒。循环本身由周期连续频谱／包络及回绕瞬态组成，无全局淡出或静音拼缝。合辑的片段起止不是运行循环接缝。

固定配音保持 UNPRODUCED，没有人声或静音文件补位。已交付revision3入水的用户接受如上；其他声音未因此自动获批。集成层的实际设备、游戏动作同步、衰减／暂停生命周期、正常游戏有声录制与当前 Shipping/Cook 仍需相应集成层证据，不由本资产目录代填。
