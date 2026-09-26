# TASK-042 交接｜首版边界与第一体验切片提案

## 最新补充：2026-09-26 Owner批准042

Owner明确回复“批准042”。DSGN-003首版范围及切片方向已生效，CURRENT与R17／R25子范围同步；详细数值和实测未批准。远端main已通过PR #45集成此前草案（35ac8706e50fe8b7cf099b3b73e803572d5a3ff0）。TASK-043的D1—D6仍待决定。本补充及关联文档随TASK-043分支提交推送，未合并main。原记录中的待审批／父分支未合并描述仅反映当时状态。

## 原交付记录

日期：2026-09-26。工作树：`G:/GameFactory/Hearthward/.agent-local/TASK-042`；分支：`codex/TASK-042-first-slice`；基线：`origin/main@a4998b10da54f162def767fa8bb7e308594bd490`（PR #44 已合入）。原主工作区 `G:/GameFactory/Hearthward` 在 TASK-027 分支上的未提交及未跟踪文件未修改。本地原规划稿 044 按用户指定偏移映射为本单 canonical 042；原规划稿及审计保持在主工作区，未纳入任务分支。

## 本轮产物

- [DSGN-003 待审批提案](../design/DSGN-003-first-release-slice.md)列出首版必交付范围、首版外事项、建议救援切片的流程、最小内容和退出条件；明确 GDD 与 R17／R25 的有效边界。
- [任务单](../tasks/TASK-042.md)和 [JSON](../tasks/TASK-042.json)登记来源、依赖、允许路径、验收与权限；[CURRENT](../design/CURRENT.md)仅增加待审查入口。
- [README](../../README.md)和 [PROJECT_STATE](../PROJECT_STATE.md)同步 PR #44 后的 `main@a4998b1`，将切片标为本分支提案，不写成已实现或已获批准。

本单没有改游戏代码、GDD 原件、R 项状态、地图、二进制资产、配置、试玩数据或保存格式。先例研究为 Double Fine 的《Psychonauts》制作复盘，其切片方法不构成本游戏玩法来源。

## 验证与权限

| 检查 | 结果 | 边界 |
|---|---|---|
| 主干与任务编号 | PASS | 远端 main 为 `a4998b10da54f162def767fa8bb7e308594bd490`，含 PR #44；开工时远端无 TASK-042 分支；规划稿 044→canonical 042 |
| 仓库文档与范围 | PASS | `python -X utf8 scripts/validate_repo.py` 检查 43 份任务快照，0 项错误；`git diff --check` 通过；7 条改动路径均在 TASK-042 `allowed_paths`。主干无 TASK-042 快照，`--task --base a4998b1` 不能证明预先批准的范围 |
| UE 构建、PIE、Shipping、物理试玩 | NOT_RUN | 本单只交设计提案；30—60 分钟是内容预算，没有实测 |

当前 `DSGN-003` 状态为 `PROPOSED`。需要 Owner 审阅首版范围及“营地→近郊救援→回营成长”方向；具体任务卡、奖励、升阶费用、敌人和量化试玩阈值仍归后续任务。R17／R25 保持 OPEN。用户于 2026-09-26 明确要求本单提交并推送，授权已登记到任务 JSON；该授权用于草案交付，设计审批仍待 Owner 决定，合并未授权。没有资产锁操作。

## 后续

1. Owner 批准或要求修改 DSGN-003 的范围与切片方向；若批准，只将获准子范围登记为有效设计，余下 R 项保持 OPEN。
2. 按本任务授权提交草案并推送至 codex/TASK-042-first-slice；核对远端和工作树状态后，按 WORKFLOW 清理本任务的闲置工作树。提交号、推送与清理结果由本轮交付回执记录。
3. 后续设计任务补 R01/R03、R11—R13、R17/R25 等具体数值与脚本；集成试玩单独记录物理输入、受测 SHA、耗时和 Owner 体验结论。
