# SceneCapture曝光调查与当前冻结口径

2026-10-04，子代理只读调查和本人QA草稿。Root独占UE/editor、资产、构建和Git。当前目标为取得可判断的材质/几何图片；Owner三组风格、真实手握及动作验收仍未通过。

## 已有实际运行事实

- `Saved/Task070/weapon-render-import-red/render-facts.json`：3/1 lux，Manual physical=false bias0；原样样板图片近黑。
- `Saved/Task070/weapon-render-import-red-visible/render-facts.json`：10000/2000 lux、Sky1，同非物理Manual；两件oblique纯白。
- `Saved/Task070/weapon-render-import-red-physical-ev10/render-facts.json`，2026-10-03T23:47:58.050479+00:00：capture_complete=true，Root确认closed。actual PPS读回physical=true、f4、shutter64（1/64秒）、ISO100、bias0、EV100=10、blendWeight1；图片仍纯白，Root目视与EV0无可见变化。参数写入已确认，实际1/1024曝光变化未确认。
- `Saved/Task070/weapon-render-import-red-calibrated/render-facts.json`，2026-10-03T23:50:29.335805+00:00：同PPS、30/10 lux、Sky1，capture_complete=true；Root实际查看两PNG可判断。该30/10选择用于此离屏路径可视采样，没有声称真实太阳照度或物理曝光接受。
- `Saved/Task070/weapon-render-pbr-fixed-calibrated/render-facts.json`，2026-10-03T23:54:48.486678+00:00：同30/10 lux、Sky1和PPS，capture_complete=true。Root随后运行独立disk reload probe，正式结果由Root汇总。

旧facts字段 `gain_relative_to_same_fixture_manual_ev0=1/1024` 仅来自物理公式。在实际图片没有相应变化后，该字段不能作为测量结论；Root当前改用 `theoretical_gain_relative_to_ev0_not_verified`。保留原始PNG和facts，未修改历史证据。

## 本机5.8.2路径证据

`Engine/Source/Runtime/Renderer/Private/PostProcess/PostProcessEyeAdaptation.cpp:520` 的物理公式为 log2(fstop²×shutterReciprocal×100/ISO)。f4、64、ISO100对应EV100=10；在相同渲染路径并实际消费该Manual物理参数时，理论曝光相对EV0为1/1024。该前提仍需实际渲染证明。

`Runtime/Engine/Private/Components/SceneCaptureComponent.cpp:169` 以ESFIM_Game初始化ShowFlags。`Runtime/Engine/Public/ShowFlags.h:390` 初始常规flags全开，SceneCapture构造只关闭MotionBlur、SeparateTranslucency、HMDDistortion、OnScreenDebug，2D还关闭TemporalAA。没有找到该构造默认关闭EyeAdaptation的证据。`Runtime/Engine/Private/ShowFlags.cpp:788` 正交override目前为空，注释说明正交功能默认开启，因此不能归因于ortho天然禁止曝光。

项目 `Config/DefaultEngine.ini:14` 的 `r.DefaultFeature.AutoExposure=False` 通过 `Runtime/Engine/Private/SceneView.cpp:2060` 在初始FinalPostProcessSettings设置min/max=1；`Runtime/Renderer/Private/SceneCaptureRendering.cpp:818` 随后覆盖capture PPS，Manual物理分支使用摄影参数。此项目默认值独立不足以解释本次EV10无效。

当前capture_every_frame=false，always_persist_rendering_state保持默认false。`SceneCaptureComponent.cpp:407` 因此不分配持久ViewState；`Runtime/Renderer/Private/PostProcess/PostProcessing.cpp:786` 跳过有状态eye-adaptation pass。但 `PostProcessTonemap.cpp:622` 的无buffer分支调用 `GetEyeAdaptationFixedExposure(View)`，后者继续读取ScalarParameters，所以无ViewState也不能独立证明Manual物理参数被忽略。

`PostProcessEyeAdaptation.cpp:650` 仅在最终EyeAdaptation flag启用时消费Manual物理参数；674的flag关闭分支会把曝光补偿/白点锁1。这个条件能解释参数读回与图像不同步，但本次facts未记录最终ViewFamily flag，尚无运行证据证明该分支被采用。不追加Engine改动或新Runtime接口。

## 与现有等待和截图路径的准确关系

`Engine/Source/Developer/FunctionalTesting/Private/AutomationBlueprintFunctionLibrary.cpp:702` 的 `FinishLoadingBeforeScreenshot` 只FlushAsyncLoading、等待资产/着色器编译并更新纹理流送，未直接修改曝光、ShowFlags或创建ScreenshotEnv。该文件258的AutomationTestScreenshotEnvSetup和181的AutomationViewExtension是独立截图环境路径；它们能关闭EyeAdaptation，但现070/055脚本只调用FinishLoading，没有调用该ScreenshotEnv或TakeAutomationScreenshot，不能将其当本次原因。

Root已有实际 `generator already executing` 异常，等待编译可能泵入Slate回调。现070沿Root的IN_TICK保护；055完整稿也已加入同保护，finally释放，防止嵌套tick再次推进正在执行的generator。

现脚本通过 `capture_scene()`→下一Slate tick→`RenderingLibrary.export_render_target()` 导出SCS_FINAL_COLOR_LDR/RTF_RGBA8。`Runtime/Engine/Private/KismetRenderingLibrary.cpp:278` 调用PNG导出，`ImageUtils.cpp:1266` 从真实RenderTarget读像素后PNG压缩，没有在导出时重新按CameraISO/Fstop计算曝光。未取得证据表明导出器吞掉摄影参数；曝光实际是否被消费发生在前面的View/tonemap路径。

## 冻结交付

本人QA055 `sample_stone_axe_hero_pose.py` 最新完整300行，灯30/10 lux、Sky1；同Manual physical=true f4/64/ISO100/bias0，actual PPS与灯读回、IN_TICK均保留。AST通过，未执行UE。Root复制该完整稿，后续以actual hand/axe变换、真实clip pose误差和PNG确定Grip/BladeBase/Tip及接触时段，不把高度峰值当接触。

旧QA055 `fixed-exposure-ev10.patch` 仅保留历史候选，不能单独当当前冻结稿；`visible-fixture-30-10-lux.patch` 是相对先前300行EV10稿的灯修正，`slate-reentrant-guard.patch` 是相对旧EV10稿的guard。优先复制当前完整稿，避免错用这些不同基线的补丁。

曝光调查停止扩大，真实30/10可判断图片支持继续局部资产与几何QA。未宣称EV10实际生效、原因确证、材质Owner接受、真实手握、world动作、性能或cook通过。

公开主源：[Epic Auto Exposure](https://dev.epicgames.com/documentation/en-us/unreal-engine/auto-exposure-in-unreal-engine)、[Epic Python SceneCaptureComponent2D 5.8](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/SceneCaptureComponent2D)。接口和分支以本机5.8.2源码核对为准。
