# Agent 执行约定｜TASK-084—103

## 0. 这是什么

这是20份待派发执行任务，不是已执行结果。所有任务初始化为Backlog，Owner沿项目现有记录为XLingyyy，Reviewer未指派，分支名是建议名称且尚未创建。当前用户授权范围是“编写任务单并交付”，不是本批游戏修改、提交/推送、合并、付费生成或新Release发布。

任务包可一次性由Owner确认范围并派发，不要求Agent对已获得且没有变化的同一授权反复询问。未派发前只读接手；正常依赖已满足后按步骤施工，不自行添加新系统。仅在真正需要扩大范围、改变已批准规则或涉及未授权外部动作时报告具体决策项。

## 1. 依据和现状

固定规划参考main：`6fcf5c22e965f0f7409438f19bc7b09e96ffb058`，读取日期2026-10-06。最新预览报告为`docs/releases/demo-20261006-2/REPORT.md`。AGENTS/WORKFLOW、CURRENT指向的GDD及已批准增量为上位项目约束；本包是这些约束下的增量计划，不把上一轮建议自动当成批准的新玩法。

先读`AGENTS.md`、`WORKFLOW.md`、`docs/START_HERE.md`、`docs/PROJECT_STATE.md`和本单MD/JSON，再读任务列出的最小相关源码/契约。文档中有多期记录，最新发行事实与历史受测证据分开；旧单Active不必等于其代码未合入，不能为解锁依赖伪填旧单Done。

## 2. 开工和基线快照（避免范围校验陷阱）

Owner派单时确认一个可写Agent、专用目录、实际branch/HEAD、当前允许路径与所需准确资产包，以及提交/推送/合并/发布各自权限。将授权原话/时间/范围写入本单任务快照与交接；记录真实状态。Reviewer未指派保持null，不冒名填写。

仓库`validate_repo.py --task ... --base ...`从**base提交中的任务JSON**读取允许范围，不从工作区修改后的JSON授予权限。参考main还没有084—103，不能拿这个旧SHA直接做新任务的通过基线。应先在Owner授权的文档登记步骤中纳入本批快照/准确包清单，得到实际存在的登记或激活提交，再以包含获准范围的真实SHA校验代码变更。没有提交权限时交付待审文档补丁，明确范围检查未就绪；不能改验证器、排除失败或用空diff假装通过。

后续扩大资产/共享接口范围也先登记获批快照，再实施，不先改再把JSON加宽来掩盖越界。执行时若远端已有同号文件或任务归属变化，停止覆盖并报告冲突，不私自重编号或删除对方任务。

## 3. 路径与新文件约定

JSON的allowed_paths是拟定/派发时使用的版本控制范围；不在其中的路径默认不可写。精确文件可为拟新增（尤其本轮ReadModels/Tracker/Guidance和测试文件）；先检索同义实现并复用，不盲目复制同名结构。任务MD里的“必读入口”不等于写权限。

公共`README.md`为每单必要收尾路径，但要单写者串行更新，修改当前说明而不是向错误旧说明后追加历史。`Resources/UI/layout.json`、`interface.json`、ScreenWidget.h/ScreenActions.cpp、CampaignWorld.cpp、WorldPresentation.cpp与PresentationComponent.cpp分别按调度表开独占写窗口。

资产任务没有授予整`Content/`目录。先从094与现场引用生成准确`.uasset`、`.umap`、外部Actor/必要派生包列表，获准加入本单JSON后取得LFS锁；多Agent不能文本合并二进制。保留原源与可回退指纹；未知锁/他人未保存编辑器时不覆盖。

制作源放`art_source/TASK-xxx/`，导入资产放已批准的`Content/`包，运行图/布局/数据放`Resources/`。临时实机/验证输出放`.agent-local/qa/TASK-xxx/<唯一run>/`，只提升报告引用的必要证据到`docs/qa/TASK-xxx/`。不要新增根Resource/ui pic/日期素材目录，也不删除既有失败证据。

本包不含模型/字体/UE资产，禁止把字体文件作为任务交付附件。GameFactory为独立工具仓库；本批不授权修改上层工具、安装依赖、改引擎关联或更换本地模型。

## 4. 通用执行顺序

1. 核对实际仓库根、branch、HEAD、未提交清单、任务归属和权限；运行`python -X utf8 scripts/agent_context.py --task TASK-xxx`。
2. 核对任务依赖的实际产物与SHA；缺哪个产物就报告哪个，不以旧任务状态标签机械推断。先用旧入口复现问题，建立RED或当前基线。
3. 对公共接口/资产包/配置写入确认独占窗口。先做最小样板，再扩大到本单完整范围；每次改动可回退，稳定ID与原始记录不改。
4. 实施任务逐步要求和本单用例。Native/夹具/注入输入用于定位，正常正式地图键鼠路线和真实模型、真人样本分开验证。
5. 对同一最终源码/资产版本重跑受影响检查，记录未测和失败。代码改变后不能无依据沿用旧PASS。
6. 更新README、本单MD/JSON与handoff，保留证据指纹、命令、限制和下一步；提交、推送、合并、发布按本次实际权限分别执行或明确未执行。

