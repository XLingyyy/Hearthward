# TASK-072 Standalone RC 接入 smoke 提案

2026-10-05，仅 QA 提案。当前 AST PASS、真实 `--help` exit 0。UE、HTTP、Bootstrap 换图、模型及 CSV 均 **NOT_RUN**。Root 审阅并等待当前 UI/构建窗口结束后执行。

## 最窄执行入口

Root 复制 `run_standalone_rc_smoke_proposal.py` 到自身 `docs/qa/TASK-072/` 后：

```powershell
& 'G:/GameFactory/.venv/Scripts/python.exe' -X utf8 'G:/GameFactory/Hearthward/.agent-local/task051/docs/qa/TASK-072/run_standalone_rc_smoke_proposal.py' --project 'G:/GameFactory/Hearthward/.agent-local/task051/Hearthward.uproject' --ue-root 'G:/UnrealEngine/UE_5.8' --label standalone-reference-smoke --timeout 240
```

每次生成 UUID，自有 pool 为 `-HearthwardSaveTestPool=<uuid>`，输出为项目 `Saved/Task072/standalone-rc-smoke-<uuid>/`，另设此输出内 `UserDir`。不会使用正式 Compatible-v8 存档。该 save-pool 参数在 Editor Development 有效，源码 `HearthwardSaveSubsystem.cpp:73–78` 将其限定在非 Shipping；此次入口为 UnrealEditor `-game`，不提供 Shipping 信用。

生命周期复用公开 `UEClient.runtime.launch_editor` / `stop_editor`；立即取得 launch 返回的自有 PID，再写 launch artifact。所有 HTTP/函数返回异常均进入 `finally`，只停止本次 UEClient 追踪的 PID。`stop_request_ok` 仅记录停止调用成功：adapter `runtime/client.py:146–151` 调用 terminate 后未 wait，`process_exit_verified=NOT_VERIFIED_BY_UECLIENT` 保留这一边界。Root 串行操作下一次 UE 前仍需按已有流程确认本次 PID 已退出。

## 公共引用链与一次 seed attempt

固定路径只包含三个引擎原生 CDO 类标识以及本次明确请求的 Bootstrap 地图资产对象路径。动态 Pawn、Controller、HUD、Screen、GameInstance、WorldSubsystem、Companion 路径全部读取实际 HTTP 返回，不猜 `Hero_0` 等 Actor 名，不读取私有字段。

1. `/remote/info` 返回 200、JSON object 并公布 `/remote/object/call`。仅启动阶段连接尚未建立时重试；HTTP error、异常 JSON、缺函数/字段立即失败并保留原返回。
2. 对 `/Game/Hearthward/Bootstrap/L_Bootstrap.L_Bootstrap` 记录 **bootstrap_seed_attempt**，先 `KismetSystemLibrary.IsValid`。该成功本身不计 live-world 成功：RC 的 UObject 参数反序列化可 `StaticLoadObject` 仅加载地图资产。
3. 公共 `GameplayStatics.GetPlayerPawn(seed,0)` 必须返回实际 Pawn，再 `Pawn.GetLevel → KismetSystemLibrary.GetOuterObject(actual Level)` 获取实际 World；核对 `GetPathName(actual World)`、`GetCurrentLevelName(actual Pawn)` 与请求地图。任何失败均停止，不增加 Engine/GameViewport 或 guessed Actor fallback。
4. `GameplayStatics.GetPlayerController(actual Pawn,0) → PC.GetHUD → HUD.Screen READ_ACCESS → Screen.GetPage` 必须为 title。Screen 是当前生产 HUD 的 public BlueprintReadOnly 属性；无 property write。
5. `GetGameInstance(actual Pawn)` 返回的实际 GI 留存；唯一 UI 动作是当前 Screen 公开 `ExecuteAction('new')`。这是生产 Bootstrap 正常新游戏分支，不预置设施、资源、角色位置、时钟或自然状态。
6. `new` 返回 true 只说明 OpenLevel 已请求。用实际 GI 的 `GetCurrentLevelName` 等到 `L_HearthwardWilds`，随后重取 Pawn/Level/World/Controller/HUD/Screen；新 World 必须与旧 World 不同。GI 的 `GetWorld()` 在正常 OpenLevel 后随 WorldContext 更新，可作为跨地图公共 context；旧 World-owned Actor 路径不复用。
7. 公共 LoadingSubsystem `IsLoading` 严格返回 bool，待 false，再检查新 Screen 为 HUD。实际 `GetAllActorsOfClass` 返回一个 HearthwardCharacter、一个 CompanionFixture；当前 Pawn 必须与实际角色列表相同。自然 SaveSubsystem `IsNaturalWorldEnabled` 必须 true，记录实际 CampaignId。
8. 公共 `GetWorldSubsystem(ContextObject=actual Pawn,Class=HearthwardLocalAISubsystem)` 返回 LocalAI；只读 `IsBusy/IsModelReady/GetGenerationCalls/GetServerProcessId`。此次 fresh new 预期 false/false/0/0。不调用 SubmitPlayerText、StartServer、ConfirmCandidate；无模型文件部署前置。

