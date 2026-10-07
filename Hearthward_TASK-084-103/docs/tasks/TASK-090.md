# TASK-090｜HUD与日志目标层级、首次准备引导

> 状态：Backlog（任务单已编写，实施未派发）。优先级：P1。阶段：B 玩法与UI。日期：2026-10-06。Owner：XLingyyy。Reviewer／Issue：未指派。建议分支：`codex/TASK-090-hud-journal-onboarding`（未创建）。

[本批总入口](../planning/TASK-084-103/README.md) · [执行约定](../planning/TASK-084-103/EXECUTION_GUIDE.md) · [元数据](TASK-090.json) · [交接模板](../handoffs/TASK-090.md)

## 1. 目标与预期结果

让玩家在首次营地准备和外出时知道当前目标、必要补给与弟弟状态，减少全系统菜单堆叠；引导基于真实阶段，不替玩家完成任务。

## 2. 当前基础与事实边界

已有任务日志与追踪、金色方向标记、生存信息、材料追踪和弟弟反馈；本单整合信息层级，不新增剧情目标或奖励机制。

本单参考基线为 `main@6fcf5c22e965f0f7409438f19bc7b09e96ffb058`；实际开工必须复核届时HEAD与依赖产物。这里不是本轮运行结果，旧报告PASS/FAIL均绑定原受测实现。新行为是本单计划，不能写成当前已经存在。

## 3. 依赖、开工门槛与最小必读

前置：[TASK-086](TASK-086.md)、[TASK-088](TASK-088.md)、[TASK-089](TASK-089.md)

依赖的约定产物与实际实现SHA就绪、Owner派发且共享写窗口空闲后开始。依赖不要求伪改旧任务Done；读取其实际交接并核对采用版本。

共同必读：`AGENTS.md`、`WORKFLOW.md`、`docs/START_HERE.md`、`docs/PROJECT_STATE.md`、`docs/design/CURRENT.md`及本单MD/JSON，然后执行本批指南中的接手/路径核对。

本单额外入口（路径为只读定位，不等于修改授权；前置任务产物需先实际生成）：

- `Source/Hearthward/UI/HearthwardScreenContent.cpp`
- `Source/Hearthward/UI/HearthwardHUD.cpp`
- `Source/Hearthward/UI/HearthwardQuestGuidance.cpp`
- `Source/Hearthward/Campaign/HearthwardCampaignQuests.cpp`
- `Source/Hearthward/Gameplay/HearthwardGameplayComponent.h`
- `Resources/Data/gameplay.json`

## 4. 修改范围与协作边界

除本单MD/JSON、handoff、QA目录、专用规划目录和根`README.md`固定收尾外，候选范围如下；最终以**已获准基线JSON**为准。

- `Source/Hearthward/UI/HearthwardScreenContent.cpp`
- `Source/Hearthward/UI/HearthwardHUD.cpp`
- `Source/Hearthward/UI/HearthwardScreenWidget.h`
- `Source/Hearthward/UI/HearthwardScreenActions.cpp`
- `Source/Hearthward/UI/HearthwardScreenLayout.cpp`
- `Source/Hearthward/UI/HearthwardScreenPaint.cpp`
- `Source/Hearthward/UI/HearthwardPreparationView.h`
- `Source/Hearthward/UI/HearthwardPreparationView.cpp`
- `Resources/UI/layout.json`
- `Resources/UI/interface.json`
- `Source/Hearthward/Tests/HUDJournalPreparationTests.cpp`

Content与Save等未授权领域只读；例外仅以JSON实际范围为准，不能用必读清单扩大写权限。

拟新增辅助/测试文件不表示仓库已存在；先搜索可复用类型。共享UI配置、公共头文件、WorldPresentation/CampaignWorld等按[写入调度](../planning/TASK-084-103/DISPATCH.md)串行。临时输出使用 `.agent-local/qa/TASK-090/<唯一run>/`，不得写入用户原档。

## 5. Agent具体实施步骤

### 1. 定义三层信息

常驻HUD保留必要生存/当前任务/重要伙伴异常；展开日志显示阶段与下一步；制作/工作页保留详单。标注每条信息的来源、显示条件、隐藏条件和对应操作入口，禁止把全部营地数据塞HUD。

### 2. 准备卡只读推导

依据084路线已有任务阶段，展示本次目标、所需设施/工具/补给与弟弟分工入口。只有已批准且可知的条件才展示；不存在硬性补给要求时用“建议”不加任务门槛。

### 3. 渐进展示

新档到对应阶段再出现取遗物/跟随/查看日志/制作或分工提示；按真实交互完成隐藏。旧档和自由探索不重演已完成提示；不修改Claimed或强制接受/自动提交任务。

### 4. 连接已有页面

目标详情可打开相关制作/营地/工作页并选中对应已有条目；材料追踪和主线追踪用不同图形/文案，取消材料追踪不能取消主线。返回恢复原页与焦点，不遗留暂停状态。

### 5. 任务完成反馈

