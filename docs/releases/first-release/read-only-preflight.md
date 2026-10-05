# TASK-074 首版发行前实物审计

2026-10-04。Owner / Reviewer：XLingyyy；无 Issue。实际 074 对应原稿 076。批准依据为 `docs/planning/TASK-053-074/TASK-074.md`，工程读取目标为 `G:/GameFactory/Hearthward/.agent-local/task051` 的当前工作文件；子树仅改批准的打包脚本、一个 Cook 配置项和本单证据。

**状态：发行准备未完成，未冻结 RC。** TASK-073 尚未验收，当前施工尚未提交；没有唯一 RC_SHA、当前正式包或可发布结论。未执行 UE、build、cook、package、安装器、游戏启动、模型下载/复制或 Git 写操作；未重新计算任何 checksum。模型权重仅检查路径和 stat，未读取内容。历史 20260924 包只作旧部署实物记录。

## 1. 证据层级和当前结果

| 范围 | 本轮确认 | 尚未确认 |
|---|---|---|
| 正常入口 | DefaultEngine、MapsToCook、实际新游戏调用均指向 Bootstrap → 正式 Wilds | 冻结版本 Shipping 正常启动、菜单、新游戏和继续 |
| Animal MotionR3 | 14 mesh、14 skeleton、303 clip 路径对应实物均齐；已补精确目录显式 Cook 配置 | 真实 cooker 是否生成全部包及 Shipping 的全部实际 LoadObject |
| 055 | 24 Motion 候选包、2 Physics 包存在；正式 Mesh 绑定 Physics 的作者脚本明确 | 最终动作绑定与对应许可；最终保存的 Mesh → Physics Cook 依赖 |
| 模型 | 原目录真实 bundle 齐；Shipping 所用工作树 bundle 缺 GGUF 和两后端 server | 最终包自带模型与 DLL，干净机器离线 model-ready |
| 脚本 | 隔离树旧 --help 导入真实 RED；局部修复后 --help、AST 和受限导入探针通过 | 实际 UAT/归档/安装器结果 |
| 存档 | 本机引擎 Shipping 默认 LocalAppData 路由；备份、原子替换和安装器保留规则均有代码 | 冻结 RC 的普通用户、中文空格路径、升级、卸载实测 |
| 发行许可 | 模型、llama、OpenMP、两字体的来源与随包规则可核对 | Sketchfab 正式岩石、当前人物/055 动作、最终 070 资产的完整发行来源核准 |

