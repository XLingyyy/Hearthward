# TASK-103｜候选证据审计与交付准备

## 当前候选8

2026-10-08 当前内部候选为8，版本0.2.0-preview.20261008.1，受测实现38927738f488b9b5f184b85c88d53f374388de1e。TASK-096已修复候选6的Niagara启动异常，并修复候选7缺少火烟Cook包的问题。候选8 Shipping Build/Cook/Stage/Archive成功，Cook 0错误/1条MCP许可提示；97个新增资产中96个入包，全部5个火烟包存在，仅无运行引用的独立箭袋网格被剔除。独立UserDir下真实键鼠标题、新游戏、手动保存1→2、选择手动节点并确认恢复卧室通过。1280×720窗口实际应用，15秒未确认自动恢复通过；保留显示设置、拖动缩放及正常UI退出未验证。自有进程经UEClient关闭。完整Shipping路线、随包模型请求、性能、旧档、真人和二机仍未完成；无候选8 ZIP，无发布。详见docs/qa/TASK-103/CANDIDATE8_PARTIAL.json。

## 历史记录（仅适用于对应候选）

## 2026-10-08 候选6启动回归

2026-10-08 候选6 Shipping Build/Cook/Stage/Archive成功，Cook 0错误/1条MCP许可提示；97个新增资产中96个进入IoStore，未引用的独立箭袋网格正常剔除。实际启动失败：轻量Niagara在石堡CDO构造期间反序列化异常，进入ReportCrash后黑屏无响应；已通过原生线程栈定位，原候选保留并标记不可交付，公开UEClient关闭自有进程。 证据见docs/qa/TASK-103/CANDIDATE6_STARTUP_FAILURE.json及CANDIDATE6_GAME_THREAD.txt。后续修复与重建单独记录，以下历史候选记录不代表当前资产版本。

2026-10-07，Active/partial。共享分支codex/TASK-084-103-iteration，参考HEAD 6fcf5c22e965f0f7409438f19bc7b09e96ffb058加本批未提交差异。root负责Source冻结、UE生命周期、package、OS及finalizer执行；本Agent只处理获准QA/docs。未提交、推送、合并、上传或发布，无伪造实施SHA。

## 历史候选5

候选5/version.4在准确冻结Source上已实际公开Shipping Build/Cook/Stage/Archive成功：UAT0/dry_run=false，总235.52s，Build71.70/Cook63.43/Stage39.73/Archive58.81s，Cook0error/1 MCP EULA warning保留。当前BUILD_SUCCESS_INTERNAL_CANDIDATE_ZIP_CREATED。双CMD脚本启动＋真实OS开局/保存/明确Continue局部验证通过；最终ZIP为F:/HearthwardDemo/iteration-084-103-20261007-5.zip，4,645,167,974 bytes、166运行文件。finalizer运行树/ZIP文件名与大小一致、有界文本扫描BOUNDED_TEXT_CLEAN、17event/12WAV/9许可/water98顶点96三角形配对通过；最终ZIP SHA256恰计算1次，sidecar存在，无重算。正式验收false，NOT_PUBLISHED。

原结果：[构建安全快照](CANDIDATE5_PACKAGE_BUILD_SUCCESS.json)；private原件为.agent-local/qa/TASK-103/package-20261007-5/result.json、package.log、tracked.patch、untracked-files.txt。准确元数据由root单写[CANDIDATE5_BUILD_INFO.json](../../releases/iteration-084-103-rc/CANDIDATE5_BUILD_INFO.json)。Archive为F:/HearthwardDemo/iteration-084-103-20261007-5/Windows，Stage为E:/HearthwardQA/TASK-103/package-20261007-5/Staged，ZIP目标F同名.zip；不覆盖任何旧候选。

候选5两CMD实际SCRIPT_LAUNCH_AND_OS_GAME_INPUT_VERIFIED，Title.4双见，共9张真实截图；fresh profile且未复制旧/QA档。CPU新游戏卧室/main01、Tneutral、F6自动1→鼠标手动保存2；Vulkan明确鼠标Continue恢复同卧室/main01，F6仍02:26manual/02:25auto共2、Tneutral。正常退出后全自有bootstrap/game进程gone且无候选模型残留。本轮无模型请求/中文输入/IME/完整路线/5旧档兼容信用。 [5 OS安全记录](OS_CANDIDATE5_PARTIAL_NORMAL_INPUT.json)原件独立。

