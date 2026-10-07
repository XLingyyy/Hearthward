# TASK-096｜石堡开场必经区域、近景材质与夜袭氛围

> 状态：Active（独立技术调查已实施；Owner新资产样板暂缓，运行验收未完成）。优先级：P1。阶段：D 关键资产。日期：2026-10-07。Owner：XLingyyy。Reviewer／Issue：未指派。实际分支：`codex/TASK-084-103-iteration`。

[本批总入口](../planning/TASK-084-103/README.md) · [执行约定](../planning/TASK-084-103/EXECUTION_GUIDE.md) · [元数据](TASK-096.json) · [执行交接](../handoffs/TASK-096.md)

## 2026-10-07 执行进展

3个正式石堡立面候选包、原生碰撞/装饰职责、光照关卡与读档回调生命周期已盘点。

077石质山堡方向已确认；近景局部灯光材质/补面和火烟舒适度首件实机样板待Owner审定，不重复询问石堡/木堡方向。

采用084真实路线，092连续通行与真实地形Z未完成，未冻结碰撞；原生077净空夹具、现有材质只读查询和反复进入/读档计数可独立执行。 准确候选包已引用094，`selected_for_change=[]`；未编辑Content或共享源码。

实际证据：[QA报告](../qa/TASK-096/REPORT.md)、[技术盘点](../qa/TASK-096/TECHNICAL_AUDIT.json)、[本单包范围](../assets/TASK-096/PACKAGE_SCOPE.json)。下表为完整验收要求，本轮运行结果均单列在QA报告，不沿用历史PASS。

## 1. 目标与预期结果

把已经可走的石堡开场收尾为完整可读的卧室—回廊—楼梯—庭院—侧门体验，优先近景结构与有限夜袭表现，不做全城室内。

## 2. 当前基础与事实边界

TASK-077保留可运行布局与裁分立面，主体多为外观体；已有无光照源与世界时钟调色不能直接当作最终近景/火光验收。必须保留原可走路线和撤离规则。

本单参考基线为 `main@6fcf5c22e965f0f7409438f19bc7b09e96ffb058`；实际开工必须复核届时HEAD与依赖产物。这里不是本轮运行结果，旧报告PASS/FAIL均绑定原受测实现。新行为是本单计划，不能写成当前已经存在。

## 3. 依赖、开工门槛与最小必读

前置：[TASK-091](TASK-091.md)、[TASK-094](TASK-094.md)、[TASK-095](TASK-095.md)

本轮已获用户执行授权；采用当前依赖中的明确可用产物先行完成独立技术调查，资产替换和共享源码修改仍需依赖与准确写窗口。依赖不要求伪改旧任务Done；读取其实际交接并核对采用版本。

共同必读：`AGENTS.md`、`WORKFLOW.md`、`docs/START_HERE.md`、`docs/PROJECT_STATE.md`、`docs/design/CURRENT.md`及本单MD/JSON，然后执行本批指南中的接手/路径核对。

本单额外入口（路径为只读定位，不等于修改授权；前置任务产物需先实际生成）：

- `docs/qa/TASK-077/REPORT.md`
- `art_source/TASK-077/README.md`
- `Source/Hearthward/Campaign/HearthwardCampaignWorld.cpp`
- `Source/Hearthward/Gameplay/HearthwardWorldPresentation.cpp`
- `Source/Hearthward/Experience/HearthwardPresentationComponent.cpp`
- `Resources/Data/quest_guidance.json`

## 4. 修改范围与协作边界

除本单MD/JSON、handoff、QA目录、专用规划目录和根`README.md`固定收尾外，候选范围如下；最终以**已获准基线JSON**为准。

- `art_source/TASK-096/`
- `docs/assets/TASK-096/`
- `Source/Hearthward/Campaign/HearthwardCampaignWorld.cpp`
- `Source/Hearthward/Gameplay/HearthwardWorldPresentation.cpp`
- `Source/Hearthward/Gameplay/HearthwardWorldPresentation.h`
- `Source/Hearthward/Experience/HearthwardPresentationComponent.cpp`
- `Source/Hearthward/Experience/HearthwardPresentationComponent.h`
- `Resources/Data/quest_guidance.json`
- `Source/Hearthward/Tests/StoneholdOpeningPolishTests.cpp`

本单不授予整个Content目录。准确包名单由现场引用/094清单确定，先补入已批准快照并核验LFS锁，才可编辑。未批准包仅可只读检查。

拟新增辅助/测试文件不表示仓库已存在；先搜索可复用类型。共享UI配置、公共头文件、WorldPresentation/CampaignWorld等按[写入调度](../planning/TASK-084-103/DISPATCH.md)串行。临时输出使用 `.agent-local/qa/TASK-096/<唯一run>/`，不得写入用户原档。

## 5. Agent具体实施步骤

### 1. 保护已有通路

记录077净空/门槛/台阶/碰撞和091关键节点，存好当前源与包指纹。判断每个面是否玩家可触达，区分近景游戏结构、远景外观和诊断试验网格，不把被拒整网格重新接回。

### 2. 优先结构收边

修卧室地墙交界、门框、回廊栏杆、楼梯、庭院基座和侧门等近景缺面/穿插/浮空；用清晰可控模块补背面/收边，简化碰撞继续独立于杂乱生成网格。

### 3. 处理材质差异

对无光照源先保留诊断对照，测试白天/20:00开局/火光下与原生结构的接缝。需要受光的可达近景按批准方向重建材质，不能只换shader就宣称PBR完成；远景保留源也需避免夜间不合逻辑发亮。

### 4. 补有限生活痕迹

