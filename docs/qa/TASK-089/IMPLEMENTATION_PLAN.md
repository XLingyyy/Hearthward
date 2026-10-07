# TASK-089｜定向复现与修复安排

2026-10-07。mcp_setup只读调查；根Agent尚在统一构建，构建结束通知前未改源码。

修改前源码条件证据：.agent-local/qa/TASK-089/20261007-source-before/reproduction.json。仓储格子将装备按Definition聚合，详情取同类首个实例，执行Transfer(kind)，每次创建新OperationId，成功反馈没有MovedCount。运行时RED尚未执行，不能将源码predicate当作游戏测试通过。

最小改动沿现有Snapshot、FindInstance、Available、Transfer/TransferInstance与稳定GUID。仓储装备分件、素材保持堆叠；分类仅过滤显示。原背包InventoryPositions、QuickItems、Save与容器事务不动。只读负重预览在FHearthwardInventoryState副本中走现有转移规则。当前设施/安全状态、源可用量及epoch在执行瞬间复核；一个预览绑定一个OperationId，明确新预览才创建新事务。

优先定向原生：真实widget两件不同耐久的同类装备选择与存取；真实stack预览后数量变化及重复提交；旧epoch/远离仓储拒绝并保留选择。直接覆盖UI实际调用，底层容量/实例交易/Replay可复用既有Storage与047/055测试，避免重复测试实现。

背包详情实现在ScreenMenu.cpp，当前用类型级Durability显示；本任务JSON未列该文件。已向根Agent申请最小增补，由其协调现有会话授权和单写窗口。未经通知不编辑该文件。

参考Epic官方[Lyra库存与装备](https://dev.epicgames.com/documentation/unreal-engine/lyra-inventory-and-equipment-in-unreal-engine)的物品定义/实例状态分离。采用本工程已有实例，不引入Lyra依赖。

所有编译、原生、真实输入、渲染及保存恢复为NOT_RUN，待根Agent通过公开UEClient执行。README由根Agent负责；未提交/未推送/未发布。
