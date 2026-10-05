# TASK-069 输入、焦点、IME 与显示矩阵：调查、RED 与最小修正

2026-10-04。首阶段只读调查基于集成树 `G:/GameFactory/Hearthward/.agent-local/task051` 当时当前源码，随后按根登记的准确窗口在隔离树 `task055-combat` 增加真实输入回归及两文件最小修正，见末节。本 Agent 未启动 UE、未构建、未提交或推送；原生结果由根 Agent 运行。Owner／Reviewer 为 XLingyyy，无 Issue。

## 已确认的源码机制

- `Input/HearthwardInputBindings.cpp:30` 为动作目录，保存两个键位槽、一个 Shift／Ctrl／Alt 修饰键和上下文。Validate 拒绝同上下文非共享族冲突、保留键及已列系统快捷键；Load／Save 使用 GameUserSettings，未改变世界存档 schema。
- `UI/HearthwardScreenWidget.cpp:151` OpenPage 清除绑定捕获、调用角色 ResetHeldInput、维护返回页及稳定 action 焦点、刷新输入模式。HUD 为 GameOnly，其余正式页为 UIOnly。Character:109／110 在应用失焦及读档时也清理持键。
- Character:212 ResetHeldInput 清除 ActiveKeys、冲刺、跳跃、格挡、瞄准与钓鱼持线。CombatComponent:473 的 Aim(false) 会取消 draw；因此当前源码没有将此清理调用当作松键射箭。不要未经运行将清理链描述为会发射。
- ScreenWidget:214 按同一 MenuPause 设置确定普通页是否暂停；title、pause、save 固定暂停，dialogue 持续运行。WorldClock 统一推进游戏时间；这份调查没有另加 UI 倒计时。
- ScreenWidget:50 使用 UEditableTextBox，:548 仅 OnEnter 提交。PreviewKeyDown:465 在 Draft 聚焦时将事件交回 Slate。UE 5.8 的 SEditableTextBox::HasKeyboardFocus 会同时检查内部 EditableText；不能仅凭内层 Slate 控件存在而断定这里漏检文本焦点。
- ScreenSettings:152 已列出 1280×720、1920×1080、1920×1200、2560×1440、3440×1440、3840×2160。设置提供正文 100／125／150%，字幕 32／40／48、说话人／背景、FOV 70—110、模糊、震动与饥饿视觉选项。
- ScreenSettings:132 创建 15 秒 FPlatformTime deadline，ScreenWidget:381 超时恢复；OpenPage:168 离页也恢复；FinishDisplayChange 仅恢复模式和分辨率，再 ConfirmVideoMode／SaveSettings，保留其他已应用偏好。
- ScreenWidget:390 的 CanvasPoint 与 ScreenPaint:16 的绘制使用相同 ScaleToFit 和居中偏移。实际鼠标命中仍需按真实 viewport／Windows DPI 验证。

## 已定位且新增测试的契约偏差

`CT-TASK-051-input-traversal.md:9` 要求危险确认没有默认 Enter 执行。当前 ScreenWidget:488—495 在没有合法选中按钮时，只要 ConfirmAction 非空就直接执行 confirm。Refresh:373—376 在确认期间没有建立默认安全焦点。给定正常 pause／giveUp 入口且没有显式焦点，该调用链会进入 Survival::GiveUp 和 save 页。

新增 `Hearthward.UI069.DangerousConfirmationNeedsExplicitFocus`：

1. 沿既有 EquipmentPresentationTests 的 Standalone GameInstance、LocalPlayer、SceneViewport 和真实 UI 夹具建立角色。
2. 调用实际 Survival.ReceiveDamage(MaxHealth, 新 Event, 当前 Epoch)，确认进入可救援 Downed；打开 pause 并执行正常 giveUp 入口。
3. 经 NativeOnPreviewKeyDown 发送非重复 Enter，要求保持 Downed、原救援期限及真实 confirm 命中目标。
4. Escape 取消作负控；重开确认后通过 Down 键选择按钮，再 Enter，要求进入 Dead 和真实 save 页。

