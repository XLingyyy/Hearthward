# TASK-100｜四区差异化内容与首版全流程接续

## 2026-10-09 当前交付

2026-10-09：四类 Blender MCP 建筑已接入正式四区，共16处；地形调整后16/16局部穿行、12/12相关原生测试及Editor构建通过。TASK-100保持Active，完整区域路线、连续主线、15支线、第二营地保存继续、Owner视觉与新Shipping包尚未验收。

详见[四区场景接入报告](../qa/TASK-100/ZONE_INTEGRATION_20261009.md)。七个UE包和新Blend源已取得LFS锁；制作源、模型尺寸、场景落点和QA均随本单交付。以下2026-10-07记录为历史范围与证据，source-only/未选包等描述不代表当前实现。

> 状态：Active（四区正式场景已接入；16/16局部穿行与12/12原生通过；完整流程与Owner验收未完成）。优先级：P0。阶段：E 完整流程。日期：2026-10-09。Owner：XLingyyy。Reviewer／Issue：未指派。实际分支：`codex/TASK-084-103-iteration`。

[本批总入口](../planning/TASK-084-103/README.md) · [执行约定](../planning/TASK-084-103/EXECUTION_GUIDE.md) · [元数据](TASK-100.json) · [执行交接](../handoffs/TASK-100.md)

## 2026-10-07 执行进展

已实施无设计依赖的配置/源码覆盖：8main、15side、4zones、80base、2×4固定增援、10rescues全部追溯，127行无结构错误；真实条件、奖励唯一性与知识门槛逐项记录。四区已有入口/巡逻/兵种差异和同一028房屋/旗的呈现复用已分清，没有新增或假落空间几何。

side05首轮真实RED（1 error）与waiting偏离旧点RED（4 error）分别保存；root在091展示窗口最小修复后，最新三条100真实Game/controller夹具全Success、0 warning/0 error。未知side隐藏且原Available保持，waiting消费State.Position/真实Actor并去旧fork，following仍camp，prologue行动目标保留。7条049/067原事务本轮复用Success并保留7 warning；各快照独立登记。Campaign规则/经济/护送/奖励/Save字段未改，区域首件Owner待审，正常全流程和继续仍NOT_RUN。

证据：[QA报告](../qa/TASK-100/REPORT.md)、[覆盖JSON](../qa/TASK-100/CONFIG_COVERAGE.json)、[逐ID CSV](../qa/TASK-100/CONFIG_COVERAGE.csv)、[四区现有说明](../planning/TASK-084-103/TASK-100/ZONE_COVERAGE.md)。选包为空、未改Content/设计/economy/Save。历史传送/清兵结果不转作正常通关。

## 1. 目标与预期结果

在现有主支线、驻军、控制区和第二营地规则内，把已验证首线延伸到永久夺回与通关后继续，补实际内容和空间表现缺口；不另起战争系统。

## 2. 当前基础与事实边界

首版边界约十小时、四区、80基础驻军、最多救回10人、第二营地使用已批准；主支线/增援/胜利规则以当前gameplay.json和049设计为准。旧脚本逐点传送测试不等于正常全通关。

本单参考基线为 `main@6fcf5c22e965f0f7409438f19bc7b09e96ffb058`；实际开工必须复核届时HEAD与依赖产物。这里不是本轮运行结果，旧报告PASS/FAIL均绑定原受测实现。新行为是本单计划，不能写成当前已经存在。

## 3. 依赖、开工门槛与最小必读

前置：[TASK-087](TASK-087.md)、[TASK-090](TASK-090.md)、[TASK-091](TASK-091.md)、[TASK-092](TASK-092.md)、[TASK-093](TASK-093.md)、[TASK-095](TASK-095.md)、[TASK-096](TASK-096.md)、[TASK-097](TASK-097.md)、[TASK-098](TASK-098.md)、[TASK-099](TASK-099.md)

本轮已获用户逐单执行授权，先采用明确可用依赖完成独立技术子范围；共享源码和资产实际替换继续核对具体依赖版本/写窗口。依赖不要求伪改旧任务Done；读取其实际交接并核对采用版本。

共同必读：`AGENTS.md`、`WORKFLOW.md`、`docs/START_HERE.md`、`docs/PROJECT_STATE.md`、`docs/design/CURRENT.md`及本单MD/JSON，然后执行本批指南中的接手/路径核对。

