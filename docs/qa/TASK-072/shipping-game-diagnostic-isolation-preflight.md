# Shipping 工程构建与公开引用 smoke 的只读前置

此调查针对TASK-072工程诊断，尚未构建或运行Shipping，未创建junction、修改Target/Adapter、迁移或删除文件。074正式RC冻结与发行验收保持独立。

只将`Intermediate/Build/Win64/x64/UnrealGame/Shipping`指向F不能覆盖新SharedPCH。实际已完成Development的Game PCH为：

`G:/GameFactory/Hearthward/.agent-local/task051/Intermediate/Build/Win64/x64/Hearthward/Development/Engine/SharedPCH.Engine.Project.ValApi.ValExpApi.Cpp20.h.pch`，2153247296 bytes。

[UEBuildModuleCPP.cs](G:/UnrealEngine/UE_5.8/Engine/Source/Programs/UnrealBuildTool/Configuration/UEBuildModuleCPP.cs:1593)对`bUsePrecompiled`引擎模块将SharedPCH放在`Target.ProjectIntermediateDirectory/ModuleName`；[UEBuildTarget.cs](G:/UnrealEngine/UE_5.8/Engine/Source/Programs/UnrealBuildTool/Configuration/UEBuildTarget.cs:1945)用`TargetName`建立该目录，因此Hearthward目标需要独立Hearthward/Shipping目录。模板第1774行向同一OutputDir编译PCH。下列三个Shipping叶目录目前均不存在；应由Root核实并登记精确窗口后决定是否建立新空目录junction。

| 新增叶目录（Root绝对路径） | 覆盖范围 | 未覆盖范围 |
|---|---|---|
| `G:/GameFactory/Hearthward/.agent-local/task051/Intermediate/Build/Win64/x64/Hearthward/Shipping` | precompiled Engine SharedPCH、目标makefile/metadata | AppName目录的项目obj/link、项目plugin |
| `G:/GameFactory/Hearthward/.agent-local/task051/Intermediate/Build/Win64/x64/UnrealGame/Shipping` | Hearthward项目module obj及monolithic链接中间产物 | SharedPCH、plugin独立BaseOutputDirectory |
| `G:/GameFactory/Hearthward/.agent-local/task051/Plugins/A3GamePlayable/Intermediate/Build/Win64/x64/UnrealGame/Shipping` | 项目plugin新obj | plugin无架构UHT生成目录、主程序最终Binaries |

A3GamePlayable实际存在，Development obj为5072017 bytes，位于上述plugin路径的Development/A3GamePlayable。根目录junction不会覆盖plugin路径。[GetModuleIntermediateDirectory](G:/UnrealEngine/UE_5.8/Engine/Source/Programs/UnrealBuildTool/Configuration/UEBuildTarget.cs:5342)以各ModuleRules的BaseOutputDirectory分别构造AppName/config/module目录。引擎规则在本installed Engine默认使用precompiled，见[RulesCompiler.cs](G:/UnrealEngine/UE_5.8/Engine/Source/Programs/UnrealBuildTool/Configuration/Rules/RulesCompiler.cs:96)；既有引擎plugin库可复用，不能把其已安装文件算作新增工程输出。

仍有G盘增量：最终`Binaries/Win64/Hearthward-Win64-Shipping.exe`及链接PDB、receipt；无架构目标metadata与UHT生成目录；project plugin UHT输出在`Plugins/A3GamePlayable/Intermediate/Build/Win64/UnrealGame/Inc`。Development实际UHT manifest已经记录这些路径。`-nodebuginfo`控制Stage调试文件收集，不能保证不生成PDB。Root报告当前G余约1.28GiB，单项Development PCH已约2.01GiB，新增PCH需外置；源码不能给出这次Shipping全部新增字节上界，仍需Root按实际阶段监视空间。当前没有Shipping磁盘失败证据。

UBA存储有独立根目录，与三个junction无关。[UBAExecutor.cs](G:/UnrealEngine/UE_5.8/Engine/Source/Programs/UnrealBuildTool/Executors/UnrealBuildAccelerator/UBAExecutor.cs:257)优先使用Config.RootDir，其次`UBA_ROOT`/`BOX_ROOT`，Windows默认`C:/ProgramData/Epic/UnrealBuildAccelerator`。Root Development日志记录local executor与40GB存储容量配置，不等于消耗40GB。如需外置，可在Root自有构建host环境设置`UBA_ROOT`至已登记F诊断目录；它不会改变UBT编译输出目录，不能代替上述junction。未调查或修改现有UBA缓存。

Shipping存档隔离可以依靠唯一绝对`-UserDir=<owned empty path>`。[Paths.cpp](G:/UnrealEngine/UE_5.8/Engine/Source/Runtime/Core/Private/Misc/Paths.cpp:1940)解析UserDir无Shipping条件，绝对路径规范化后优先作为ProjectUserDir；ProjectSavedDir由它拼出Saved。[CommandLine.h](G:/UnrealEngine/UE_5.8/Engine/Source/Runtime/Core/Public/Misc/CommandLine.h:14)默认命令行allowlist关闭，当前Hearthward.Target.cs未开启allowlist。自然地图存档的全部当前/legacy pool路径均由ProjectSavedDir拼出。[SaveSubsystem.cpp](G:/GameFactory/Hearthward/.agent-local/task051/Source/Hearthward/Save/HearthwardSaveSubsystem.cpp:70)

`HearthwardSaveTestPool`在Shipping被第73–78行编译条件移除，不能用于Shipping隔离。Shipping实际文件为`<UserDir>/Saved/SaveGames/HearthwardPrototype/Compatible-v8/pool.hws`；legacy fallback也位于该UserDir。fixture EnablePrototype同样在Shipping拒绝，正常new所用EnableNaturalWorld不受该条件限制。公开smoke必须保持正常Bootstrap→new入口，先用现有公开GetProjectSavedDirectory验证返回路径位于本次唯一UserDir，再执行会建立存档的动作。此路径验证需要读取函数的exact allowlist；不读取受禁Settings CDO。

RC启动条件：`-RCWebControlEnable`在WebRemoteControl.cpp231及340–343支持非Editor，包括Shipping。`-ExecCmds=WebControl.StartServer ...`受[UnrealEngine.cpp](G:/UnrealEngine/UE_5.8/Engine/Source/Runtime/Engine/Private/UnrealEngine.cpp:2543)的Shipping编译条件移除，不能作为Shipping listener启动依据。已有RemoteControlSettings.h303默认`bAutoStartWebServer=true`，WebRemoteControl.cpp361–364按该设置启动；最窄QA session ini应显式写`bAutoStartWebServer=True`及本次独立端口，沿用精确函数规则、不改正式Config。纯引用smoke不需要开启远程console gate。仍需Root实际HTTP检查listener及返回对象，源码支持不等于Shipping运行通过。

Shipping默认`CSV_PROFILER_ENABLE_IN_SHIPPING=0`、debug features关闭，见CsvProfilerConfig.h9–16；当前Target无override。因此下一步可做工程构建与公开引用/正常new/隔离存档smoke，无法沿现有CSV命令给Shipping帧门槛信用。正常Quit与host finally仍需Root实际核对进程退出。未提供正式发行、Shipping性能、第二机器或全动作验收信用。
