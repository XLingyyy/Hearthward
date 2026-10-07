Hearthward（归火）内部候选 — 0.2.0-preview.20261007.4
CandidateID: iteration-084-103-20261007-5
交付状态: BUILD_SUCCESS / OS_PARTIAL_VERIFIED / ZIP_EXTERNAL_RECORD_PENDING / INTERNAL_NOT_PUBLISHED

这份候选包含工作状态、制作搜索与缺料追踪、装备和仓储预览、阶段准备、任务途经点、营地反馈，以及真实事务、战斗、脚步和已加载湖水的声音事件。
候选5/version.4已在准确冻结Source上实际公开Shipping Build/Cook/Stage/Archive成功：UAT0，总235.52秒；Build71.70/Cook63.43/Stage39.73/Archive58.81秒，Cook0错误/1条MCP EULA提示保留。两个CMD的脚本启动与OS局部输入已验证；最终闭合/ZIP/hash由包外实测record及sidecar提供。候选4/version.3的构建与局部OS证据保留为历史；导航06实际到达楼梯顶指引节点后，指引ID和坐标没有切换，root已暂停候选4 ZIP。候选5包含楼梯顶切换与侧门三维到达圆的两项窄修复，不新增地图或设计目标。两次独立同case Native均已RED→GREEN；Development/API实际四节点完成。本包Shipping构建与OS开局/保存继续局部验证成功，完整路线仍未通过。准确冻结和实际结果以随包BUILD-INFO.json为准。运行树内资料属于ZIP制作前的实测元数据快照；最终ZIP及其单次SHA256由外部finalization-record与sidecar记录，包内资料不自填最终ZIP哈希。
本版沿用中性AI重置提示。候选5正常新游戏和显式鼠标“继续游戏”后均实际观察中性交流提示；候选4历史证据另留。固定AI contract继续绑定Source.2，Prompt、模型、Schema、参数、预算与质量门槛未变。

运行（完成打包核验后）
保留完整Windows目录，在可写目录二选一运行Start-Hearthward-CPU.cmd或Start-Hearthward-Vulkan.cmd。两项直接启动随包Hearthward.exe，无需UE或Python。
两个启动器均传GpuLayers=16；Vulkan使用16层，CPU后端实际0层。程序默认CPU，切换后端前先关闭游戏。
两个启动器共用本候选专用档：%LOCALAPPDATA%/Hearthward/Candidates/iteration-084-103-20261007-5。该目录独立于其他候选、原有档和QA档，不自动复制旧档；可继续本候选自己的档。
候选5两启动脚本已实际通过隐藏命令进程启动＋正常OS游戏输入，状态SCRIPT_LAUNCH_AND_OS_GAME_INPUT_VERIFIED；Explorer双击仍NOT_RUN。不要只复制exe；Resources、模型、运行库、Pak/IoStore和许可证均需保留。若缺VC运行库，核对随包Engine/Extras/Redist/en-us。

默认操作
WASD移动，鼠标转视角，Shift冲刺，空格跳跃，E交互/符合条件时攀越。
X弟弟跟随，Z等待，C弟弟进攻；Tab背包，T交流，R营地仓储。
B建筑目录，Q旋转建造预览，左键确认，右键取消；靠近工作台按E制作。
J任务，K技能，M地图，F6存读档，P暂停/设置，Esc返回。
鼠标左键攻击、Shift配合重击；右键格挡/瞄准，Alt闪避，中键锁定。
1药品，2食物，3弓箭栏，4投掷栏；地点/传送遵守发现和安全条件。若改过键位，以当前绑定为准。

建议内部检查路线（候选5构建成功，实际路线未验；候选4开局局部历史已测）
新游戏从石堡卧室开始，靠近遗物包按E，叫弟弟跟随，按任务指引撤离。到营地按J看准备条件，实际采集建工作台，检查制作搜索、材料追踪和真实仓储转移。
交流确认明确任务后查看个人/队伍状态；停工、恢复和取消按当前卡片提示操作。联系首位获救者并实际回营，等待时目标应指人物当前位置，恢复跟随后指营地。
F6保存、正常退出、重启后明确鼠标点击“继续游戏”，核对保存节点和物品状态。不要用Return默认选项推定Continue。

