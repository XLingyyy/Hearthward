# fix2：加载转场与自然地图画面／性能

日期：2026-10-01。工程：`G:/GameFactory/Hearthward-ui-fix/Hearthward.uproject`。引擎：UE 5.8.2。分支：`Hearthward-ui-fix`，受测游戏源码及材质已归入提交 `7eeecf52b4f2b1a72d7aa45d1f8d1e65e78a0486`，源码基线为 `9058ee2978f87cafa02e13666ebd0d96b81cd2f2`。本报告记录提交前完成的实际验证，覆盖 UI／fix1 与本轮 fix2 的组合源码及最终材质。原 `G:/GameFactory/Hearthward` 检出保留。本机 `.uproject` 的引擎注册 GUID 属于本地生成改动，提交中的 EngineAssociation 保持 `5.8`；上述验证通过明确指定的 UE 5.8 安装路径运行。

依据：用户要求执行 `C:/Users/admin/Desktop/fix2.docx` 的两项修复。文档提出存档进入时增加美观加载页，以及改善粗糙画面、向商业游戏品质推进，并以项目“极高”、原生 1080p、13 代 i7／RTX 4060 超过 60 FPS 为目标。异环／NTE 是视觉参考；本报告不将灯光与渲染改动算作整套商业美术资产完成。

## 实现与根因

加载前已复现 HUD 先出现、地图开场仍处于准备状态的情况。新增 `UHearthwardLoadingSubsystem`，跨地图阻塞加载使用原生 Slate／MoviePlayer，地图流送及同地图读档使用同一视口覆盖层。沿用已有插画和中文字体，增加暗色底栏、准备文案及循环动效。覆盖层等待世界开始、玩家生成、World Partition 流送、地形碰撞、开场／转场逻辑及纹理资源准备，并短暂稳定后淡出；准备期间屏蔽操作并暂停角色移动，结束后恢复输入与原移动模式。没有虚构加载百分比。

自然地图新增世界展示子系统，统一暖色日光、冷色夜间补光、手动曝光、轻量调色、Bloom 与体积雾。雾层移到当前地形高度。模板 `SM_SkySphere` 在初始及后续流送关卡中隐藏，避免遮挡大气天空；营地与序章日夜切换继续由既有玩法状态驱动。修正夜间补光被玩法灯光缓存再次压低的问题。关闭运动模糊。营地仓储、资源点及后勤员原来用不含中文字形的引擎世界字体显示中文，实际画面出现方框；改为 R／E／Tab 操作键，中文详情继续由 HUD 与行装页显示，已增加两项实景检查。

