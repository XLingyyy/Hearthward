# TASK-085｜共享显示契约、动作语义与UI增量接入基础

> 状态：Backlog（任务单已编写，实施未派发）。优先级：P0。阶段：A 基础。日期：2026-10-06。Owner：XLingyyy。Reviewer／Issue：未指派。建议分支：`codex/TASK-085-presentation-contract`（未创建）。

[本批总入口](../planning/TASK-084-103/README.md) · [执行约定](../planning/TASK-084-103/EXECUTION_GUIDE.md) · [元数据](TASK-085.json) · [交接模板](../handoffs/TASK-085.md)

## 1. 目标与预期结果

为伙伴状态、制作缺口、回营反馈和导航建立最小的只读显示契约与安全动作接入点，后续UI不重复计算或直接改世界。保留现有Slate/Widget架构。

## 2. 当前基础与事实边界

当前界面通过 HearthwardScreenWidget、各 Compose 页面、Gameplay/Companion/Camp 子系统和 Resources/UI 配置构成；已有权威事务与epoch检查，不能另建第二套库存/进度账本。

本单参考基线为 `main@6fcf5c22e965f0f7409438f19bc7b09e96ffb058`；实际开工必须复核届时HEAD与依赖产物。这里不是本轮运行结果，旧报告PASS/FAIL均绑定原受测实现。新行为是本单计划，不能写成当前已经存在。

## 3. 依赖、开工门槛与最小必读

前置：[TASK-084](TASK-084.md)

本包CT为DRAFT而非已批准接口；085激活时确认采用范围。允许改变的仅是展示适配与必要读取接口，业务规则冲突提交设计差异，不自行覆盖。

共同必读：`AGENTS.md`、`WORKFLOW.md`、`docs/START_HERE.md`、`docs/PROJECT_STATE.md`、`docs/design/CURRENT.md`及本单MD/JSON，然后执行本批指南中的接手/路径核对。

本单额外入口（路径为只读定位，不等于修改授权；前置任务产物需先实际生成）：

- `Source/Hearthward/UI/HearthwardScreenWidget.h`
- `Source/Hearthward/UI/HearthwardScreenActions.cpp`
- `Source/Hearthward/UI/HearthwardScreenLayout.cpp`
- `Source/Hearthward/Companion/HearthwardCompanionFixture.h`
- `Source/Hearthward/Camp/HearthwardCampSubsystem.h`
- `Resources/UI/LAYOUT.md`

## 4. 修改范围与协作边界

除本单MD/JSON、handoff、QA目录、专用规划目录和根`README.md`固定收尾外，候选范围如下；最终以**已获准基线JSON**为准。

- `docs/contracts/CT-TASK-085-presentation-readmodels.md`
- `Source/Hearthward/UI/HearthwardPresentationReadModels.h`
- `Source/Hearthward/UI/HearthwardPresentationReadModels.cpp`
- `Source/Hearthward/UI/HearthwardScreenWidget.h`
- `Source/Hearthward/UI/HearthwardScreenActions.cpp`
- `Source/Hearthward/UI/HearthwardScreenLayout.cpp`
- `Resources/UI/layout.json`
- `Resources/UI/interface.json`
- `Source/Hearthward/Tests/PresentationReadModelTests.cpp`

Content与Save等未授权领域只读；例外仅以JSON实际范围为准，不能用必读清单扩大写权限。

拟新增辅助/测试文件不表示仓库已存在；先搜索可复用类型。共享UI配置、公共头文件、WorldPresentation/CampaignWorld等按[写入调度](../planning/TASK-084-103/DISPATCH.md)串行。临时输出使用 `.agent-local/qa/TASK-085/<唯一run>/`，不得写入用户原档。

## 5. Agent具体实施步骤

### 1. 梳理真实读取链

逐项核对 GetPhase/GetRequested/GetAcquired/GetCarried/GetDelivered/BlockReason、库存Available、CraftingStatus、任务/营地数据读取点；找现有同义结构。已有结构能满足就复用，不因本任务名称另造服务层。

### 2. 落实共享契约

以随包 CT 草案列明字段来源、单位、unknown/invalid含义、时间线失效、数量归属、动作权限和更新时机；源定义与实际接口不一致先修草案。Owner激活共享契约后，才添加最小适配结构。

### 3. 实现只读投影

优先纯函数/轻量适配器，输入真实对象或值快照，输出显示数据。只做显示格式与可用性投影，不拥有库存、奖励、任务完成和持久状态；缺对象返回可解释不可用状态。新文件为拟新增，类型名先全仓检索避免重复。

### 4. 定义动作语义

继续/暂停/取消工作、取消提案、取消模型请求、关闭页面分别映射原行为；显示enabled/reason与执行时重新校验分离。按钮不能通过直接写 Phase/State/Inventory 或拼假结果完成动作。

### 5. 整理UI约束

复用现有字体/色系/间距与layout解析；为后续新控件登记稳定LayoutId和Component，明确最小窗口、100%/150%字号、右侧对话区域、长文本滚动。只修改必要布局节点，不翻新整个主题。

