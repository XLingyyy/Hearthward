# 内部Windows候选执行计划

当前计划candidate-3/version.2，独立输出`F:/HearthwardDemo/iteration-084-103-20261007-3`；NOT_FROZEN/NOT_BUILT。战斗成功回执/消费者和新声音映射仍在实施，等待root最终冻结、实际构建与OS核验。当前骨架为[CANDIDATE3_BUILD_INFO.json](CANDIDATE3_BUILD_INFO.json)，旧BUILD-INFO/CANDIDATE_BUILD_INFO原样保留candidate-2证据。以下首次/二次结果均属历史。

首次CandidateID `iteration-084-103-20261007-1` 已实际FIRST_COOK_FAIL/UAT25，详见 [失败记录](FIRST_COOK_FAIL.md)。第二次CandidateID `iteration-084-103-20261007-2`、独立F同名输出已实际BUILD_SUCCESS，公开result ok=true/ready/Exit0。第二次冻结产品版本为 `0.2.0-preview.20261007.1`。当前AI/audio源码已增至version.2，candidate-3计划中、尚未Shipping构建；旧第二次实测不迁移为最终同版PASS。root/MCP agent的GameFeatureData规则最小配置微修已纳入第二次冻结；旧包/第一输出/旧档保留，远端唯一UNKNOWN。

## 实际可用入口

公开入口来自 `G:/GameFactory/engine_adapters/ue5/build/client.py` 的 `UEBuildClient.package`，由 `UEClient.build` 暴露：

```python
from engine_adapters.ue5 import UEClient
ue = UEClient(
    project_path="G:/GameFactory/Hearthward/Hearthward.uproject",
    ue_root="G:/UnrealEngine/UE_5.8",
)
result = ue.build.package(
    archive_dir="F:/HearthwardDemo/iteration-084-103-20261007-3",
    configuration="Shipping",
    maps=("/Game/Hearthward/Bootstrap/L_Bootstrap",
          "/Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds"),
    extra_args=("-stagingdirectory=F:/HearthwardDemo/iteration-084-103-20261007-3/Staged",
                "-nodebuginfo"),
    log_path="G:/GameFactory/Hearthward/.agent-local/qa/TASK-103/package-20261007-3/package.log",
    timeout=10800,
)
```

以上公开签名已核对，路径现准备独立candidate-3；本子Agent未调用任何模式，当前路径不代表实际构建。以下第二次日志是历史candidate-2。第一次实际Shipping C++成功，Cook因GameFeatureData规则报2 errors/1 warning并UAT25中止。第二次log26编译成功、639 Cook 0 error/1 warning、654 Cook完成、903 Stage完成、907 Archive完成、908 BuildCookRun 124.78秒、911 Exit0。公开API使用Win64/Shipping/build/cook/stage/pak/iostore/archive/prereqs/utf8/unattended/NoLiveCoding。构建成功仅证明打包步骤，正常输入/模型/二机仍分验。没有IgnoreCookErrors或删旧插件，最小配置微修已由root/MCP agent执行。

现有 `scripts/release/package_demo.py <显式新目录>` 也调用同一公开入口，但直接使用会写旧TASK-078—082 scope、复制旧20261006.2说明，并生成32 GPU层Vulkan launcher；无参数默认输出旧包目录。`play_demo.py`默认旧共享PlaytestProfile且同样32层。这些实际不匹配已记录，不能不加区分复用。当前配置为CPU默认、GPU层16；先保持已批准设置，其他层数必须另有102真实采样。本轮不改发行脚本、模型锁或配置来赶打包。

## 源码、磁盘和输出隔离

根HEAD为 `6fcf5c22e965f0f7409438f19bc7b09e96ffb058` 加dirty。首尝试冻结保留于package-20261007-1 QA；第二次准确冻结在package-20261007-2，tracked.patch 156619 bytes、untracked-files.txt 11565 bytes，含Config/DefaultGame.ini的GameFeatureData空扫描目录/AlwaysCook规则。Header仍本preview；不造实施提交SHA。此前15项Native GREEN仍为其自身快照，第二次Shipping实际独立构建已成功。

root最近只读磁盘记录：C约21.57GiB、F约20.81GiB、G约23.56GiB可用。该值是该时刻余量；Cook缓存、staging、archive、模型和最终压缩合计峰值 **UNKNOWN**，当前容量不登记PASS。旧083 zip 4,882,366,341 bytes仅是历史产物大小，不能代入当前实际峰值或预计压缩率。必要时仅选择另一个未占用独立输出并记真实路径，不覆盖或递归清理旧包。

UAT的Archive/Staged/log及诊断UserDir彼此分开；实际交付仅新的 `Windows/` 运行树和经核验的说明/BUILD-INFO。打包log、QA、原始模型请求、性能CSV、测试Profile、源码制作资料不放入运行树。预览测试Profile使用 `.agent-local/qa/TASK-103/<唯一run>/Profile`，不能沿旧脚本默认路径。

## 当前装包契约与最终包核验

`Hearthward.Build.cs` 的NonUFS契约为：