测试保留真实生存状态与 UI 路径；Native FKeyEvent 为引擎合成事件，不登记为桌面物理输入，也没有构造或假称 IME composing。生产修正未实施。`dangerous-confirmation-red.patch` 是相对根当前 ExperienceTests.cpp 的新增 hunk；本 Agent 仅执行 diff --check，原生 RED 结果待根运行。

## 其余待复现边界

| 边界 | 已确认代码事实 | 最窄实际验证 |
|---|---|---|
| 中文组合 Enter | UEditableTextBox／OnTextCommitted 已沿 Slate；引擎 WindowsApplication 处理 WM_IME 消息 | Windows 中文输入法输入含 E／R 的拼音，第一次 Enter 接受候选，第二次 Enter 提交；检查命令仅一次且没有世界动作 |
| 中文组合 Escape | 正式页 PreviewKeyDown 在 Draft 焦点检查前消费 Escape 并离页；Windows IME 可能先处理按键 | 真实候选会话中 Escape，观察先取消组合还是离页；不能用普通 Escape FKeyEvent 代替合成状态 |
| 右修饰键 | capture 将右 Shift／Ctrl／Alt 规范为左名称；Matches 接受事件的聚合 modifier bool，但 Held 只查已编码左键；Enhanced 移动 chord 也仅映射左 modifier | 设置 Ctrl+I 移动，分别按左／右 Ctrl；比对真实位移、Triggered／Held 和相同普通动作 chord。复现后才决定 Character／Input 的准确写范围 |
| 捕获期间应用失焦 | Character 已注册失焦清理；Screen 的 BindingCapture 仅在切页／完成／Esc 清理，没有自己的失焦回调 | 开捕获、按住 modifier、Alt-Tab、释放后回来，确认旧捕获不会将新的操作吃成绑定 |
| 滚动区外鼠标目标 | Paint 跳过 TextScrollClipped；Hit 只排除 Hidden、disabled，未排除正文 viewport 之外的元素 | 150% 长设置页在滚动区域下边界外按鼠标，检查被裁掉的按钮不会激活；使用真实 ActionAt 和 Native pointer 事件定位，随后桌面点击 |
| 应用暂停选项的当页语义 | apply 更新 MenuPause，但当页 OwnPause 计算发生于 OpenPage | 在 settings 页改变并应用暂停选项，观察当页世界状态及退出后的倒地／药品／钓鱼计时；先核对批准语义再改 |
| 显示试用中退出或销毁 | ApplySettings 本身会 SaveSettings；NativeDestruct 当前只 PopSoundMix，没有显示回滚；内存 deadline 属于该 Screen | 仅当根确认要覆盖这一流程时，在隔离配置中验证试用中关窗／切换世界后重启。当前不新增持久化协议或 fallback |

这些边界尚未运行，本报告没有填写失败或通过。前三类普通接口测试不能证明真实 IME 行为。

## 既有证据的实际边界

`docs/qa/TASK-051/input-pie.json` 记录过真实主键 Y、左 Ctrl+U、移动 I 的捕获与保存，实际移动约 2.276 cm；`verify_input_pie.py` 明确由主 Agent 的 computer-use 按键同步执行。它未覆盖右 modifier、应用失焦或中文组合输入。

`experience-pie.json` 是渲染 PIE／原生 API／Enhanced Input injection 的证据，包含 150% 设置、绑定、48 字幕等截图，脚本明确不宣称物理键盘／IME。当前同目录 REPORT.md 内容为后来动物实机任务，不能将其视为此次输入契约的完整报告。此次调查只引用具体 input／experience 文件，不复用旧版本 PASS 作为 069 结果。

## TASK-069 最小显示及输入验收矩阵

