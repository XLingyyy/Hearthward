# TASK-084—103 本地执行记录

2026-10-07 最新Owner决定：[七项推荐方向已批准](../../design/DSGN-004-iteration-art-local-ai.md)，按1A—6A、7B继续制作与本地模型对照。以下候选5与测试记录保留其原版本范围；新首件尚未视觉签收，新模型尚未选定。


2026-10-07。实际分支codex/TASK-084-103-iteration；参考HEAD 6fcf5c22e965f0f7409438f19bc7b09e96ffb058加本批实现差异。用户2026-10-07已授权将本批成果提交推送到任务分支，并清理旧工作树和无用残留；尚未合并或发布。用户已授权本地实施和UE MCP配置；设计首件暂缓时继续独立技术工作。所有任务保持Active/partial，工程原生、真实模型、性能、OS、Owner、真人与第二实体机分别记账。

## 当前候选与当前路线

当前只指候选5/version0.2.0-preview.20261007.4。root已准确冻结Source并实际公开Shipping Build/Cook/Stage/Archive成功，UAT0/dry_run=false，总235.52s；Build71.70、Cook63.43、Stage39.73、Archive58.81s，Cook0error/1 MCP EULA warning。原result/log/freeze patch/untracked清单在.agent-local/qa/TASK-103/package-20261007-5/，[安全构建快照](../../qa/TASK-103/CANDIDATE5_PACKAGE_BUILD_SUCCESS.json)与root单写[CANDIDATE5_BUILD_INFO.json](../../releases/iteration-084-103-rc/CANDIDATE5_BUILD_INFO.json)定位。Archive为F:/HearthwardDemo/iteration-084-103-20261007-5/Windows，Stage为E:/HearthwardQA/TASK-103/package-20261007-5/Staged，ZIP目标F同名.zip。双CMD脚本启动＋真实OS开局/保存/明确Continue局部验证已通过，9截图，Title.4双见、fresh profile不复制旧QA档；正常退出所有自有PID gone。本轮无模型请求/中文/IME/完整路线/旧档信用。最终ZIP4,645,167,974bytes/166运行文件、filename-size一致/BOUNDED_TEXT_CLEAN、17event12WAV9许可/water98/96配对P，最终SHA256恰计算1次、外部sidecar存在；[最终record](../../qa/TASK-103/CANDIDATE5_FINALIZATION.json)与[5 OS摘要](../../qa/TASK-103/OS_CANDIDATE5_PARTIAL_NORMAL_INPUT.json)。包内pre-ZIP实测快照不刷新，源Info5/外部record是ZIP完成后元数据。formal acceptance false，历史候选不代当前包通过。

Nav15最新Development/API序章至营地流程通过：正常新游戏、遗物、跟随、四空间节点、真实正式终点完整路径、撤离交互与剧情转场完成，phase=occupied且prologue_complete由游戏产生；等待实际加载结束后营地保存1→2，owned editor退出确认。无QA位置/速度/奖励/进度/时钟写入。Nav12部分路径失败与13/14调查保留；完整OS输入、首次救援、Shipping仍未验。[最新安全摘要](../../qa/TASK-084/NAV15_EARNED_CAMP_CHECKPOINT.json)。

Source.4仅新增楼梯入口条带内顶节点切换、原侧门≤50cm三维到达圆交接和版本Header；未变地图/设计/Save/奖励/传送。两项各同LoadedFortressSpatialNodes单case独立RED1Fail0W1E→GREEN1Success0W0E：顶01.35.33/01.39.30、侧门01.44.03/01.45.37，四份原件及两个public Editor build独立。实际四节点交接补充验证两Sourcefix，不等于整路线完成。

## UE MCP

UE5.8.2原生ModelContextProtocol与AllToolsets启用，工程/Factory Codex配置使用loopback8000/mcp；保留旧插件和EngineAssociation。实际initialize/tools-list、Bootstrap只读场景查询、codex mcp list发现记录见[连接记录](../../qa/MCP/20261007/SETUP.md)。配置写入不会使已有聊天即时热载工具目录，旧PID52580连接记录保留；当前托管Bootstrap Editor PID78676已通过initialize/tools-list、Bootstrap只读、RC/Python与Codex配置检查，[当前连接记录](../../qa/MCP/20261007/managed-resident-connection.json)。该实例由持续运行的UEClient所有者管理。当前chat native目录没有热挂载；端点之后存活依赖该Editor，runtime-input30020仅configured不算输入实测。AllToolsets带GameFeatures依赖及MCP EULA告警保留。

## 逐任务当前结果

