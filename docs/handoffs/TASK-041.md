# TASK-041 交接｜基线、编号、验收账本与流程规则

日期：2026-09-25。原工作区：`G:/GameFactory/Hearthward/.agent-local/TASK-041`；分支：`codex/TASK-041-baseline`；起点：`origin/main@ee3f4c2eede9f856c4589fd4b85b1b05c82a1b6b`。原 `G:/GameFactory/Hearthward` 的 TASK-027 分支及未提交改动保留原样。

## 完成的工作

- [任务单](../tasks/TASK-041.md)和 [JSON](../tasks/TASK-041.json)登记本单范围、编号偏移、权限和流程阻塞。
- [基线账本](../planning/TASK-041-baseline-ledger.md)对齐主干、Demo 发行源码、历史 AI 标签和 TASK-004、026—031 的五栏状态。
- [README](../../README.md)、[PROJECT_STATE](../PROJECT_STATE.md)、[START_HERE](../START_HERE.md)改为当前主干口径。已修正 PR #40—43、Windows Shipping、自然图 M/J 页面、木椅提灯陈设和旧 AI 027/028 重号说明。
- 根 [WORKFLOW](../../WORKFLOW.md) 增加成果推送后的独立工作树清理规则。用户后续确认 GitHub Issue 不必作为任务条件；任务快照与交接记录归属和状态，Issue 可选，Reviewer 在正式 `Review` 阶段才强制要求独立人选。
- [仓库校验器](../../scripts/validate_repo.py)与[定向测试](../../scripts/tests/test_repo_tools.py)按新规则处理可选 Issue 和审查阶段；同时更新 AGENTS、CONTRIBUTING、PR／任务／交接模板和接手提示。TASK-027/028 的历史任务字段未补造。

本单只改文档和仓库检查／接手脚本，没有改 C++、游戏配置、设计决定或二进制资产。输入 `docs/Hearthward_TASK-043-076.md` 与 `docs/Hearthward_Audit_2026-09-24.md` 原先是主工作区的未跟踪文件，本工作树没有移动或覆盖它们。规划稿 043→实际 041 是用户本次明确指定；没有把规划稿其他条目自动登记为任务。

## 验证边界

| 检查 | 结果 | 范围 |
|---|---|---|
| 主干与发行源码 | PASS | `git ls-remote origin refs/heads/main` 得到 `ee3f4c2eede9f856c4589fd4b85b1b05c82a1b6b`；first-parent 日志核对 PR #37、#38、#40—43；发行报告与发布页均指向 `8b54550b5f9d7d01c9e9e0f7444826090667f3f5` |
| 初次交付 `940de52` | 仓库自检 FAIL（4 项旧规则报错） | 当时 `python -X utf8 scripts/validate_repo.py` 报 TASK-027/028 各缺 Reviewer 与 Issue URL；基线 `ee3f4c2` 尚无 TASK-041 快照，故 `--task TASK-041 --base ee3f4c2` 也不能完成范围校验 |
| 本次仓库自检 | PASS，0 错误 | `python -X utf8 scripts/validate_repo.py` 检查 42 份任务快照；TASK-027/028 保持 Reviewer／Issue 为空，按本次规则不再报错 |
| 本次任务范围 | PASS，0 错误 | 用户后续授权的扩展范围先记录于 `01c97ccc2572024d2a55a6ed32e4c9742fbaa1dc`；`python -X utf8 scripts/validate_repo.py --task TASK-041 --base 01c97cc` 检查本轮变更路径，均在批准范围内 |
| 定向校验测试与差异 | PASS | `python -X utf8 -m unittest scripts.tests.test_repo_tools.DocumentTests -v` 通过 17 项；`git diff --check` 通过 |
| UE Editor、PIE、Shipping、模型、完整试玩 | NOT_RUN | 本单没有运行时代码或资产改动；旧 PASS 只保留在各自受测版本 |

初次交付的四项失败来自“Active 必须先有 Reviewer 和 Issue”的旧校验策略。用户后续取消 Issue 必填；此次只调整策略与配套测试，没有修改 TASK-027/028 JSON 来掩盖错误。UE 构建与游戏测试未运行。

此前本机 `gh issue list` / `gh pr list` 返回 HTTP 401；Git 远端主干与本分支可以核对。2026-09-25 用户指定 TASK-041 的 Owner 与 Reviewer 都是本人 `XLingyyy`，并授权提交、推送本任务分支。Issue 未登记且无需补建；同一人不能作为自己的独立 PR 审查人，故 JSON 保持 `Blocked`，不伪造流程通过。本单未获合并授权，无资产锁操作。

## 后续

1. 本单提交与推送已获授权；交付提交以远端 `codex/TASK-041-baseline` 的 HEAD 为准。核对远端分支与本地干净状态后按 WORKFLOW 第6.2节清理本任务工作树，实际结果在本轮交付回执记录。
2. 后续若要进入主干，另走 PR 独立审查与合并；Issue 可按需要建立。合并后以新 main SHA 单独记录集成状态。
3. TASK-042 及以后按用户指定的规划稿 `原编号 − 2` 建立各自任务，不把历史集成标签 041/042 误作新任务成果。