仅在必经视线补床边、灯笼、木箱、织物等已批准风格道具，保证遗物包、门口和可交互对象区别清楚；摆件不遮路、不抢交互、不增加奖励容器。

### 5. 有限夜袭氛围

在已发生夜袭的正确阶段接入局部火光、烟尘和远处动静的表现；沿既有阶段重建/清理，Load不能重复叠加。不可用烟雾完全遮住入口/敌人，不新增伤害火区、强制镜头或新剧情目标。

### 6. 输入与时钟不变

新档仍按当前第1日20:00；无光照对照/临时日光只用于诊断，不写正式环境。对话和暂停仍按既有规则，天气/灯光表现不得改变权威感知亮度与生产难度规则。

### 7. 实走与Cook验证

通过逐包授权与锁后导入并在正式地图新档连续撤离，检查弟弟跟随、遗物拾取、日夜、存读档与独立Cook。补图注明诊断视角或真实游戏视角。

### 8. 保留未完成边界

未制作的主堡/石屋内室保持外观体并给合理视觉边界，不用假门诱导不存在的玩法。报告列已收尾的必经范围，不宣称整堡或整个夜袭演出完成。

## 6. 不可破坏的规则

- 不移动主线触发/撤离条件，不新增火焰伤害、强制过场或整城可探索内室。
- 任何新碰撞替换后重跑玩家/弟弟路线；不可用瞬移证明通行。
- 主umap/ExternalActors默认只读，确需编辑按准确包授权，不覆盖未保存编辑器。

所有更改继续遵守只读显示、原事务执行、稳定ID、时间线隔离和不伪填验收规则。规则未明确的新玩法不硬编码；不删测试/吞错误/全仓重构来让当前检查通过。

## 7. 验收用例与执行方式

每条用例在隔离档/明确诊断夹具中建立前置，实际记录操作与断言。夹具、注入输入、真实OS键鼠、真实模型、真人样本分层统计；下面所有结果尚未运行。

|局部编号|场景|前置/具体操作|通过标准|初始结果|
|---|---|---|---|---|
|T096-C01|新档撤离|真实键鼠从卧室取物、叫弟弟、下楼到侧门撤离。|全程自然通行和推进，导航不中断。|NOT_RUN|
|T096-C02|近景几何|沿玩家正常视点检查地墙、门框、栏杆、楼梯、基座。|无阻断通路的穿插/浮空/缺面，关键对象可识别。|NOT_RUN|
|T096-C03|昼夜材质|同视角对照白天、正式开局夜间和局部火光。|源与补面协调；临时Unlit/日光诊断不冒充正式效果。|NOT_RUN|
|T096-C04|表现生命周期|夜袭阶段进入/退出，Load、重开新游戏。|火烟正确重建和清理，不无限叠加音光/VFX。|NOT_RUN|
|T096-C05|可读与舒适|正常亮度/大字号/对话开启时观察出口和任务提示。|路面与入口可辨，烟火不遮关键动作/救援。|NOT_RUN|
|T096-C06|原规则|对比时钟、遗物数量、任务阶段及撤离前后状态。|无免费物资或新增危险判定，游戏规则不变。|NOT_RUN|
|T096-C07|独立包|Cook后正常启动并走同路线。|所有新模块/材质/效果可加载，无编辑器依赖。|NOT_RUN|


全局挂接索引：T-002, T-003, T-011, T-021。它们来自现有TEST_MATRIX；正式数值以已批准增量/ACCEPTANCE和本单细化步骤为准，不恢复历史灰盒规则。

本单需新增/复用定向原生测试；新前缀建议 `Hearthward.Iteration.Task096.`。先注册并核验找到用例数量>0，再按公开UEClient运行，不能拿本表局部ID当UE过滤器。正常输入、渲染、真实模型或真人用例另行执行。

公共检查：`python -X utf8 scripts/validate_repo.py`、`python -X utf8 -m unittest discover -s scripts/tests -v`、`git diff --check`；在包含获批本单快照的实际基线上运行 `python -X utf8 scripts/validate_repo.py --task TASK-096 --base <实际完整SHA>`。UE操作/证据要求见执行约定；缺环境则明确NOT_RUN，不能将文件编写完成等同测试通过。

## 8. 交付物与完成定义

- 近景模块/材质/有限夜袭表现清单与源
- 逐包范围/LFS/碰撞净空记录
- 连续撤离、昼夜与独立运行证据

固定收尾：更新`README.md`当前说明、同名MD/JSON与`docs/handoffs/TASK-096.md`；在`docs/qa/TASK-096/REPORT.md`绑定最终完整SHA、资产/模型指纹、环境、命令、逐用例结果和明确限制。本轮已生成REPORT，已执行静态调查与待运行验收分别登记。

工程实现完成、Owner视觉、真人体验、二机、性能与发布各自登记。依赖下游可用产物写入handoff，不把未验项目涂为PASS。没有授权时交付本地改动和证据并写“未提交/未推送/未发布”，不得自增权限。

## 9. 本单明确不做

- 整城内室、完整新电影过场或战斗剧情重写
- 整份Marble试验网格直接作为碰撞
- 把诊断Unlit视口当正式日夜材质

## 10. 失败、范围变化和恢复

首先保留最短复现、受测状态和原始证据，再定位到本单或其他负责任务；不能通过清空存档/赠送物资/瞬移/改验收阈值绕过阻塞。回退只针对本单经核对的改动或资产备份，禁止未经授权reset/clean、覆盖他人工作和强制解锁。确需改变共享契约、保存格式、正式配表或包范围时提交具体差异与影响，先获准再施工。