| 场景 | 目标状态与输入 | 证据方式 |
|---|---|---|
| 1280×720、正文 150% | 最大长度中文设置、空／满装备列表、确认、返回及底部按钮可达 | 实际窗口截图＋鼠标／键盘命中；离屏 CaptureUI 另记 |
| 1920×1080、正文 100／125／150% | title→设置→HUD→dialogue／memory，正文与输入框相对位置、焦点返回 | 实际窗口与 FKeyEvent 定向回归分开记录 |
| 1920×1200、3440×1440 | 居中／留边，地图平移缩放及边缘按钮、长中文 | 实际 viewport 尺寸＋点击位置 |
| 3840×2160；Windows 100／150% | 正文与字幕 32／40／48；子控件 Draft 和手绘 UI 对齐，候选窗位置 | 桌面窗口截图及真实 IME；只设 RenderTarget 尺寸不能覆盖 OS DPI |
| 改显示模式／分辨率 | 保留、Esc、15 秒超时、切页恢复；其余偏好保留 | Native 设置路径＋实际窗口与配置断言 |
| 改键与确认 | 主／副键、一个 modifier、上下文冲突；危险确认无焦点 Enter、显式导航确认 | 已有绑定纯函数测试＋新增真实 UI native 回归＋桌面键盘 |
| 失焦／加载／读档 | 持弓、冲刺、钓鱼或捕获期间切页／失焦／恢复，无旧释放发射／旧按钮世界动作 | 原生状态断言及桌面失焦；加载输入按实际 overlay 窗口复现 |

完整新游戏→经营／对话→失败→读档→通关菜单、23 个任务条件／领奖和音频实听仍依赖 054／064／067 等当前实现及根运行安排。本调查没有把状态标签、截图或旧脚本结果登记为这些流程已完成。

## 工程依据

Epic 的 [UGameUserSettings API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UGameUserSettings) 提供 ConfirmVideoMode／RevertVideoMode 等既有显示模式流程；本机 UE 5.8 `GameUserSettings.cpp:590—600` 明确 ApplySettings 包含 SaveSettings，调查据此检查试用状态的实际保存链。保持原架构即可，不需新的显示设置框架。

Epic 的 [SEditableTextBox API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Slate/Widgets/Input/SEditableTextBox?application_version=5.3) 说明其 IME context；本机 UE 5.8 `SlateEditableTextLayout.cpp` 和 `WindowsApplication.cpp:3208—3227` 为本项目实际引擎的组合输入路径依据。继续让 Slate 管理组合及提交，以真实 Windows 输入法验证外层 Preview 事件边界，不新增自制拼音处理。

## 首项真实 RED 及第二项修饰键测试

根 Agent 已反馈首项原生 RED：确认刚打开时 Enter 导致 Dead、救援剩余时间由 120 变为 0、pending confirm 消失。根随后最小删除默认 confirm 分支，构建已通过；本次子任务不重复其生产修改或代记 GREEN。

复查根当前 Input 和 Character 后，追加 `Hearthward.UI069.ModifierSidesAgreeForHeldAndMovementInput`。增量见 `modifier-sides-red.patch`，仅新增 ExperienceTests.cpp 测试 hunk，未修改生产。

- 创建真实 AHearthwardCharacter，沿其公开 SetupPlayerInputComponent／RebuildInputBindings 建立实际 Enhanced Input 映射和生产回调；绑定只改测试 GameInstance 内存，不 Persist。
- 分别设置规范 LeftControl+I、LeftShift+I 的 move.forward。Validate 保持原契约，先用裸 I 验证不能满足 Held 或产生移动请求。
- 通过真实 Controller::InputKey(FInputKeyEventArgs) 输入左／右 modifier 和 I，再调用公共 UEnhancedPlayerInput::Tick／ProcessInputStack，处理角色的实际 EnhancedInputComponent。UE 5.8 Controller::TickPlayerInput／ProcessPlayerInput 为 protected，测试不改可见性或加入测试专用 Controller 子类。
- 查询实际 IsInputKeyDown、生产 Matches、Held、Character::SemanticHeld；再读取生产 Move 回调产生的 PendingMovementInputVector。左侧必须明确产生非零前向输入，右侧必须满足相同 Held 并产生与左侧相同的向量。这样两侧均未消费时不会通过对比。
- 释放 I 和 modifier 后检查 Held 清除。该测试使用原生合成键输入，验证映射、触发器和角色移动请求；没有推进世界物理，不宣称角色实际位移或桌面键盘验证。