## 5. 公共检查和可复制命令

下列命令在**完整且已正确纳入本包的游戏仓库**执行；任务包本身不是UE工程。python指实际项目Python，使用既有环境配置，不硬编码他人磁盘路径。

```text
python -X utf8 scripts/agent_context.py --task TASK-084
python -X utf8 scripts/validate_repo.py
python -X utf8 -m unittest discover -s scripts/tests -v
git diff --check
python -X utf8 scripts/validate_repo.py --task TASK-084 --base <已包含获准084快照的实际完整SHA>
```

把084换成当前任务编号。尖括号项是必须从Git取得的真实值，不能逐字当命令参数。若全仓有既有失败，原样报告并区分本批新增问题，不改验证器或删除旧文件。

UE构建/Automation/PIE/Standalone/Shipping使用项目既有公开UEClient和当前任务/发行报告验证过的入口，先读`docs/qa/BUILD_AND_TEST.md`与最新报告，不把该文件早期灰盒段落当当前操作/数值。当前README示例`python -X utf8 scripts/ui/verify_input_client.py --build --label TASK-xxx`也需先检查脚本参数与本机环境解析，再执行。

本包没有实现各任务未来的测试脚本或C++测试。标注“拟新增”的`...Tests.cpp`由执行Agent编写，新增原生测试前缀建议`Hearthward.Iteration.TaskNNN.`；注册后按公开接口运行：

```python
# ue是按本机.environment配置显式构造的公开UEClient，不使用未知全局默认工程。
# report_dir为本单隔离输出；filter_name为实际已注册前缀，不是随意猜的旧用例名。
result = ue.testing.run_automation_tests(
    filter_name, report_dir=str(report_dir), extra_args=["-NullRHI"], timeout=240
)
assert result["ok"]
assert result["payload"]["tests_found"] > 0  # 0个用例不能算通过
```

NullRHI只用于不依赖渲染的原生逻辑测试。布局/截图/场景/动作/音效必须有渲染并按实际输入运行。084、094属于调查/制作准备，不强制编造原生测试；103以实际候选包/真人和二机验证为主。

## 6. 测试ID与原验证器兼容

JSON `required_tests`只使用现有`docs/qa/TEST_MATRIX.md`中T-001—026的已登记索引；`blocked_by`是已登记R项引用数组，不塞入任意自然语言。未派发和依赖产物门槛写notes/本单说明，而不是伪造R编号。

每单细化用例采用`T084-C01`这种**本单局部编号**，写在MD中并汇总到本批TEST_CASES.csv，不冒充已登记的全局T号或已存在UE测试名。JSON中的测试族与本单细化用例并行使用；完整全局族没执行不得标整个族PASS。

全局TEST_MATRIX是早期索引，部分数值/叙述具有历史性。正式期望使用CURRENT及已批准增量和`docs/planning/TASK-053-074/ACCEPTANCE.md`，不恢复过时的灰盒移动速度、战斗传送或未定规则。发现冲突记录来源与采用理由，不静默改历史矩阵。

## 7. 证据、缺陷与完成定义

每次报告至少有：完整源码SHA、dirty diff或补丁指纹、资产包/源指纹、模型/后端配置、UE/系统/硬件、命令、隔离档路径和初始哈希、每用例步骤/期望/实际、原始日志/截图/录像位置、结果与限制。只有操作确实执行且断言成立才PASS；FAIL、BLOCKED、NOT_RUN、NOT_REPRODUCED不可互换。

工程完成＝限定实现+相关Native/渲染/正常输入验证+README/交接；不等于Owner视觉通过、真人节奏通过、全批性能达标或已发布。所有真人/录音/第二机器不足均可交付已完成子范围，但保留门槛未完成；禁止伪造样本。

范围验证器在Review及后续状态要求实际独立Reviewer，而一些历史批次记录Owner与Reviewer同人。新单当前null不会触发冲突；升状态前按Owner实际协调处理，不造Reviewer、不修改校验器来绕过。

## 8. 本批明确不做

不新增联机、好感数值门槛、普通族人死亡/床位门槛、两营地运输、食物腐败、强制资源预留、任意多任务队列、敌军长期自适应、敌方腹地反击或整城室内。个人自动跨资源点、采集队定量停止需另行具体设计，不混入本批可靠性修复。固定人声继续暂缓，动态回复文字；不擅自付费调用/采购或换模型。
