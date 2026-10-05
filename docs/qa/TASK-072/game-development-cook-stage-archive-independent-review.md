# TASK-072 Development Cook/Stage/Archive独立清单审阅

Root自有公共BuildCookRun实际完成：package-green-attempt.log1344 ExitCode0，Build Succeeded，Cook97.62秒/Stage29.51秒/Archive27.83秒，整次204.89秒。Cook summary1080明确0error/0warning。其它平台SetupSDK失败属于本次未请求的平台，未构成Win64失败。子代理仅只读日志、manifest及文件stat，不启动UE/模型，不读weights，不计算hash。

下一Game诊断真实路径：

- exe：[Archive/Windows/Hearthward/Binaries/Win64/Hearthward.exe](F:/uagent-task-temp/task072-game-development-20261005/Archive/Windows/Hearthward/Binaries/Win64/Hearthward.exe)，338,619,904B。
- bundle：[Archive/Windows/Hearthward/Runtime/LocalAI](F:/uagent-task-temp/task072-game-development-20261005/Archive/Windows/Hearthward/Runtime/LocalAI)。
- 顶层Archive/Windows/Hearthward.exe是171,520B bootstrap；应启动内层真实Game binary以持有对应PID。

实际NonUFS manifest150行，Stage与Archive缺失均0。Resources45文件/115,353,220B；Runtime/LocalAI68文件/2,875,713,165B，全部在Stage/Archive存在，来源路径及文件size匹配。GGUF2,740,937,888B，两后端server各9216B，server-impl、CPU动态后端/Vulkan及OpenMP许可均有实物。UI两字体、两OFL、三Data JSON、interface/layout与policy均部署。

Archive Paks实体存在且非空：Hearthward-Windows.pak11,219,414B，ucas1,569,224,240B、utoc845,937B，global.ucas3,268,608B/global.utoc794B。UFS manifest5287行；两张正式地图按本体.umap精确匹配，不把generated cell数算额外地图。

Data JSON339次/Game引用/334个唯一package，UFS manifest全部覆盖，缺失0。其中animal_motion331=14mesh+14skeleton+303clip，uasset/uexp均有记录；gameplay8次引用/3唯一package，experience无/Game引用。该证据仅为cook/stage清单与容器实物，未解析容器、未授予真实Game LoadObject/303动作验证。

现有seven tool impl DLL每后端仍部署，Resources items-clean.prompt.txt/LAYOUT.md/art-provenance.json也仍部署；这是已记录发行排除待办，未作为本次可逆Development诊断的阻断条件。录音仍暂缓，没有用Audio/README充当录音实物。

本次没有Cook或磁盘失败，输出参数实际进入Cook invocation并完成F Cook/Stage/Archive。外层UAT转发少量compiler行存在replacement字节，原编译RED以Root正确UTF8 compile-red.txt为准；本稿不重复损坏文本。

完整路径/size/清单与限制见[JSON](./game-development-cook-stage-archive-independent-review.json)。Game/模型运行、DLL真实加载、单场景帧时、Shipping、独立机器、正式RC冻结/发布均不由本清单授予。
