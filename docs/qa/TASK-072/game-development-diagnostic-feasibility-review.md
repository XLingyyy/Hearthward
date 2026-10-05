# TASK-072 独立Game Development工程诊断路径

2026-10-05。仅只读本机public adapter、UBT/UAT、Target/Rules、原有产物metadata与Root本次package.log；子代理不启动UE/build/Cook、不创建硬链接/junction、不移动删除Intermediate、不改Source/参数/Git。

**当前已具备现有公共构建和启动路径，可作为TASK-072可逆工程诊断。** 正式设计明确包含正常打包、联合性能和瓶颈定位；Epic也允许生产/测试期间打包。TASK-074正式RC冻结、最终许可证/真人验收/发布门槛仍待独立收口，不禁止这次独立输出的本机Development诊断。该诊断产物不能宣称正式RC、Shipping性能或Ready。[TASK-072](G:/GameFactory/Hearthward/.agent-local/task051/docs/planning/TASK-053-074/TASK-072.md)、[Epic打包阶段说明](https://dev.epicgames.com/documentation/en-us/unreal-engine/packaging-your-project)。

## 最小公共调用

`build.package`有configuration但没有target keyword；通过extra_args明确`-target=Hearthward`。Target本来就是Game。以下是已核实API形状，变量目录由Root本次登记，不在子代理执行：

```python
ue.build.package(
    configuration="Development",
    archive_dir=diagnostic_root / "Archive",
    maps=(
        "/Game/Hearthward/Bootstrap/L_Bootstrap",
        "/Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds",
    ),
    extra_args=(
        "-target=Hearthward",
        "-CookOutputDir=" + str(diagnostic_root / "Cooked/Windows"),
        "-stagingdirectory=" + str(diagnostic_root / "Staged"),
        "-AdditionalCookerOptions=-ShaderWorkingDir=" + str(diagnostic_root / "Shaders"),
        "-nodebuginfo",
    ),
    log_path=diagnostic_root / "package.log",
    timeout=10800,
)
```

参见[package API](G:/GameFactory/engine_adapters/ue5/build/client.py:190)。直接给target keyword会TypeError。原package_demo保持默认Shipping/既有maps，当前诊断无需改它或Adapter/Target。

成功后复用[runtime.launch_packaged](G:/GameFactory/engine_adapters/ue5/runtime/client.py:154)：

```python
launch = ue.runtime.launch_packaged(
    executable=actual_game_executable,
    extra_args=existing_owned_smoke_arguments,
)
```

actual_game_executable取本次真实stage/receipt中的Game binary，选择`Hearthward/Binaries/Win64`中真实Game进程，避开先启动子进程的顶层bootstrap exe。此API不自动添加Editor/.uproject/RC/input参数；复用现有UUID、UserDir、RC最小ownedini、Bootstrap正常new、actor公共引用检查、同原话与backend参数。先验证相同public reference smoke，再做唯一实际CPU请求/CSV；若引用或正常场景前置失败就留FAIL，不定位actor/授予物品。

现有host只需加一个真实Game exe可选启动分支，保留其launch/poll/正常quit/finally stop；不用复制runner或另造服务。stop_editor能停止同一UEClient持有的packaged PID，但函数仅terminate而未wait，Root仍核实实际PID退出。Root已实际确认CPU日志诊断UE30100/model46992均退出；此事实不能移植给未来Game运行。

## 当前真实构建状态与前置

`Hearthward.Build.cs`对所有非Editor target要求工作树project-relative GGUF和两后端server。先前缺失是实际Rules前置；Development runtime的bundle override不能绕过它。Root现已登记Runtime/models+bin并将64个既有准备bundle文件硬链接到本树，保持Build.cs gate。仅对同G NTFS现有文件创建新目录项不复制GGUF数据块；硬链接共享同一文件，不能把其内容或attributes当成独立可修改副本。本稿不写链接或model bytes、不算hash。

Root现公共package session37219已启动，实际日志[F package.log](F:/uagent-task-temp/task072-game-development-20261005/package.log:4)记录`-clientconfig=Development`、`-target=Hearthward`及以下F独立输出。当前读到Game UHT完成162文件，Editor+Game联合compile正在进行，没有实际Cook/Stage完成或Disk失败信用。

- archive：`F:/uagent-task-temp/task072-game-development-20261005/Archive`
- cook：`F:/uagent-task-temp/task072-game-development-20261005/Cooked/Windows`
- stage：`F:/uagent-task-temp/task072-game-development-20261005/Staged`
- shader working：`F:/uagent-task-temp/task072-game-development-20261005/Shaders`

当次工具链实际打印MSVC14.44.35228与WindowsSDK10.0.22621.0。日志不是目录推断；CookOutputDir随后是否真正生效仍待实际cook invocation/产物。

## 内存诊断与Shipping界限

Game Development不包含Editor target运行组件，适合验证Editor -game约4–4.8GiB工作集对内存余量的影响；是否降低多少及CPU prefill是否改善必须由同场景真实运行确认。不能先给移除Editor开销授予性能PASS。[Epic Build Configurations](https://dev.epicgames.com/documentation/en-us/unreal-engine/build-configurations-reference-for-unreal-engine)区分独立Game与Editor target，Game要求平台cooked content。

本机[CsvProfilerConfig.h](G:/UnrealEngine/UE_5.8/Engine/Source/Runtime/Core/Public/ProfilingDebugging/CsvProfilerConfig.h:8)默认CSV_PROFILER_ENABLE_IN_SHIPPING=0、CSV_PROFILER_ALLOW_DEBUG_FEATURES=!UE_BUILD_SHIPPING；[Hearthward.Target.cs](G:/GameFactory/Hearthward/.agent-local/task051/Source/Hearthward.Target.cs)未覆盖。Development Game可继续用现有CSV/console入口；现有Shipping无法直接复用该采样方式。CLI参数不能打开已编译掉的profiler。为当前瓶颈无需改Target/引擎或新增Shipping测量能力；本机Development证据不代替最终Shipping/第二机器。

RemoteControl Runtime仍在项目plugin配置内；[WebRemoteControl.cpp230](G:/UnrealEngine/UE_5.8/Engine/Plugins/VirtualProduction/RemoteControl/Source/WebRemoteControl/Private/WebRemoteControl.cpp:230)明确packaged默认不开，显式RCWebControlEnable可开。不能由源码可行性授予Game实际HTTP/引用PASS。Shipping bundle override编译掉，未来Shipping须验证完整内置bundle；Development也需当前stage清单真实部署。

## G盘Intermediate/PDB风险

构建前只metadata统计Root现有Intermediate3,082,293,900B（2.870610GiB），最大既有Editor PCH2,574,909,440B；现有Binaries110,892,879B，其中Editor PDB104,361,984B。它们是已占用空间，不删除/迁移。原工程历史Development Game exe335,214,592B/PDB374,099,968B，旧20260924 Shipping exe168,135,680B/PDB234,803,200B；这些体积仅参考，不能准确预测当前Game新PCH/obj/PDB/Cook/DDC峰值。

Root实际Gfree已从4.538降至约2.433GiB；本子后续单次disk_usage读到2.407440GiB，F84.167591GiB。这是本次真实增长和余量风险，没有Disk失败。F Cook/Stage/Archive不能承诺G零写入；项目/插件/引擎Intermediate和Game Binaries仍有本地写入。

已核UBT中ProjectIntermediate由uproject目录和固定Intermediate/Build/平台/架构/target/config组合，本次未找到干净public Intermediate-root flag；IntermediateEnvironment只改target子目录后缀。[UEBuildTarget](G:/UnrealEngine/UE_5.8/Engine/Source/Programs/UnrealBuildTool/Configuration/UEBuildTarget.cs:1929)。

- PdbAltPath仅链接器/PDBALTPATH，修改二进制记录的debug路径，不能移动实际PDB。[Microsoft定义](https://learn.microsoft.com/en-us/cpp/build/reference/pdbaltpath-use-alternate-pdb-path?view=msvc-170)。
- LinkerArguments可传raw/PDB，但UBT ProducedItems/receipt仍登记原Game目录；并有installed/shared environment约束，不当作完整外置方案，不使用。
- nodebuginfo控制stage不带debug产物，不能当作编译不生成PDB。NoDebugInfo+NoLinkerDebugInfo可以改变debug产物生成，但它们不迁Intermediate，本次未更改已有构建。

Root当前构建已开始，本稿停止junction/迁移调查，不改运行中的Intermediate，不制造另一UE或并行cook。Root按阶段只读检查空间/首次错误；没有实际磁盘失败就保持“风险/进行中”。
