# TASK-085｜只读显示适配与动作基础

2026-10-07，执行Agent：task_audit，根Agent协调。根目录`G:/GameFactory/Hearthward`，分支`codex/TASK-084-103-iteration`，基线完整SHA为`6fcf5c22e965f0f7409438f19bc7b09e96ffb058`。本报告记录dirty实现。主代理本轮Development Editor构建成功，085原生测试4/4成功，无警告/错误；真实输入、模型和渲染仍待运行，不将原生结果代填体验验收。

## 授权与源缺口

用户当前会话已授权逐项执行084—103，设计需确认时暂缓，其余自主继续。085只采用批次计划的展示适配，没有改变已批准玩法或Save。根Agent委派本单ReadModels/定向测试/契约/单任务文档；README与UE执行由根Agent负责。

修改前读取现有权威getters、CampRegion、Storage epoch、UI与动作调用。以下为基线源码确认的缺口，未宣称运行时RED：

- 无可供状态UI共享的类型化个人/队伍显示视图；模型`FHearthwardNPCContextSnapshot`用于可知/裁剪上下文，缺少UI动作快照和完整计数，不宜用作第二套界面账本。
- ScreenContent的个人phase私有格式化未显式处理Cancelled；Dialogue/HUD有另一组进度拼接，个人`Requested>Delivered`判定隐藏终态记录。显示消费由086收尾。
- agentRetryPath/cancelTask直接指向当时的伙伴；cancelTask缺少dialogue页面限制，没有独立可复用的快照epoch/CommandId检查。085接入现值检查，086负责新卡携带旧快照。
- 已存在的CancelPending通过Serial、HTTP取消和DiscardProposal使回复/候选失效；ClearClarification进一步清澄清，不取消执行。CancelExecution调用原伙伴Cancel并保留实际物资，原API无需重新设计。

## 实际改动

ReadModels以值快照和纯读适配构成，不建立服务、订阅、后台任务或对象缓存。个人保留真实Requested/Delivered/Acquired/Carried，剩余为尚未交付；终态依phase。Missing/失效数值为TOptional未设置，数量不一致显示不可用且不修正权威数据。目的地只给现有容器角色名称，无坐标。

队伍视图复制现有Region/Camp/Job/Workers和开关；AssignedBrother与原BrotherWorking分开，CompletedBatches为原批次数，产物份数仍沿DescribeWorkParty。素材/材料追踪/导航/成长类型等实际后续消费者需要时再增补。

ScreenActions的个人续接/取消读取当前投影、检查现时合法阶段和交流/页面状态，再调用原ResumeBlocked/CancelExecution。CanApplyPersonalAction支持旧快照的epoch/CommandId及来源销毁检查；新状态卡绑定在086实施。Shared动作文件已释放；ScreenWidget、布局JSON、Save、Content、字体及刷新框架未改。

## 验证状态

|用例|本单覆盖/实际状态|
|---|---|
|T085-C01|原生计数与只读对照成功；实际UI显示另由086验证|
|T085-C02|WorldSources实际Actor销毁/缺来源原生测试成功；真实页面/世界卸载另待运行|
|T085-C03|TimelineAndOperations旧epoch/另一CommandId原生测试成功；新状态卡绑定在086|
|T085-C04|提案Discard与执行Cancel/计数原生测试成功；实际HTTP/页面输入回归另待运行|
|T085-C05|085未新增控件/布局；086实际消费后由根Agent做字号/宽高比渲染验证，NOT_RUN|
|T085-C06|本单未建立刷新/订阅/缓存；当前Widget原0.2秒刷新保留，运行开关/成本采样NOT_RUN|

原生过滤器`Hearthward.Iteration.Task085.`，注册四项：PersonalReadOnly、TimelineAndOperations、MissingAndInvalidCounts、WorldSources。主代理实际发现四项并全部成功，无警告/错误。证据见[原生结果](native-baseline-20261007.json)，源报告为`.agent-local/qa/TASK-085-088/native-baseline-20261007/index.json`（该联合轮087两项及088一项失败，不能称整轮通过）。NullRHI逻辑/来源测试不证明布局或真实输入。

构建：本轮Development Editor成功。定向原生：4/4成功。`git diff --check`相关已跟踪文件通过。真实输入/模型/渲染：NOT_RUN。README由根Agent同步。范围基线检查：NOT_RUN，当前基线未含本批快照且未有提交权限；不改验证器。提交/推送/合并/Release均未执行。084调查和后续消费/验证由根Agent协调；工程结果、Owner视觉、真人和二机分别记录。
