# TASK-099 西湖环境声当前结果

Root实际第13轮Development Editor buildSUCCESS；Environment3/3 Native Success、0用例warnings/errors，包含实际exactmesh+tag/current transform三角最近点、registered/begun/currentWorld/visible、far/hidden/unregister/re-register/destroy、多个候选最多一源、真实Save/Load/epoch/Pause/dead/EndPlay及连续PCM。源码/Resources继续冻结。[完整13 index](../native-final-audio-20261007-13/index.json)、[本单23子集](../native-final-audio-20261007-13/task099-subset.json)和[当前REPORT](../REPORT.md)为正式实际技术证据；旧09真实RED保留，原结果不改。

本轮明确-NoSound/-NullRHI。Native有真实组件与实际PCM生成信用，没有设备听声、正常自然地图流送、audible seam、混音或Owner信用。当前water candidate维持mono44100 PCM16/15.5s，整个sound_events17/独立WAV12。Root唯一后续Shipping/OS/录音执行。

## 采样、RED与施工阶段历史


Root经公开UEClient在独立Bootstrap Editor只读加载精确mesh及ExternalActor。首次几何失败报告完整保留；第二次实际READ报告已归档为 `second-read-water-source-20261006T235439Z-44d9531a.json`，原始启动/停止/runtime见 `.agent-local/qa/TASK-099/water-geometry-20261007-02/`，owned stop成功。未改Engine、Content或自然地图。

二次实际UE5.8.2几何为LOD0局部厘米98 vertices / 96 triangles。精确对象路径 `/Game/Hearthward/Assets/NaturalWorld/Rebuild/Meshes/Lake_0/StaticMeshes/SM_Lake_0.SM_Lake_0`，CPUAccess=false。不可用运行时CPU顶点猜测；`Resources/Data/TASK-099-water-audio.json`只保存此次真实UE局部三角几何、精确mesh、water tag及sound event ID。运行时必须读取当前已注册、HasActorBegunPlay、当前World的实际组件变换，不能使用保存对象world缓存identity、配置椭圆或AABB声学替代。

保存ExternalActor标签为water/TASK026.REBUILD，NoCollision，材质M_RiverWater，relative translation=(-93000,-36000,15700)cm。Bootstrap没有GameWorld；注册与BeginPlay未被Python公开API读出。保存对象证据没有证明自然地图当前流送实例已加载。

首次报告 `first-read-water-source-20261006T234707Z-01e8347d.json` 几何NOT_READ/Invalid actual triangle vertex IDs。已核本机MeshDescriptionBase.cpp GetTriangleVertices先SetNumUninitialized(3)，继而Algo::Copy用Output.Add追加，实际数组长度6；不读取未初始化头。修正脚本只走indexed GetTriangleVertexInstance(0..2)→GetVertexInstanceVertex，校验实际ID。第二次READ为此最小修正的实际结果。

## 候选音源

