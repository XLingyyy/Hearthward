# TASK-089｜实例详情与仓储转移

2026-10-07，mcp_setup实施，根Agent协调。根目录`G:/GameFactory/Hearthward`；实际分支`codex/TASK-084-103-iteration`；基线完整HEAD`6fcf5c22e965f0f7409438f19bc7b09e96ffb058`。当前为该HEAD上的未提交实现，未修改Content、模型锁、Save或底层库存事务；运行环境/资产/模型与初始档由根Agent实际执行时补充。用户本会话授权逐单本地实施；README及UE生命周期仍归根Agent。

## 修改前证据与最短夹具

源前证据：`.agent-local/qa/TASK-089/20261007-source-before/reproduction.json`。确认仓储按kind合并装备、详情取同类首件、执行Transfer(kind)、每次创建新OperationId、成功反馈没有实际MovedCount。该证据为源码条件复现，运行时RED为NOT_RUN。

双实例短夹具已保存为`Source/Hearthward/Tests/InventoryStorageUXTests.cpp`的InstanceSelection：真实Standalone World/玩家/Bag/ResourceInteraction仓储/Widget，加入两件斧头，分别磨损30和10（耐久50与70）；通过实际第二件GUID选择、存入、重复卡、取出与返回背包详情验证不串件。它是明确诊断夹具，不是玩家档或真实OS输入结果。PreviewAndReplay与AccessAndTimeline另建立真实来源变化、旧卡/距离/战斗诊断；未注入生产成功或放宽距离。

参考Epic官方[Lyra库存与装备](https://dev.epicgames.com/documentation/unreal-engine/lyra-inventory-and-equipment-in-unreal-engine)中定义与实例状态分离的UI实践；实施复用本工程已有GUID和事务，没有新增依赖或容器架构。

## 最小实现与映射

|路径|实际变化与执行口径|
|---|---|
|ScreenStorage|装备格子逐GUID显示，素材保持堆叠；分类仅筛选视图。详情含总量/可用/真实耐久/装备状态，来源变化后不自动选另一件|
|仓储预览|方向、本次数量、可用量与转移后个人负重；在FHearthwardInventoryState副本中复用TransferTo/TransferInstanceTo做容量预览，不修改真实容器|
|仓储执行|预览绑定epoch/OperationId；执行时复查原NearStorage设施、CanChangeSkills、可用数量/实例、容量；沿Transfer/TransferInstance，反馈实际MovedCount|
|重复/新操作|同一成功预览保持处理状态，旧卡不再移动；明确选择、调整数量或新转移才生成新OperationId。旧时间线/旧预览被拒绝|
|ScreenEquipment|实例ID与可用量详情；转交方向及目标负重/容量预览。实例/维修/丢弃/升级卡绑定epoch、owner、选择、数量与会话操作ID；原Gameplay/Workshop复查执行|
|行装重复|UI记录已提交转交的当前卡标识，重复点击拒绝；新转移/选择产生新预览。底层仍调用原TransferInventory，不扩展事务/Save格式|
|背包选择/详情|鼠标已有拖拽入口保留该格的GUID；ScreenMenu仅展示实际所选GUID/耐久/装备状态；正常装备使用走同一GUID与选择epoch校验|
|保留路径|InventorySlots/InventoryPositions、稀疏格子、四快捷栏、原装备/维修/唯一物品丢弃确认、原安全和设施要求不改|

ScreenMenu.cpp原未在候选JSON路径中，根Agent依据当前用户逐单实施授权做最小增补，已登记本单JSON，范围仅明确实例详情和选择入口，不修改槽位布局算法。详细仓储文字支持独立详情翻页，列表滚轮/分页不压缩背包槽位；键位提示沿语义绑定。

## 定向验证与验收限制

定向`git diff --check`：PASS。根Agent通过公开UEClient实际构建成功，并在本轮31项定向批次中找到及执行`Hearthward.Iteration.Task089.`三项：InstanceSelection、PreviewAndReplay、AccessAndTimeline，均为Success，0 errors。三项各有1条Standalone LocalPlayer未初始化PlayerInput导致EnhancedInput设置未加载的warning，共3条，原始警告保留；成功结果仅证明诊断Widget/事务路径。批次其余任务的失败不能归为089结果。

证据：`.agent-local/qa/TASK-085-099/build-repair-20261007/result.json`（returncode 0）；`.agent-local/qa/TASK-085-099/native-first-20261007/index.json`；本单原生子集`native-first-20261007.json`。受测环境为Windows 11 24H2、UE5.8.2、RTX4060 Laptop、Win64 Development Editor；资产/模型未由089更改。注册发现数量为3，执行3/3 Success；运行RED仍为NOT_RUN，修改前最短条件保留于source-before证据。

|用例|本轮覆盖/后续最窄验证|实际结果|
|---|---|---|
|T089-C01 实例唯一|实际InstanceSelection通过第二GUID存/取/详情及重复卡；真实穿戴/维修/OS点击待执行|Native PASS；游戏/OS NOT_RUN|
|T089-C02 稀疏拖放|源码未改槽位算法，鼠标选择增加GUID保留；实际拖换/空格/快捷栏与保存恢复待复验|NOT_RUN|
|T089-C03 数量边界|PreviewAndReplay实际大负调整/来源耗尽拒绝与数量断言通过；容量边界及真实游戏路径待执行|Native PASS；游戏 NOT_RUN|
|T089-C04 预览失效|PreviewAndReplay实际先选3份再真实移除2份，执行拒绝且余量/仓储不变；生产/委托真实并行待执行|Native PASS；并行游戏 NOT_RUN|
|T089-C05 距离与epoch|AccessAndTimeline实际移出设施、AdvanceTimeline、NotifyCombat后点击无移动；真实Load待执行|Native PASS；实际Load NOT_RUN|
|T089-C06 重复提交|PreviewAndReplay实际同一绑定卡只移动一次，明确新转移可用|Native PASS；实际OS NOT_RUN|
|T089-C07 可读可操作|空/长详情、大字号与4:3/超宽需实际渲染及键鼠焦点验证；源/语义元数据不能证明视觉通过|NOT_RUN|
|T089-C08 存档保真|沿原Snapshot/实例事务/Save格式；稀疏布局、完整装备/快捷栏/耐久实际保存独立重启待验证|NOT_RUN|

构建/测试由根Agent执行，本代理不启动或关闭UE。全部089共享源码写锁已释放，后续源修正由根Agent协调。仓库公共检查与含任务快照的范围检查仍待统一执行；旧HEAD未包含任务快照且未获提交权限，不伪造基线或修改验证器。

任务保持Active，无正式review/验收结论。Owner视觉、真人、二机、性能和发行未验；未发现需要人工确定的新玩法设计。未提交、未推送、未合并、未发布。