本单额外入口（路径为只读定位，不等于修改授权；前置任务产物需先实际生成）：

- `Source/Hearthward/Campaign/HearthwardCampaignState.cpp`
- `Source/Hearthward/Campaign/HearthwardCampaignWorld.cpp`
- `Source/Hearthward/Campaign/HearthwardCampaignQuests.cpp`
- `Source/Hearthward/Campaign/HearthwardCampaignInteraction.cpp`
- `Resources/Data/gameplay.json`
- `docs/design/CURRENT.md`
- `docs/planning/TASK-053-074/ACCEPTANCE.md`

## 4. 修改范围与协作边界

除本单MD/JSON、handoff、QA目录、专用规划目录和根`README.md`固定收尾外，候选范围如下；最终以**已获准基线JSON**为准。

- `Source/Hearthward/Campaign/HearthwardCampaignWorld.cpp`
- `Source/Hearthward/Campaign/HearthwardCampaignActor.cpp`
- `Source/Hearthward/Campaign/HearthwardCampaignQuests.cpp`
- `Source/Hearthward/Campaign/HearthwardCampaignInteraction.cpp`
- `Source/Hearthward/UI/HearthwardScreenContent.cpp`
- `Resources/Data/quest_guidance.json`
- `docs/assets/TASK-100/`
- `Source/Hearthward/Tests/CampaignCompletionRouteTests.cpp`

本单不授予整个Content目录。准确包名单由现场引用/094清单确定，先补入已批准快照并核验LFS锁，才可编辑。未批准包仅可只读检查。

拟新增辅助/测试文件不表示仓库已存在；先搜索可复用类型。共享UI配置、公共头文件、WorldPresentation/CampaignWorld等按[写入调度](../planning/TASK-084-103/DISPATCH.md)串行。临时输出使用 `.agent-local/qa/TASK-100/<唯一run>/`，不得写入用户原档。

## 5. Agent具体实施步骤

### 1. 建立完整覆盖表

读取现有全部主线、15一次性支线、4区、80基础驻军、固定增援和10救援对象，列稳定ID、前置、可知条件、触发位置、胜利/奖励条件、对应源码与缺口。以真实配置核对数量，若与批准设计不一致先记录差异，不能直接重写数据。

### 2. 选一控制区作样板

在既有地形/驻军职责内整理入口、可侦察视线、掩体、撤退路径、旗帜交互区与回程；用普通/射手/重型布局表现差别，不增血量当差异。样板通过后才处理其他三区。

### 3. 完善其余三区

为每区写“空间特征—已有敌人职能—玩家现有选择—清敌/占旗反馈—夺回后变化”；只用现有能力与获批内容，避免四次同走廊复刻。新增资源/敌人/奖项/任务门槛需要单独设计决定，不在本任务暗增。

### 4. 接续主支线

从第一救援成长自然推进侦察、各区控制和夺回；日志/地图目标随真实阶段变化。支线确保可接受、可到达、条件可满足、只领奖一次；缺少线索优先改善可知提示，不直接解锁未知地点。

### 5. 验证占领与清敌

按已有规则清敌、E交互及站定5秒等条件逐一实测；击晕等效击杀、故乡永久清除、固定增援只按既定触发结算。未清敌/未满足阶段时明确拒绝，不强改胜利Flag。

### 6. 救援与第二营地

检查已获救人数上限/唯一性、安全承接、永久夺回后第二营地实际可用；共享仓储/等阶而建筑独立，获胜不复制资源/建筑/劳动力。通关后继续可保存和正常生活。

### 7. 先全工程通路

用正常输入从新游戏完成至少一条全主线，允许多次自然保存继续并记录连续时间线；不得通过给物资/清敌控制台/逐点传送跳过缺口。支线可用独立合法存档覆盖，但不能把它伪称同一角色一次全收集。

### 8. 把体验指标交真人

记录阶段实际耗时、停滞和资源流作调表依据，不用强制计时凑8—12小时。103按3名新玩家与支线合计覆盖做真人验收；未闭合的旧049/065/073验收继续留原来源。

## 6. 不可破坏的规则

- 不增加反击敌方腹地、部族战斗小队、最终投降或长期自适应敌军。
- gameplay.json和CampaignState的规则/存档字段默认只读；确需修差异先列准确字段、契约与授权，不绕过配置在C++硬编码替代。
- 所有改地图/分区包仍需逐包登记；本任务优先复用已收尾095—099资产，不临时引入风格不一致的批量新素材。

