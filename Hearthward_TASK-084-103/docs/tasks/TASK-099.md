# TASK-099｜动作音效、环境声与事件反馈同步

> 状态：Backlog（任务单已编写，实施未派发）。优先级：P1。阶段：D 关键资产。日期：2026-10-06。Owner：XLingyyy。Reviewer／Issue：未指派。建议分支：`codex/TASK-099-audio-feedback`（未创建）。

[本批总入口](../planning/TASK-084-103/README.md) · [执行约定](../planning/TASK-084-103/EXECUTION_GUIDE.md) · [元数据](TASK-099.json) · [交接模板](../handoffs/TASK-099.md)

## 1. 目标与预期结果

补齐首线与已打磨场景的脚步/接触/命中/采集/环境声，使声音来自真实事件并可正确暂停、清理和混音；固定录音继续保持原暂缓决定。

## 2. 当前基础与事实边界

固定对白有已批准清单但录音未生产，动态弟弟回复仅文字。070明确本轮不录制、不用AI代替固定人声；不能用静音文件完成覆盖率。

本单参考基线为 `main@6fcf5c22e965f0f7409438f19bc7b09e96ffb058`；实际开工必须复核届时HEAD与依赖产物。这里不是本轮运行结果，旧报告PASS/FAIL均绑定原受测实现。新行为是本单计划，不能写成当前已经存在。

## 3. 依赖、开工门槛与最小必读

前置：[TASK-095](TASK-095.md)、[TASK-096](TASK-096.md)、[TASK-097](TASK-097.md)、[TASK-098](TASK-098.md)

依赖的约定产物与实际实现SHA就绪、Owner派发且共享写窗口空闲后开始。依赖不要求伪改旧任务Done；读取其实际交接并核对采用版本。

共同必读：`AGENTS.md`、`WORKFLOW.md`、`docs/START_HERE.md`、`docs/PROJECT_STATE.md`、`docs/design/CURRENT.md`及本单MD/JSON，然后执行本批指南中的接手/路径核对。

本单额外入口（路径为只读定位，不等于修改授权；前置任务产物需先实际生成）：

- `Source/Hearthward/Experience/HearthwardPresentationComponent.cpp`
- `Source/Hearthward/Experience/HearthwardPresentationComponent.h`
- `Source/Hearthward/Gameplay/HearthwardWorldPresentation.cpp`
- `Resources/Data/experience.json`
- `Resources/Audio/`
- `docs/planning/TASK-053-074/TASK-070.md`

## 4. 修改范围与协作边界

除本单MD/JSON、handoff、QA目录、专用规划目录和根`README.md`固定收尾外，候选范围如下；最终以**已获准基线JSON**为准。

- `art_source/TASK-099/`
- `docs/assets/TASK-099/`
- `Resources/Audio/TASK-099/`
- `Source/Hearthward/Experience/HearthwardPresentationComponent.cpp`
- `Source/Hearthward/Experience/HearthwardPresentationComponent.h`
- `Source/Hearthward/Gameplay/HearthwardWorldPresentation.cpp`
- `Resources/Data/experience.json`
- `Source/Hearthward/Tests/AudioFeedbackLifecycleTests.cpp`

本单不授予整个Content目录。准确包名单由现场引用/094清单确定，先补入已批准快照并核验LFS锁，才可编辑。未批准包仅可只读检查。

拟新增辅助/测试文件不表示仓库已存在；先搜索可复用类型。共享UI配置、公共头文件、WorldPresentation/CampaignWorld等按[写入调度](../planning/TASK-084-103/DISPATCH.md)串行。临时输出使用 `.agent-local/qa/TASK-099/<唯一run>/`，不得写入用户原档。

## 5. Agent具体实施步骤

### 1. 建立事件声音表

从现有Presentation入口及095动作时间轴列脚步材质、挥动、真实命中、受击、格挡、采集接触/完成、救援/升级提示、火/水/风等环境事件；每条记录触发源、位置、去重键、停止条件和素材状态。

### 2. 准备合法音源

优先复用已有来源可查素材，必要编辑裁切/降噪/循环点并保留源。未知许可或付费来源先登记阻塞，不下载未授权素材。固定voice条目继续UNPRODUCED，动态回复不合成配音。

### 3. 接触与结算分离

挥动/脚步可在实际动画接触点触发，命中/采集成功/升级成功必须来自真实成功事件。播放声音不能再执行伤害、消耗或发奖；取消动作不能补播“已完成”。

### 4. 去重不吞合法声音

用现有事务/事件ID或动作实例区分，同一成功回执重复处理只播一次；两次不同合法攻击可各播一次。不能用全局一秒节流掩盖重复回调、吞掉所有快速动作。

### 5. 环境生命周期

营地火/河水等声源按当前位置和已加载场景建销，离区、Load、结束夜袭和退出游戏清理。睡眠跳时不连播经过八小时的历史脚步/生产提示；新阶段只重建当前循环声。

### 6. 混音与设置

