# TASK-095｜独立技术工作交接

## 2026-10-08 当前进展

射手已替换Campaign运行角色，专用27骨骨架与4段动作已导入；0.6秒可见前摇经Owner批准，受击/读档取消与单次出箭原生通过。Editor构建成功，3项定向原生3/3通过、零警告；重开UE后的隔离PIE记录66帧和两箭，材质绑定、骨骼用途与实际颜色已核对。见[射手运行报告](../qa/TASK-095/archer-runtime/REPORT.md)。其他角色服装、其余武器持握、倒地/扶起、完整移动/昼夜验证、实际Cook与Owner视觉未完成，TASK-095保持Active。以下旧调查按日期保留。

## 2026-10-07 已批准方向后的进展

方向已由DSGN-004批准。已制作兄弟、守卫、射手、重兵五份可编辑源样板并渲染；骨架头尾和层级保持。射手仍带原网格连体短刀/盾，须继续清理，持握/拉弓/倒地/扶起和UE替换未完成。见samples/source-results.json。

以下为本单此前调查与验证记录；旧“待批准方向/暂缓制作/未改Content”的描述以本节更新为准，旧测试只保留其原版本信用。

[任务单](../tasks/TASK-095.md) · [元数据](../tasks/TASK-095.json) · [QA报告](../qa/TASK-095/REPORT.md) · [技术盘点](../qa/TASK-095/TECHNICAL_AUDIT.json)

## 当前状态与范围

Active，2026-10-07。用户授权逐单执行，设计问题可暂缓；主Agent已派发本单独立技术子范围。工作树 `G:/GameFactory/Hearthward`，分支 `codex/TASK-084-103-iteration`，参考 HEAD `6fcf5c22e965f0f7409438f19bc7b09e96ffb058`，含共享未提交改动。独立调查由 route_audit 执行，UE生命周期/构建/运行与根README由主Agent统一。未提交、未推送、未合并、未发布；获批任务快照的真实提交和范围基线验证仍 NOT_RUN。

本单新增 QA报告/TECHNICAL_AUDIT、准确候选包引用及执行文档。未编辑任何 Content、共享源或正式玩法/UI配置。[包范围](../assets/TASK-095/PACKAGE_SCOPE.json)共127个Content候选，`selected_for_change=[]`，锁 NOT_ACQUIRED。未选择新风格资产，未自批Owner视觉。

## 已实施与采用依赖

14段主角/弟弟绑定文件存在；13 weapon ID与石斧挂点/权威播放时钟已盘点，专用射手/族人和倒地/扶起/拉弓槽缺口已定位。

机器证据由 `../.venv/Scripts/python.exe -X utf8 docs/qa/TASK-095/audit_bindings.py` 生成，限定实际源码/配置、094登记和PNG头，无资产hash。094样板与来源/许可边界沿用当前 ASSET_REGISTER，来源 UNKNOWN不升级为合规。084 ROUTE_MANIFEST为静态路线/部分运行证据；091尚无新路线替换，092连续自然通行未完成，未冻结碰撞。

## 具体设计暂缓与工程前置

角色服装/身份轮廓、专用射手/族人、首线武器和倒地/扶起/拉弓动作首件样板待Owner确认；不自行更改既有比例或风格。

实际持握入口HearthwardCharacter.cpp和角色入口CampaignActor.cpp不在095允许路径；需要修改时先由主Agent扩大到准确文件。现有石斧3条原生测试和UE只读骨架查询可独立执行。

## 下一轮最小验证

主Agent已实际运行以下原生过滤器，原始条目见本单NATIVE_REUSE.json；历史报告不移成本轮结果：

- `Hearthward.Equipment055.HeldAxeFollowsCurrentInstanceAndMode` — 本轮 Success，0 warning／0 error。
- `Hearthward.Equipment055.StoneAxeLightUsesClipPhaseAndPhysicalBlade` — 本轮 Success，0 warning／0 error。
- `Hearthward.Equipment055.StoneAxeHeavyUsesCurrentMoveAndPhysicalBlade` — 本轮 Success，0 warning／0 error。

本单未新增重复配置的测试。完整T095-Cxx逐项均在QA REPORT登记，运行、真实模型、正常输入、Cook和Owner签收分别标记NOT_RUN；静态子检查仅证明其具体范围。依赖下游可采用技术盘点/准确路径与现有行为入口，资产替换需准确包授权、锁和单写者窗口。正式完工和Owner签收仍由主Agent复核。

实际原生：3/3 Success，0 warning，0 error；完整Cxx、正常输入、渲染、Cook和Owner仍NOT_RUN。证据：[NATIVE_REUSE.json](../qa/TASK-095/NATIVE_REUSE.json)，警告逐条原样保留于REPORT。