所有更改继续遵守只读显示、原事务执行、稳定ID、时间线隔离和不伪填验收规则。规则未明确的新玩法不硬编码；不删测试/吞错误/全仓重构来让当前检查通过。

## 7. 验收用例与执行方式

每条用例在隔离档/明确诊断夹具中建立前置，实际记录操作与断言。夹具、注入输入、真实OS键鼠、真实模型、真人样本分层统计；下面所有结果尚未运行。

|局部编号|场景|前置/具体操作|通过标准|初始结果|
|---|---|---|---|---|
|T100-C01|覆盖表一致|配置与设计逐项核对主支线、驻军、救援和控制区。|实际ID全部可追溯，缺口有归属，无编造完成项。|NOT_RUN|
|T100-C02|四区可走|正常步行/战斗进入退出各区、接近旗帜。|每区有可读空间与合法退路，无必须调试的阻断。|NOT_RUN|
|T100-C03|清敌占旗|不满足/满足清敌、击晕与占旗条件分别交互。|严格沿原胜利规则，一生命代次只结算一次。|NOT_RUN|
|T100-C04|主线全程|新档完成夜袭、营地、救援、成长、夺回和第二营地。|连续进度无强制跳阶段，必要资源真实取得。|NOT_RUN|
|T100-C05|支线唯一|逐条完成可获得的15一次性支线并尝试重复领取。|奖励/人口不重复，不可获得条目明确原因而非假PASS。|NOT_RUN|
|T100-C06|第二营地|永久夺回后使用其真实设施、共享仓储并再回第一营地。|共享/独立字段正确，不新增两地搬运或复制建筑。|NOT_RUN|
|T100-C07|通关继续|胜利前后分别保存、自然退出、继续和离区重载。|永久清敌/奖励/救援/第二营地状态保留，游戏可继续。|NOT_RUN|


全局挂接索引：T-002, T-003, T-005, T-011, T-015, T-022。它们来自现有TEST_MATRIX；正式数值以已批准增量/ACCEPTANCE和本单细化步骤为准，不恢复历史灰盒规则。

本单需新增/复用定向原生测试；新前缀建议 `Hearthward.Iteration.Task100.`。先注册并核验找到用例数量>0，再按公开UEClient运行，不能拿本表局部ID当UE过滤器。正常输入、渲染、真实模型或真人用例另行执行。

公共检查：`python -X utf8 scripts/validate_repo.py`、`python -X utf8 -m unittest discover -s scripts/tests -v`、`git diff --check`；在包含获批本单快照的实际基线上运行 `python -X utf8 scripts/validate_repo.py --task TASK-100 --base <实际完整SHA>`。UE操作/证据要求见执行约定；缺环境则明确NOT_RUN，不能将文件编写完成等同测试通过。

## 8. 交付物与完成定义

- 全主支线/救援/驻军/控制区覆盖表
- 四区样板与差异化空间说明、限定代码/包改动
- 合法全主线时间线与支线/第二营地证据
- 供103真人试玩使用的阶段检查表，不含伪填样本

固定收尾：更新`README.md`当前说明、同名MD/JSON与`docs/handoffs/TASK-100.md`；在`docs/qa/TASK-100/REPORT.md`绑定最终完整SHA、资产/模型指纹、环境、命令、逐用例结果和明确限制。本轮已生成REPORT，已执行静态覆盖、待原生复现和正常全流程分层记录。

工程实现完成、Owner视觉、真人体验、二机、性能与发布各自登记。依赖下游可用产物写入handoff，不把未验项目涂为PASS。没有授权时交付本地改动和证据并写“未提交/未推送/未发布”，不得自增权限。

## 9. 本单明确不做

- 首版边界外剧情与战争系统
- 未经实玩依据任意调资源/经验/时长
- 用旧049传送脚本冒充完整游玩

## 10. 失败、范围变化和恢复

首先保留最短复现、受测状态和原始证据，再定位到本单或其他负责任务；不能通过清空存档/赠送物资/瞬移/改验收阈值绕过阻塞。回退只针对本单经核对的改动或资产备份，禁止未经授权reset/clean、覆盖他人工作和强制解锁。确需改变共享契约、保存格式、正式配表或包范围时提交具体差异与影响，先获准再施工。
