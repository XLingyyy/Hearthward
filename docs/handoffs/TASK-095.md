## 2026-10-10 工程收尾增量

主角与弟弟走跑按实际源片段位移和左右脚相位重新校准，保持游戏速度。原生4/4（零警告/错误）及576次PIE骨骼采样通过，350cm/s主角脚滑中位数约238→18cm/s。平地接触缺陷已修复；坡地连续观感、完整剧情与Owner主观验收单列。 见[报告](../qa/TASK-095/locomotion/REPORT.md)。本节为当前进展，以下旧日期内容保留其历史范围。

# TASK-095｜独立技术工作交接

2026-10-10：候选13已从干净源码 `6bfe7ed9a020da7c40c65ed3a136d5634de0eb64` 完成Shipping构建、实际Cook核对和正常键盘新游戏卧室检查，自有进程正常关闭。两项批准增量均已包含，详细证据见[前摇与出箭报告](../qa/TASK-095/windup/REPORT.md)。以下按日期保留历史范围；当时待答复的两项设计现已批准并实现。

2026-10-09 两项Owner批准的增量已实现：弟弟0.25秒可中断前摇、原1.2秒周期，原创双手长枪动作及刀斧相位修正；玩家弓弩箭尖发射与近墙阻挡。最终Editor构建和2项定向原生零警告通过，三种近战108帧连续PIE及两种远程贴墙检查通过。见[前摇与出箭报告](../qa/TASK-095/windup/REPORT.md)。完整实玩按用户要求后置，完整坡地/昼夜、Owner视觉及来源权利验收保持未闭合。

2026-10-08 弟弟13种近战装备已按实际实例/耐久和生存状态呈现，新增右臂携带姿势修复长刃/战斧待机穿肩。Editor构建、装备回归1/1（零警告）及六件装备/正侧视PIE通过。弟弟攻击接触与长枪专用动作、完整Cook和Owner验收继续，TASK-095保持Active。 见[弟弟装备报告](../qa/TASK-095/brother-weapons/REPORT.md)。

2026-10-08 玩家弓弩四段动作与可弯曲长弓已接入，瞄准俯仰/水平朝向错位已修正。4项回归及最终移动腿部/装备呈现定向1项通过，实际PIE29帧和三段视频通过。实际弹道起点仍按旧逻辑，改到箭尖并检查遮挡的方案待Owner答复；弟弟装备、完整接触/Cook/视觉验收继续。见[弓弩报告](../qa/TASK-095/player-ranged/REPORT.md)。

2026-10-08 首线五类原创武器及长枪双手突刺已接入，共11个UE包；17种非石斧装备的实例/耐久/移除呈现和原石斧回归4/4通过，最终长枪姿态重开后定向1/1通过。七件装备PIE和长枪轻重攻击20帧采样已归档。玩家弓弩专用动作、弟弟装备、全距离接触及Cook/Owner验收继续，TASK-095保持Active。见[武器运行报告](../qa/TASK-095/weapons/REPORT.md)。

2026-10-08 追加实施：Owner批准两米入口、自动靠近约0.8米后施救五秒。六段兄弟倒地/起身/施救动作已由CMU 113_08与原创施救姿态制作、导入原骨架；Editor构建及九项定向原生通过。原骨架根缩放和长帧靠近过冲已修复；最终两项原生回归及正反向PIE、真实障碍、移动输入取消通过，见[救援运行报告](../qa/TASK-095/rescue-runtime/REPORT.md)。完整TASK-095仍未完成。批准记录见[救援靠近](../assets/TASK-095/RESCUE_APPROACH.md)。


## 2026-10-08 当前进展

2026-10-08 衣料材质已接入主角、弟弟、Campaign族人和军需角色入口；新增5包，复用原人体、动作及原PBR贴图。Editor编译及3项定向原生通过；实际PIE六类角色加载、绑定与同光照截图通过。详见[服装接入](../qa/TASK-095/costumes/REPORT.md)。其余武器持握、倒地/扶起、完整昼夜/动作验证、Cook与Owner视觉继续实施。

射手已替换Campaign运行角色，专用27骨骨架与4段动作已导入；0.6秒可见前摇经Owner批准，受击/读档取消与单次出箭原生通过。Editor构建成功，3项定向原生3/3通过、零警告；重开UE后的隔离PIE记录66帧和两箭，材质绑定、骨骼用途与实际颜色已核对。见[射手运行报告](../qa/TASK-095/archer-runtime/REPORT.md)。其余武器持握、倒地/扶起、完整移动/昼夜验证、实际Cook与Owner视觉未完成，TASK-095保持Active。以下旧调查按日期保留。

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