已测范围与当前限制
既有099 Native13定向原生23/23成功，连同既有战斗/生存/存档5项共28/28成功，0测试警告/错误；13条frame0启动Smoke错误和1条MCP启动告警单列。该轮Development Editor使用NoSound/NullRHI，不提供Shipping实听或完整路线信用；候选4历史局部OS检查另列，不迁移为候选5通过。
声音配置为17个事件、12个独立运行WAV。实际消费包括仓储、玩家/NPC成功事务、营地进度、hit/block/damage/有效空挥及真实脚步。落地与入出水回调已接入观察，专用声音cue仍未绑定。
水声只关联当前世界已注册、开始运行且可见的目标湖水组件，按98顶点/96三角的实时变换求水面最近点；30米内至多一个循环源。暂停、死亡、读档、卸载及退出清理源；连续PCM原生通过不证明实际设备接缝或混音听感。
AI质量结果继续绑定Source.2/version.2实现：CPU原始理解33/60、受限3/20、明确端到端33/40、执行25/30；Vulkan分别32/60、2/20、33/40、24/30。两后端边界各20/20、未观察到白得物品，语言质量均FAIL，自然语言委托未达批准门槛。
独立辅助后的60暖请求组件p95为CPU22.656秒、Vulkan8.672秒，达到各30/10秒限制；不包含UI绘制、确认执行或联合帧性能。102稳定单场1%Low为CPU51.349、Vulkan49.005 FPS，均未达60；完整六场及候选5 Shipping性能未验。
历史候选4已实际通过CPU.cmd正常新游戏、F6保存1→2及退出；Vulkan.cmd显式鼠标继续恢复同一卧室，F6仍为原手动/自动共2节点，两个后端共用候选4 profile。真实输入“跟随我”后，随包Vulkan16层模型生成候选，鼠标确认后回复“好，我跟着你”，HUD显示跟随中；该单条Shipping CASE成功，不代表完整60语言或性能门槛通过。CPU4没有重复模型请求，候选3/version.2 CPU单条失败或超时保留为历史；中文粘贴不提供真实IME信用。候选5双CMD已实际通过隐藏脚本启动＋正常OS游戏输入，状态SCRIPT_LAUNCH_AND_OS_GAME_INPUT_VERIFIED；双标题实见0.2.0-preview.20261007.4，共9张真实游戏窗口截图。新profile运行前不存在，未复制旧档或QA档。CPU新游戏卧室/main01，T中性提示，F6自动节点1后鼠标手动保存至2；Vulkan明确鼠标点击继续游戏恢复同一卧室/main01，F6仍为原手动02:26/自动02:25共2节点，T中性提示通过。两后端正常退出后各游戏与bootstrap进程已不在，未遗留模型进程。本轮没有模型请求、中文字面输入、真实IME、完整路线或旧档兼容样本；这些项目仍未验。
路线证据分层记录：导航06到楼梯顶40.056cm仍同指引、07到侧门38.089cm仍同指引，分别复现两项既有门槛冲突。091同一用例各自独立RED1Fail/0warning/1error→GREEN1Success/0warning/0error，四个原始报告不合分母。导航08—12已实际通过卧室门、楼梯顶、楼梯底、侧门的普通API到达与切换。导航09—11的末段插值候选Z未跟随地形，原投影失败及只读诊断保留；12只用实际确认地形加Z100作QA查询，真实完整路径普通移动首段1976.914cm，第二段FindPath valid=true/partial=true后停止，未移动该段。完整撤离、营地、首救与新保存仍未通过，普通步行受阻或碰撞根因未定。该Development/API证据不提供OS路线信用；未改地图、Invoker、时钟、位置或进度。
连续首切片、四新保存节点、全主线/15支线、通关后生活、完整交易制作、Shipping语言、真人和第二实体机尚未验收。角色/武器、石堡、自然作物与设施图标首件及音效听感待Owner；固定录音UNPRODUCED，动态回复无TTS，字幕预览未签收。

来源与交付边界
声音候选来自Kenney RPG Audio（CC0）及RandomMind Sea and river wave sounds的VistulaShort（CC0）。许可证应随Hearthward/Resources/Audio/TASK-099/License-Kenney-RPG-Audio.txt和License-RandomMind-Vistula.txt保留。
字体、模型与运行库许可也须保留；候选4 install已核9项许可文件存在及水网格/来源报告配对，不证明全部Content来源闭合；候选5最终文件清单/有界扫描/ZIP结果由包外record另记，TASK-094既有Content未知来源继续记录。音源来源和原生通过不代表Owner已实听通过。
技术诊断设备为Windows11、i7-13650HX、RTX4060 Laptop、16GB内存、UE5.8.2。完整证据与旧包历史见项目docs/qa/TASK-103/REPORT.md和对应任务报告。本候选未提交、推送、上传或创建GitHub Release。
