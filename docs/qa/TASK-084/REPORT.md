# TASK-084 运行调查记录

## 2026-10-07 Nav15：序章至营地 API 流程通过

[Nav15 安全记录](NAV15_EARNED_CAMP_CHECKPOINT.json)：正常 Bootstrap 新游戏、真实遗物交互、跟随指令、四个空间节点、完整终点导航路径、撤离交互与剧情转场均完成。phase=occupied，真实产生 prologue_complete；等待加载结束后保存 accepted=true，存档点1→2，owned editor 退出确认。未直接写角色位置、速度、进度、奖励或时钟。证据属于 Development/API，OS连续输入、首次救援和 Shipping 全流程仍未验。 实际仅生成遗物阶段截图；营地截图请求未在关闭前产出，营地视觉验收不计通过。

末段移动约20米后，真实正式终点已可投影，生成路径 valid=true/partial=false，沿该路径到达，避免继续使用落入角塔范围的合成查询点。改动限于QA脚本，未改变地图、导航范围或正式路由。

[Nav13/14 调查](NAV13_14_INVESTIGATION.json)保留失败：13在启动HTTP响应阶段终止；14完成撤离但保存被拒。15实际读取 loading_after_travel=true，等待现有加载完成后保存通过；加载中的 ExecuteAction 拒绝契约保留。启动等待另只增加对 BadStatusLine 的有界重试。

## 2026-10-07 Nav12：正式节点完成，末段首阻塞保留

安全摘要见 [NAV12_GROUNDED_FINAL_FIRST_BLOCKER](../TASK-103/NAV12_GROUNDED_FINAL_FIRST_BLOCKER.json)。绑定独立 Development/API run `fresh-prologue-navigation-api-20261007-12`，由Root公开UEClient启动/停止，owned editor退出确认。四个正式空间节点 bedroom_door、escape_stair_top、escape_stair_bottom、courtyard_side_gate 均通过真实完整查询路径、controller路径和普通PathFollowing到达。该组是API证据，OS连续路线仍NOT_RUN。

显式grounded末段仅使用实际确认的Landscape命中点加Z100，固定XY250/Z220查询。第一段完整路径和controller路径通过，普通移动1976.913883cm到(89908.000413,58269.965095,21967.389454)，距实际投影点94.313087cm。第二段确认地面Z21965.253550、查询Z22065.253550，实际投影返回Z22150；导航building/locked=false、start/goal投影均true，但FindPath valid=true/partial=true，严格停止在第二段SimpleMove之前。正式world保持(90000,62000,22145.612411)，院门route在末段中再次显示已如实记录。

第二候选位于西北角Tower和TowerFoundation共同XY覆盖；真实射线先命中TowerCornice、忽略精确Home后命中LandscapeStreamingProxy。实际Nav点比地面高184.746450cm，存在另一结构高度Nav层的解释有证据支持，但尚未证明poly归属或partial的原因，不能登记为已确认全路线不可达或几何Bug。没有改Source、Invoker、正式路由或位置；不继续猜点或重跑。原Nav09/10/11证据保持。

撤离交互、自然travel到营地、阶段/退出完成事实、营地保存、首次救援及完整救援往返均未取得本run信用。本单保持Active；该首阻塞作为后续调查项交接，不把局部通过升级为完整验收。

## 开工早期历史记录

受测基线和逐项边界见 [BASELINE](../../planning/TASK-084-103/BASELINE.md)。本轮状态 Active，完整连续体验尚未验收。

2026-10-07 使用公开 UEClient 启动真实 Development 游戏窗口，独立 UserDir/存档测试池；实际点击新游戏、任务日志、存档、保存、返回主菜单、继续游戏。已确认主线一日志地点误标，证据见 `baseline-journal.jpg`，后续由TASK-090修正。保存界面从1/50增加至2/50并提示快照已保存；继续进入同一序章地点。未对旧用户存档进行操作。

本轮启动二进制来自开工时现有构建，标题版本 `0.2.0-preview.20261006.2`；新增085/086源码尚未构建，不能以这组截图证明新增功能通过。源码基线 HEAD `6fcf5c22e965f0f7409438f19bc7b09e96ffb058`，本地未提交。路线任务、模型矩阵、性能、真人、二机及发行验收均未覆盖。

上述早期运行当时仅完成卧室阶段；持续移动输入尚未完成，不将未走通登记为已确认游戏Bug。正式路线表的地形Z/动态设施GUID保持待捕获。详见基线表中 PARTIAL/NOT_RUN 登记。
