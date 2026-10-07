# TASK-091｜地图可读性与关键入口转折引导

> 状态：Backlog（任务单已编写，实施未派发）。优先级：P1。阶段：C 地图与场景。日期：2026-10-06。Owner：XLingyyy。Reviewer／Issue：未指派。建议分支：`codex/TASK-091-map-route-guidance`（未创建）。

[本批总入口](../planning/TASK-084-103/README.md) · [执行约定](../planning/TASK-084-103/EXECUTION_GUIDE.md) · [元数据](TASK-091.json) · [交接模板](../handoffs/TASK-091.md)

## 1. 目标与预期结果

保留现有矩形地图与传送规则，解决目标在墙后/楼下而玩家不知道入口的问题；只对关键现有路线增加入口和转折引导，不做全世界逐拐点导航。

## 2. 当前基础与事实边界

当前Map代码分别处理4032米级底层地形和3000×2000米查看区域；任务标记是直线距离和目标投影，不保证可行走。地图自己暂停不应再拦截合法传送。

本单参考基线为 `main@6fcf5c22e965f0f7409438f19bc7b09e96ffb058`；实际开工必须复核届时HEAD与依赖产物。这里不是本轮运行结果，旧报告PASS/FAIL均绑定原受测实现。新行为是本单计划，不能写成当前已经存在。

## 3. 依赖、开工门槛与最小必读

前置：[TASK-090](TASK-090.md)

依赖的约定产物与实际实现SHA就绪、Owner派发且共享写窗口空闲后开始。依赖不要求伪改旧任务Done；读取其实际交接并核对采用版本。

共同必读：`AGENTS.md`、`WORKFLOW.md`、`docs/START_HERE.md`、`docs/PROJECT_STATE.md`、`docs/design/CURRENT.md`及本单MD/JSON，然后执行本批指南中的接手/路径核对。

本单额外入口（路径为只读定位，不等于修改授权；前置任务产物需先实际生成）：

- `Source/Hearthward/UI/HearthwardScreenMap.cpp`
- `Source/Hearthward/UI/HearthwardQuestGuidance.cpp`
- `Source/Hearthward/UI/HearthwardScreenMapTest.inl`
- `Resources/UI/interface.json`
- `Source/Hearthward/Campaign/HearthwardCampaignWorld.cpp`
- `docs/planning/TASK-084-103/ROUTE_MANIFEST.json`

## 4. 修改范围与协作边界

除本单MD/JSON、handoff、QA目录、专用规划目录和根`README.md`固定收尾外，候选范围如下；最终以**已获准基线JSON**为准。

- `Source/Hearthward/UI/HearthwardScreenMap.cpp`
- `Source/Hearthward/UI/HearthwardQuestGuidance.cpp`
- `Source/Hearthward/UI/HearthwardQuestGuidance.h`
- `Source/Hearthward/UI/HearthwardScreenWidget.h`
- `Source/Hearthward/UI/HearthwardScreenActions.cpp`
- `Source/Hearthward/UI/HearthwardGuidanceRoutes.cpp`
- `Source/Hearthward/UI/HearthwardGuidanceRoutes.h`
- `Resources/Data/quest_guidance.json`
- `Resources/UI/layout.json`
- `Resources/UI/interface.json`
- `Source/Hearthward/Tests/MapGuidanceRouteTests.cpp`

Content与Save等未授权领域只读；例外仅以JSON实际范围为准，不能用必读清单扩大写权限。

拟新增辅助/测试文件不表示仓库已存在；先搜索可复用类型。共享UI配置、公共头文件、WorldPresentation/CampaignWorld等按[写入调度](../planning/TASK-084-103/DISPATCH.md)串行。临时输出使用 `.agent-local/qa/TASK-091/<唯一run>/`，不得写入用户原档。

## 5. Agent具体实施步骤

### 1. 核对地图真值

列明地形原点/范围、视窗尺寸、MapPoint/MapWorldAt坐标换算、地图朝向与人物图标；先跑082旧回归，保护等比例、全屏覆盖、拖动边界和传送落点逻辑。

### 2. 配置关键路线

从084清单选择卧室出口、楼梯、庭院侧门及首救路线入口；为各任务阶段配置可知入口/过渡节点，使用稳定任务/地点ID和真实采样坐标。拟新增quest_guidance.json只描述导航表现，不写奖账本/任务状态。

### 3. 区分目标和途经点

主目标继续指向任务终点；在存在有效路线且玩家处在适用区域时显示“先到侧门/楼梯”等下一可达入口，可附高/低处说明。入口标记不得被计作任务完成点，直线距离明确不伪称步行路程。

### 4. 处理节点进退

节点通过依据正式位置/阶段或可验证空间条件；允许玩家走另一合法路，不强制逐点触发。玩家偏离、返走、Load、传送、阶段变化时重新求下一有效节点；找不到适用路线则退回普通目标标记，不盲目画穿墙导航线。

### 5. 优化地点标签