两项生产.4修复仅为既有楼梯入口条带内顶节点切换、原侧门目标≤50cm三维到达圆交接，Header.4；固定AI contract仍Source.2。099最新17event/12独立运行WAV、9项必需许可、水网格98顶点/96三角形与来源报告契约保留。候选5root实际install/execute已独立核闭合/有界扫描；文件存在及技术配对不证明全部Content许可或Owner实听。root已实际提取候选5一个cooked INI，known-config-secret-candidate5-20261007/inspection.json记录ACTUAL_CANDIDATE5_FIELD_FILTERED、目标节/字段均false；没有输出值或新hash。该单项不等于未知Content许可闭合。

release README-DEMO、RELEASE-NOTES、两CMD已对齐5/.4/profile5及构建成功、OS/ZIP未验。运行树内资料属于ZIP制作前的实测元数据快照，最终ZIP/hash由外部finalization-result/sidecar提供，不在包内预填最终ZIP哈希。root已实际完成finalizer5准确F/F绑定的install→真实OS→execute。最终实际结果见[CANDIDATE5_FINALIZATION.json](CANDIDATE5_FINALIZATION.json)、private finalization-result.json/final-file-manifest.json/bounded-secret-scan.json以及ZIP的外部.sha256 sidecar。包内BUILD-INFO和发行文本保留pre-ZIP实际构建/OS快照；源Info5与外部矩阵是ZIP完成后的元数据，不刷新运行树或重做ZIP。

## 工程与真实路线

099 Native13原index为.agent-local/qa/TASK-099/native-final-audio-20261007-13/native/index.json，report2026.10.07-00.32.03：本单23/23、完整28/28 Success，均0test warning/error；wrapper另13条启动Smoke errors及1 EULA warning，完整保存。NoSound/NullRHI不提供Shipping实听或Owner信用。087生命周期3+投影4原生P，兼容3项单列；其他任务详细证据见各自REPORT，不合并为一个全套PASS。

091同filterLoadedFortressSpatialNodes的四份独立原件：楼梯顶RED01.35.33为1Fail/0W/1E→GREEN01.39.30为1Success/0W/0E；侧门RED01.44.03为1Fail/0W/1E→GREEN01.45.37为1Success/0W/0E。各GREEN目录build.json是独立public Editor Development真实成功；四次同case不加为2条测试。生产两项修复由普通API四节点实际到达/切换补充验证。

Nav12的四个既有指引节点实际通过；只用实际确认地形+Z100、固定XY250/Z220作QA查询，首段完整query/controller路径普通PathFollowing移动1976.913883cm。第二段双端投影true、FindPath valid=true/partial=true，严格停在SimpleMove前，自有Editor退出true。完整撤离/营地/首救/新保存和正常OS路线未完成，地图/碰撞根因未定。

历史06到顶未切、07到侧门未切的原RED后续已被对应修复更新；08四节点已成功但远端终点FindPath无效。09原100/100/220、10独立预声明250/250/220候选投影均false，无末段move；11只读确认候选Z22167.1354、屋顶ImpactZ22413.1097、忽略已确认Home后地形21871.2367，宽Z1000 Nav22419.4832（+252.3478）未用于移动。QA插值Z未贴地形，12纠正查询方式后确实前进19.77m；普通步行必不可达或地图障碍原因仍未证明。12移动后侧门指引再次可见、正式world不变如实记录。原件及安全派生：[06](NAV06_GUIDANCE_FIRST_BLOCKER.json)、[07](NAV07_GUIDANCE_FIRST_BLOCKER.json)、[08](NAV08_FINAL_TARGET_FIRST_BLOCKER.json)、[09—11](NAV09_11_PROJECTION_HISTORY.json)、[12](NAV12_GROUNDED_FINAL_FIRST_BLOCKER.json)。03—05先前失效路径/readiness、稀疏HTTP输入和tick02 PY安全拒绝原件均保留；未绕过安全策略，不把它们归因为游戏碰撞。

