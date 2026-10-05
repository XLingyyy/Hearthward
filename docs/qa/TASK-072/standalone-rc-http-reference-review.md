# Standalone RC HTTP 返回与引用链复核

2026-10-05。只读本机 UE5.8 与 Root 生产代码，复核当前224行 smoke 提案；未启动 UE、HTTP、模型、构建或 Git。**未找到需要修改解析代码的确定缺陷。** 本记录不提供实际接入成功信用。

## 首次 seed 的用途

当前 HTTP 顶层 `objectPath` 是原生函数库 CDO，例如 `/Script/Engine.Default__KismetSystemLibrary`；Bootstrap seed 位于 `parameters.Object` 或 `parameters.WorldContextObject`。两处解析不同：

- `RemoteControlModule.cpp:1261–1287` 的 call 目标经 `StaticFindObject` 解析，并要求公开 BlueprintCallable / BlueprintEvent。它不会因 seed 参数加载地图而自动找到动态 Actor。
- `WebRemoteControlInternalUtils.cpp:188–189` 对参数采用 `FRCJsonStructDeserializerBackend`；该 backend `RCJsonStructDeserializerBackend.cpp:120–133/157–161` 的 UObject 参数可 `StaticLoadObject`。`IsValid(seed)` 可以只证明地图资产对象存在。
- `UObjectGlobals.cpp:1426–1438` 对完整对象名先解析 outer，再尝试内存对象 reconciliation；后续实际返回的完整 UObject path 可以作为公函参数重新查找，无需把路径改成资产名或猜 Actor 名。

因此现稿的单次 seed attempt 及后续验证必须保留：`GetPlayerPawn(seed,0)` 返回非空实际 Pawn，`Pawn.GetLevel → GetOuterObject(Level)` 返回实际 World，地图名为 Bootstrap，World path 等于明确请求的地图 seed。此链失败即报告当前 stage FAILED，无额外 Engine/GameViewport/private fallback。只有实际 HTTP 回包能判断本次 standalone 的 seed 是否对应正在运行的 World；静态代码不宣称该 attempt 必然成功。

Root `Config/DefaultEngine.ini:4–7` 的默认地图与默认 GameMode、`HearthwardGameMode.cpp:5–8` 的 Hero/HUD 默认类提供正常 Bootstrap 入口依据。此前场景中的动态 Actor 名不应移植到本次进程。

## 回包形态

以下是序列化规则示意，斜杠路径代表实际 HTTP 回包字段，非伪造运行结果。

| 公开入口 | 实际 JSON 根字段 | 当前脚本用法 |
|---|---|---|
| 返回 UObject 的 `/remote/object/call` | `ReturnValue` 为 GetPathName 字符串，null 为空字符串 | `.get('ReturnValue')` 后严格非空 `/` 开头检查 |
| `GetAllActorsOfClass` | `OutActors` 为完整 UObject path 字符串数组 | `.get('OutActors')`、类型/数量与当前 Pawn 一致性检查 |
| `/remote/object/property`，`propertyName=Screen`、READ_ACCESS | `Screen` 为 GetPathName 字符串，null 为空字符串 | `.get('Screen')` 正确 |
| `/remote/info` | `HttpRoutes` 数组，元素含 `Path/Verb/Description` | 当前 route Path 检查正确 |

`WebRemoteControl.cpp:985–996` object/call 调用后直接输出 `SerializeCall(...,true)`；`WebRemoteControlInternalUtils.cpp:237–245` 仅保留 ReturnParm/OutParm 及其子项。`JsonStructSerializerBackend.cpp:192–198` UObject 写 GetPathName 或空串。即使 HTTP200，仍需检查业务返回、非空实际引用及类型；现稿已经这样处理。

`HUD.Screen` 没有 ReturnValue、PropertyValue 或 nested object description 包装。`WebRemoteControl.cpp:1016–1024` 将 READ_ACCESS 序列化 buffer 直接写入 response body；`RemoteControlModule.cpp:1587–1592` 对单属性调用 SerializeElement；`StructSerializer.cpp:341–342` 外层创建匿名 JSON object；`JsonStructSerializerBackend.h:81–84` 使用属性名 Screen 作 key。Root `HearthwardHUD.h:13–14` 明确它是 public BlueprintReadOnly UPROPERTY，READ_ACCESS 不写属性。当前107–108行无需改为其他层级。

`RemoteControlResponse.h:16–39` 的 `/remote/info` 字段确为 HttpRoutes；`RemoteControlRoute.h:58–70` 将 route Path 与 Verb 放入描述。当前脚本仅检查 call route Path，后续实际 PUT/HTTP状态仍逐次检查。

## HUD、GI、LocalAI 的关系

`PC.GetHUD` 返回当前 HUD；读取其 public Screen 后调用正常 `ExecuteAction('new')`。Root `HearthwardScreenActions.cpp:321–327` 该分支正常 BeginLoading/OpenLevel 后即返回 true，所以现稿没有在这个 return 上宣称换图完成。

GI 经公开 `GameplayStatics.GetGameInstance(actual Pawn)` 获取。它跨正常地图切换保留；随后从该实际 GI 的 WorldContext 重新获取 Pawn/PC/HUD/Screen。LocalAI 是 **WorldSubsystem**，现稿通过 `SubsystemBlueprintLibrary.GetWorldSubsystem(ContextObject=actual Pawn,Class=HearthwardLocalAISubsystem)` 获取实际 LocalAI ref，没有读取名为 GI.LocalAI 的属性。Loading 是 GI Subsystem，Save 与 LocalAI 是 WorldSubsystem，此类关系与现公开头文件相符。

未发现需要新增 private property、动态对象名 fallback 或解析兼容分支的证据。Root 执行时以 `http-events.jsonl` 保存每个实际返回；若 seed、Screen 或 subsystem lookup失败，保留原HTTP/阶段并停止本次 smoke后审阅首个实际失败。