|任务|已落地的工程/调查|实际验证与剩余门槛|
|---|---|---|
|084|Nav15真实新开局至营地API流程完成，沿完整正式路径撤离，游戏产生prologue_complete，营地保存1→2|公开UEClient正常停止且退出确认；Nav12—14失败证据保留。OS连续输入、首次救援和Shipping全流程未验|
|085|共享只读显示模型、未知值/时间线/动作身份契约|4项原生通过；更多实际页面/渲染验收未完成|
|086|个人/持续采集队工作状态、原因及受保护恢复入口|3项原生通过；正常输入的长回复/14至32恢复未复测|
|087|拒绝忙请求保持原输入；失败与取消使旧回调失效；真实上下文事实/身份缺口 RED→GREEN|生命周期3项、投影4项通过。固定AI Source.2双后端原60均完成：CPU原始33/60、受限3/20、明确端到端33/40、执行25/30；Vulkan原始32/60、受限2/20、明确端到端33/40、执行24/30；各边界20/20、白得0，质量均FAIL。每后端原59暖请求不足，另加预声明性能辅助1条后各60样本，CPU组件p95 22.656s≤30s、Vulkan8.672s≤10s；完整Shipping60/IME/UI绘制未验；4单条Vulkan Shipping跟随确认P仅历史，5无模型请求。102独立CPU单场HTTP120s超时及旧完整33/60分别保留|
|088|中文配方搜索、筛选、缺料投影、单会话材料目标|2项原生通过，真实搜索状态同步 RED→GREEN；Load/epoch 清理和正式材料事务覆盖|
|089|准确装备 GUID、材料栈、转移预览与重放保护|3项原生通过；真实 OS 拖放和渲染未验|
|090|实际任务阶段的 HUD/日志提示、手动领奖入口、倒地反馈优先|3项原生通过；倒地两路径 RED→GREEN，实际日志与地图的等待目标同步通过；最终屏幕比例/150%绘制仍待实测|
|091|.4顶/侧门两项窄修复，各独立单case Native RED→GREEN及真实四节点交接|顶01.35.33 RED→01.39.30 GREEN、侧门01.44.03 RED→01.45.37 GREEN，各RED1F0W1E/GREEN1P0W0E，四原件独立不相加。Nav12末段首段19.77m、第二段partial停止；正常OS完整路线、渲染、传送未验|
|092|显式停止跟随时终止仍活动的 AI MoveTo|真实导航 Fixture RED→GREEN；正常600m往返、流送与 Live 净空未验|
|093|本营设施/岗位、实际缺料、绑定 tier/epoch 的升阶卡、真实回营回执|3项原生通过，其中2项夹具焦点初始化已修正；原 RED 留存，正式 UI/回营实际操作未验|
|094|845行资产登记；150个精确包实际Registry/ImportData/引用读取|150 READ；114有ImportData，36类不适用。自然97包指向96个本地源、25生成类有本地依赖，草网格/草类型外部源未闭合；17候选源与实导入源不同。原26未知包/10需求、逐件许可和Owner首件保留|
|095|五角色源首件与原骨架核对|DSGN-004方向批准；兄弟服装源预览、守卫/射手/重兵源预览已生成，射手连体持物需清理。UE替换、专用动作/持握和Owner视觉未完成|
|096|灰石/旧木/铁件/暖灯同门口昼夜源首件|方向已批准；源场景已生成，UE PBR烘焙、有限火烟、实机路线及Owner视觉未验|
|097|R3猪衍生野猪五包、六个作物网格与20包已保存接入游戏|方向已批准；新Editor编译与作物原生2/2通过、1警告，重启后UE渲染和尺寸已核对；野猪共享原50骨/物理/27动作且家猪保持，四动作逐骨兼容测试通过；路线、新Cook、Owner视觉未验|
|098|三个设施新UE资产与建造绑定、四类武器图标|12个精确包锁；导入与UE尺寸/底部检查通过。真实PIE两设施扣材/生成/登记通过；锻造建造、DPI、保存/Cook、Owner视觉未验|
|099|CC0 PCM源、正式玩家/NPC/进度/战斗/空挥事件、真实脚步Notify、水网格关联与连续水声；同步帧去重和暂停/时间线/声源清理|四条Walk/Run精准包16个Notify，当前17event/12运行WAV；water READ98顶点/96三角形与源报告配对。Native13本单23/23、完整28/28均0warning/0test error，wrapper13启动errors/1 EULA warning分列。历史06失败、07完整24/本单19、独立Save08 1/1保留；正常有声、Owner试听和所有环境覆盖未验|
|100|既有主支线工程回归与新增作坊入口/旗点源布局|原3新增/7复用原生保留原版本；复用098设施源场景已生成，正式四区布置/巡逻/通关和Owner视觉未验|
|101|原档只读副本、schema/字段矩阵、已有存读档集成回归|7项选定原生通过；候选2最新营地副本恢复/第三进程明确Continue仍7是历史；候选4原档副本实际列4旧节点/Load最新户外营地准备任务旧节点→手工5→第二进程鼠标Continue同状态仍5、6图。只可见字段，其余3旧节点仅列；四新阶段/全字段/5旧档兼容NOT_RUN|
|102|严格CSV解析；两backend单场景完整游戏/模型请求采样，保留原口径|实际1920×1080/DX12：Vulkan61.19s/6183帧，p99 16.7755ms、1%Low49.005fps；CPU129.87s/11603帧，p99 16.5324ms、1%Low51.349fps。完整请求分别语义FAIL/HTTP120s超时，帧联合门槛未过；API注入Development层，六场稳定矩阵/Shipping/OS未验。原9000启动诊断与第二次Renderer AV单列|
|103|候选5/.4准确冻结、实际Shipping Build/Cook/Stage/Archive成功；旧1—4和OS历史完整保留|5 UAT0/dry_run=false，总235.52s（Build71.70/Cook63.43/Stage39.73/Archive58.81），Cook0E1MCP EULA warning；Archive F5/Stage E/ZIP目标F5。5双CMD＋OS标题/新档保存1→2/Vulkan鼠标Continue同卧室仍2/Tneutral/正常退出局部P，9图；最终ZIP4,645,167,974bytes/166files、scan/closureP、单次SHA/sidecar。Source两fix已Native/API验，完整路线未完成；5无模型/中文/IME/旧档信用。固定模型质量与102联合性能FAIL；Owner/真人0/二机未验，NOT_PUBLISHED|

