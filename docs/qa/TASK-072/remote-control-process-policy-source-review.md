# TASK-072 Remote Control 单次进程白名单：源码核对

此调查只读取本机 UE 5.8 和 Hearthward 的调用路径。未启动 UE、未修改生产代码或项目 Config。`remote-control-process-policy.proposal.ini` 是待根代理审查的 generated Saved 配置提案，含当前 smoke 的 21 个具体函数；每条 `bAllowChildClasses=False`，数组使用无前导 `+` 的重复键。首次自己的提案误用了 default 层 `+` 语法，已按第五轮实际失败及读取源码纠正。

## 临时 ini 能力

`-RemoteControlINI=<绝对路径>` 已由本机源码确认：

- `Engine/Source/Runtime/Core/Private/Misc/ConfigCacheIni.cpp:6288–6312` 用 `BaseIniName + "INI="` 读取命令行覆盖并标准化生成/保存配置路径。
- `Engine/Source/Runtime/Core/Private/Misc/ConfigContext.cpp:1275–1289` 在当前 uncooked 进程读取该路径到 `SavedLayer`；`1367–1375` 合并到当前配置。
- 同文件 `1118`、`1372` 的命令行配置覆盖沿逻辑 `BaseIniName` 解析。因此文件可叫 `process-RemoteControl.ini`；现有 `-ini:RemoteControl:...` 端口/关闭 WebSocket 参数仍匹配 RemoteControl 分支。
- `ConfigContext.cpp:687–699` 允许随后将该分支写到同一个临时路径。本次路径应位于独立 QA Saved/session 目录。

Python subprocess 参数列表只需传入完整单项：

```python
f"-RemoteControlINI={session_dir / 'RemoteControl.ini'}"
```

将双引号放到 ini 文件中的函数名，免去 Windows argv 内部引号传递。临时文件是覆盖生成配置路径，现有配置层级仍会加载；本次只能以实际正/负请求确认最终权限。

**generated Saved 与 default 层的数组语法不同。** `ConfigContext.cpp:1288` 传 `bHandleSymbolCommands=false`。`ConfigCacheIni.cpp:2048–2051` 在此模式将重复键默认作为 ArrayAdd；`2076–2097` 仅在该 bool 为 true 时剥去前导 `+`，`2106` 将剩余字符串直接作为 KeyName。所以该临时文件必须写重复 `CustomAllowedRemoteFunctionCalls=...`，不能写 `+CustomAllowedRemoteFunctionCalls=...`。后一写法会形成带加号的字面键，真实 Settings 属性取不到它；这一层错误发生在结构 ImportText 之前。

## 字段导入与执行门禁

`Plugins/VirtualProduction/RemoteControl/Source/RemoteControlCommon/Public/RCAllowedRemoteFunctionCall.h` 定义 `FSoftClassPath ClassPath`、`TOptional<FString> FunctionName`、`bool bAllowChildClasses`。

通过本次 `-RemoteControlINI` generated Saved 路径加载时，合法最窄行是：

```ini
CustomAllowedRemoteFunctionCalls=(ClassPath=/Script/Engine.KismetSystemLibrary,FunctionName=("IsValid"),bAllowChildClasses=False)
```

`CoreUObject/Private/UObject/PropertyOptional.cpp:361–411` 按括号包裹的内部值导入 Optional；`StrProperty.cpp.inl:115–145` 对结构字段的 delimited FString 要求双引号。因此 `(IsValid)` 不应当用作可审计的单函数规则。`ClassPath` 可以使用无空格的裸 `/Script/...` 路径：`SoftObjectPath.cpp:623–679`、`Property.cpp:636–649` 接受该 token。

`RemoteControlCommon/Private/RCAllowedRemoteFunctionCallUtilities.cpp` 按目标对象实际 `GetClass()` 和反射函数 `GetName()` 比较；FunctionName 未设置时整类函数均可获准。故继承的 `GetLevel` 匹配当前真正的 `/Script/Hearthward.HearthwardCharacter`，`GetHUD` 匹配 `/Script/Engine.PlayerController`，本次两个实例都无需允许子类。`ClassPath` 使用类路径，CDO 对象路径的 `Default__` 不放入规则。

