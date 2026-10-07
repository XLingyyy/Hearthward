# TASK-103 candidate-3 运行树闭合入口

2026-10-07。状态：**PREPARED_STATIC_ONLY**。入口 [finalize_candidate3.py](finalize_candidate3.py)，[静态检查记录](FINALIZE_CANDIDATE3_STATIC.json)。未执行install／finalize、未启动UE／模型、未构建或打包；当前Windows输入目录尚不存在，CANDIDATE3_BUILD_INFO仍NOT_BUILT，静态读取已确认该状态不能进入安装。Root持最终源码冻结、真实构建、正常OS和执行窗口；本文没有修改共享103报告／发行元数据或宣称候选完成。

输入固定为 `F:/HearthwardDemo/iteration-084-103-20261007-3/Windows`；证据固定 `.agent-local/qa/TASK-103/package-20261007-3`；最终ZIP为 `F:/HearthwardDemo/iteration-084-103-20261007-3.zip`，旁置 `.zip.sha256`。路径必须是该F盘绝对候选3，拒绝候选1／2、UNC、相对路径、`..`、junction／reparse和已存在最终产物。无参数只输出PREPARED_NOT_EXECUTED，不检查或修改运行树。

## 两阶段顺序

1. Root实际Source冻结和package成功后，更新 `docs/releases/iteration-084-103-rc/CANDIDATE3_BUILD_INFO.json` 的真实BUILD_SUCCESS、SUCCESS、冻结证据和正确版本；其余四份说明／启动器同步准确候选3／版本.2。先执行：

   ```text
   G:/GameFactory/.venv/Scripts/python.exe -X utf8 G:/GameFactory/Hearthward/docs/qa/TASK-103/finalize_candidate3.py --install
   ```

   安装只核已构建／冻结，不要求未来OS证据。复制README-DEMO.txt、RELEASE-NOTES.txt、CANDIDATE3_BUILD_INFO.json→BUILD-INFO.json、两个Start-Hearthward-*.cmd到Windows root；首次目标必须不存在。`document-install.json`记录本候选五文件尺寸／mtime，不计算hash。该阶段不删除三说明文件，不创建ZIP。Root随后通过获授权的 exec_command 隐藏启动两个cmd，再进行真实OS游戏输入，记录实际后端／标题／独立运行结果；每次关闭进程后继续。该证据不包含Explorer双击或WinRun启动。静态启动参数不代替实测。

2. Root将实际OS、版本观察和限制写回发行源文档。final必须有 `actual_embedded_version=0.2.0-preview.20261007.2`、`shipping_normal_input`为PARTIAL_NORMAL_INPUT_VERIFIED／NORMAL_INPUT_VERIFIED／FULL_NORMAL_INPUT_VERIFIED之一、存在的shipping_os_evidence，及launchers.state精确为 `SCRIPT_LAUNCH_AND_OS_GAME_INPUT_VERIFIED`。随后执行：

   ```text
   G:/GameFactory/.venv/Scripts/python.exe -X utf8 G:/GameFactory/Hearthward/docs/qa/TASK-103/finalize_candidate3.py --execute
   ```

   final允许刷新本脚本安装的同候选五文件，必须先匹配安装记录的尺寸／mtime；外来或已变化文件拒绝覆盖。从发行源刷新最新说明／BUILD-INFO／cmd之后才闭合，并保留首次owned_install收据UUID，更新五源文档和目标文件的尺寸／mtime。Root只更新发行源文档，Windows已安装五文档的外改仍拒绝。Root需预先将最终文件清单引用写为FILE-MANIFEST.json，ZIP／hash生成后证据在外部finalization-result及sha256旁文件；不要在ZIP生成后修改运行树BUILD-INFO来追填ZIP自身hash。

两个阶段均要求原公开package result `ok:true`、returncode0、dry_run:false、正确候选3archive_dir；source_freeze必须含肯定FROZEN且不含UNFROZEN或NOT／PENDING／PLANNED／FAIL前缀，冻结证据必须存在于项目内。最终ZIP／manifest／scan／result／hash输出均采用不存在guard；失败保留部分产物和失败结果，不自动删除／覆盖或重试。需要恢复时由Root先核对具体失败阶段和本候选产物，旧Stages、candidate1／2和用户档全程不触碰。

