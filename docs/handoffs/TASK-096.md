# TASK-096｜独立技术工作交接

## 2026-10-08 近景增量

2026-10-08 近景增量：原创灰石/旧木受光材质、四张1024贴图与2700三角面卧室门框已接入，新增七个准确资产包。门框净宽280厘米、净高380厘米，仅装饰；原碰撞和导航不变。Editor构建及卧室/撤离净空原生1/1通过，正式地图隔离新档的三个游戏视口已检查。夜间卧室入口与回廊地面可辨，梁下和回廊门框背侧仍较暗；没有据离屏截图调亮正式灯光。完整火烟、连续路线、新版Cook与Owner视觉验收未完成，TASK-096保持Active。 证据见[近景报告](../qa/TASK-096/nearfield/REPORT.md)。

以下为本单此前调查与验证记录；旧“待批准方向/暂缓制作/未改Content”的描述以本节更新为准，旧测试只保留其原版本信用。

[任务单](../tasks/TASK-096.md) · [元数据](../tasks/TASK-096.json) · [QA报告](../qa/TASK-096/REPORT.md) · [技术盘点](../qa/TASK-096/TECHNICAL_AUDIT.json)

## 当前状态与范围

Active，2026-10-07。用户授权逐单执行，设计问题可暂缓；主Agent已派发本单独立技术子范围。工作树 `G:/GameFactory/Hearthward`，分支 `codex/TASK-084-103-iteration`，参考 HEAD `6fcf5c22e965f0f7409438f19bc7b09e96ffb058`，含共享未提交改动。独立调查由 route_audit 执行，UE生命周期/构建/运行与根README由主Agent统一。未提交、未推送、未合并、未发布；获批任务快照的真实提交和范围基线验证仍 NOT_RUN。

本单新增 QA报告/TECHNICAL_AUDIT、准确候选包引用及执行文档。未编辑任何 Content、共享源或正式玩法/UI配置。[包范围](../assets/TASK-096/PACKAGE_SCOPE.json)共3个Content候选，`selected_for_change=[]`，锁 NOT_ACQUIRED。未选择新风格资产，未自批Owner视觉。

## 已实施与采用依赖

3个正式石堡立面候选包、原生碰撞/装饰职责、光照关卡与读档回调生命周期已盘点。

机器证据由 `../.venv/Scripts/python.exe -X utf8 docs/qa/TASK-095/audit_bindings.py` 生成，限定实际源码/配置、094登记和PNG头，无资产hash。094样板与来源/许可边界沿用当前 ASSET_REGISTER，来源 UNKNOWN不升级为合规。084 ROUTE_MANIFEST为静态路线/部分运行证据；091尚无新路线替换，092连续自然通行未完成，未冻结碰撞。

## 具体设计暂缓与工程前置

077石质山堡方向已确认；近景局部灯光材质/补面和火烟舒适度首件实机样板待Owner审定，不重复询问石堡/木堡方向。

采用084真实路线，092连续通行与真实地形Z未完成，未冻结碰撞；原生077净空夹具、现有材质只读查询和反复进入/读档计数可独立执行。

## 下一轮最小验证

主Agent已实际运行以下原生过滤器，原始条目见本单NATIVE_REUSE.json；历史报告不移成本轮结果：

- `Hearthward.Hometown077.BedroomAndEscapeClearance` — 本轮 Success，0 warning／0 error。

本单未新增重复配置的测试。完整T096-Cxx逐项均在QA REPORT登记，运行、真实模型、正常输入、Cook和Owner签收分别标记NOT_RUN；静态子检查仅证明其具体范围。依赖下游可采用技术盘点/准确路径与现有行为入口，资产替换需准确包授权、锁和单写者窗口。正式完工和Owner签收仍由主Agent复核。

实际原生：1/1 Success，0 warning，0 error；完整Cxx、正常输入、渲染、Cook和Owner仍NOT_RUN。证据：[NATIVE_REUSE.json](../qa/TASK-096/NATIVE_REUSE.json)，警告逐条原样保留于REPORT。

## 火烟首件更新

2026-10-08 火烟首件已制作并通过隔离PIE可见性与停止消散检查；修复轻量Niagara无材质NormalizedAge输出导致透明度为零的问题。五个包均先锁定再编辑。首件视频已交Owner审样，未接入正式地图，完整任务仍Active。 证据：docs/qa/TASK-096/fire/REPORT.md。
