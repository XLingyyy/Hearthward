# TASK-081：右侧对话界面与族人采集队

> 集成更新：2026-10-06 用户明确要求“提交并推送到main吧”，授权 TASK-078—081 成果提交、推送及 main 集成；本次不更新 Release，不代填人工验收。 以下实现与验证记录保留开发时状态；源码随本次集成进入 main，历史“未提交／未推送”描述仅指验证当时。

2026-10-06。本地分支 `codex/TASK-081-dialogue-work-party`，基线 `ca21120cef4421d039c44248a0cfa5043c248feb`，叠加本会话 TASK-078／079／080 及本单未提交改动。未推送 main，未更新 Release。UE 5.8.2 Development Editor，Windows；真实模型为项目现有 Qwen3.5 4B / llama.cpp Vulkan。

## 实现

- 对话页停用旧整屏插画、Logo、标语和整屏遮罩。仅右侧约 37% 绘制炭灰半透明面板，保留现有头像、字体和铜金色系。左侧世界持续运行。
- 对话期间的剧情字幕限制在左侧场景区域，避免盖住右侧表单和确认卡。
- 首页四个完整动作入口：采集物资、搬运物资、带族人工作、查看工作。采集表单显示数量、指定点余量及入库位置；搬运先明确五种来源／目的地组合。制作、维修、护送等原能力留在“更多委托”。
- 确认卡沿用原有提案、时间线、记忆版本与明确确认检查；取消卡片不取消正在执行的任务。较长卡片和回复可滚动查看，返回首页保留已收到的回复。保留 Slate 原生中文输入框、Enter 与 Esc。
- `camp_team` 的 quantity 表示 1–4 名族人，弟弟另占一个岗位名额。资源为 wood／stone；空闲人员由 UE 选择，已占岗族人不会被抽调。开启前检查安全营地、交流范围、弟弟个人委托、既有岗位、可用来源与人数。
- 同一次确认内更新完整队伍与生产开关，复用现有每岗五人、族人后台生产、弟弟真实到岗计劳动力、共享入库和资源刷新规则。没有新增存档字段。`camp_team_stop` 暂停并保留人员和批次进度；界面可重新继续。
- 工作页显示岗位累计入库份数、弟弟到岗／赶路／受阻情况；HUD显示族人协作状态。`task_status` 在回复展示时读取真实游戏状态，避免模型生成期间产生的旧进度直接展示给玩家。

## 验证

| 范围 | 结果 | 证据 |
|---|---|---|
| 最终 Editor 编译 | PASS | [build.json](build.json) |
| 定向原生测试 `Hearthward.Dialogue081.` | 2/2 PASS | [native-results.json](native-results.json) |
| 灰盒 PIE：UI、真实入库、暂停、过期卡、模型请求 | 48/48 PASS | [pie-results.json](pie-results.json)、[脚本](verify_pie.py) |
| 最终状态反馈、弟弟实际移动到岗、继续队伍、完整回复 | 26/26 PASS | [final-pie-results.json](final-pie-results.json)、[脚本](verify_final.py) |
| 正式地图与 4:3 显示 | 10/10 PASS | [visual-results.json](visual-results.json)、[脚本](verify_visual.py) |
| 普通仓库校验、`git diff --check` | PASS | 82份任务快照，普通仓库检查0错误；无差异空白错误 |

原生测试覆盖规划不写状态、避开占岗族人、五人容量、缺来源、弟弟已有其他岗位、现有劳动力产出、暂停、原格式恢复，以及人数缺失／错配、物资定量要求和禁止采集约定。

PIE 使用独立 UUID 档池和明确标注的地面、资源测试夹具。实际营地生产进入共享仓储，暂停不继续产出。弟弟在最终检查中实际移动约 230 厘米并到岗；到岗状态来自现有真实劳动力计算。真实本地模型识别并通过 UE 确认的指令包括“带两名族人一起采集木材”“现在采集队做得怎么样了？”“暂停木材采集队”。最终另测扩展状态上下文仍可推理，显示实时入库数量及到岗状态。没有把结构化手动指令计为语言理解证据。

最终原生测试后只调整了界面文案、长回复、状态展示和 HUD；没有修改通过测试的规划、分工、经济和契约逻辑。对变化的界面与实时状态进行了后续 PIE 检查，未重复全仓测试。

首轮构建被用户运行中的 Live Coding 锁阻止，用户保存并关闭游戏后继续。首次编译发现局部变量 `Slot` 隐藏 UWidget 成员，已改名并构建通过。正式地图首轮脚本误把物品显示名“石材”断言为“石头”；核对 `Resources/Data/gameplay.json` 后修正测试，未为此改物品数据。原始本地记录保留于 `.agent-local/qa/TASK-081/`。

## 画面

以下 PNG 为引擎实际 UI 捕获，左侧无界面元素处 alpha 为零，查看器可能显示为黑色；剧情字幕仍会显示。没有将生成概念图冒充游戏画面。

- [对话首页](dialogue-home.png)
- [采集表单](gather-form.png)
- [队伍表单](team-form.png)
- [队伍确认卡](team-confirmation.png)
- [工作状态](team-status.png)
- [完整回复](full-reply.png)
- [正式地图实际画面（含编辑器窗口）](dialogue-world.png)
- [4:3 队伍表单](team-4x3.png)

## 限制

- 集体采集是营地持续生产，暂不支持“全队采满32份后自动停止”；此类定量要求需澄清，个人定量委托保持原实现。
- 普通族人的产出沿用现有后台生产规则；未新增每个族人独立搬运、交谈和集体寻路系统。弟弟本人实际导航到岗。
- 已有岗位可通过营地分工调整；不会自动迁移已有生产队到其他资源岗位。缺资源显示等待刷新。
- 仅验证列出的语言场景，不宣称任意自然语言或多步骤任务都能正确理解。模型仍使用现有上下文裁剪及响应预算。
- 没有更改关卡美术、经济配表、用户存档、存档格式或发布包。本地开发启动入口仍需 UE 环境。
- 任务路径校验 `validate_repo.py --task TASK-081 --base origin/main` 受基线流程限制：origin/main 尚无本任务的已批准快照。未绕过该检查或代填审批；本地新增任务与此前三单成果仍待提交整合。普通仓库校验另行执行。

工程参考：[Epic Slate UI 文档](https://dev.epicgames.com/documentation/unreal-engine/slate-user-interface-programming-framework-for-unreal-engine?lang=en-US)、[Anthropic：Writing tools for agents](https://www.anthropic.com/engineering/writing-tools-for-agents)。采用明确动作、明确参数及实际执行检查；实现以项目现有 Slate、AI 契约和营地规则为准。
