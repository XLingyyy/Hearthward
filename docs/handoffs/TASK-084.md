# TASK-084｜运行调查交接

2026-10-08 窗口异常的原始入口已补测：真实编辑器菜单启动独立进程，1280×720请求在150%缩放下外框1922×1128且完整在屏幕内；实际拖至1612×954后新游戏、HUD、暂停页可见，正常退出成功。第二次菜单启动因Computer Use两次激活失败未执行。证据见[窗口报告](../qa/TASK-084/WINDOW_PREVIEW_20261008.md)，不计完整路线验收。

## 2026-10-07 Nav12：正式节点完成，末段首阻塞保留

安全摘要见 [NAV12_GROUNDED_FINAL_FIRST_BLOCKER](../qa/TASK-103/NAV12_GROUNDED_FINAL_FIRST_BLOCKER.json)。绑定独立 Development/API run `fresh-prologue-navigation-api-20261007-12`，由Root公开UEClient启动/停止，owned editor退出确认。四个正式空间节点 bedroom_door、escape_stair_top、escape_stair_bottom、courtyard_side_gate 均通过真实完整查询路径、controller路径和普通PathFollowing到达。该组是API证据，OS连续路线仍NOT_RUN。

显式grounded末段仅使用实际确认的Landscape命中点加Z100，固定XY250/Z220查询。第一段完整路径和controller路径通过，普通移动1976.913883cm到(89908.000413,58269.965095,21967.389454)，距实际投影点94.313087cm。第二段确认地面Z21965.253550、查询Z22065.253550，实际投影返回Z22150；导航building/locked=false、start/goal投影均true，但FindPath valid=true/partial=true，严格停止在第二段SimpleMove之前。正式world保持(90000,62000,22145.612411)，院门route在末段中再次显示已如实记录。

第二候选位于西北角Tower和TowerFoundation共同XY覆盖；真实射线先命中TowerCornice、忽略精确Home后命中LandscapeStreamingProxy。实际Nav点比地面高184.746450cm，存在另一结构高度Nav层的解释有证据支持，但尚未证明poly归属或partial的原因，不能登记为已确认全路线不可达或几何Bug。没有改Source、Invoker、正式路由或位置；不继续猜点或重跑。原Nav09/10/11证据保持。

撤离交互、自然travel到营地、阶段/退出完成事实、营地保存、首次救援及完整救援往返均未取得本run信用。本单保持Active；该首阻塞作为后续调查项交接，不把局部通过升级为完整验收。

## 开工交接历史

2026-10-07 已接手，状态 Active。实际目录 `G:/GameFactory/Hearthward`，分支 `codex/TASK-084-103-iteration`，基线 `6fcf5c22e965f0f7409438f19bc7b09e96ffb058`，本地未提交。当前用户授权逐个实施；提交、推送和发布未执行。

使用 [BASELINE](../planning/TASK-084-103/BASELINE.md)、[ROUTE_MANIFEST](../planning/TASK-084-103/ROUTE_MANIFEST.json) 和 [REPORT](../qa/TASK-084/REPORT.md)。正常新游戏已进入卧室并实际保存/同进程继续。主线一日志相关地点误标已复现，交由090修正。持续路线和首次救援未完成，不能登记PASS；详见基线逐项登记。085只读投影、086显示修正、094清单可独立实施。

下方为导入任务包的历史模板，保留原始编写背景；本段及上述证据为当前执行记录。

[任务单](../tasks/TASK-084.md) · [元数据](../tasks/TASK-084.json) · [本批指南](../planning/TASK-084-103/EXECUTION_GUIDE.md)

## 当前状态

任务单编写完成；实施未派发；所有工程/运行/人工检查为NOT_RUN。此模板不是执行报告。规划参考SHA：`6fcf5c22e965f0f7409438f19bc7b09e96ffb058`；建议分支`codex/TASK-084-playable-baseline`尚未创建。Owner沿项目记录为XLingyyy，实际执行Agent和Reviewer尚未指派。

## 接手后填写

- 实际根目录、工作树、分支、完整HEAD、dirty清单与授权引用。
- 已获批任务快照所在完整SHA、准确写范围/JSON指针、资产包和锁责任。
- 前置产物的实际SHA及已核对接口；当前阻塞与可独立完成的子范围。

## 实施与证据

记录实际修改、未修改/复用项及理由；每个`T084-Cxx`写命令/步骤、期望、实际、PASS/FAIL/BLOCKED/NOT_RUN和证据路径。注明源码/资产/模型/环境/初始档指纹，不能填历史PASS为本单结果。

## 交付与权限

README同步：NOT_RUN。原生/渲染/真实输入/模型/真人/二机/发行：NOT_RUN。提交：未执行；推送：未执行；合并：未执行；发布：未执行。仅在实际完成对应动作后改写。

## 下一位Agent

列可使用的具体产物、接口版本、剩余缺陷、不可覆盖文件/锁及下一步验证。下游依赖见本单元数据；工程交付不自动替代Owner或真人签收。
