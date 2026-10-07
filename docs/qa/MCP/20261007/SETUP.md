# UE 5.8 原生 MCP 接入

验证日期：2026-10-07（Asia/Shanghai）。初次连接历史见同目录 [connection.json](connection.json)；最终驻留验证见 [final-resident-connection.json](final-resident-connection.json)。配置启用、HTTP 协议实连和当前聊天工具挂载分别记录。

## 配置

- 使用本机 UE 5.8.2 自带的 `ModelContextProtocol` HTTP 服务。
- `Hearthward.uproject` 保留原有 GUID、`MCPClientToolset` 与 `ModelContextProtocol` 改动，仅新增 `AllToolsets`，加载引擎自带工具集。
- `Config/DefaultEditorPerProjectUserSettings.ini` 设置自动启动、端口 `8000`、路径 `/mcp`，保留默认工具搜索模式。
- `G:/GameFactory/.codex/config.toml` 与游戏目录的 `.codex/config.toml` 均登记同名 `unreal-mcp`，地址为 `http://127.0.0.1:8000/mcp`。同名配置按作用域覆盖。
- 用户级配置已有同名服务，未改写；其启动超时为 30 秒，工具超时为 120 秒。

引擎生命周期仍通过 `from engine_adapters.ue5 import UEClient` 执行。启动参数为 `-ModelContextProtocolStartServer -ModelContextProtocolPort=8000`；Remote Control 另带 `-RCWebControlEnable`。本次使用现有引擎插件，未安装第三方桥接、MCP SDK 或修改全局 Python 环境。

## 初次实测（历史）

初次记录时间为 `2026-10-07T03:46:10+08:00`。通过标准库 HTTP 客户端按 MCP 顺序执行只读连接验证：

1. `initialize` 返回 HTTP 200，协商协议 `2025-06-18`，建立有效会话。
2. `notifications/initialized` 返回 HTTP 202。
3. `tools/list` 返回 HTTP 200，列出 `list_toolsets`、`describe_toolset`、`call_tool`。
4. `list_toolsets` 与 `describe_toolset` 成功发现引擎工具集和 SceneTools 参数契约。
5. 经 `call_tool` 调用 SceneTools 的 `get_current_level`，返回 `/Game/Hearthward/Bootstrap/L_Bootstrap`。
6. `UEClient.observe.check_status` 返回 `ok=true`，Remote Control 和 Python `remote_execution` 均已就绪。
7. 本机 `codex.exe mcp list --json` 已读取并启用 `unreal-mcp` 的 Streamable HTTP 配置。

验证过程未操作 PIE、游戏输入或修改地图。引擎启动与原始握手证据分别保留于 `.agent-local/qa/MCP/20261007/launch.json` 和 `handshake.json`；完整工具描述留在本地证据内。

## 最终驻留实测

root 使用公开 `UEClient.runtime.launch_editor` 实际启动正常 Bootstrap Editor。runner 启动前记录时间为 `2026-10-07T02:29:25.535341+00:00`，启动就绪用时 `49.37684509996325` 秒，完整检查用时 `52.31618620001245` 秒。最终检查时 owned Editor PID 为 `52580`，正常 Editor 保持运行，当前地图为 `/Game/Hearthward/Bootstrap/L_Bootstrap`。启动使用 MCP 端口 `8000`、Remote Control 端口 `30010` 和配置的 runtime input 端口 `30020`，没有使用 `-game`、`NullRHI`、`NoSound`、PIE 或退出命令。

- `initialize` 实际返回 HTTP 200，协议 `2025-06-18` 匹配，session header 存在；只保留存在标志，不保存 session ID。
- `notifications/initialized` 返回 HTTP 202。
- `tools/list` 返回 HTTP 200，恰好三个元工具：`list_toolsets`、`describe_toolset`、`call_tool`。
- 经 `call_tool` 调用 `editor_toolset.toolsets.scene.SceneTools.get_current_level`，实际返回 HTTP 200 和 Bootstrap 地图，结果不是工具错误。
- 公开 `UEClient.observe.check_status` 返回 `ok=true`，Remote Control 与 Python 均就绪。
- 在 `G:/GameFactory` 工作目录执行只读 Codex CLI 配置检查，返回码为 0；`unreal-mcp` 已启用，地址匹配 `http://127.0.0.1:8000/mcp`。此项确认配置读取，不提供当前聊天工具挂载信用。

本次只发现三个元工具并执行上述只读地图查询，没有重新运行完整工具集发现。初次 `connection.json` 中的 52 个工具集属于历史范围，未迁移为本次断言。安全派生记录见 [final-resident-connection.json](final-resident-connection.json)；私有安全原始结果、启动记录及运行日志分别保留在 `.agent-local/qa/MCP/final-resident-20261007-01/results.json`、`launch.json`、`editor-runtime.log`。本次文档收尾没有再次请求端点、操作 Editor 或 Codex UI。

## AllToolsets 依赖与首轮 Cook 失败

2026-10-07 TASK-103 首轮真实 Package：Shipping 编译成功；Cook 处理1637个包，其中1630个实际Cook、7个按平台跳过。Editor Cook 启动产生2条错误，最终 AutomationTool 返回 `ExitCode=25 (Error_UnknownCookFailure)`，不能把编译或包处理完成计为打包成功。原始证据 `.agent-local/qa/TASK-103/package-20261007-1/package.log`：第257行 `LogGameFeatures`、第259行 `LoadErrors` 均指向缺少 GameFeatureData 类型规则；末尾保留相同错误和退出码。