详细结果见[任务入口](README.md)与各TASK-xxx/REPORT.md。本表只摘要实际分层证据，不宣称142项全部通过。audit最终一次已生成19行DELIVERY_MATRIX（084—102）/62 Resources元数据，当前准确绑定5/12。另[142原用例矩阵](../../qa/TASK-103/ACCEPTANCE_MATRIX.md)全分母保留：87项partial、35项NOT_RUN、6项已测FAIL子范围、14项Owner视觉/体验子范围未验（属094—100七任务，制作方向已批准）。这些状态不合成正式通过或Done，formal acceptance false/Done0。

## 最新独立Native与模型/性能

Native13原index.agent-local/qa/TASK-099/native-final-audio-20261007-13/native/index.json，report2026.10.07-00.32.03：完整28/28、099子集23/23 Success，均0test warning/error；wrapper另13条frame0 Smoke启动errors与1 EULA warning单列。旧Native06完整25=20P5F、本单19=15P4F；修后07完整24/本单19均GREEN；独立fresh Save08只1P，原RED/警告均保留，不加为同一全套。091上列四次Native单case另有独立时间、分母，不能并到13。

固定AI Source.2原60双后端均已结束：CPU原始33/60、受限3/20、明确E2E33/40、执行25/30；Vulkan32/60、2/20、33/40、24/30，各边界20/20、白得0，质量FAIL。每后端原59暖不足保留；另预声明辅助1条补足60后，CPU组件p95 22.656s≤30、Vulkan8.672s≤10仅组件PASS。Source.3中性提示/Source.4路线修复未改Prompt、模型、Schema、参数、预算或阈值，不宣称重跑完整语言矩阵。

102单稳定场Development/API双后端：Vulkan6183帧/61.1867968s、p99 16.7755ms、1%Low49.0051591fps、>50ms0；CPU11603帧/129.8656161s、p99 16.5324ms、1%Low51.3488456fps、>50ms2，联合门槛FAIL。完整请求均包含CSV：VulkanHTTP24.407s正常返回后TARGET_REQUIRED→clarify语义FAIL；CPUHTTP120.007s超时→MODEL_UNAVAILABLE/raw空。两次实际1920×1080/D3D12/SM6/Epic3/100/Vsync0，余五稳定场、Shipping/OS联合未验。9000启动parse失败/reparse与Renderer CPU worker AV保留独立，不算六场、不归因为OOM或已知GPU错误。

## 历史截点（已被后续验证更新）

以下仅绑定各原候选或原轮次，不作为当前“尚未”状态：

