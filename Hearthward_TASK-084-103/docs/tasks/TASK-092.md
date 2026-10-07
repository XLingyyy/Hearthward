# TASK-092｜首次救援往返路线与遭遇灰盒打磨

> 状态：Backlog（任务单已编写，实施未派发）。优先级：P0。阶段：C 地图与场景。日期：2026-10-06。Owner：XLingyyy。Reviewer／Issue：未指派。建议分支：`codex/TASK-092-rescue-route`（未创建）。

[本批总入口](../planning/TASK-084-103/README.md) · [执行约定](../planning/TASK-084-103/EXECUTION_GUIDE.md) · [元数据](TASK-092.json) · [交接模板](../handoffs/TASK-092.md)

## 1. 目标与预期结果

以084冻结的既有首救路线为唯一对象，修复通行、侦察、撤退和救回后的返营链路，形成供美术替换的可玩的关卡样板。

## 2. 当前基础与事实边界

既有自然世界与049战役含任务、敌人、救援和回营逻辑；本单不重新设计奖励、驻军和人口规则。先查已有路段与遭遇，缺什么补什么。

本单参考基线为 `main@6fcf5c22e965f0f7409438f19bc7b09e96ffb058`；实际开工必须复核届时HEAD与依赖产物。这里不是本轮运行结果，旧报告PASS/FAIL均绑定原受测实现。新行为是本单计划，不能写成当前已经存在。

## 3. 依赖、开工门槛与最小必读

前置：[TASK-091](TASK-091.md)

本单允许候选C++文件；地图资产和gameplay.json默认不开放。确需修改时以准确包/JSON指针追加范围，取得授权并在基线记录，不用绕改其他文件规避。

共同必读：`AGENTS.md`、`WORKFLOW.md`、`docs/START_HERE.md`、`docs/PROJECT_STATE.md`、`docs/design/CURRENT.md`及本单MD/JSON，然后执行本批指南中的接手/路径核对。

本单额外入口（路径为只读定位，不等于修改授权；前置任务产物需先实际生成）：

- `Source/Hearthward/Campaign/HearthwardCampaignWorld.cpp`
- `Source/Hearthward/Campaign/HearthwardCampaignActor.cpp`
- `Source/Hearthward/Campaign/HearthwardCampaignInteraction.cpp`
- `Source/Hearthward/Campaign/HearthwardCampaignQuests.cpp`
- `Resources/Data/gameplay.json`
- `docs/design/DSGN-003-first-release-slice.md`
- `docs/planning/TASK-084-103/ROUTE_MANIFEST.json`

## 4. 修改范围与协作边界

除本单MD/JSON、handoff、QA目录、专用规划目录和根`README.md`固定收尾外，候选范围如下；最终以**已获准基线JSON**为准。

- `Source/Hearthward/Campaign/HearthwardCampaignWorld.cpp`
- `Source/Hearthward/Campaign/HearthwardCampaignActor.cpp`
- `Source/Hearthward/Campaign/HearthwardCampaignInteraction.cpp`
- `Source/Hearthward/Campaign/HearthwardCampaignQuests.cpp`
- `docs/planning/TASK-084-103/ROUTE_MANIFEST.json`
- `Resources/Data/quest_guidance.json`
- `Source/Hearthward/Tests/RescueRouteTests.cpp`

本单不授予整个Content目录。准确包名单由现场引用/094清单确定，先补入已批准快照并核验LFS锁，才可编辑。未批准包仅可只读检查。

拟新增辅助/测试文件不表示仓库已存在；先搜索可复用类型。共享UI配置、公共头文件、WorldPresentation/CampaignWorld等按[写入调度](../planning/TASK-084-103/DISPATCH.md)串行。临时输出使用 `.agent-local/qa/TASK-092/<唯一run>/`，不得写入用户原档。

## 5. Agent具体实施步骤

### 1. 复现路线断点

重走084记录路线，列碰撞/坡度/门洞/导航/流送/触发五类缺口，标明受影响现有对象。检查实现是否运行时生成；可在生成代码修正的，不随意改主umap。

### 2. 画功能灰盒

在规划文件中标出营地出口、过渡段、可观察敌情位置、救援交互区、退出与返营接续；使用既有地形，不新开大地图。为每一处调整说明“为什么玩家要经过/能作何选择”。

### 3. 修实际通行

按当前角色与弟弟/获救对象胶囊、移动/攀越规则调整障碍、台阶和门洞；确保地面连续、落脚稳定、导航可达，避免只对玩家跳跃可过而跟随者过不去。更改碰撞包前完成逐包路径授权和LFS锁。

### 4. 打磨单处遭遇

在既有敌数、职业、警戒和增援规则内调整视线/掩体/入口；玩家能在被迫近战前观察，存在符合原规则的绕行/处理/撤退方式。不能为造选择关闭敌人AI、使其永不发现或直接减少正式驻军。

### 5. 救援与安全承接

验证解救、跟随/等待/再次接续、回到营地的人口增长与奖励；危险条件不允许解救/承接时原样提示。不能通过UI点击把被救人直接入库或空降营地。

### 6. 路标与非奖励内容