缩放时按重要性裁剪/避让标签，保持关键地点可点；保留红蓝火焰朝向与任务/个人路标区别。无黑雾不意味着显示未知剧情，地点列表继续排除物品/人物。不要恢复边框、黑边和常驻操作说明。

### 6. 保护传送限制

展开地点和现行传送流程只改善失败说明，不跳过激活/剧情/危险/集合/落点准备。地图造成的暂停与其他禁止条件分开，传送后重新定位人物并清除旧途经点。

### 7. 完整路线实走

正式新游戏按实际键鼠由卧室走至侧门，再测营地路线；弟弟真实跟随，不用传送验证可达性。092修改几何后只重测受影响节点与地图映射，保持同一路线清单。

## 6. 不可破坏的规则

- 不新增地图黑雾、室内分层地图、全局自动寻路或未知资源泄漏。
- 导航数据不是任务真值；缺/坏导航数据可安全降级，不能使任务不可完成。
- 3000×2000是查看区域，不据此缩裁实际世界或移动游戏对象。

所有更改继续遵守只读显示、原事务执行、稳定ID、时间线隔离和不伪填验收规则。规则未明确的新玩法不硬编码；不删测试/吞错误/全仓重构来让当前检查通过。

## 7. 验收用例与执行方式

每条用例在隔离档/明确诊断夹具中建立前置，实际记录操作与断言。夹具、注入输入、真实OS键鼠、真实模型、真人样本分层统计；下面所有结果尚未运行。

|局部编号|场景|前置/具体操作|通过标准|初始结果|
|---|---|---|---|---|
|T091-C01|坐标互逆|边界/中心/缩放/平移点做世界到图再反变换。|误差按原精度界限记录，人物与建筑位置对应，无横纵拉伸。|NOT_RUN|
|T091-C02|卧室转折|目标在墙后/楼下，正常实走门口、楼梯、侧门。|提示可解释实际入口，接近终点不指向错误高度层。|NOT_RUN|
|T091-C03|偏离返走|绕另一合法路径、返走、取消追踪再恢复。|不强迫过点，不累积旧节点，不改任务进度。|NOT_RUN|
|T091-C04|时间线|Load/阶段推进/领奖/合法传送后重新开图。|旧标记清除，新阶段目标正确，未知信息不泄漏。|NOT_RUN|
|T091-C05|传送|未激活/危险/伙伴条件不足及全部合法的目的地各测。|拒绝有原因，合法地图暂停不误挡，落点可走。|NOT_RUN|
|T091-C06|布局|最低/最高缩放、4:3/超宽/大字号、密集标签。|保持全屏矩形与比例，关键入口可识别且无深色留边。|NOT_RUN|
|T091-C07|无数据降级|缺失/坏路线条目或定位对象失效。|普通目标标记仍安全工作，无崩溃、不阻断任务。|NOT_RUN|


全局挂接索引：T-002, T-011, T-025。它们来自现有TEST_MATRIX；正式数值以已批准增量/ACCEPTANCE和本单细化步骤为准，不恢复历史灰盒规则。

本单需新增/复用定向原生测试；新前缀建议 `Hearthward.Iteration.Task091.`。先注册并核验找到用例数量>0，再按公开UEClient运行，不能拿本表局部ID当UE过滤器。正常输入、渲染、真实模型或真人用例另行执行。

公共检查：`python -X utf8 scripts/validate_repo.py`、`python -X utf8 -m unittest discover -s scripts/tests -v`、`git diff --check`；在包含获批本单快照的实际基线上运行 `python -X utf8 scripts/validate_repo.py --task TASK-091 --base <实际完整SHA>`。UE操作/证据要求见执行约定；缺环境则明确NOT_RUN，不能将文件编写完成等同测试通过。

## 8. 交付物与完成定义

- 关键入口/节点配置及权威ID映射
- 地图/指引实现与回归
- 正常输入路线录像、坐标/传送/缩放验证

固定收尾：更新`README.md`当前说明、同名MD/JSON与`docs/handoffs/TASK-091.md`；在`docs/qa/TASK-091/REPORT.md`绑定最终完整SHA、资产/模型指纹、环境、命令、逐用例结果和明确限制。该REPORT为未来执行产物，本包不预填。

工程实现完成、Owner视觉、真人体验、二机、性能与发布各自登记。依赖下游可用产物写入handoff，不把未验项目涂为PASS。没有授权时交付本地改动和证据并写“未提交/未推送/未发布”，不得自增权限。

## 9. 本单明确不做

- 改正式地图边界或任务触发/奖励
- 全地图地面导航线和室内分层系统
- 扩大传送权限、揭露未知剧情

## 10. 失败、范围变化和恢复

首先保留最短复现、受测状态和原始证据，再定位到本单或其他负责任务；不能通过清空存档/赠送物资/瞬移/改验收阈值绕过阻塞。回退只针对本单经核对的改动或资产备份，禁止未经授权reset/clean、覆盖他人工作和强制解锁。确需改变共享契约、保存格式、正式配表或包范围时提交具体差异与影响，先获准再施工。