## 固定模型与联合性能限制

固定AI contract Source.2双后端原60均完成：CPU原始33/60、受限3/20、明确E2E33/40、执行25/30；Vulkan32/60、2/20、33/40、24/30。各边界20/20、白得0，语言质量均FAIL。原59暖不足单列；预声明辅助1条补成各60样本后，CPU组件p95 22.656s≤30、Vulkan8.672s≤10，只代表暖组件门槛。Prompt/模型/Schema/参数/预算/阈值未改，候选5两项路线修复不宣称重跑语言矩阵。

102独立Development/API单一稳定新游戏固定视角：Vulkan6183帧/61.1867968s，p99 16.7755ms、1%Low49.0051591fps、>50ms0；CPU11603帧/129.8656161s，p99 16.5324ms、1%Low51.3488456fps、>50ms2。两者联合帧门槛均FAIL。完整单请求均在CSV内：VulkanHTTP24.407s正常完成后TARGET_REQUIRED→clarify/无candidate语义FAIL；CPUHTTP120.007s超时→MODEL_UNAVAILABLE/raw空。两次单场均实际1920×1080/D3D12/SM6/Epic3/100/Vsync0；余五场、Shipping/OS联合性能未验，未把CPU超时与GPU语义错误混算。

原9000启动CSV首次parser失败/reparse及第二次Renderer CPU worker AV分别保留；启动帧不补六稳定场景，CrashContext零内存字段不作OOM归因。官方相似Nanite问题与本次CPU worker AV边界见102报告，不关Nanite规避阈值。中文粘贴与单一模型请求不提供IME、原60整体质量或UI Paint时延信用。

## 历史候选与实际OS（均仅绑定原候选）

|历史候选|实际已结束证据|保留限制|
|---|---|---|
|1|Shipping C++成功；首Cook缺GameFeatureData规则、UAT25/2error1warning|FIRST_COOK_FAIL原件保留，无运行树|
|2/version.1|GameFeatureData空扫描目录/AlwaysCook最小微修后公开Shipping成功124.78s；局部新游戏/J/F6；101原档隔离副本最新营地节点实际恢复并第三进程明确鼠标Continue，仍7节点|旧Return默认New game误操作不得当Continue；四新节点与当前包兼容未验|
|3/version.2|Shipping成功307.11s、Cook0E1MCP warning；CPU隐藏CMD启动+OS新游戏/J/F6保存1→2/中文原文粘贴/正常退出|CPU跟随请求89.488s仍pending、144.443s观察失败UI，精确HTTP/raw NOT_READ；Vulkan/Continue/IME未验。新游戏误示恢复提示RED保留|
|4/version.3|Shipping成功373.90s；install5资料、17events/12WAV/9许可/water配对、BOUNDED_TEXT_CLEAN。双CMD隐藏脚本启动+OS实见Title.3、中性提示；CPU新游戏/Settings/保存1→2/退出，Vulkan显式鼠标Continue同卧室/F6仍2及时间戳相同；单一ngl16 Unicode跟随请求生成proposal后鼠标确认→HUDFollowing；全自有PID退出|CPU4不重复generation。39.122s最后pending/60.985s见proposal是观察时刻；Explorer双击/IME/全路线/真人二机未验。因06冲突ZIP暂停，全部旧Stage/archive和5份Source文档snapshot保留|

候选4原档隔离副本追加真实兼容历史：[安全记录](../TASK-101/OS_CANDIDATE4_ORIGINAL_COPY_COMPATIBILITY.json)列4旧节点，实际加载最新户外营地节点、正常手工保存4→5，第二独立进程明确鼠标Continue恢复同户外营地准备任务/HP/快物品且仍5，无新自动节点，6图。只验证可见字段；原件与readonlycopy未写/本次0hash。四新阶段、四旧节点全部Load、全字段等值及5旧档兼容仍NOT_RUN。