|历史|实际证据与更新|
|---|---|
|首Cook/候选1|Shipping C++成功但GameFeatureData规则缺失，Cook2E1W/UAT25；[首FAIL](../../qa/TASK-103/FIRST_COOK_FAIL.json)原件保留|
|候选2/.1|空扫描目录/AlwaysCook微修后Shipping成功124.78s。旧Return默认New game误作Continue已[撤回](../../qa/TASK-103/OS_CONTINUE_ERRATUM.json)；101 readonly原档副本最新营地Load/第三进程明确鼠标Continue仍7节点，仅绑定.1|
|候选3/.2|Shipping成功307.11s。CPU隐藏CMD启动+OS新游戏/J/F6保存1→2/中文粘贴/退出；跟随请求144.443s观测FAIL，精确HTTP/raw NOT_READ。新游戏恢复提示文案RED触发.3中性修复|
|候选4/.3|Shipping成功373.90s、install5资料/17events12WAV9许可/water配对/BOUNDED_TEXT_CLEAN。双CMD隐藏脚本启动+OS实见.3，CPU新游戏/Settings/保存1→2；Vulkan显式Continue同卧室/F6仍2/中性提示、单一ngl16跟随proposal→鼠标确认→HUDFollowing，全PID退出。39.122/60.985s为观测时刻，Explorer双击/IME/全路线不P；[4 OS安全摘要](../../qa/TASK-103/OS_CANDIDATE4_PARTIAL_NORMAL_INPUT.json)。ZIP暂停，Stage/archive与5源资料snapshot完整保留；[4原档副本兼容](../../qa/TASK-101/OS_CANDIDATE4_ORIGINAL_COPY_COMPATIBILITY.json)列4旧nodes/最新户外营地准备任务旧节点真实Load→手工5→独立鼠标Continue仍5，6图；5旧档/四新阶段/全字段不P|
|Nav03—05|artifact/普通follow实际取得；远端终点路径无效。04 busy=true，05实际readiness2.5545532s后busy=false但goal投影false；无普通步行/碰撞原因结论。稀疏HTTP入力和tick02 PY安全拒绝原件保留，未绕过策略|
|Nav06/07|顶40.0558cm与侧门38.0887cm实到仍同指引的RED，分别由.4两项Source修复及Native/后续API验证更新；[06](../../qa/TASK-103/NAV06_GUIDANCE_FIRST_BLOCKER.json)/[07](../../qa/TASK-103/NAV07_GUIDANCE_FIRST_BLOCKER.json)独立|
|Nav08|四节点完成，侧门交回普通world；终点FindPath valid=false/partial=false，goal投影false，无末段移动。[08摘要](../../qa/TASK-103/NAV08_FINAL_TARGET_FIRST_BLOCKER.json)|
|Nav09—11|100/100/220与独立250/250/220候选投影false；11只读屋顶22413.1097/真实地形21871.2367/宽Z1000 Nav22419.4832，宽Z点未移动。QA插值Z未贴地形，由12实际地形查询纠正并前进19.77m；[09—11历史](../../qa/TASK-103/NAV09_11_PROJECTION_HISTORY.json)|

早期原生批次的真实FAIL与后续定向修复见各原index：TASK-085-088/native-baseline-20261007、TASK-085-099/native-first-20261007、TASK-088-101/native-integration-20261007、TASK-090-100/native-red-20261007、native-green-20261007。旧截图/源码as-run/日志/用户原档均保留；新的通过不篡改旧raw。4Stage递归删除自动审批拒绝后root停止清理并完整保留，5Stage移E，未把清理拒绝伪称成功。

## 需要Owner与尚未取得的验收资源

- 094三类风格及衍生首件/未知来源许可；095角色/武器/动作、096石堡近场材质/有限火烟、097自然/动物/作物衍生、098设施/道具/图标首件；077石堡方向已有确认，不重问。
- 100一区空间差异化首件；四区现入口与重复房屋不登记为已完成新美术空间。
- 099实际听感、固定录音UNPRODUCED时外部字幕预览是否接受；动态回复无TTS。
- 固定模型双后端质量FAIL、102联合性能FAIL；Owner已批准离线模型对照；Qwen3-4B-Instruct-2507原60为26/60、受限0/20、明确26/40、行为22/30、边界20/20，质量FAIL。8B的32/37层配置分别仅13题11正确、5题5正确；37层四个暖请求36—38秒，按固定60暖样本p95门槛提前拒绝。部分诊断不算完整质量成绩，正式模型未替换，阈值不变。

未知Content逐项许可、真实正常路线/IME与全部UI、完整六场/Shipping性能是工程验收缺口；真人首切片/全流程/阶段战斗和第二实体机是实际样本缺口。分层保持NOT_RUN，不泛化成全部需设计裁定。

正式validate_repo.py --task --base需要含获批任务快照的真实基线；参考HEAD不含084—103快照，保持NOT_RUN，不修改验证器或伪造SHA。独立Reviewer、提交/推送/合并/Release均未发生。本次用户的本地授权和最小文件范围见任务notes；任务Active，NOT_PUBLISHED。

Root最终默认仓库校验已实际通过：104份任务快照、0错误；git diff --check退出0。首次18处错误仅来自私有临时旧草稿的相对链接，草稿内容保留并归档为文本快照，原失败记录保留。此次检查不替代UE、玩法、模型或正式task/base验收，原生与模型测试未重复。[实际检查记录](../../qa/TASK-103/FINAL_REPOSITORY_VALIDATION.json)。