真实条件已满但需在J领取时明确“回日志提交/领取”，不继续引导玩家跑向旧地点；领取后隐藏旧目标。复用实际奖励与进度回执，不在通知函数发奖。

### 6. 重要性和遮挡

弟弟倒地、受阻、生存危险优先于普通产量提示；使用颜色+图标/文字组合，长原因可展开。地图页、对话页、加载/倒地状态按已有页面约束排版，不能挡住救援/放弃确认。

### 7. 首次流程验证

从正常新游戏和一个已有营地档分别操作，记录玩家卡住时能否在界面找到下一步；本人或自动输入只是工程测试，首次玩家的完成率与时间另在103收集。

## 6. 不可破坏的规则

- 不增加新手强制封路、补给硬门槛、自动接/领奖或新的任务ID。
- HUD“目标完成”只能引用权威已满足条件；显示层不自动使目标满足。
- 遵守对话运行、菜单暂停设置、Esc优先级和快捷键再次关闭，不用引导重置玩家设置。

所有更改继续遵守只读显示、原事务执行、稳定ID、时间线隔离和不伪填验收规则。规则未明确的新玩法不硬编码；不删测试/吞错误/全仓重构来让当前检查通过。

## 7. 验收用例与执行方式

每条用例在隔离档/明确诊断夹具中建立前置，实际记录操作与断言。夹具、注入输入、真实OS键鼠、真实模型、真人样本分层统计；下面所有结果尚未运行。

|局部编号|场景|前置/具体操作|通过标准|初始结果|
|---|---|---|---|---|
|T090-C01|新档阶段|按序完成遗物、跟随、撤离与营地准备。|提示随真实阶段出现/隐藏，不提前剧透。|NOT_RUN|
|T090-C02|旧档恢复|读取营地中期及已完成首救的兼容档副本。|不强制重走新手步骤，不重复奖励。|NOT_RUN|
|T090-C03|两类追踪|同时主线追踪与材料追踪，分别取消。|互不覆盖进度或标记状态。|NOT_RUN|
|T090-C04|待领取目标|满足任务条件但未领奖，之后在J领取。|清楚指向提交动作，领取后旧标记消失。|NOT_RUN|
|T090-C05|页面链路|HUD→日志→制作/工作页→返回；重复快捷键和Load。|焦点与暂停状态正确，无输入丢失或无法退出。|NOT_RUN|
|T090-C06|紧急状态|真实受伤/倒地/弟弟受阻及长中文提示叠加。|重要操作可达，文本不重叠；普通通知不遮挡救援。|NOT_RUN|
|T090-C07|可读性|100%/150%、4:3/16:9/超宽和低对比场景。|完整文字可读，状态不只靠颜色分辨。|NOT_RUN|


全局挂接索引：T-002, T-011, T-013, T-025。它们来自现有TEST_MATRIX；正式数值以已批准增量/ACCEPTANCE和本单细化步骤为准，不恢复历史灰盒规则。

本单需新增/复用定向原生测试；新前缀建议 `Hearthward.Iteration.Task090.`。先注册并核验找到用例数量>0，再按公开UEClient运行，不能拿本表局部ID当UE过滤器。正常输入、渲染、真实模型或真人用例另行执行。

公共检查：`python -X utf8 scripts/validate_repo.py`、`python -X utf8 -m unittest discover -s scripts/tests -v`、`git diff --check`；在包含获批本单快照的实际基线上运行 `python -X utf8 scripts/validate_repo.py --task TASK-090 --base <实际完整SHA>`。UE操作/证据要求见执行约定；缺环境则明确NOT_RUN，不能将文件编写完成等同测试通过。

## 8. 交付物与完成定义

- HUD/日志信息层级表与阶段显示规则
- 首次准备与跨页引导代码/测试
- 新档/旧档/倒地/大字号截图与输入记录

固定收尾：更新`README.md`当前说明、同名MD/JSON与`docs/handoffs/TASK-090.md`；在`docs/qa/TASK-090/REPORT.md`绑定最终完整SHA、资产/模型指纹、环境、命令、逐用例结果和明确限制。该REPORT为未来执行产物，本包不预填。

工程实现完成、Owner视觉、真人体验、二机、性能与发布各自登记。依赖下游可用产物写入handoff，不把未验项目涂为PASS。没有授权时交付本地改动和证据并写“未提交/未推送/未发布”，不得自增权限。

## 9. 本单明确不做

- 改写主线任务、自动领奖或新手强制流程
- 新建全屏百科/技能系统
- 将引导时间冒充真人节奏数据

## 10. 失败、范围变化和恢复

首先保留最短复现、受测状态和原始证据，再定位到本单或其他负责任务；不能通过清空存档/赠送物资/瞬移/改验收阈值绕过阻塞。回退只针对本单经核对的改动或资产备份，禁止未经授权reset/clean、覆盖他人工作和强制解锁。确需改变共享契约、保存格式、正式配表或包范围时提交具体差异与影响，先获准再施工。
