# TASK-089｜本地实例与转移实现交接

[任务单](../tasks/TASK-089.md) · [元数据](../tasks/TASK-089.json) · [报告](../qa/TASK-089/REPORT.md)

## 当前状态

2026-10-07。mcp_setup实施、根Agent协调；根目录`G:/GameFactory/Hearthward`；分支`codex/TASK-084-103-iteration`；完整基线HEAD`6fcf5c22e965f0f7409438f19bc7b09e96ffb058`上的dirty实现。前置085/086和088当前本地接口已消费。根Agent已实际构建成功，089三项Widget/事务Native全部Success，0 errors，Standalone EnhancedInput warning各1条；真实游戏、OS输入、渲染和独立恢复未验。

## 实际产物与窗口

仓储装备逐件GUID选择，详情取所选真实实例；材料仍堆叠。转移方向、当前可用和转移后个人负重按真实InventoryState副本预览；提交绑定epoch/OperationId，原Transfer/TransferInstance负责原子执行，反馈实际MovedCount。重复成功卡不再移动；明确新预览生成新操作，缺失所选实例不自动转移另一件。

行装操作卡绑定owner、epoch、选中GUID/stack、数量和操作标识；实际执行仍沿原Gameplay/Workshop。UI转交记录当前已处理卡，避免重复点击结算；双方距离/设施/安全由原业务复查。缺失弟弟背包明确拒绝，保持当前源身份。背包鼠标选择保留原格GUID，详情与装备使用沿所选实例；稀疏位置/四快捷栏/Save格式及原维修/丢弃确认不改。

根Agent在当前逐单授权下增补ScreenMenu.cpp用于明确实例详情与选择，已登记089 JSON，未改槽位算法。全部089源码写锁已释放给根Agent接090/091；内部只读核对提出的空弟弟Bag/已选GUID离开容器/TestNotNull三处已获根Agent临时窗口修正并再次释放。后续不得覆盖root共享UI修改。

## 实施与证据

源码条件复现为`.agent-local/qa/TASK-089/20261007-source-before/reproduction.json`，未宣称游戏运行RED。双实例短夹具保存在InventoryStorageUXTests.cpp：同类斧50/70耐久，通过真实Widget选择第二GUID、存取/重复旧卡及返回背包详情；其他两项覆盖源变化/明确新预览和原访问/时间线/战斗限制。

过滤器`Hearthward.Iteration.Task089.`，实际找到和执行3项：InstanceSelection、PreviewAndReplay、AccessAndTimeline，3/3 Success。定向diff --check通过。证据为`.agent-local/qa/TASK-085-099/build-repair-20261007/result.json`和`native-first-20261007/index.json`，本单子集见`docs/qa/TASK-089/native-first-20261007.json`。保留全部3条EnhancedInput warning；C01—08分层与限制见REPORT，实际OS拖放/渲染/独立保存重启尚未验。

## 交付与权限

README根Agent收尾。无新玩法设计确认项；任务保持Active，无正式review或验收结论。范围检查旧HEAD未包含任务快照且未获提交授权，明确NOT_RUN，不改验证器。未提交/未推送/未合并/未发布。Owner视觉、真人、二机和发行均未验。

## 下一位Agent

本单构建及3项Native已完成，无本单失败待修。根Agent按REPORT复验不同耐久装备操作、原稀疏拖放/快捷栏、预览变化/重复点击、远离仓储/真实Load、字号与独立恢复。诊断Widget结果不代表实际输入/恢复层通过。
