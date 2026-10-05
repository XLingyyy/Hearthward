# TASK-072 本机Shipping工程诊断

2026-10-05，当前未提交源码/资产批次，未冻结TASK-074正式RC或发布。公开UEClient.build.package使用Shipping/Hearthward及Bootstrap/Natural两图；独立F:/uagent-task-temp/task072-shipping-diagnostic-20261005，全部Build/Cook/Stage/Pak/IoStore/Archive成功、宿主总耗时6分12秒（UAT自报BuildCookRun360.83秒），Cook0error/0warning。原始返回值shipping-diagnostic-package-result.json、完整日志shipping-diagnostic-package-green.txt保留。UAT转发日志有一行已含12个U+FFFD，未重复输出该行；可读阶段、Cook错误数和Exit0记录不受影响。独立质量复核见shipping-diagnostic-independent-review.md。

G只余约1.326GiB，而Game Development SharedPCH已实测2.153GB；登记后仅在原先不存在的Hearthward/Shipping、UnrealGame/Shipping及A3GamePlayable独立plugin Shipping叶目录创建junction，解析目标全部属于本次F根。没有搬动/删除既有Development产物。host TEMP/TMP/UBA_ROOT仅指F，系统环境不改。Binaries/PDB/receipt/UHT仍有G写入，完成后G约0.816GiB；没有磁盘失败或“全部外置”信用。准确准备记录shipping-junction-preparation.json。

本次真实内层Shipping exe为Archive/Windows/Hearthward/Binaries/Win64/Hearthward-Win64-Shipping.exe、169228800字节。通过public runtime.launch_packaged持有实际PID43772，独立poolfb3b0aca-8dc1-46d6-b084-33e6267d7f1b。Shipping的SaveTestPool和ExecCmds被编译关闭；本次只用独立绝对UserDir，并在正常new之前经公开GetProjectSavedDirectory确认实际路径完全等于own UserDir/Saved，原始actual/expected在results中。session ini显式bAutoStartWebServer=True及RCWebControlEnable，精确22函数、childfalse、AllowAnyfalse；列入调用200/同类未列入只读函数明确400。没有使用console权限或CSV开关。

正常Bootstrap Title→public ExecuteAction new→Loading false→正式Natural HUD实际通过，World/Pawn/PC/HUD/Screen/Companion/Save/AI引用全部取公开真实返回值。Natural enabled、一个玩家/一个伙伴，AI busy=false/ready=false/gen0/server0。host0/REFERENCE_SMOKE_PASS，public stop_ok，Root核实PID43772已退出；该次finally只stop，没有normal_quit_return，不给正常或物理退出信用。原始launch/results/stop/HTTP/session ini保存shipping-reference-green-*。

首次引用smoke仅生成本池SaveGames/HearthwardPrototype/Compatible-v8/pool.hws169543字节，当时没有执行显式手动save或Continue，文件存在只计路径和生成证据。后续实际独立保存继续结果如下。模型请求和CSV均NOT_RUN；Shipping CSV默认已编译关闭，不用Development帧值替代。未提供第二机器、未验证非管理员/断网/中文空格路径/安装升级/最终完整内容。

本结果证明当前批次可构建并在本机从Shipping正常入口切到正式地图；TASK-072性能/兼容及TASK-074正式发行仍Active。完整性能、第二机器和正式部署继续保留未验，不改模型/画质/线程/预算/资产许可。

2026-10-05独立Shipping手动保存／重启继续：同一已归档内层exe，writer pool34f4495c-5604-4432-9d9a-b6fbee42a8ff/PID45404，正常new之前公开Saved目录精确匹配唯一own UserDir。正式图SavePoint(true)实际成功，GetPoints从1到2，新节点Manual=true、同CampaignId，Created.Ticks=639267533724500000且唯一最新；真实pool.hws338704字节。没有物品marker、直接定位、fixture或安全覆盖。

writer结束并由Root exactPID核实退出后，reader另池6b63a78e-b551-4e8f-9a0c-b2090091b496/PID46432复用仅该writer UserDir。正常continue之前公开LoadPointIndex/GetPoints从磁盘读取同一唯一最新Manual SaveId、CampaignId和Created；经正常Title ExecuteAction continue、Loading false进入Natural HUD，公开CampaignId相同、GetStatus精确为“世界与知识已恢复；旧时间线请求已废止”，2节点仍在、AIgen0/server0。writer/read均host0，状态SHIPPING_SAVE_WRITE_PASS/SHIPPING_SAVE_READ_PASS，公共quit返回true/stop_ok且Root exactPID核实两个进程均退出。没有现场区分自然退出或public stop结束，不给正常自然退出信用。原始两组launch/results/stop/HTTP/session ini及独立进程核实保存为shipping-save-{write,read}-green-*。

当前没有CurrentSaveId公共getter；所获有限证据是磁盘中唯一最新手动节点、正常Continue的实际选点调用路径以及成功Restore与同进度ID。完整库存／装备GUID／跨世界权威逐项核对、物理键鼠操作和性能不由本项替代，inventory_verification=NOT_RUN。TASK-072/074保持Active。

两池实际HTTP与results的独立质量复核通过，见[独立保存继续审阅](shipping-save-roundtrip-independent-review.md)。当前最终仓库元数据检查75份task、0错误；git diff --check通过（仅既有CRLF归一化提示）。这些静态结果不增加玩法或性能信用。

本轮实际Game/Shipping生产源码与资产现归档为tested_commit=aa174675e9bf2f3de797fde7cb6795c1152a9d1b；运行当时为此提交的本地施工快照。提交后的证据绑定只改文档，不改已打包的Source/Content/Config/Resources，也不补计未运行的模型/帧/完整库存项目。