`WebRemoteControl/Private/WebRemoteControlInternalUtils.cpp:537–566` 在执行前检查 `bAllowAnyRemoteFunctionCall` 及内置/自定义规则。内置列表并未允许本次 Kismet 的 `IsValid` 或负对照 `IsDedicatedServer`。`RemoteControlSettings.h:350–377` 确认 AllowAny 默认 false、自定义数组为 Config。无需启用远程 Python、Console 或 AI 生成接口。

## 精确 21 项

| ClassPath | FunctionName |
| --- | --- |
| `/Script/Engine.KismetSystemLibrary` | `IsValid`, `GetOuterObject`, `GetPathName` |
| `/Script/Engine.GameplayStatics` | `GetPlayerPawn`, `GetCurrentLevelName`, `GetPlayerController`, `GetGameInstance`, `GetAllActorsOfClass` |
| `/Script/Hearthward.HearthwardCharacter` | `GetLevel` |
| `/Script/Engine.PlayerController` | `GetHUD` |
| `/Script/Engine.SubsystemBlueprintLibrary` | `GetGameInstanceSubsystem`, `GetWorldSubsystem` |
| `/Script/Hearthward.HearthwardScreenWidget` | `GetPage`, `ExecuteAction` |
| `/Script/Hearthward.HearthwardLoadingSubsystem` | `IsLoading` |
| `/Script/Hearthward.HearthwardSaveSubsystem` | `IsNaturalWorldEnabled`, `GetCampaignId` |
| `/Script/Hearthward.HearthwardLocalAISubsystem` | `IsBusy`, `IsModelReady`, `GetGenerationCalls`, `GetServerProcessId` |

`HUD.Screen` 通过对象属性 READ_ACCESS 获取，无新增函数授权。已有独立保存池、loopback、临时端口约束沿根脚本保留。

## 已发生结果与当前门禁

1. 根报告最初 `KismetSystemLibrary IsValid` HTTP400，具体错误指向函数白名单。
2. 第三轮 bare FunctionName 的 21 项参数让原生引用调用 HTTP200、seed 验证通过。这只证明请求获准，未证明单函数过滤；Optional 未设置导致类级权限是由导入/权限源码支持的解释，尚无本次实际反序列化读回证据。该轮新游戏 Action 被真实 Loading guard 提前拒绝，不能称为完成正常路线。
3. 第四轮尝试读取 RemoteControlSettings CDO 实际 HTTP400、对象不可访问。`RemoteControl/Private/RemoteControlModule.cpp:2579–2582` 明确拒绝该 CDO；取消这项读回符合源码契约。不能将读回失败归因于白名单格式。
4. 根第五轮已切换为独立 Saved/session/RemoteControl.ini，规则的 FunctionName 使用带引号值；实际 `IsValid` 仍返回相同函数未允许 HTTP400。未见 ImportText/Opening warning，文件 21 行保持。根 helper `run_standalone_rc_smoke_proposal.py:132` 确认写入 `+CustomAllowedRemoteFunctionCalls`，`136` 确认通过 `-RemoteControlINI` 载入。上述 SavedLayer 源码确认带加号键名在此读取模式不被剥离，解释了权限值缺失和没有 ImportText 警告。当前最小候选仅移除文件键名前导 `+`；尚未执行此候选，不能记为 PASS。

根此时准备下一轮 quoted CLI 传输对照；无论采用 CLI 或修正后的 Saved 文件，当前最窄运行判据：同一实际进程的明确列入函数返回 HTTP200；相同 KismetSystemLibrary CDO 的无害未列入 `IsDedicatedServer` 返回 HTTP400，且错误准确为该函数不允许。负对照不列入白名单、不触发命令/Python/游戏动作。完整 smoke 还须等待 Loading 结束后再发新游戏 Action，分别记录权限门禁与游戏入口结果。

本机源码优先于通用文档描述。官方配置文档的命令行数组限制没有完整描述本机新动态层实现；`ConfigCacheIni.cpp:2398–2401` 确实支持 `+` ArrayAdd，但文件表达可以避免内部引号传递问题，但必须使用上述 generated Saved 重复键语法。参考官方类型说明：[URemoteControlSettings](https://dev.epicgames.com/documentation/unreal-engine/API/Plugins/RemoteControlCommon/URemoteControlSettings)、[Configuration Files](https://dev.epicgames.com/documentation/en-us/unreal-engine/configuration-files-in-unreal-engine)。