性能检查发现原 Windows 配置走 SM5，而场景中高面数树木已按 Nanite 制作；因此该路径未发挥 Nanite 的作用。改为 DX12 SM6 后，CSV 中出现实际 Nanite 渲染通道。随后定位传统 ShadowDepths 平均约 4.44 ms，启用虚拟阴影后营地对应通道约 0.92 ms。Nanite 的 SM6 要求及虚拟阴影的缓存／Nanite 配合依据 Epic 的[硬件要求](https://dev.epicgames.com/documentation/en-us/unreal-engine/hardware-and-software-specifications-for-unreal-engine)与[虚拟阴影文档](https://dev.epicgames.com/documentation/en-us/unreal-engine/virtual-shadow-maps-in-unreal-engine)。

“极高”为项目定制的 `sg.*Quality=3`，配置详见 `Config/DefaultScalability.ini`：原生 TAA、保留资产设定的可见距离、草密度 1.35、草距离 0.8、纹理池 2048 MB、体积雾及 VSM 采样预算。使用 SSGI／SSR；没有启用 Lumen、DLSS、动态分辨率或帧生成。现有用户设置继续保存；设置页“恢复默认”使用本轮极高默认值。8 个 Fir／Pine 树干材质原来缺少 Nanite 使用标记，独立游戏日志明确提示使用默认材质。通过 UE MaterialEditingLibrary 补齐并保存这 8 个材质，避免默认材质替代及每次启动重新编译；改动及前后标记见 [materials.json](materials.json)与[资产修复脚本](fix_nanite_materials.py)。未修改地图几何、存档 schema 或敌人感知规则。

## 构建与功能验证

- Development Editor 构建通过：[build.json](build.json)。
- 最终 DX12 SM6／VSM／原生 TAA 配置下，真实渲染 PIE **35/35**：[verification.json](verification.json)，脚本 [verify_pie.py](verify_pie.py)。
- 覆盖标题进入、新游戏加载、加载期间拒绝命令、开场就绪后显示 HUD、同地图读档／位置恢复、回标题继续／跨地图读档、默认画质应用、日夜展示参数、移动与跳跃、背包、护符交互、弟弟跟随、撤离交互与返营存档。
- 移动通过真实 Enhanced Input action 注入验证，跳跃通过实际角色行为验证。撤离检查使用测试夹具将兄弟移到出口，随后调用真实交互；这项检查不代表完成整段步行路线。
- 独立游戏中通过物理键鼠验证跳跃、W 输入和 Tab／Esc 背包开关。使用独立 QA 存档池，没有覆盖玩家常用档池。

## 原生 1080p 性能

硬件：i7-13650HX、RTX 4060 Laptop 8 GB，驱动 577.00。UE Development 独立游戏 `-game`，DX12 SM6。帧缓冲实际为 **1920×1080**，`r.ScreenPercentage=100`、动态分辨率关闭、VSync 关闭、不限帧。全部图形档位设为 3；运行模型推理的负载未纳入采样。

每次 CSV 采集 6000 帧，排除前 1800 帧的启动、初始流送与截图，再统计剩余 4200 帧。受测视角固定，世界 Actor 继续运行；计时区间没有截图与测试输入。平均 FPS 按总帧数／总耗时计算，1% Low 为最慢 1% 帧平均帧时的倒数。通过条件为 p99 ≤ 16.67 ms 且 1% Low ≥ 60 FPS。

| 场景 | 有效时长 | 平均 FPS | 1% Low FPS | p99 帧时 | >16.67 ms 帧数 |
| --- | ---: | ---: | ---: | ---: | ---: |
| 日间营地 | 54.63 s | 76.88 | 66.34 | 14.30 ms | 1 / 4200 |
| 夜袭序章室内 | 44.63 s | 94.11 | 69.85 | 12.77 ms | 2 / 4200 |

两个场景均通过上述采样条件。各有一次 39.94／41.18 ms 的长帧，对应 CSV 中的 GC 事件；序章另有一帧 16.70 ms，CSV 未标出对应事件，不能据此声称每一帧均超过 60 FPS。营地平均 GPU 时间 12.31 ms，GameThread 8.26 ms；GPU 明细见 [gpu-camp.json](gpu-camp.json)。

首次有效营地诊断为 43.39 FPS／1% Low 35.50 FPS；该次已经包含本轮画面增强，使用 SM5／TSR，**不作为原游戏完整基线**。结果见 [performance-before.json](performance-before.json)。补齐材质标记后已重新采样两个场景，最终结果见 [performance-camp.json](performance-camp.json)及 [performance-prologue.json](performance-prologue.json)，压缩 CSV 随同保存，便于检查原始帧时与事件。

复测入口，在 `G:/GameFactory` 下运行：

```powershell
.venv/Scripts/python.exe -X utf8 Hearthward-ui-fix/docs/qa/fix2/run_performance.py --label prologue --frames 6000
.venv/Scripts/python.exe -X utf8 Hearthward-ui-fix/docs/qa/fix2/run_performance.py --label camp --frames 6000 --pool <隔离测试池UUID> --save <该池营地存档GUID>
```

脚本通过公开 `UEClient` 启停本次进程，使用 UE CSV Profiler。读档运行需存在同池 QA 营地档；缺少该档时不能将标题页性能当作场景性能。另一个已有编辑器进程不会自动取得本轮编译结果。

## 可审查画面

- [加载页](loading.png)：已检查文字、背景和动效位置；该图为编辑器 PIE 取证。
- [最终营地原生 1080p](camp-native-1080p.png)、[最终序章室内原生 1080p](prologue-native-1080p.png)：独立游戏原生帧缓冲及完整 HUD。
- [阶段检查夜间外景](night-exterior.png)、[阶段检查 PIE 日间营地](day-camp.png)：用于核对日夜切换与雾层，拍摄早于最后的材质标记／悬浮文字修正。最终材质画面以原生 1080p 截图和录像为准。
- [操作场景录像](gameplay-review.mp4)：独立游戏原生视口采集，原图 1920×1080，视频 1280×720，记录操作后的角色／视角变化。原生采集器不录 Slate／UMG；界面与加载效果以包含 UI 的截图及功能检查为准。
- 录像由低频截图编码，时间间隔保留截图完成时间，采集本身会阻塞渲染；录像帧率不作为运行性能证据。已经解码并复查视频，无空编辑器视口插入：[视频复查图](video-review.png)、[采集说明](video.json)。

## 验证边界

本轮确认加载行为、展示参数与两个有限场景的目标硬件帧时。没有完成全地图连续行走／跨区流送、持续大规模战斗、GPU 模型推理同时运行、其他分辨率／显卡或 Shipping 包性能验证。

现有角色与敌人模型、专用握持／跳跃动作、植物与地表资产、建筑与场景布置仍有制作质量限制；本轮主要改善渲染、日夜氛围和帧时。商业成品整体视觉与 Owner 实玩验收尚未完成，不能将这些局部通过记录等同于异环级美术交付或全流程持续 60 FPS 保证。