可定位原件：[首Cook](FIRST_COOK_FAIL.json)、[2构建](PACKAGE_BUILD_SUCCESS.json)、[旧OS部分](OS_PARTIAL_NORMAL_INPUT.json)、[Continue更正](OS_CONTINUE_ERRATUM.json)、[101原档副本](../TASK-101/OS_ORIGINAL_COPY_COMPATIBILITY.json)、[3构建](CANDIDATE3_PACKAGE_BUILD_SUCCESS.json)、[4构建](CANDIDATE4_PACKAGE_BUILD_SUCCESS.json)、[4 OS安全摘要](OS_CANDIDATE4_PARTIAL_NORMAL_INPUT.json)。4的SCRIPT_LAUNCH_AND_OS_GAME_INPUT_VERIFIED只含隐藏脚本启动和真实OS游戏输入，不提供Explorer双击信用。旧原档与readonlycopy未写，原SHA已一次完成，不重复hash。候选4Stage清理自动审批拒绝后root停止清理、历史完整保留，因此5Stage改E；磁盘读数是历史十进制GB，无新磁盘扫描。

## 最终UE MCP驻留

root实际normal Editor Bootstrap PID52580在最后检查时在线，startup49.3768451s。initialize200/initialized202、tools-list200返回3个meta工具、只读Bootstrap场景、公有observe RC/Python及CLI enabled端点匹配全部通过。[安全连接记录](../MCP/20261007/final-resident-connection.json)与SETUP保留初次52toolsets历史。runtime-input30020仅configured，不记输入实测；当前聊天native目录没有热挂载，新Hearthward聊天须确认服务工具可见。关闭该Editor会使端点离线，未伪造后续持续存活。

## 当前门槛与完成定义

|本单用例|当前真实范围|剩余门槛|
|---|---|---|
|T103-C01 证据|19任务分层记录及原报告可定位，RED与各时间/分母独立|audit最终实际运行一次：19任务、62 Resources文件名/大小元数据；[142原用例矩阵](ACCEPTANCE_MATRIX.md)完整保留原分母，正式acceptance false/Done0。Fonts/Runtime未遍历，inventory新hash0|
|T103-C02 独立运行|5Shipping实际构建成功；3/4局部OS和随包模型单请求历史保留|5双CMD/新游戏/T/F6手工保存/明确鼠标Continue/退出局部P；5无模型/中文/IME/完整路线信用，固定语言质量FAIL|
|T103-C03 二机离线|暂无第二实体机器样本|第二实体机离线操作NOT_RUN；当前最终ZIP/hash实际存在，单机结果不补二机|
|T103-C04 首切片真人|0名正式样本|NOT_RUN，原完成率和30—60分钟口径保持|
|T103-C05 全流程真人|0名正式样本|NOT_RUN，原无阻塞和8—12小时口径保持|
|T103-C06 战斗真人|Native事务不计真人|各阶段15次/≥12成功的真实分母未取得|
|T103-C07 视听|技术事件/PCM/脚步/水源测试已记录|设备实听/Owner首件/全部Content许可未闭合，固定录音UNPRODUCED，动态回复无TTS|
|T103-C08 发布|内部本地候选，未上传|实际内部ZIP与单次SHA/外部sidecar已完成；NOT_PUBLISHED，正式验收false，不迁移为上传或发布|

Owner待决集中在095—098风格首件、100一区空间首件、099实听及固定录音UNPRODUCED时外部字幕预览是否接受；077石堡方向已确认。模型/预算/部署契约改动需Owner决定。正常路线、许可工程缺口、模型/性能失败、真人与二机资源分别登记，不全部称为设计问题。远端tag唯一性UNKNOWN，未提交/推送/合并/Release。正式scope基线验证要求实际包含任务快照的提交，现参考HEAD不满足，保持NOT_RUN，不伪造基线或调整验证器。

Root最终默认仓库校验已实际通过：104份任务快照、0错误；git diff --check退出0。首次18处错误仅来自私有临时旧草稿的相对链接，草稿内容保留并归档为文本快照，原失败记录保留。此次检查不替代UE、玩法、模型或正式task/base验收，原生与模型测试未重复。[实际检查记录](FINAL_REPOSITORY_VALIDATION.json)。