本机插件描述确认依赖链为 `AllToolsets → GameFeaturesToolset → GameFeatures`。`AllToolsets` 聚合并启用 `GameFeaturesToolset`；后者为Editor工具集，直接启用GameFeatures；GameFeatures含Runtime/PreDefault模块。`ToolsetRegistry.AgentSkillToolset` 注册日志位于错误之前，但ToolsetRegistry.uplugin未声明GameFeatures依赖，不能将相邻日志当依赖来源。当前Shipping receipt也包含GameFeatures构建插件及其运行依赖。

本机UE5.8官方源码要求类型规则可用：

- `Engine/Plugins/Runtime/GameFeatures/Source/GameFeatures/Private/GameFeaturesSubsystem.cpp:780–785` 对GameFeatureData的 `GetPrimaryAssetRules` 执行 `IsDefault()` 校验，输出第一条错误。
- `Engine/Plugins/Runtime/GameFeatures/Source/GameFeaturesEditor/Private/GameFeaturesEditorModule.cpp:426–433` 执行同一校验并通过LoadErrors提示添加规则，输出第二条错误。
- 同文件 `AddDefaultGameDataRule` 第400–412行的官方编辑器修复只新增类型/基类、两个false标志及 `CookRule=AlwaysCook`，没有添加扫描目录。
- `Engine/Source/Runtime/Engine/Private/AssetManagerTypes.cpp:104` 明确允许类型没有扫描目录；`AssetManager.cpp:1308–1322` 先创建TypeData，即空Paths也登记类型，第3887–3889行随后设置类型Rules。
- `UAssetManagerSettings` 使用 `Config=Game`，规则写入 `Config/DefaultGame.ini`。首轮受测DefaultGame/DefaultEngine未配置GameFeatureData规则；不需要扩大扫描目录或新增Feature资产。

对应最小必要配置为：

```ini
[/Script/Engine.AssetManagerSettings]
+PrimaryAssetTypesToScan=(PrimaryAssetType="GameFeatureData",AssetBaseClass="/Script/GameFeatures.GameFeatureData",bHasBlueprintClasses=False,bIsEditorOnly=False,Directories=(),SpecificAssets=(),Rules=(Priority=-1,ChunkId=-1,bApplyRecursively=True,CookRule=AlwaysCook))
```

该条与原生编辑器修复一致，其余Rules值为构造默认；Directories/SpecificAssets保持空。配置追加及公开UEClient完整Cook重试由root统一处理，后续重试成功；最终候选5的真实 `build.package` 也已返回 `ok=true`、`dry_run=false`、`returncode=0`，证据为 `.agent-local/qa/TASK-103/package-20261007-5/result.json`。首轮失败证据保留；未使用IgnoreCookErrors、禁用原有插件或扩大扫描目录。MCP连接成功与完整游戏Package成功分别统计。

## Vulkan 矩阵启动 Smoke 日志

`.agent-local/qa/TASK-087/full-vulkan-20261007-01/vulkan-runtime.log:1762–1774` 保留13条frame 0的 `LogAutomationTest: Error: Condition failed`，均在引擎初始化前。UE5.8 `LaunchEngineLoop.cpp:4376` 调用启动 `RunSmokeTests`；该路径仅运行SmokeFilter、关闭栈采集，默认Warning级别也未记录逐项测试名。

MCP及AutomationTestToolset源码未检出同一CHECK宏或SmokeFilter，当前无直接证据将这13条归因于MCP配置。日志显示zh-Hans语言及UnifiedError测试的中文文本，而Core测试含硬编码英文断言；二者关联仅为推断，不能确认全部13条的具体来源。后续引擎初始化与真实请求已正常继续（root实测）。原始错误不删除、不屏蔽，本轮不扩大调查或调整代码/配置。

## 当前会话限制

最新检查时 HTTP 服务在线，正常 Editor PID `78676` 保持运行。原PID3184按用户一次性授权核对路径与启动时间后正常关闭；当前常驻启动器保留UEClient所有者，后续可通过其stop_editor结束。[本次握手证据](managed-resident-connection.json)包含MCP、Bootstrap关卡、RC/Python及Codex配置验证。服务随该Editor进程关闭而离线，配置登记仍保留。

正在执行的 Codex 聊天工具表没有 Unreal MCP 工具，也未提供重连操作；本次未重启 Codex、未尝试工具热挂载。用户下一步是在 Hearthward 项目中新建聊天，重新读取项目配置后确认 `unreal-mcp` 工具出现。新聊天的工具挂载尚未由本次检查验证，不能将配置启用或 HTTP 实连计为原聊天已挂载。

## 参考

- [Epic：Unreal MCP in Unreal Editor](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-mcp-in-unreal-editor?application_version=5.8)
- [Epic：UAssetManagerSettings（Config=Game 与 PrimaryAssetTypesToScan）](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UAssetManagerSettings)
- [Epic：Game Features and Modular Gameplay](https://dev.epicgames.com/documentation/unreal-engine/game-features-and-modular-gameplay-in-unreal-engine)
- [OpenAI：Model Context Protocol](https://learn.chatgpt.com/docs/extend/mcp?surface=cli)

本机插件源码的设置类使用 `config=EditorPerProjectUserSettings`，因此自动启动配置写入 `DefaultEditorPerProjectUserSettings.ini`。
