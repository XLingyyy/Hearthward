# TASK-069 交流页底色与低对比：只读审阅

2026-10-05。只读 Root `G:/GameFactory/Hearthward/.agent-local/task051` 当前 UI 源码、主题、布局和 069/071 记录。本文件仅为局部施工候选，未改 Root、Source、Content、Resources，未执行 UE、构建或 Git。Owner/Reviewer 仍为 XLingyyy；不登记本子任务为正式审查或验收。

Root 已反馈桌面实际交流页在亮灰地面上显示低对比金色文字、按钮和 Draft。本子任务没有取得并核看该桌面截图，不把源码推理、已有回调通过或资源图片核看登记为视觉 PASS。

## 已确认的绘制链

1. `Resources/UI/interface.json:31–32` 定义 `dialogueBackground → Art/dialogue-background.png`。该文件实际存在，是 1672×941 RGB PNG，已只读核看：现有营火与兄弟插画，右侧较暗，非缺失文件。`HearthwardScreenWidget.cpp:115–141` 加载全部主题资产，并为每项建立 Brush；缺失纹理会触发检查。因此本次证据支持“页面未选择背景”，不支持“资产导入失败”。
2. `interface.json:5153–5155` 的 dialogue 页面明确 `background: ""`。`HearthwardScreenWidget.cpp:301–304` 仅在 background 非空时建立全页 image。资源名在当前 UI 源码中没有另外的消费点。
3. `Resources/UI/layout.json` 的 dialogue.header/footer/panel 三组没有 asset；panel 自身位于 `[950,278,650,625]`（863 行）。`HearthwardScreenLayout.cpp:52–62` 仅为有 asset 的组生成 image。分组名称与 rect 是布局元数据，没有自动的填充绘制。
4. `HearthwardScreenPaint.cpp:23–24` 明确把 dialogue 排除于通用黑底。`ComposeDialogue`（`HearthwardScreenContent.cpp:480–579`）添加文字与 choice，没有添加面板底色。文字直接叠在真实世界上，符合 Root 的灰地面反馈。
5. 已有实际运行记录 `docs/qa/TASK-071/physical-os-8618582c-layouts.jsonl` 的 dialogue 样本 417、layout_sequence 215 也没有 background 或 surface；只有 logo、dialogueEmblem 等图片。dialogue.panel 可见是元数据可见，不能解释为已有深色面板。

## 字号放大时的额外缺口

`HearthwardScreenWidget.cpp:565–669` 的现有 ReadableLayout 在非 HUD、非编辑、非确认且 TextScale>100 时运行：保留左上原点的大幅全页 image；随后跳过无字、无 action 的小 image/panel，清空复制行的 Asset，并把有 action 的 choice 转成普通 button。当前 dialogue 没有待保留的全页底图；以后只给 dialogue.panel 加小 surface 也会在该分支丢失。

`HearthwardScreenPaint.cpp:127–150` 中，未聚焦 choice 有 alpha=.58 的深色填充；普通 button 未聚焦时没有填充，聚焦主要绘制 selection 和边框。因此 125%/150% reflow 还会丢掉原 choice 的局部深底。以上为源码已确认的行为；尚无本子任务的 125%/150% 桌面截图。

只在固定 dialogue.panel rect 填底也不完整：实际手动任务按钮位于 y=200/240，提示与 footer 分布到 y=914；ReadableLayout 会把行移到 x=190。固定右侧 y=278 面板覆盖不到这些内容。

## 最小施工候选

建议首先只改已登记的 `Source/Hearthward/UI/HearthwardScreenPaint.cpp`：在 Elements 绘制前、现有底层 Layer，为 `Page=="dialogue"` 按现有 `Geometry/Box` 绘制 DesignSize 的 `Color("panel")` 深底。主题已有 panel 色 `15130FF5`，无需新颜色、样式系统、布局数据、资产或依赖。直接在页面底层绘制可覆盖 header、手动卡按钮、正文、状态、输入区和 footer，并能保留于字号 reflow。该补丁不需要改变 OpenPage 暂停/输入语义；对话世界继续运行。

另一个准确候选是去掉通用黑底条件中的 dialogue 排除，沿其它页面使用现有纯黑底。两者只选一个。前者沿现有 panel 调色与透明度，更接近当前主题。

复用 dialogueBackground 插画也可实现全页底图，但会更改当前明确为空的页面背景选择。它不属于资源加载修复，也没有现成小面板资产；本次优先建议现有 panel 色，避免为局部对比修复改动 Resources 或重新选择展示插画。

绘制调用依据为 Epic UE 5.8 的 [FSlateDrawElement::MakeBox](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/SlateCore/FSlateDrawElement/MakeBox)，现有 WhiteBrush、PaintGeometry、层级和 tint 可直接复用。

## Draft 的证据边界

`HearthwardScreenWidget.cpp:51–56` 已给 Draft 设置普通/聚焦金色文字与不透明深色 BackgroundColor。实际 UE 5.8 `UMG/Private/Components/EditableTextBox.cpp:94–118,121–134` 把自身 WidgetStyle 交给 SEditableTextBox，没有另传 BackgroundColor override；`Slate/Private/Widgets/Input/SEditableTextBox.cpp:348–351` 在无 override 时读取 Style.BackgroundColor。故当前源码不支持“该深色设置被 UMG 白色属性盖掉”的推断。

Root 已修正 Refresh 使用 Draft 自身持久 WidgetStyle（Widget.cpp:369–371），071 同一分帧公共 UI/Load 路线实际 96/96 通过。任何后续 Draft 修改都应保留该持久引用。先验证页面深底的效果；若 Draft 自身仍异常，需记录实际聚焦/未聚焦样式、Slate 几何及像素后定位，不提前换控件或增加 fallback。

## 最窄验证门禁

- 保留当前灰地面场景的修前实际桌面图；修后只跑同一实际交流页，100% 和 150% 各一张，分别核对空回复/正常回复与候选卡文字、按钮、状态、Draft 聚焦和未聚焦。确认全部文字有稳定深底、底部发送/返回可见可点击。125% 沿同一 reflow 路径，仅当 150% 暴露问题或正式显示矩阵要求时补跑。
- 一次真实发送/返回和重新打开即可覆盖此次页面底层修改的操作风险；单纯绘制变化不重跑已经通过的所有跨读档/HTTP路线。仍需保留 Root 当前 071 跨档证据原有输入来源边界。
- `CaptureUI`（`HearthwardScreenCapture.cpp:15–20`）是透明 ClearColor 的离屏 Widget 绘制，没有真实世界地面。它还临时清空 Hover/KeyboardFocus（13 行）。直接看其 PNG 不能代替亮地面上的桌面合成与聚焦验证，也不能登记为 Windows DPI/物理输入通过。
- 完整 069 分辨率/IME/Windows DPI/九页全流程仍按已批准设计另行验收；本报告仅覆盖交流页对比与绘制链。

结论：根因已定位到当前页面缺少底层填充，以及放大字号对 choice 填充的丢失。建议先用一个 dialogue 专属、复用 panel 调色的底层 Box 完成局部修正。生产、构建、实际视觉复测均待 Root 执行；此报告无 PASS 声明。