### 6. 控制状态刷新

事件可用则失效重算；没有事件则仅对可见页面做有界刷新。地图朝向/屏幕投影保持实时；不得每帧重新遍历全世界生成物品目录，也不得缓存已销毁对象裸指针。

### 7. 建立契约测试

实现 Projection 纯读测试、缺对象/过期epoch/场景销毁测试和动作路由不写状态测试。为各字段提供真实或明确隔离的夹具；展示例子中的14/32不能写成正式逻辑。

### 8. 交接公共写窗口

记录类型、接口、字段来源与后续消费者；将ScreenWidget.h、ScreenActions.cpp和UI两个JSON的后续写入顺序交给调度者。只提交本单获授权范围，不替其他任务改实现。

## 6. 不可破坏的规则

- 不增加存档字段；读模型只缓存可丢弃投影，Load/新游戏/换档销毁。
- DisplayReason不是权限真值；实际事务每次使用现有安全/距离/epoch校验。
- 保持右侧约37%对话、深灰物品格和无黑雾矩形地图；不换框架、不动字体二进制。

所有更改继续遵守只读显示、原事务执行、稳定ID、时间线隔离和不伪填验收规则。规则未明确的新玩法不硬编码；不删测试/吞错误/全仓重构来让当前检查通过。

## 7. 验收用例与执行方式

每条用例在隔离档/明确诊断夹具中建立前置，实际记录操作与断言。夹具、注入输入、真实OS键鼠、真实模型、真人样本分层统计；下面所有结果尚未运行。

|局部编号|场景|前置/具体操作|通过标准|初始结果|
|---|---|---|---|---|
|T085-C01|投影无副作用|读取个人任务/库存/营地快照前后比较权威状态。|所有余额/任务ID/事务记录不变。|NOT_RUN|
|T085-C02|无对象与销毁|无伙伴、角色销毁、世界卸载时构造页面。|无崩溃/野指针，显示不可用且动作禁用。|NOT_RUN|
|T085-C03|时间线|取快照后读档，在旧按钮/回调上触发动作。|旧引用失效，不能跨epoch提交。|NOT_RUN|
|T085-C04|动作分离|依次测试关闭页、丢弃确认卡、取消请求、取消正在执行工作。|四种语义不互相代替，实际任务变化符合原契约。|NOT_RUN|
|T085-C05|布局兼容|默认/大字号与4:3/16:9/超宽窗口检查新控件。|有用信息不重叠、不越界，主动作始终可达。|NOT_RUN|
|T085-C06|刷新成本|连续开关页面与切场景，记录投影调用次数。|无重复订阅增长、后台无限刷新或每帧全表重建。|NOT_RUN|


全局挂接索引：T-002, T-024, T-025, T-011。它们来自现有TEST_MATRIX；正式数值以已批准增量/ACCEPTANCE和本单细化步骤为准，不恢复历史灰盒规则。

本单需新增/复用定向原生测试；新前缀建议 `Hearthward.Iteration.Task085.`。先注册并核验找到用例数量>0，再按公开UEClient运行，不能拿本表局部ID当UE过滤器。正常输入、渲染、真实模型或真人用例另行执行。

公共检查：`python -X utf8 scripts/validate_repo.py`、`python -X utf8 -m unittest discover -s scripts/tests -v`、`git diff --check`；在包含获批本单快照的实际基线上运行 `python -X utf8 scripts/validate_repo.py --task TASK-085 --base <实际完整SHA>`。UE操作/证据要求见执行约定；缺环境则明确NOT_RUN，不能将文件编写完成等同测试通过。

## 8. 交付物与完成定义

- CT-TASK-085-presentation-readmodels.md：从草案到实际采用版及批准记录
- 最小投影/路由代码及相关原生测试
- 字段—权威来源映射、公共文件写入交接、UI对照证据

固定收尾：更新`README.md`当前说明、同名MD/JSON与`docs/handoffs/TASK-085.md`；在`docs/qa/TASK-085/REPORT.md`绑定最终完整SHA、资产/模型指纹、环境、命令、逐用例结果和明确限制。该REPORT为未来执行产物，本包不预填。

工程实现完成、Owner视觉、真人体验、二机、性能与发布各自登记。依赖下游可用产物写入handoff，不把未验项目涂为PASS。没有授权时交付本地改动和证据并写“未提交/未推送/未发布”，不得自增权限。

## 9. 本单明确不做

- 完整MVVM/UMG/CommonUI迁移
- 新增保存格式、资源预留或新的任务执行系统
- 用显示层修正真实数量错误

## 10. 失败、范围变化和恢复

首先保留最短复现、受测状态和原始证据，再定位到本单或其他负责任务；不能通过清空存档/赠送物资/瞬移/改验收阈值绕过阻塞。回退只针对本单经核对的改动或资产备份，禁止未经授权reset/clean、覆盖他人工作和强制解锁。确需改变共享契约、保存格式、正式配表或包范围时提交具体差异与影响，先获准再施工。