遵守现有主音量/分类设置/暂停例外，测试近远衰减和循环接缝。环境声不长期盖住关键命中/警戒反馈；不新增未设计的听声AI感知半径或天气难度。

### 7. 字幕与无声可玩

已确认固定台词仍有文字，未产出配音的状态如实保留；重要拒绝/完成反馈不能只有声音。可访问文本与真实事件一致，不写新剧情。

### 8. 实听与独立包

正常输入走序章、营地、采集、遭遇、救援返回，实际录带音轨片段并试听；仅波形/文件存在检查不能算声音体验通过。验证Cook能找到非UE运行音频及许可证随包需要。

## 6. 不可破坏的规则

- 不录固定人声、不用AI声替代、不给动态弟弟加TTS，除非Owner另有明确新决定。
- experience.json仅允许本单音效/环境事件映射；不得修改固定文本、voice生产状态或玩法参数来凑覆盖率。
- 音效触发读取事实，不产生事实；暂停、读档、区域卸载不能留下重复循环声源。

所有更改继续遵守只读显示、原事务执行、稳定ID、时间线隔离和不伪填验收规则。规则未明确的新玩法不硬编码；不删测试/吞错误/全仓重构来让当前检查通过。

## 7. 验收用例与执行方式

每条用例在隔离档/明确诊断夹具中建立前置，实际记录操作与断言。夹具、注入输入、真实OS键鼠、真实模型、真人样本分层统计；下面所有结果尚未运行。

|局部编号|场景|前置/具体操作|通过标准|初始结果|
|---|---|---|---|---|
|T099-C01|事件同步|真实挥击空中、命中、受击、采集成功/中断分别操作。|只有正确事件播对应声，不未命中播成功、不取消播完成。|NOT_RUN|
|T099-C02|去重|重复同一事务回执和连续两次不同合法动作。|前者一次，后者各一次，不以粗暴节流隐藏错误。|NOT_RUN|
|T099-C03|材质位置|走石/木/土等已支持表面，靠近/远离火水。|来源位置、接触类别与衰减合理，无明显循环接缝。|NOT_RUN|
|T099-C04|音量暂停|调主音量/既有分类，暂停/对话/恢复。|遵守设置及现有世界运行例外，恢复无音量突变叠声。|NOT_RUN|
|T099-C05|生命周期|区域卸载、睡眠、Load、重开新游戏。|当前音源数量有界，不重放历史事件或残留旧循环。|NOT_RUN|
|T099-C06|无配音状态|固定台词与动态回复在无录音时查看。|文字可用，voice仍未生产，不以静音文件冒充完成。|NOT_RUN|
|T099-C07|实听Cook|独立包录制带音轨正常路线并实际试听。|音效资源完整，主观试听与技术检查分别记录。|NOT_RUN|


全局挂接索引：T-002, T-003, T-011, T-025。它们来自现有TEST_MATRIX；正式数值以已批准增量/ACCEPTANCE和本单细化步骤为准，不恢复历史灰盒规则。

本单需新增/复用定向原生测试；新前缀建议 `Hearthward.Iteration.Task099.`。先注册并核验找到用例数量>0，再按公开UEClient运行，不能拿本表局部ID当UE过滤器。正常输入、渲染、真实模型或真人用例另行执行。

公共检查：`python -X utf8 scripts/validate_repo.py`、`python -X utf8 -m unittest discover -s scripts/tests -v`、`git diff --check`；在包含获批本单快照的实际基线上运行 `python -X utf8 scripts/validate_repo.py --task TASK-099 --base <实际完整SHA>`。UE操作/证据要求见执行约定；缺环境则明确NOT_RUN，不能将文件编写完成等同测试通过。

## 8. 交付物与完成定义

- 事件—声音映射、来源许可及可编辑音频源
- 音效生命周期/去重测试
- 带音轨的正常流程证据、实际试听记录与未生产录音清单

固定收尾：更新`README.md`当前说明、同名MD/JSON与`docs/handoffs/TASK-099.md`；在`docs/qa/TASK-099/REPORT.md`绑定最终完整SHA、资产/模型指纹、环境、命令、逐用例结果和明确限制。该REPORT为未来执行产物，本包不预填。

工程实现完成、Owner视觉、真人体验、二机、性能与发布各自登记。依赖下游可用产物写入handoff，不把未验项目涂为PASS。没有授权时交付本地改动和证据并写“未提交/未推送/未发布”，不得自增权限。

## 9. 本单明确不做

- 固定人声录制/TTS、重写剧情和音乐系统大改
- 声音触发伤害/奖励或改变AI听觉规则
- 用空音频/静帧冒充完整音频验收

## 10. 失败、范围变化和恢复

首先保留最短复现、受测状态和原始证据，再定位到本单或其他负责任务；不能通过清空存档/赠送物资/瞬移/改验收阈值绕过阻塞。回退只针对本单经核对的改动或资产备份，禁止未经授权reset/clean、覆盖他人工作和强制解锁。确需改变共享契约、保存格式、正式配表或包范围时提交具体差异与影响，先获准再施工。
