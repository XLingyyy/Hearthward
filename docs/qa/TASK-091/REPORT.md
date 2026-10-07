# TASK-091 本轮记录

当前状态：Active。生产.4两项指引修复的四次独立同case Native红绿已验；Development/API实际四节点完成。Nav12确认地形后首段普通PathFollowing移动1976.913883cm，第二段FindPath valid=true/partial=true在SimpleMove前停止。自有Editor退出true，完整撤离/营地/首救/新保存/正常OS路线仍未完成；地图或碰撞根因未定。 候选5/version.4实际Shipping Build/Cook/Stage/Archive成功；本单OS路线和视觉签收未完成。共享分支codex/TASK-084-103-iteration，参考HEAD 6fcf5c22e965f0f7409438f19bc7b09e96ffb058加未提交差异；未提交、推送、合并或发布。

HUD金色终点与青色途经点、地图独立途经标记已接入。HearthwardGuidanceRoutes按正式阶段、已加载堡垒、真实地面和当前位置派生节点；首救仅提示已发现route_fork。返走重新解析，不新增Save节点进度，不改Campaign事实、奖励、传送或设计目标。

## 实际原生证据

完整filter Hearthward.Iteration.Task091.LoadedFortressSpatialNodes始终只有1条；下列四份报告独立，不合成2条或一次全套GREEN。两份GREEN各有同目录build.json：独立public build.project的HearthwardEditor/Win64/Development、ok=true/dry_run=false/returncode0。

|独立轮次|原报告时间|同一用例结果|原index目录（.agent-local/qa/TASK-091/）|
|---|---|---|---|
|楼梯顶RED|2026.10.07-01.35.33|1Fail / 0warning / 1error|native-stair-top-20261007-red/native/index.json|
|楼梯顶GREEN|2026.10.07-01.39.30|1Success / 0warning / 0error|native-stair-top-20261007-green/native/index.json|
|侧门RED|2026.10.07-01.44.03|1Fail / 0warning / 1error|native-side-gate-20261007-red/native/index.json|
|侧门GREEN|2026.10.07-01.45.37|1Success / 0warning / 0error|native-side-gate-20261007-green/native/index.json|

楼梯顶RED期望bottom而实际仍top；修复限于既有楼梯入口条带的切换条件。侧门RED唯一失败为到达后Visible期待false；修复仅在原目标三维距离≤50cm内结束侧门途经点，100cm内侧仍保留指引。测试使用真实生成堡垒UWorld中的位置参数，保护解析及Campaign快照不变，不提供OS移动信用。早期[GREEN子集](native-green-20261007.json)1/1、0warning/error及[首次记录](native-first-20261007.json)保留各自时间与受测实现，未挪作本轮分母。

## 实际普通API路线与历史边界

|轮次|已结束的实际观察|结论边界|
|---|---|---|
|06|楼梯顶40.0558cm到达仍同ID/坐标|原RED，后续楼梯顶修复已更新|
|07|顶→底已切换；侧门38.0887cm到达仍同ID/坐标|原侧门RED，后续侧门修复已更新|
|08|四NODE_REACHED；侧门交回world终点。終点FindPath valid=false/partial=false、goal投影false|未开始末段移动，不能定普通步行或碰撞原因|
|09|预声明100/100/220、首≤2000cm QA候选投影false|没有末段move；插值候选不等于真实地面|
|10|独立预声明250/250/220仍false|窄XY假说未获支持，无失败后扩张|
|11|只读屋顶ImpactZ22413.1097、忽略已确认Home后地形21871.2367；宽Z1000 Nav22419.4832（候选+252.3478）|没有移动宽Z点；QA插值Z未贴地形，不改地图|
|12|实际地形+Z100、固定XY250/Z220；首段完整query/controller路径普通移动1976.913883cm；第二query valid=true/partial=true、双端投影true|严格停在SimpleMove前、自有退出true；完整撤离/营地/首救/新保存未到达|

09—11原对象见[安全历史](../TASK-103/NAV09_11_PROJECTION_HISTORY.json)；12最新原值见[安全摘要](../TASK-103/NAV12_GROUNDED_FINAL_FIRST_BLOCKER.json)，各private results.json原件完整保留。第二段不同高度层的解释未验证；不写普通步行必不可达。12移动后侧门指引再次可见也如实保留，正式world终点未被QA替换。整个系列属于Development/API，未写人物位置/速度/MovementMode/时钟/原任务进度，也不提供Shipping正常OS路线信用。

## 验收边界

|用例|实际证据|未验范围|
|---|---|---|
|T091-C01 坐标互逆|本单节点夹具不测试MapPoint/MapWorldAt互逆|原082映射在本轮实现上的坐标/缩放回归NOT_RUN|
|T091-C02 卧室转折|已加载堡垒节点/真实高度/门点胶囊碰撞原生Success；Nav08—12普通API四节点实际到达/切换；Nav12末段首段19.77m实际移动|卧室至楼梯/庭院侧门正常OS输入实走与全路线可达性NOT_RUN|
|T091-C03 偏离返走|离开适用区退回目标、返走重新求门点原生Success|另一合法路线、取消追踪再恢复的正常操作NOT_RUN|
|T091-C04 时间线|正式Phase改变/无关Quest/非撤离目标清除节点，Campaign快照不变原生Success|ActualLoad、领奖、合法传送后重新开图及完整首救路线NOT_RUN|
|T091-C05 传送|本单未改现行权限、Campaign和奖励|各拒绝条件/地图暂停/伙伴集合/落点正常操作NOT_RUN|
|T091-C06 布局|HUD/地图消费者已接入并通过本轮build|4:3/超宽/字号/缩放/密集标签渲染与Owner可读性NOT_RUN|
|T091-C07 降级|空world、无关任务、阶段不适用原生Success|缺/坏配置与定位对象失效的运行注入NOT_RUN|

本单原生测试未覆盖已发现route_fork的完整首救段，也未覆盖等待获救者、取消outbound路线和未知目标的所有UI路径；这些由对应本轮路由/目标测试另行登记，不并入本单1条成绩。092几何如再变动，只重测受影响节点与映射。

真实OS输入、正常UE实走、渲染、Owner/真人、二机、Shipping、联合性能与发布均未由此夹具验收。范围基线检查NOT_RUN：没有包含获批任务快照的实际提交，不用参考HEAD代填，也不修改验证器。README由root单写者统一收尾。

当前候选5由root实际冻结/打包，Source仅上述两项指引修复及Header.4；Shipping构建成功不补本单OS、渲染、首救、传送、Owner/真人或第二机器结论。范围基线验证仍需包含本任务快照的真实提交，未以参考HEAD代填。README与最终Source生命周期归root统一收尾。