当前源码预期故障链仍为：Matches 对 modifier 使用聚合左右键布尔值；Held 仅检查绑定编码的左键；Character 只给该左键建立 PhysicalAction 和移动 chord。实际 Native RED 由根运行后记录。本 Agent 未运行或构建；diff --check 已通过。

## 第二项真实 RED、夹具同步及修正补丁

根运行 `render062-baseline069modifier` 后反馈：三项 Farming 渲染和危险确认通过；ModifierSides 实际 RED。右 Ctrl／Shift 的 Matches 为 true，而 Held、Character::SemanticHeld、Enhanced Move 消费为 false；左侧非零正控和裸 I 负控通过。这一差异确认了修正前的持键及映射问题。

第一版 modifier 测试引用 IPlatformInputDeviceMapper::Get 产生 LNK2019，需要 ApplicationCore 链接；根明确保持现有模块依赖，将单控制器隔离夹具改为 FInputDeviceId::CreateFromInternalId(0) 并移除 mapper include 后构建通过。此 Device 0 为隔离夹具的明示输入设备，未登记桌面设备验证。本 Agent 已将根当前 ExperienceTests.cpp 同步到隔离树，保留这一修正和所有原断言。旧 `modifier-sides-red.patch` 是包含该 mapper 引用的历史初稿，当前整合使用下述生产补丁。

关联 062 首轮渲染曾在 NativePaint 的缺失 PresentationComponent 调用处失败；根补齐真实夹具 PresentationComponent 后三项渲染通过，未给生产 NativePaint 增加空指针 fallback。该事实只说明夹具修复和根运行结果，不作为 069 的完整视觉验收。

`modifier-sides-production.patch` 相对根当时当前文件仅包含：

- InputBindings.cpp，Held 增加三种既有 canonical modifier 的右侧持键检查；主 Key 保持准确物理键判定，Matches／编码／Validate 未改变。
- Character.cpp，在 RebuildInputBindings 内建立局部 ChordModifiers 表。每种移动 modifier 共用一个 Boolean action，左右物理键映射到它；每项 movement mapping 使用一个 Chord。该 action 不绑定 KeyPressed／KeyReleased，既有裸物理 action 保持独立且参数准确。先创建 modifier mappings，再取得 movement mapping 引用，避免 MapKey 扩展数组后继续使用旧引用。
- ExperienceTests.cpp，在同一既有测试增加双侧同时按下、保持 I 时释放左侧、再释放右侧的断言。它防止双侧要求同时保持、单侧释放过早结束以及双侧导致重复前向请求的回归；原裸 I、左侧非零、右侧持键／移动断言全部保留。

本机 UE 5.8 `EnhancedInput/Public/InputAction.h:127` 的 Boolean action 默认 AccumulationBehavior 为 TakeHighestAbsoluteValue；两个 modifier mappings 取最大绝对值，未设置 Cumulative，双侧无需 AND。修正没有增加公共字段、生产 helper、类、模块或依赖。

Ctrl／Shift 同类问题由原生 RED 确认；Alt 的 canonical Matches／Held 和 movement modifier 路径具有同样源码结构，最小修正也覆盖既有 LeftAlt 规范。未扩展输入法 AltGr、系统快捷键或平台范围，未宣称右 Alt 已有桌面或原生专项运行结果。

此补丁尚未由本 Agent 构建或运行；根后续原生 GREEN 需另行登记。生产增量 InputBindings 为 +7／−1 行，Character 为 +14／−1，既有测试为 +11；未覆盖根 053／055 Character 其他修改，也未回退根的 Device 0 夹具修正。