## 闭合范围与证据层

仅删除已确认三项非运行说明：`Hearthward/Resources/UI/items-clean.prompt.txt`、`Hearthward/Resources/UI/LAYOUT.md`、`Hearthward/Resources/Audio/README.md`。其余未查证文件保留；Source／Saved／profile／cache／QA／项目文件／私钥等出现在运行树会拒绝闭合，不扩展自动删除清单。

核对游戏exe／Shipping exe、GGUF、CPU与Vulkan llama-server、Pak／IoStore非空；字体二进制和模型／DLL／pack等仅列文件元数据，归档时按文件流读取，不解析内容或逐件hash。许可必须实际存在：NOTICES、Qwen／llama.cpp、双后端LLVM OpenMP、两字体OFL、Kenney RPG Audio、RandomMind Vistula，共9个必需许可文件。按已Cook运行树的experience.json要求17个唯一sound_events、12个独立WAV（原transfer＋9候选＋water＋swing），检查精确依赖存在，并核水声与挥击的event_id／文件绑定。该计数是候选3最终要求；资源和Source／Native尚未冻结时，不提供通过信用，旧15／10或16／11运行树会被拒绝。未产固定录音仍UNPRODUCED，未要求不存在的28组voice文件。

水声还必须有非空 `Hearthward/Resources/Data/TASK-099-water-audio.json`，source_id为lake_west、actor tag为water，完整mesh对象路径为 `/Game/Hearthward/Assets/NaturalWorld/Rebuild/Meshes/Lake_0/StaticMeshes/SM_Lake_0.SM_Lake_0`。98顶点／96三角及引擎版本必须直接匹配既有只读UE第二次源报告 `docs/qa/TASK-099/water/second-read-water-source-20261006T235439Z-44d9531a.json` 的实际READ数组。源报告不放进运行树、不hash，不从glTF坐标或包名推算几何；此配对不证明运行时水域交互或许可。

许可文件PRESENT只提供文件闭合证据；TASK-094的既有Content作者／权利缺项保持UNKNOWN，整包许可验收NOT_PASSED，Owner试听NOT_RUN。本入口不输出发行授权、六场／模型质量／真人／二机PASS或PUBLISHED。

纯文本扫描只针对明确文本后缀和LICENSE／NOTICE／OFL名称：每文件≤1MiB、总≤8MiB，UTF8／BOM UTF16／GB18030明确解码，检测凭据赋值、已知服务token和私钥标记。仅输出文件定位、规则名称与次数，不输出匹配内容、headers或密钥。超限、解码异常或疑似binary文本记录NOT_READ并阻断闭合；二进制密钥内容扫描明确NOT_RUN，不能由有界文本CLEAN外推整包不存在秘密。

`FILE-MANIFEST.json`列全部payload尺寸／mtime，排除manifest自身和外部ZIP／hash以避免循环。private证据同步final-file-manifest／bounded-secret-scan；归档后核对源树未变化及ZIP central-directory文件名／尺寸，无解压和全量重复校验。ZIP采用标准库ZIP64与exclusive `x`模式；GGUF／Pak／IoStore采用STORE，其余DEFLATE。行为依据 [Python zipfile官方文档](https://docs.python.org/3/library/zipfile.html#zipfile.ZipFile)。最终ZIP关闭／目录核对后，仅调用一次SHA256，记录attempt count并写外部旁文件；源文件／manifest／模型不计算SHA。

原14项静态历史记录保留；本次另核17／12／9资源门槛、实际98／96源几何配对、精确脚本启动＋OS游戏输入状态和保留安装收据UUID。静态检查涵盖路径拒绝、NOT_BUILT真实门槛、负向OS状态拒绝、内存凭据计数／无值输出、合法占位、编码／NUL、三unlink集合、唯一SHA调用、ZIP64 exclusive、同候选安装刷新、实际事件分母和无UE入口。它们未修改真实package，未创建ZIP，也未提供真实运行树闭合／许可读取／launchers OS验证信用。
