# TASK-069 显示设置后 Continue 输入边界只读调查

状态：Root 第七次原Apply路径的实际日志与OS按键确认HUD输入门被加载Hide恢复旧true，见文末更新；修复后验证仍待Root。第六次cold的before=0、saved=0及未完成按键对照仍按原事实记录。未修改 Root / Source / Content / Resources，未运行 UE / build / Git。

## 已知运行事实

Root 报告路径：Title150 → Settings → Apply100，窗口实际变为 borderless2560×1600 → Continue。HUD 正常 tick，OS Escape / P / Tab 未打开菜单。同 Source 冷启 windowed100 → Continue 后 Escape 立即成功。两次观察的显示模式、Apply 路径和按键时刻不同，不能直接推出冷启修复或窗口模式就是原因。

## 可核查调用链

1. `HearthwardScreenSettings.cpp:102–137`：Apply 会无条件执行 `UGameUserSettings::ApplySettings(false)`，包括仅修改文字缩放；这同时重新提交显示模式和分辨率。ChangedDisplay116比较 UserSettings 的存储状态与 draft，未与实际 Slate window 的模式/尺寸比较。125之后写入 Bindings/Comfort，131重建角色输入映射，136 Refresh；此处没有重新设置输入模式。
2. `HearthwardScreenWidget.cpp:218–226`：OpenPage(hud) FlushPressedKeys 后设置 GameOnly；其它页 UIOnly 聚焦当前 widget。
3. `HearthwardScreenActions.cpp:62–85`：自然世界 Continue 在当前自然地图时 BeginLoading→PrepareSession/LoadPoint→FinishSession；从 Bootstrap 时 BeginLoading→OpenLevel，目标 HUD InitializeScreen85–89在读档成功后 OpenPage(hud)→FinishSession。
4. `HearthwardLoadingSubsystem.cpp:89–94`：第一次附加当前 viewport 时保存 `PreviousIgnoreInput=V->IgnoreInput()`，再 SetIgnoreInput(true)。同一 viewport 上重复 BeginLoading 不重新保存。
5. `FinishSession:116–119` 只释放 AwaitingSession；成功路径不会立即 Hide。Tick138–146等待世界/地面/资源就绪及0.7秒淡出。
6. `Hide:154–158` 移除遮罩并以原快照 SetIgnoreInput(PreviousIgnoreInput)，没有检查期间页面已建立的新模式。

## 本机 UE5.8 源码确认的契约

- `G:/UnrealEngine/UE_5.8/Engine/Source/Runtime/Engine/Private/PlayerController.cpp:6372–6385`：UIOnly 设置 viewport IgnoreInput=true；6439–6450：GameOnly 对有效 SViewport 排入 SetUserFocus，同时设置 IgnoreInput=false。公共 API 的 GameOnly 语义是将输入交给 PlayerInput/PlayerController。[Epic FInputModeGameOnly](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/FInputModeGameOnly)
- 同文件6454–6466：SetInputMode 将焦点操作交给 LocalPlayer SlateOperations，并更新 debug 模式字符串；该字符串不会因稍后的单独 SetIgnoreInput 改写。因此“debug模式仍GameOnly”不能证明输入门已开放。
- `Runtime/Engine/Private/GameViewportClient.cpp:767–770`：IgnoreInput=true 时不进入后面的玩家按键分发；InputAxis815–817同样直接返回。
- `Runtime/UMG/Private/WidgetBlueprintLibrary.cpp:157–159`：SetFocusToGameViewport 仅调用 SetAllUserFocusToGameViewport；`Runtime/Slate/Private/Framework/Application/SlateApplication.cpp:2655–2664` 查找游戏 viewport 的 Slate 路径并设置用户焦点，未改 IgnoreInput。补焦点的效果须与输入门状态分开测量。
- `Runtime/Engine/Private/GameUserSettings.cpp:590–600` 的 ApplySettings 包含 ApplyResolutionSettings；563–586会请求分辨率和模式变化。公开文档同样说明 ApplySettings 应用全部设置，参数控制是否检查命令行覆盖。[Epic UGameUserSettings](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UGameUserSettings?lang=en-US)

## 当前证据支持的推断

若 BeginLoading 时保存了 UIOnly 的 true，随后 HUD 建立 GameOnly 的 false，而 Hide 恢复旧 true，则玩家按键会被 viewport gate 截断，HUD tick 可继续。这是明确的待验证状态顺序，不依赖缓存、IME或窗口猜测。

此链也出现在普通 Title→Continue，不能仅凭 Source 解释 RED 与冷启即时成功的差别。需要确认两次 Begin/Hide 的实际快照值、GameOnly创建时 viewport 是否有效，以及首次 Escape 距 Hide 的时间。显示 Apply 的事实只证明它重新提交显示请求，尚无焦点丢失的运行证据。

