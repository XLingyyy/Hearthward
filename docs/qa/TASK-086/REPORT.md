# TASK-086｜工作状态显示与动作校验

2026-10-07，执行Agent：mcp_setup；根Agent协调。根目录 `G:/GameFactory/Hearthward`，分支 `codex/TASK-084-103-iteration`，基线完整HEAD `6fcf5c22e965f0f7409438f19bc7b09e96ffb058`。本报告绑定该HEAD上的未提交085/086实现，不将规划参考SHA当作已包含任务快照的范围校验基线。引擎为本机UE5.8.2；未修改Content、模型锁或Save格式，资产/模型运行指纹待根Agent实际运行记录。

## 事实与实现

修改前源码确认：个人状态正文只在Active时显示，完成与取消记录被隐藏；队伍存在时个人控件被else-if抑制；旧HUD漏部分phase；模型回复getter返回缓存。证据为 `.agent-local/qa/TASK-086/20261007-source-before/reproduction.json`，方法为源码条件复现，运行时结果明确NOT_RUN。

086消费085值快照与权威getter，不建立第二套工作账本。工作页保留终态，缺失数量显示未知；自然语言task_status与旧HUD展示时重读当前状态。个人/队伍按钮独立，界面不更改物资、岗位成员或生产规则。当前源点耗尽只显示已有停止原因与原入口条件，不补资源或改来源。

|显示/动作|权威字段及实际映射|
|---|---|
|个人数量|Requested、Delivered、Carried分别显示；RemainingToDeliver沿085的requested-delivered，Acquired不当作入库|
|个人阶段/终态|统一CompanionPhaseText，Completed/Cancelled仍HasTask；终态没有恢复/取消操作|
|目的地/停工|仅已知容器角色名称；BlockReason映射现有原因，未识别原因原样保留；无坐标推断|
|个人恢复|CanResume、ResumeReason；绑定epoch/CommandId，执行瞬间CanApplyPersonalAction复核，然后原ResumeBlocked|
|个人取消|CanCancel、CancelReason；绑定epoch/CommandId，执行原CancelExecution；原契约保留已取得物资|
|队伍人数/弟弟|Workers.Num与AssignedBrother分开；BrotherWorking只消费实际到岗劳动力，移动/个人委托/受阻另行说明|
|队伍累计|CompletedBatches沿既有木石每批2份换算累计入库；未知保持未知，不显示百分比|
|队伍暂停/继续|绑定epoch/Region/worker IDs；当前开关与成员重验，PreviewWorkParty复核；继续使用当前Workers.Num和原候选确认入口|
|主HUD/回复|主HUD给阶段/受阻与工作页入口；详细计数在工作页；两个task_status显示路径每次重读|

## 本轮检查

定向 `git diff --check`：PASS（仅文本差异格式检查）。task_audit只读内部核对未发现formatter/API、标识校验和实时读取的编译级问题，未执行正式review或验收。其发现的085 fish目的地错误已由根Agent修正：实际只到Nature并CommitNature，显示弟弟背包，不显示营地仓储。

原生过滤器 `Hearthward.Iteration.Task086.`，实际发现并运行3项：PersonalStatus、PartyStatus、StatusActionBinding，均Success且零警告/错误。数量与终态采用纯Command/投影夹具，队伍采用值视图，标识测试覆盖另一个Command/时间线/worker集合。2026-10-07根Agent通过公开UEClient统一构建成功，证据为`.agent-local/qa/TASK-085-088/build-repair2-20261007/result.json`；原始Native证据为`.agent-local/qa/TASK-085-088/native-baseline-20261007/index.json`，本单子集见[native-baseline-20261007.json](native-baseline-20261007.json)。同批其他任务失败不计入086通过数；这3项不代表端到端行为通过。

|验收用例|当前证据/最窄后续验证|实际结果|
|---|---|---|
|T086-C01 个人进度|PersonalStatus源包含requested32/delivered14/carried6与剩余18；须在实际工作页和真实入库前后核验|NOT_RUN|
|T086-C02 耗尽|源覆盖depleted真实原因文案；复用078隔离源点耗尽夹具，确认恢复不补资源、不换来源|NOT_RUN|
|T086-C03 队伍到岗|PartyStatus源检查移动与实际劳动力口径；复用081真实队伍2人/弟弟走向岗位前后观察|NOT_RUN|
|T086-C04 暂停续接|源显示个人/队伍独立且暂停保留累计；实际分别点击两类操作并观察仅所属工作受影响|NOT_RUN|
|T086-C05 查询无执行|两个task_status显示入口重读；复用079真实模型查询，比较请求前后状态/生产开关|NOT_RUN|
|T086-C06 过期重复|StatusActionBinding源检查标识变化；实际换Command、Load、重复确认与迟到回复仍须端到端验证|NOT_RUN|
|T086-C07 布局|工作页使用可滚动文本、独立许可按钮；大字号/4:3/长原因/终态/无任务须实际渲染与输入验证|NOT_RUN|
|T086-C08 真实恢复|UI每次读取当前权威投影；个人携货与队伍暂停须保存后独立重启并比对恢复状态/真实物资|NOT_RUN|

可复用诊断脚本：`docs/qa/TASK-078/verify_gathering_ui.py`、`docs/qa/TASK-079/verify_dialogue_pie.py`、`docs/qa/TASK-081/verify_final.py`。它们绑定原任务实现，原报告PASS不迁移；尤其旧081按钮可见性宽松分支需改为本轮直接断言。夹具注入、真实模型、OS输入与真人样本分开记录，不将补资源/瞬移作为本单恢复通过证据。

## 限制与交付

086源码窗口已交还根Agent，之后089再次取得共享UI窗口。UE生命周期、构建及原生由根Agent实际执行；真实地图/保存恢复与README仍由根Agent统一完成。公共验证器/脚本测试与含任务快照的范围检查待根Agent运行，当前NOT_RUN。尚未发现需用户确定的新玩法设计。

工程实现已落盘，任务保持Active；Owner视觉、真人、二机、性能与发行各自未验，无正式验收结论。提交、推送、合并、Release均未执行。