静态覆盖检查表示配置已声明相应包。磁盘存在、旧包成功、当前 Editor 用例成功均不授予当前 Shipping/Cook 信用。[Epic 打包文档](https://dev.epicgames.com/documentation/en-us/unreal-engine/packaging-your-project)说明 build、cook、stage 的独立作用及未引用资产可能被裁去的规则。

## 2. 正式入口与已有 Cook 覆盖

`Config/DefaultEngine.ini:5-7` 的 EditorStartupMap / GameDefaultMap 均为 `/Game/Hearthward/Bootstrap/L_Bootstrap`，GameMode 为原生 HearthwardGameMode。`HearthwardScreenWidget.cpp:69` 先开 title；`HearthwardScreenActions.cpp:321-332` 的正常 new 操作进入 `/Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds` 并携带 `HearthwardNewGame=1`。到图后 `HearthwardScreenWidget.cpp:76-79` 调用 EnableNaturalWorld / StartNewProgress；新战役由 `HearthwardSaveSubsystem.cpp:279` 的 Campaign.Initialize 初始化。调试动物地图和灰盒未配置为默认入口。

两项 MapsToCook 都有真实 .umap：Bootstrap 36,831 bytes；正式 Wilds 13,476 bytes。地图包体积不表示整个场景依赖体积。当前设定为 Win64 Shipping、Pak + IoStore、压缩、包含 prerequisites、不包含 debug files、ZenStore 关闭。`package_demo.py` 的两项 maps 参数与配置一致。

以下为修复前 DefaultGame 已有的全部 DirectoriesToAlwaysCook，数量是当前 root 磁盘包数量；均无 .umap。

| 显式目录 | 实物 .uasset 数 | 实际覆盖 |
|---|---:|---|
| `/Game/Characters/Hero/AnimationV2` | 41 | 当前 HeroAnimInstance 的八项旧 A_Hero_* 构造加载及该目录其他既有派生 |
| `/Game/Characters/Brother/Animation` | 17 | 当前 BrotherAnimInstance 六项旧 A_Brother_* 构造加载及既有派生 |
| `/Game/Hearthward/Assets/Demo` | 24 | 床、篝火、木箱、木桌和贴图/材质；gameplay.json 有该目录八个包字符串 |
| `/Game/Hearthward/Assets/TASK-028` | 77 | 已有设施、房屋、道具、篝火依赖与装备斧等 |
| `/Game/Hearthward/Nature` | 14 | NatureActor 当前静态动物外形加载 |
| `/Game/Hearthward/Campaign` | 34 | Guard / Heavy Mesh 与 Idle/Walk/Run/Attack 等动态字符串加载 |
| `/Game/Hearthward/Assets/NaturalWorld/Rebuild/Meshes` | 33 | 重建正式地形的 Mesh；材质/纹理仍依靠这些 Mesh 与地图的依赖 |

`bCookAll` 未开启，未配置全 Content 兜底。两张正式地图和上述选定目录之外的调试地图未被主动加入。本轮没有实际 cooker 清单，不能断言无调试/测试资产经间接依赖进入包；最终查生成清单及正常菜单即可，避免扩大到全部 Content。

## 3. MotionR3 唯一确定的显式覆盖缺口及已做局部修复

`Resources/Data/animal_motion.json` 是 NonUFS 外部 JSON，含 14 个 mesh + 14 个 skeleton + 303 个 clip，共 331 个包路径，均位于 `/Game/Hearthward/Animals/MotionR3`。该目录当前 471 个 .uasset，0 .umap；JSON 所列 331 个路径缺失 0。`HearthwardAnimalMotionComponent.cpp:29,47-54` 读取该 JSON，在 Configure 中实际 LoadObject 14 个 Mesh 和 303 个 Clip，并检查配对 Skeleton。普通文件里的路径字符串不能代替引擎资产引用；JSON 本身被 stage 也不会自动表达全部动画 Cook 依赖。

修复前的原有显式目录规则中，331 个 JSON 路径的**显式目录覆盖为 0**。这证实配置缺项，真实 cooked package 缺失尚未运行。批准后的唯一 Config 增量：

```ini
+DirectoriesToAlwaysCook=(Path="/Game/Hearthward/Animals/MotionR3")
```

修复后静态显式覆盖 331 / 331，物理文件缺失 0。旧目录与两项 MapsToCook 保持原值；未开启 CookAll，未把 Animal Demo 地图加入。该配置采用现有引擎功能，[Epic 的 DirectoriesToAlwaysCook 定义](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Developer/DeveloperToolSettings/UProjectPackagingSettings)允许对选定目录声明强制 Cook。

055 的 24 个 `/Game/Hearthward/Assets/TASK-055/Motion/{Hero,Brother}/knight61_*` 是候选片段。读取当时，正式两个 AnimInstance 仍绑定 AnimationV2 / Animation；不能把未绑定候选 Cook 入包并声称新动作已交付。本次配置没有加入 055 Motion。最终 055 Source 绑定通过后，再按准确运行引用登记其 Cook 规则。

两项 Physics 的精确包为 `/Game/Hearthward/Assets/TASK-055/Physics/PA_HeroCombat` 和 `PA_BrotherCombat`。`docs/qa/TASK-055/create_physics.py:35-41` 把它们设为正式 `/Game/Characters/{Hero,Brother}/UE5/SK_*` 的 physics_asset，并保存两边。Hero/Brother 正式 Mesh 是角色构造中的对象加载，Physics 的预期路径是这些 Mesh 的硬依赖。当前目录存在与作者脚本不替代最终包依赖读取；最终 Cook 清单核对这两个 PA 与真正保存的正式 Mesh，不盲加整个 055 目录。

## 4. 打包脚本的真实 RED、最小修复与有限验证

旧 `package_demo.py` 把 `Path(__file__).resolve().parents[3]` 加入 sys.path。原目录下它是 `G:/GameFactory`，在 task051 下是 `G:/GameFactory/Hearthward/.agent-local`。本轮实际执行：

```powershell
& 'G:/GameFactory/.venv/Scripts/python.exe' -X utf8 'G:/GameFactory/Hearthward/.agent-local/task051/scripts/release/package_demo.py' --help
```

exit 1；第 6 行 `from engine_adapters.ue5 import UEClient` 抛 ModuleNotFoundError。失败发生于目录创建和 UEClient 构造前。旧脚本也没有 argparse；若导入成功，`--help` 会被当作归档路径并进入打包。

批准后的局部修复仅在此文件：复用 `scripts/animals/paths.py` 已有的 `HEARTHWARD_FACTORY_ROOT` 环境名与 `engine_adapters/ue5/__init__.py` marker 校验，候选为环境配置优先、然后工程祖先；没有新增 helper、依赖、Git 调用或改变动物工具。没有调用会设置 AAAGF_OUTPUT_ROOT 的 configure_factory。内置 argparse 接受可省略 output 位置参数，默认仍为 `F:/HearthwardDemo/20260924`；--help 在目录创建、引擎构造前退出。实际打包仍调用公开 `ue.build.package`，未改 maps / timeout / extra_args。

本轮验证结果：

- 子树脚本 --help：exit 0，显示 `usage: package_demo.py [-h] [output]`。
- Python AST：通过，没有执行包操作。
- 仅执行 AST 中 output.mkdir 之前的实际语句：子树实际导入 `engine_adapters.ue5.ue_client.UEClient`，factory 为 `G:/GameFactory`，project 为自身子树，output 保留旧默认。
- 同一受限实际语句探针将 __file__ 指向原目录位置：factory 同为 `G:/GameFactory`、project 为 `G:/GameFactory/Hearthward`、默认 output 相同。该项验证原根解析/导入兼容，没有执行原根打包脚本全程。
- 当前目录 Cook 静态核对：331 / 331，MapsToCook 原值，未加入 055 候选。

增量补丁为 `docs/qa/TASK-074/release-entry-cook.patch`，仅两个批准文件。--help / import / AST / JSON 路径覆盖的成功不表示 build/package 已通过。

## 5. 本地模型、DLL 和许可部署实物

Shipping 默认从 `FPaths::ProjectDir()/Runtime/LocalAI` 读取 bundle。`HearthwardLocalAIRuntime.cpp:32-37` 的 HearthwardAIBundlePath override 仅非 Shipping 有效。`Hearthward.Build.cs:27-34` 对非 Editor 目标要求本地目标 GGUF 和 CPU/Vulkan server；它们为 NonUFS 随包依赖。

| 实物位置 | GGUF | CPU / Vulkan server | 判断 |
|---|---|---|---|
| 当前 task051 Runtime/LocalAI | 缺 | 均缺 | 仅 knowledge / 两项许可证 / README，Shipping 部署准备尚未完成 |
| 原 `G:/GameFactory/Hearthward/Runtime/LocalAI` | 存在，2,740,937,888 bytes | 两个均存在，各 9,216 bytes | 既有真实 bundle 可供后续同版本工作树准备；本轮未复制、下载或重算 hash |

server 9KB 是启动包装器，其实际代码在 llama-server-impl.dll。本轮对原 bundle 的 PE 静态导入表读取，CPU/Vulkan 各有完整八文件本地静态闭包：llama-server.exe、llama-server-impl.dll、llama-common.dll、llama.dll、ggml.dll、ggml-base.dll、libomp.dll、mtmd.dll。每后端另有 14 个按 CPU 能力动态选择的 ggml-cpu-*.dll；Vulkan 后端有 ggml-vulkan.dll。静态闭包不覆盖全部动态加载，因此不能只复制八项。两后端的 LICENSE-LLVM-OpenMP 都存在。外部导入为 Windows 系统和 MSVC/UCRT 库，需要游戏 prerequisites 与玩家对应 Vulkan 驱动；没有加载/执行这些 DLL。

Build.cs 当前排除 .downloads、.part、额外 .gguf 和除 server 之外的 .exe；模型固定只部署 Qwen3.5-4B-Q4_K_M。它同时递归带入全部其他文件。已有原 bundle 中有七个工具 impl DLL/后端：batched-bench、bench、cli、completion、fit-params、perplexity、quantize，共 4,262,400 bytes/后端。这些不在本轮 server 静态闭包中，并在旧包 manifest 中实际被带入；后续可精准排除七个已知工具 DLL，保留已证实的 server/CPU 动态后端/Vulkan/许可证，随后用最终干净包的实际 model-ready 验证。当前未改 Build.cs，未完成动态依赖剔除验收。

模型来源锁为 Qwen/Qwen3.5-4B → unsloth/Qwen3.5-4B-GGUF 的已登记 revision e87f176479d0855a907a41277aca2f8ee7a09523；[该 revision 的模型卡](https://huggingface.co/unsloth/Qwen3.5-4B-GGUF/raw/e87f176479d0855a907a41277aca2f8ee7a09523/README.md)与[Qwen 官方卡](https://huggingface.co/Qwen/Qwen3.5-4B)标记 Apache-2.0。Runtime/LICENSE-Qwen.txt 有许可全文和 Alibaba Cloud 通知；llama.cpp b10964 有[官方 MIT 许可](https://github.com/ggml-org/llama.cpp/blob/b10964/LICENSE)和 repo 的 LICENSE-llama.cpp.txt；OpenMP 通知随两后端部署。

模型安全/预算原实现保持：loopback、临时生成进程 API key、单并发、服务器 context 4096、策略 max_input_tokens 3328、max_output 256、thinking off、确认后真实执行。4096 和 3328 分别是服务器上下文和应用输入预算，不能混成一个配置缺陷。根已报告当前压缩版 CPU/Vulkan C01 各 raw/e2e/exec 1/1、generation_calls=1、3075 full input；计时含 cold。此处仅引用根的一样本结果，不授予 60 矩阵、暖性能、第二机器或 Shipping credit。无需玩家外部 API key。

## 6. Stage 排除与开发工具的具体范围

资源递归规则当前会 stage 45 个 Resources 文件，包括 PNG、运行 JSON、两字体和两项 OFL，也包括 `Resources/UI/items-clean.prompt.txt`、`Resources/UI/LAYOUT.md`、`Resources/UI/art-provenance.json`、`Resources/Audio/README.md`。旧 20260924 的真实 NonUFS manifest 中已出现前两项。可在批准后的 Build.cs 窗口精准排除开发提示词/布局/来源记录；保留 UI runtime layout.json、Data JSON、字体与许可证。音频说明属于交付说明候选，不能用移除说明掩盖 UNPRODUCED 状态。

现有依赖规则没有把 AGENTS、GDD、art_source、Source、scripts、venv、test_data 或正常玩家 Saved 作为 runtime dependencies。安装器只消费归档 PackageRoot，排除 .pdb、Manifest_*.txt 和任何 Saved 子树。本轮对旧包三项 runtime 树的命名检查未发现这些禁止目录，但当前 RC stage 尚未产生；最终必须核对**实际新包**清单，不把旧清单移植为本单成功证据。

PythonScriptPluginPreload 的本机 Build.cs 在非 Editor 设置 WITH_PYTHON=0；Python 主模块为 UncookedOnly。RemoteControl 则包含 Runtime 模块；本机 WebRemoteControl.cpp:230-231 明确 packaged 默认不开 WebControl，命令行 RCWebControlEnable 能显式开启。Hearthward.Target.cs 没有 Shipping 插件排除规则。A3GamePlayable 是 Runtime 模块且 recorder 通过 A3PlaytestOutput 命令行启用。当前没有默认启动远控的实证，也不能称这些开发能力已经从发行二进制移除。若 074 的开发工具排除要求包含编译能力，应在根登记 Shipping target/plugin 的准确窗口并核对游戏模块实际依赖，再定向移除；本轮未改插件或 Target。

DefaultEngine 仍有未启用 Shipping 的 AndroidFileServer SecurityToken 配置；本报告不输出其值。Win64 当前无该服务 Shipping 启用配置。最终只检查目标平台实际 staged 配置是否包含该无用字段，若仍入包再精确排除；未把它宣称为运行中的玩家 API 凭据。持久个人凭据和玩家私档没有进行全文导出，本轮不能给当前不存在的 RC 出具“无密钥/私档”验收。

## 7. 安装、路径与存档保留

`demo.iss` 使用固定 AppId Hearthward-Demo，默认安装到 `{localappdata}/Programs/HearthwardDemo`、PrivilegesRequired=lowest，支持 x64。默认图标启动 Vulkan / GpuLayers32，另有 CPU 图标；裸 exe 按 DefaultGame 默认 CPU / 16 层配置，CPU 实际传 ngl 0。缺 MSVC runtime 时 installer 的 redist Run 项使用 runas，可能要求管理员批准；不能宣传所有机器绝无 UAC。[Inno 的 lowest 文档](https://jrsoftware.org/ishelp/topic_setup_privilegesrequired.htm)仅定义 Setup 自身权限模式。

安装器 File 源、目标、图标文件名使用 Inno 的带引号字段。模型启动将模型路径放在双引号内，并把 Exe / 工作目录分别传给 CreateProc；保存使用 Unicode FString 和 MoveFileExW。公开 build.package 用 subprocess 参数列表传 project/archive/staging 路径。以上是支持中文/空格的实现依据；未进行中文账户、含空格安装路径或非管理员真实安装。本轮 argparse/Path 也未把最终 UAT 路径验收提前标为通过。

本机 UE 源明确：App.cpp:218-219 的 Desktop Shipping 默认 IsInstalled=true；Paths.cpp:183-189 → ProjectUserDir:460-466 使用 UserSettingsDir / ProjectName；WindowsPlatformProcess.cpp:1466-1472 获取 LocalAppData；GameSavedDir 为该用户路径再加 Saved。正常 Win64 Shipping 的存档预期路径：

`%LOCALAPPDATA%/Hearthward/Saved/SaveGames/HearthwardPrototype/Compatible-v8/pool.hws`

不需要把进度写入安装目录。非 Shipping HearthwardSaveTestPool 不进入正常 Shipping；开发 QA 的 -UserDir profile 是显式独立定位，不能作为正常玩家目录验收。

旧 `SaveGames/HearthwardPrototype/pool.hws` 在 Compatible-v8 目标不存在时只读导入，CommitPool 先给旧档创建 sibling Backups，再写兼容目录。Write 先产生 .pending、读回验证；格式/WriterVersion 更新前保留备份及已有迁移副本，最后 MoveFileExW 原子替换。WriterVersion 更高时拒绝覆盖。Compatible-v8 目录名当前仍沿用，内存 Schema 已到 9；本轮不改保存格式或目录。

Inno 没有 InstallDelete / UninstallDelete / 自定义清理存档动作，且排除 Saved。固定 AppId 和相同安装位置支持覆盖升级，用户 LocalAppData 存档未在安装文件删除表中。最终必须在 072 的第二机器，用冻结 RC 做普通用户 clean install → model-ready → 正常新游戏 → 保存退出 → 继续；代表旧档升级需保留旧原件/备份，卸载后明确验证该进度仍存在。这些项目本轮全部 NOT_RUN。

## 8. 发行许可的具体已知与缺项

| 实际来源 / 使用点 | 已有证据 | 尚待收口 |
|---|---|---|
| Qwen GGUF / llama / OpenMP | 锁定来源、三类许可及部署规则；原 bundle 两后端 OpenMP 文件实际存在 | 最终新包中完整通知与对应实际二进制，不重做已确认的 hash |
| NotoSerifCJKsc-Regular.otf | 官方[Serif OFL](https://github.com/notofonts/noto-cjk/blob/main/Serif/LICENSE)，repo OFL；仅读取 font name table，内嵌 Adobe 2017–2024 copyright、OFL、版本2.003 | 当前完整字体与通知随新包；没有删除内嵌 copyright 的修改 |
| LXGWWenKai-Regular.ttf | [官方 OFL](https://github.com/lxgw/LxgwWenKai/blob/main/OFL.txt)，repo LXGW-OFL 含作者/保留名；font name table 为 LXGW/Klee、版本1.522 | 最终未修改字体与通知完整；当前未换/子集化字体 |
| 正式 Rebuild 的 Poly Haven 植物/地表/rock_3 | TASK026 台账映射具体 asset 官方页；[Poly Haven 官方 CC0](https://polyhaven.com/license)；程序地形/水/草卡有派生记录 | 保持最终 asset → source 对应；TEMP_VISUAL 外观由 070 Owner 样图审阅收口 |
| **正式 `Rebuild/Meshes/SM_Rock`** | TASK026 ASSETS_REBUILD.md 明确来自 TASK004 Sketchfab 中岩石 `source/stone assets.obj` + `d_m.jpg` | **原页面 URL/UID、作者、逐资产许可证仍缺。**004 SOURCE 只记 Owner 允许收入共享仓库；发行资格未独立核准。需恢复证据或由已许可来源替换这一实际运行 Mesh，随后复验受影响地形路径 |
| TASK004 Tripo 动物 / Guard / Heavy / 房屋 / 设施 / 道具 / 篝火 | 各 SOURCE 记录 Owner 付费生成、输入输出身份和同步授权；051 R3 派生沿此源链 | 最终所用源与已批准发行权对应；Owner 输入图权利与付费授权不由导入操作补足 |
| **当前 61-bone Hero/Brother 和 055 knight61 动作** | 当前人物导出来自已有用户资产；055 记录 `F:/Download/medieval+knight+3d+model (2).zip`，转换 ID `2e499319-8746-4ade-b2b7-972d92793aca` | **knight61_motion_source.json 的 license_status 明确 not verified。**需确认具体 ZIP / 生成账号授权 / 输入来源 / 动作来源。旧027另一骨架的 provenance 不覆盖当前61骨架 |
| UE Mannequin 旧动作派生 | TASK027 原始包来自本机引擎 TemplateResources，台账明确属于 Epic 提供内容 | 按最终使用路径记录引擎内容规则，不当作可独立出售源动作许可 |
| 070 尚未消费的六武器/四护具候选 | QA070 对应源 UUID/生成记录；当前无这些具体 FBX 的正式 Content 导入 | 使用前补对应 source/input/account 授权；当前不能列为已打包资产，不能凭候选数量扩大 RC 清单 |
| UI PNG 和 Owner 参考图 | art-provenance 有 image_gen artifact ID 和用户参考图说明 | 最终生成图片与引用来源关联；参考图使用权需 Owner 确认，生成操作不替参考输入授权 |
| 人声 | Resources/Audio/README 明确54逻辑cue、28audio_group全部UNPRODUCED，当前无文件 | 用户已暂缓真人录音；不制作静音/TTS替身。若首发保持字幕，正式范围须获 Owner 明确接受，交付说明如实记录 |

没有把 art_source 或完整生成工具打包作为许可解决办法。发行只带获得核准的游戏派生资产和必须通知；来源记录可存发行档案。070 正在施工前准备，以上缺项需要与其最终消费资产一起收口。

## 9. 最小剩余行动及 RC 门槛

1. 根集成本次两个文件补丁，保留真实脚本 RED 与有限 GREEN。真实 MotionR3 Cook 清单、14/303 加载由根在适当打包窗口执行；055 Motion 仅在最终已验收绑定后加入具体 Cook 引用。
2. 使用原已验证 bundle 做工作树发行部署准备，保留 CPU/Vulkan 实际 DLL、动态后端和完整许可；不换模型，不下载，不重复 hash。Shipping 验证必须不带非 Shipping AIBundlePath override。
3. 优先补当前运行的 SM_Rock、当前人物/055 动作和最终070资产许可。随后仅按实际 stage 清单收窄开发 prompt/layout/tool DLL，必要插件排除另由根登记准确窗口。
4. 将 scripts/release/README-DEMO.txt、installer 版本和运行 Current 收口到真实首发范围。当前说明仍为 Demo0.1.0、自然营地起步、无正式敌人/完整故事，并宣称 DX11/12；正式 Rebuild 文档明确 Nanite 依赖 DX12/SM6。当前说明与实现不符。DefaultPackageRoot / OutputRoot / AppVersion 也仍是旧包路径/preview版本；只能在实际 RC 范围冻结后更新。package_demo 当前不自动复制交付说明、生成 BUILD-INFO 或完整许可 manifest，历史这几项是另外准备的，后续必须实际放入新 archive 再编译安装器。
5. TASK073 完成且 Owner 接受后，根才冻结唯一源提交、UE/MSVC/Windows、固定模型/runtime、实际 runtime tables 与 cooked assets；在 072 第二机器完成上述最小 release-diff 验证，保留结果与限制。发布许可和暂缓录音范围未收口时不能宣称 Ready。

历史 `F:/HearthwardDemo/20260924/Windows/BUILD-INFO.json` 的 source 为 8b54550、版本0.1.0-demo.20260924 / UE5.8.2；该旧包 NonUFS manifest 有完整模型/两后端/字体许可，也有上述开发文件/工具 DLL。旧实物与旧安装结果可以解释现有流程，无法代表本批新内容。根完成正式验收后，提交推送、上传 Release、公开宣布仍按本批任务授权执行，当前均未授权。