角色输入 `HearthwardCharacter.cpp:248–250` 在收到语义键后调用 HUD.OpenPause / ToggleInventory；这些分支位于生存 Alive 守卫255之前。若 viewport 已挡住事件，修改菜单快捷键或生存状态不能定位首个来源。

## 最窄定位建议（由 Root 执行）

1. 原 RED 路径只加 begin / HUD模式建立 / Hide 三处状态记录：当前实际 window mode+尺寸、viewport identity、PreviousIgnoreInput、当前 IgnoreInput、GameOnly debug string、Slate当前 keyboard focus type；Hide前后各记一次，保留已有加载begin/end时间。
2. 在 HUD 可见后先确认加载end，再分别试 Escape、Tab和单次 WASD；记录事件是否到达 viewport/Character。HUD正常tick不能替代键事件证据。
3. 冷启 windowed100 对照：同样等加载end后再按 Escape；另外可在淡出最后一段按一次 Escape，测定“立即成功”是否早于旧快照恢复。保持同存档和键位。
4. 若 IgnoreInput=false 且没有键事件才继续测 OS前台窗口 / Slate焦点；只点击 viewport 或 Alt-Tab 再回来的结果用于区分焦点，不能用它代替状态记录。

若日志确认 loading 的旧快照覆盖新GameOnly，Root 提出的“模式建立后更新 loading 快照并继续遮罩禁输入，Hide恢复最新模式”是聚焦该状态所有权的候选。尚未运行该 RED 前，不建议实施输入补丁、重复补焦点、延时重设模式或给每次 HUD tick 加恢复逻辑。


## 第六次 cold Continue：saved0后的收窄

Root提供的实际诊断摘要：20:46:18 loading input restore before=0、saved=0，加载已结束。按代码Hide先输出这两个值，再SetIgnoreInput(saved)，因此该次Hide计划恢复0；Root未在超时前完成OS按键对照，不能记输入PASS，也不能记该次恢复失败。此结果排除了“本次Hide恢复旧true”的解释；没有排除原Apply路径的未定位问题。

当前初始化顺序提供一个可测分支，不需要引入窗口/IME假设：

- Loading.BeforeMap97–101调用BeginLoading，早于新HUD创建时可以已保存viewport状态；重复同viewport的BeginLoading89不会重采样。
- HUDDialogue.cpp:18–24创建Screen并InitializeScreen；Widget69先OpenPage(title)，其UIOnly分支226可能设置IgnoreInput=true。初始地图的AfterMap114对无HearthwardLoad/NewGame URL会释放AwaitingSession，之后Hide可恢复先前保存的0。
- 当Cold停留Title后直接Continue时，快照可能就是0；若之后实际进入Settings并返回Title，OpenPage再次建立UIOnly，Continue快照可能变1。此顺序与两条路径差异相容，尚须记录首次Bootstrap遮罩结束及返回Title时的真实值。
- HUDDialogue.cpp:42–44的SnapshotRestored也会OpenPage(hud)；同世界读档可能通过此事件清输入门。跨图新HUD在InitializeScreen85–89明确再建HUD模式。两路径应记录实际Controller/viewport identity，避免将不同实例的记录混在一起。
- UIOnly/GameOnly引擎ApplyInputMode都以GetGameViewportWidget().IsValid()为前置；若入口仍为0且无初始Hide覆盖，还应记录该次OpenPage时viewport widget是否有效。此检查用于辨别模式调用是否实际执行，不能先认定无效。

最窄下一次：重跑原Title150→Settings→Apply100→返回Title→Continue路径，记录首次地图begin/end、返回Title的IgnoreInput、Continue所保存值、HUD模式建立后及Hide前后。在loading end后立即实际Escape/Tab，不把构建或代理等待放在OS操作之前；现有host预算需足够覆盖这段操作，不新增运行helper。

若原路径saved=1且Hide后为1，同时真实键无法分发，再确认旧快照链；若原路径saved=0、Hide后0，继续测实际前台窗口/Slate焦点和键事件到达点。当前暂不改变Loading或输入模式语义。


## 第七次原Apply路径：输入门恢复旧true已确认

Root回传实际OS RED：Apply100→Title→Continue，loading begin saved1；20:54:54 Hide日志before0/saved1，结束后实际Escape/Tab仍留HUD，时钟继续。Hide代码恢复saved值，故该次确实把HUD建立后的false覆盖为旧true。第六次cold saved0与本次saved1的差别已由运行记录确认，不再以窗口模式或字体缩放直接归因。

Root将负责最新模式通知的最小实现：Widget每次实际SetInputMode后通知loading；遮罩有效时保存当前输入门为新的恢复目标并继续禁输入；Hide恢复该最新目标；没有页面变化的travel保留原快照。对应 Native 提案见 loading-page-input-test-proposal.patch，使用真实OpenPage检验通知接入，尚未编译或执行。修复后的OS路径仍须Root运行确认。