Engine 对象未用于 WorldContext。源码 `UnrealEngine.cpp:14400–14425` 的 LogAndReturnNull 分支调用 context.GetWorld；Engine 未提供此链需要的 World，因此不做已知无效的 engine-path fallback。若地图 seed 不能取得实际 Pawn，本次 smoke 以该 stage FAILED 收束，后续由 Root 根据真实 HTTP 回包决定接入方式。

## RC 监听端口的具体修正

脚本选择一个临时 loopback TCP port，并在启动参数加入 `-RCWebControlEnable` 及下列两个进程级覆盖：

```text
-ini:RemoteControl:[/Script/RemoteControlCommon.RemoteControlSettings]:RemoteControlHttpServerPort=<chosen-port>
-ini:RemoteControl:[/Script/RemoteControlCommon.RemoteControlSettings]:bAutoStartWebSocketServer=False
```

`UEClient` 虽把端口追加在 `WebControl.StartServer <port>` ExecCmds 后，本机 `WebRemoteControl.cpp:849–855` 注册的是无参 FConsoleCommandDelegate，端口来自 `RemoteControlSettings` 默认对象（352 行）。所以当前 host 需上述覆盖才能令其选定端口与真实 RC HTTP listener 一致。`RemoteControlSettings.h:180/307/311` 确认 config=RemoteControl、WebSocket 字段和 HTTP 字段；`ConfigCacheIni.cpp:2344–2346/2447–2449` 支持逐个独立覆盖。未写 Config、adapter 或长期 QA 服务，不回退默认30010连接另一进程。

## 输出与判定边界

- `launch.json`：公开 UEClient 返回，包括实际命令和自有 PID。
- `http-events.jsonl`：每个实际请求/HTTP status/原 JSON 返回/阶段/host elapsed；这些 host 时间不提供帧时统计。
- `results.json`：实际引用链、CampaignId、AI只读状态、失败阶段/理由、停止调用结果。
- `stop.json`、`engine.log`：自有 PID 的停止调用结果与 Unreal 原始日志。

只有全部引用、正常 new 换图、Loading结束及零模型状态满足，脚本才记录 `REFERENCE_SMOKE_PASS`；退出码还要求停止调用成功。记录 `acceptance=NOT_EVALUATED`、`performance=NOT_RUN`。首次真实 Paint、OS输入、模型请求、CSV60秒、五场景/两后端、暖p95、Shipping/双机/安装均不在此次判定中。

## 主源核对

[官方 Remote Control HTTP reference](https://dev.epicgames.com/documentation/en-us/unreal-engine/remote-control-api-http-reference-for-unreal-engine) 明确 object/call 调用公开 BlueprintCallable、显式参数/返回及 property READ_ACCESS。[官方 Remote Control Quick Start](https://dev.epicgames.com/documentation/unreal-engine/remote-control-quick-start-for-unreal-engine?lang=en-US) 提供现有 RC 接入方法；本提案直接复用已有引擎插件。

本机核对：RC `SerializeCall` 输出 ReturnParm/OutParm；JSON UObject 返回 GetPathName 字符串，null为空串；RCJsonStructDeserializerBackend.cpp:120–133 可 StaticLoadObject，因此必须验证实际 Pawn/Level/World。UHT 将 BlueprintPure 设置为 BlueprintCallable。Actor.GetLevel、PC.GetHUD、GameplayStatics 与 SubsystemBlueprintLibrary 参数名称均按本机公开头文件，HUD.Screen 按 Root public BlueprintReadOnly。Root .uproject 已开启 RemoteControl，EngineAssociation=5.8。

Root Source/Content/Git、模型参数、冻结数据均未修改；未计算 hash/checksum。静态结果在 `standalone-rc-smoke-proposal-static-check.json`。剩余 gate 是 Root 实际 HTTP/换图接入 smoke，当前无运行信用。

2026-10-05 Root实际执行更新：接入已REFERENCE_SMOKE_PASS，见standalone-reference-runtime-review.md。本文上方NOT_RUN保留提案时点；当前helper已按真实失败补initial GET启动超时重试、Bootstrap Loading结束等待及精确generated Saved session配置。规则已用同类未列只读函数400负对照验证；不把此成功登记为模型或性能通过。