RandomMind原始VistulaShort_0.mp3来自[作者作品页](https://opengameart.org/content/sea-and-river-wave-sounds)，[原文件](https://opengameart.org/sites/default/files/VistulaShort_0.mp3)，[CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/)。原源12004407 bytes已保存。取真实10–26s的16s片段转mono44100Hz PCM16，0.5s线性尾头交叉淡化，输出15.5s/683550frames/1367144bytes。无合成、静音占位或增益。完整argv、编辑规则和采样指标见 `art_source/TASK-099/randommind-vistula/PROVENANCE.json`。

运行文件 `Resources/Audio/TASK-099/candidate-water-west.wav`，独立许可 `License-RandomMind-Vistula.txt`，映射 `environment.water.lake_west` / environment。当前全部sound_events16条/独立运行WAV11份；Kenney阶段历史15/10不改写。最大PCM16绝对值32211、满刻度0、文件首尾差313仅技术采样事实。实际设备播放、循环接缝、响度/主观水景语义及Owner试听均NOT_RUN。

## 先取得真实RED

新增 `Source/Hearthward/Tests/EnvironmentAudioLifecycleTests.cpp`，过滤器 `Hearthward.Iteration.Task099.Environment.` 注册3项。World夹具建立真实LocalPlayer、碰撞地面、自然grounded的Player/Brother，加载实际湖mesh、注册真实StaticMeshComponent、BeginPlay，使用与保存坐标不同的真实平移/旋转/非均匀缩放。对应断言覆盖：无tag不启声；精确mesh/tag活动关联；最近实际三角位置；远距/隐藏/组件卸载/重注册/多候选有界/销毁；真SavePoint/LoadPoint和epoch、暂停/恢复/死亡、EndPlay；实际消费者创建的procedural wave三周期连续PCM及后续小块/队列有界。

ActualLoad用Root runner真实UUID HearthwardSaveTestPool隔离；只读源不注入成功回执、设置Walking或Fake Save。当前生产环境循环尚未实现，没有生产占位声明，3项Native NOT_RUN。根代理统一构建/运行后保留实际RED；缺声源使ContinuousPCM后续波形断言尚不可达，不能据此提前说PCM已验证。之后才接入实际循环生产实现。现有099原19项GREEN属于原受测实现，不能移植给这3项。

生产计划限定Presentation生命周期与必要本地LoopWave：BeginPlay/LevelAdded/ActorSpawn维护少量matching weak candidates，逐帧仅评估当前水候选；不每帧扫全部1300+ actors、不强制加载湖资产。距离来自真实三角最近点及当前组件transform，线性0→3000uu现有听取边界；仅一个region loop。Load/暂停/死亡/隐藏/卸载/EndPlay清理，恢复仅重建当前场景。音频线程从不可变真实PCM循环填充，不用bLooping包一次性QueueAudio伪连续、不增长队列。

音乐、active fire和wind source活动规则缺少已确认玩法/源契约，当前不新增；此水声工程已获授权，不归为Owner音色设计阻塞。

## RED09与当前生产冻结

Root实际 Development Editor build成功，联合5项为Foot2Success、Environment3Fail、0warning、12个测试错误。环境错误均为实际source0 vs 1，真实活动/grounded/SavePoint/LoadPoint前置通过；ContinuousPCM的后续生成断言未达到。原始build/result/index完整保留在 `red-20261007-09/`。

真实RED后已接入Presentation环境cache/生命周期与新增HearthwardEnvironmentLoopWave。仅BeginPlay初次扫StaticMeshActor，后续current LevelAdded / ActorSpawn一次PostPhysics / PostRegister关联精确mesh弱组件。逐帧只检查匹配候选与实际已注册/begun/currentWorld/tag/visible状态，使用当帧组件真实变换的96三角最近点；监听点超过3000uu、隐藏、卸载、Pause、restore、dead、EndPlay清当前source。未强制加载未流送mesh。

LoopWave初始化后PCM不可变，音频线程按请求samples循环copy/wrap。实际wave配置INDEFINITELY_LOOPING_DURATION+bLooping，本机SoundWave Parse为LOOP_Forever，不以文件15.5秒作为生命周期，也不依赖QueueAudio补静音。文件懒加载只在实际近水首次启声，PCM保持一个period；输出块大小为当前请求，不积压历史循环。Volume(environment)与Master、线性Sphere0→3000uu沿现有契约。

Association测试新增实际T0三角barycentric内部点经当前旋转/非均匀scale变换，真实capsule movement tick维持grounded；该点距actor中心超过3000uu但贴近实际surface，保护错误中心裁剪/actorOrigin发声的回归。ContinuousPCM追加实际wave indefinite/loop生命周期断言。五份Source静态括号/公开API/资源数据检查PASS；peer mcp_setup完成只读缺陷扫描，未发现已确认编译/API/当前世界生命周期缺陷。生产Cpp编译、Environment GREEN及实际设备长时播放仍NOT_RUN，由root统一执行。

另新增独立空挥候选combat.swing：Kenney包实际Audio/knifeSlice.ogg15532bytes，转mono44100Hz PCM16 52958bytes/26440frames/0.599546s。精确源/argv见 `docs/assets/TASK-099/SWING_CANDIDATE.json`。当前完整17events/12独立运行WAV/11个新增保留源文件（10 Kenney OGG +1 RandomMind MP3）；空挥producer/consumer仍待真实Swing RED后施工，本轮不提前接受swing回执。音色/水声接缝/混音/实际设备与Cook独立保持NOT_RUN。

## 实际build12与空挥RED信用边界

Root的build10实际FAIL为USoundWaveProcedural无默认构造与GetStaticMesh返回TObjectPtr两类compile错误；按本机UE5.8明确FObjectInitializer+Super及.Get修复。build11达到link但FAIL，IAudioProxyDataFactory inherited symbol属于builtin AudioExtensions，按Root最小授权补Hearthward.Build.cs PrivateDependency并初始化Nearest消C4701，保留Best guard。旧静态审查没有compile信用。原build10/11完整结果仍在.agent-local真实run。

Root实际build12 SUCCESS，水声生产已通过Cpp编译与链接，Environment3的GREEN Native仍NOT_RUN。空挥ActualEmptySwing真实RED12只有5个missing-swing-receipt错误、0warnings，所有实际publicAttack前置通过。其原始build/result/index归档 `swing-red-20261007-12/`；该RED只证明producer缺口，不证明后来新增的真实Presentation声源断言已取得RED。

Root后批准producer代理在正式validated active窗口入口发布swing，消费者仅Presentation.cpp一行加入已存在Kind白名单，沿SuccessId/OperationId/Epoch/TargetPosition与原去重/暂停/死亡/restore/远距规则找到combat.swing真实候选。Source已冻结；新的producer→consumer声源/格式/位置/抑制断言由mcp持原同case串行追加，无新注册case、fakeReceipt或私有Play。此层与完整099回归等待Root统一Native23+5，不借旧19P宣称通过。