仅补充有用途的地标、观察点和环境线索，灰盒模型可复用；不凭空添加资源奖励/随机箱子/新敌人。与091同步入口节点坐标及任务图示，物理位置是唯一依据。

### 7. 反向与恢复测试

往返均实走，测试弟弟留营和同行两种已允许分工选择；分别在出发、救援后返营、到营前保存继续。测试区域卸载重载与旧任务进度，不复制敌人/族人或重复奖账本。

### 8. 冻结美术接入边界

输出通行走廊、角色净空、不可侵入触发区、碰撞/导航要求与测量截图，交给095—098；后续美术只替换表现不能偷改灰盒路线功能。

## 6. 不可破坏的规则

- 真实普通输入的完整路线是验收对象；注入位置/强制阶段只能另列诊断夹具。
- 若需要移动正式资源/敌人/任务坐标，先列gameplay.json精确JSON指针、规则影响和Owner批准，再扩allowed_paths；当前JSON未给整表写权。
- 新增/修改.umap和ExternalActors都需列准确包并取得锁，不授权整个Content/。

所有更改继续遵守只读显示、原事务执行、稳定ID、时间线隔离和不伪填验收规则。规则未明确的新玩法不硬编码；不删测试/吞错误/全仓重构来让当前检查通过。

## 7. 验收用例与执行方式

每条用例在隔离档/明确诊断夹具中建立前置，实际记录操作与断言。夹具、注入输入、真实OS键鼠、真实模型、真人样本分层统计；下面所有结果尚未运行。

|局部编号|场景|前置/具体操作|通过标准|初始结果|
|---|---|---|---|---|
|T092-C01|正向通行|玩家正常走营地到首救点，弟弟跟随。|无卡死/掉地/必须调试传送；弟弟自然到达。|NOT_RUN|
|T092-C02|观察与撤退|到观察点辨敌，再绕行或处理；进入风险后合法撤退。|存在可解释选择，警戒/追击遵守原规则。|NOT_RUN|
|T092-C03|救援对象|解救、等待、继续跟随、返营接续。|对象真实移动，安全条件正确，人口最多加一次。|NOT_RUN|
|T092-C04|伙伴留营|弟弟留在安全营地工作，玩家走允许的路线。|不强迫互斥任务并发；是否可完成按正式条件给反馈。|NOT_RUN|
|T092-C05|双向碰撞|按正常步行/冲刺/原有攀越回走门槛、坡与狭口。|无单向卡路，角色/弟弟/被救者均按其能力通过。|NOT_RUN|
|T092-C06|存读档流送|出发/救援后/到营前三节点保存继续，卸载重载区域。|任务/敌人/族人/掉落不重置套利，路线仍可走。|NOT_RUN|
|T092-C07|资料交接|核对美术净空图与导航配置。|所有受影响包/坐标/触发区可追溯，091显示对应实际位置。|NOT_RUN|


全局挂接索引：T-002, T-003, T-007, T-011, T-014。它们来自现有TEST_MATRIX；正式数值以已批准增量/ACCEPTANCE和本单细化步骤为准，不恢复历史灰盒规则。

本单需新增/复用定向原生测试；新前缀建议 `Hearthward.Iteration.Task092.`。先注册并核验找到用例数量>0，再按公开UEClient运行，不能拿本表局部ID当UE过滤器。正常输入、渲染、真实模型或真人用例另行执行。

公共检查：`python -X utf8 scripts/validate_repo.py`、`python -X utf8 -m unittest discover -s scripts/tests -v`、`git diff --check`；在包含获批本单快照的实际基线上运行 `python -X utf8 scripts/validate_repo.py --task TASK-092 --base <实际完整SHA>`。UE操作/证据要求见执行约定；缺环境则明确NOT_RUN，不能将文件编写完成等同测试通过。

## 8. 交付物与完成定义

- 路线灰盒方案、净空/碰撞/导航保护清单
- 限定范围的路线修复及测试
- 真实往返与救援成长证据、供美术接入的冻结清单

固定收尾：更新`README.md`当前说明、同名MD/JSON与`docs/handoffs/TASK-092.md`；在`docs/qa/TASK-092/REPORT.md`绑定最终完整SHA、资产/模型指纹、环境、命令、逐用例结果和明确限制。该REPORT为未来执行产物，本包不预填。

工程实现完成、Owner视觉、真人体验、二机、性能与发布各自登记。依赖下游可用产物写入handoff，不把未验项目涂为PASS。没有授权时交付本地改动和证据并写“未提交/未推送/未发布”，不得自增权限。

## 9. 本单明确不做

- 扩大世界面积、重配敌军总数/奖励/任务规则
- 新建营救剧情或随机事件系统
- 把一处路线测试当完整四区通关

## 10. 失败、范围变化和恢复

首先保留最短复现、受测状态和原始证据，再定位到本单或其他负责任务；不能通过清空存档/赠送物资/瞬移/改验收阈值绕过阻塞。回退只针对本单经核对的改动或资产备份，禁止未经授权reset/clean、覆盖他人工作和强制解锁。确需改变共享契约、保存格式、正式配表或包范围时提交具体差异与影响，先获准再施工。