- 精确包含 `Config/npc-agent.policy.json`；Content按DefaultGame的两入口地图及AlwaysCook目录构建。
- 递归包含**所有Resources文件**。有界资源元数据49文件/83,290,121 bytes（Fonts禁读未遍历），37 PNG、4 gameplay/experience/animal_motion/quest_guidance JSON、UI配置/来源记录及新099 WAV/CC0许可证。当前资源文件名未见env/secret/credential/password/token/key/用户pool或cache；这不是文件内容秘密检查或实际archive结论。
- `Runtime/LocalAI/models/Qwen3.5-4B-Q4_K_M.gguf` 显式NonUFS；非Editor缺该文件即BuildException。cpu和vulkan的llama-server.exe缺失亦中止；两后端LICENSE-LLVM-OpenMP显式NonUFS。
- LocalAI递归规则跳 `.downloads/`、`.part`、其他gguf及非llama-server的exe，保留其余runtime dll/config/许可证。大小写及规则仍以源码实际条件为准，不扩展成“排除了全部缓存/秘密”的结论。本单禁读Runtime/模型锁，由102/root核验已锁模型/运行时指纹和实际stage。

root在**新的候选运行树**中排除确认不参与运行的 `Resources/UI/items-clean.prompt.txt`、`Resources/UI/LAYOUT.md`、`Resources/Audio/README.md`，保留原仓库源文件以及Sources/许可归属证据。保留必需Resources数据/UI图/WAV、资源来源许可和字体许可。Fonts为本任务禁读路径，当前字体/OFL装包闭合由root实际核对；本单不伪称已核验。若某文件被实际代码读取，先检查实际引用再决定，不通过移动正式源文件达到排除。

初始Archive清单已由root保存150文件/4,885,690,146 bytes，含根Hearthward.exe、Shipping二进制、Pak/IoStore、两后端server/运行库/LLVM许可、锁定模型及Qwen/llama许可、字体/OFL文件名和099 WAV/CC0许可；本Agent仅消费QA清单元数据，未读取禁读Runtime/Fonts内容。最终清理后的闭合清单、用户Saved/QA/Profile/API钥匙/.env/本机秘密与制作工具排除仍 **NOT_RUN**。094逐资产权利缺项（36 UNKNOWN及自然源家族等）仍留发行前置，清单存在不代表全部Content权利清障。

task要求二机绑定包哈希：最终交付文件冻结后只计算实际分发zip/分片等最终文件SHA一次并保存；先前lock指纹照录来源，普通Content不逐件重新hash。包若实际变化，再按变化重新绑定，禁止把旧083文件指纹套入本包。

## 独立启动与当前未完成门槛

root已从Windows Run直接启动成功的第二次exe，无UE/Python启动器。标题版本0.2.0-preview.20261007.1、新游戏卧室、J main01修正目标、F6手动1→2、Alt+F4关闭7656后重启63156/Return误选默认New game并增序章自动至3；旧Continue声明已撤回，原result/截图元数据保留。另见101原档副本第三进程明确鼠标Continue恢复营地准备且仍7节点，绑定candidate-2/version.1；当前version.2未构建。隔离来自独立UserDir；Shipping编译掉SaveTestPool，不把参数名当实际隔离。第二进程target capture未返回foreground PID，仅保留Explorer被遮挡compositor图并明确标注。连续首救/营地、库存制作、模型请求和四保存阶段仍NOT_RUN。第二次当前 **BUILD_SUCCESS / FINALIZATION_PENDING / PARTIAL_NORMAL_INPUT_VERIFIED**，本Agent不向F复制，等待root最终核验。

`Start-Hearthward-CPU.cmd` / `Start-Hearthward-Vulkan.cmd` 已在本目录准备，最终放Windows根目录；直接start随包exe，后端明确cpu/vulkan、GpuLayers16，无Python。两项使用同一 `%LOCALAPPDATA%/Hearthward/Candidates/iteration-084-103-20261007-3` 候选专用UserDir，切后端保留本候选档、不复制旧档或QA。此处cmd静态参数已按LocalAIRuntime实际解析核对，未执行，不能替代上述直接exe或模型请求验收。

第二实体Windows机器按非管理员、中文/空格干净目录、断网及随包模型实测；记录真实硬件和最终包hash，同机多Profile/另一进程不能补这个门槛。当前二机 **NOT_RUN**，candidate-3不得复用历史候选的机器或样本声明。

真人样本严格沿103原门槛：首切片5名新玩家≥4完成且完成者中位30—60有效分钟；完整流程3名新玩家全员无阻塞、中位8—12小时、合计覆盖15支线；玩家/弟弟1v3在阶段I、II各5名真人×3次=15、成功≥12。原始加载/暂停/休息时间单记，仅扣允许项，不强制截时或用Agent循环补人。当前所有样本0，记录 [ACCEPTANCE_RECORDS.json](ACCEPTANCE_RECORDS.json) 尚为空。

Owner当前明确待决：094—098人物/营地/地表及衍生首件；100区域首件；099实际音效听感；固定录音UNPRODUCED时是否允许外部字幕预览。已确认077山地石堡方向不重新泛化为未知。087固定4B/参数真实语义门槛FAIL，模型/预算/部署策略契约修订方向待Owner；不能硬编码答案或放宽指标。技术构建、来源补证、采样和原规则正常游玩可独立继续。

## 可核对的外部工程依据

Epic官方 [Packaging Your Project](https://dev.epicgames.com/documentation/unreal-engine/packaging-your-project) 和 [Build Operations](https://dev.epicgames.com/documentation/unreal-engine/build-operations-cooking-packaging-deploying-and-running-projects-in-unreal-engine) 分别说明打包与Build/Cook/Stage/Archive职责。本轮采用仓库已有公开适配器，不引入新的打包框架。
