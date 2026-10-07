# TASK-097｜独立技术工作交接

## 2026-10-07 已批准方向后的进展

方向已由DSGN-004批准。野猪基于既有R3猪源延长吻部、添加同骨骼獠牙/鬃毛并用材质调色；原骨架层级不变。三种作物各有幼株/成熟两形态，共六份FBX制作源。六个作物网格及材料共20包已锁定、导入、显式保存并在重启前确认落盘；NatureActor按现有权威日历选择幼株/成熟网格与缩放，成长时间、产量和采收事务不变。Editor构建成功；CropGeometryConsumesCalendarStage与CropProgressAndHarvestCapacity实际2/2 Success、1 warning、0 error，证据见docs/qa/TASK-097/samples/crop-native-20261007.json。警告为测试LocalPlayer缺少PlayerInput，未隐去。已补充作物目录的AlwaysCook配置，实际Cook仍NOT_RUN。野猪新网格及烘焙毛色、三材质共五包已锁定并导入，复用原50骨R3 Skeleton与PhysicsAsset，原Skeleton未标脏且Git未变化；家猪保持原网格，野猪继续使用原27动作/行为参数，活动范围仍按原网格计算。Editor编译成功；BoarKeepsR3AndPig原生测试通过（0警告/0错误），核对家猪/野猪网格区分、共享物理资产与动作、活动边界，并比较静候/奔跑/顶撞/倒地四个真实动作中间帧的逐骨姿态。首轮夹具重复初始化World失败保留，修正为CreateWorld单次初始化后通过。连续自然路线、完整Cook及Owner视觉仍未验，原Tripo输入权利状态仍PENDING。见samples/boar-source.json及crop-source.json。

以下为本单此前调查与验证记录；旧“待批准方向/暂缓制作/未改Content”的描述以本节更新为准，旧测试只保留其原版本信用。

[任务单](../tasks/TASK-097.md) · [元数据](../tasks/TASK-097.json) · [QA报告](../qa/TASK-097/REPORT.md) · [技术盘点](../qa/TASK-097/TECHNICAL_AUDIT.json)

## 当前状态与范围

Active，2026-10-07。用户授权逐单执行，设计问题可暂缓；主Agent已派发本单独立技术子范围。工作树 `G:/GameFactory/Hearthward`，分支 `codex/TASK-084-103-iteration`，参考 HEAD `6fcf5c22e965f0f7409438f19bc7b09e96ffb058`，含共享未提交改动。独立调查由 route_audit 执行，UE生命周期/构建/运行与根README由主Agent统一。未提交、未推送、未合并、未发布；获批任务快照的真实提交和范围基线验证仍 NOT_RUN。

本单新增 QA报告/TECHNICAL_AUDIT、准确候选包引用及执行文档。未编辑任何 Content、共享源或正式玩法/UI配置。[包范围](../assets/TASK-097/PACKAGE_SCOPE.json)共539个Content候选，`selected_for_change=[]`，锁 NOT_ACQUIRED。未选择新风格资产，未自批Owner视觉。

## 已实施与采用依赖

14物种/303片段、deer→stag_a与boar→pig别名、124条家族级制作源匹配和原作物状态外观路径已盘点。

机器证据由 `../.venv/Scripts/python.exe -X utf8 docs/qa/TASK-095/audit_bindings.py` 生成，限定实际源码/配置、094登记和PNG头，无资产hash。094样板与来源/许可边界沿用当前 ASSET_REGISTER，来源 UNKNOWN不升级为合规。084 ROUTE_MANIFEST为静态路线/部分运行证据；091尚无新路线替换，092连续自然通行未完成，未冻结碰撞。

## 具体设计暂缓与工程前置

野猪的猪源衍生方案、作物成长/成熟形态和自然环境首件样板待Owner确认；不重制R3/303或更改成长/掉落/速度。

正式路线净空采用084/092；家族配对、未知许可、UE实际LOD/骨架/材质和同条件性能仍待验证。3条现有动物/成长/作物几何原生测试可独立执行。

## 下一轮最小验证

主Agent已实际运行以下原生过滤器，原始条目见本单NATIVE_REUSE.json；历史报告不移成本轮结果：

- `Hearthward.Animals.FrameBoundaryAndEscape` — 本轮 Success，0 warning／0 error。
- `Hearthward.Farming062.IndividualGrowthAndProductProgress` — 本轮 Success，1 warning／0 error。
- `Hearthward.Farming062.CropGeometryConsumesCalendarStage` — 本轮 Success，0 warning／0 error。

本单未新增重复配置的测试。完整T097-Cxx逐项均在QA REPORT登记，运行、真实模型、正常输入、Cook和Owner签收分别标记NOT_RUN；静态子检查仅证明其具体范围。依赖下游可采用技术盘点/准确路径与现有行为入口，资产替换需准确包授权、锁和单写者窗口。正式完工和Owner签收仍由主Agent复核。

实际原生：3/3 Success，1 warning，0 error；完整Cxx、正常输入、渲染、Cook和Owner仍NOT_RUN。证据：[NATIVE_REUSE.json](../qa/TASK-097/NATIVE_REUSE.json)，警告逐条原样保留于REPORT。
