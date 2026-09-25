# TASK-041 交接｜基线、编号与验收账本

日期：2026-09-25。原工作区：`G:/GameFactory/Hearthward/.agent-local/TASK-041`；分支：`codex/TASK-041-baseline`；起点：`origin/main@ee3f4c2eede9f856c4589fd4b85b1b05c82a1b6b`。原 `G:/GameFactory/Hearthward` 的 TASK-027 分支及未提交改动保留原样。

## 完成的文档

- [任务单](../tasks/TASK-041.md)和 [JSON](../tasks/TASK-041.json)登记本单范围、编号偏移、权限和流程阻塞。
- [基线账本](../planning/TASK-041-baseline-ledger.md)对齐主干、Demo 发行源码、历史 AI 标签和 TASK-004、026—031 的五栏状态。
- [README](../../README.md)、[PROJECT_STATE](../PROJECT_STATE.md)、[START_HERE](../START_HERE.md)改为当前主干口径。已修正 PR #40—43、Windows Shipping、自然图 M/J 页面、木椅提灯陈设和旧 AI 027/028 重号说明。
- 根 [WORKFLOW](../../WORKFLOW.md) 增加成果推送后的独立工作树清理规则；仅清理本单自己创建且已确认无本地未交接改动的工作树。

本单只改文档，没有改 C++、配置、设计决定或二进制资产。输入 `docs/Hearthward_TASK-043-076.md` 与 `docs/Hearthward_Audit_2026-09-24.md` 原先是主工作区的未跟踪文件，本工作树没有移动或覆盖它们。规划稿 043→实际 041 是用户本次明确指定；没有把规划稿其他条目自动登记为任务。

## 验证边界

| 检查 | 结果 | 范围 |
|---|---|---|
| 主干与发行源码 | PASS | `git ls-remote origin refs/heads/main` 得到 `ee3f4c2eede9f856c4589fd4b85b1b05c82a1b6b`；first-parent 日志核对 PR #37、#38、#40—43；发行报告与发布页均指向 `8b54550b5f9d7d01c9e9e0f7444826090667f3f5` |
| 文档和范围检查 | 局部 PASS，仓库自检 FAIL（4 项既有流程错误） | `git diff --check` 通过；工作树改动限于 TASK-041 JSON `allowed_paths`；`python -X utf8 scripts/validate_repo.py` 检查 42 份任务快照与相对链接，未报本单新增错误，仍报 TASK-027/028 各缺 Reviewer 与真实 Issue URL |
| 基线任务快照检查 | FAIL（任务单在本分支新建） | `python -X utf8 scripts/validate_repo.py --task TASK-041 --base ee3f4c2eede9f856c4589fd4b85b1b05c82a1b6b` 除上述四项外，报告基线提交没有可用的已批准 TASK-041 快照；本单是从该基线新建，故不能用此命令证明先前已批准的基线路径范围 |
| UE Editor、PIE、Shipping、模型、完整试玩 | NOT_RUN | 本单没有运行时代码或资产改动；旧 PASS 只保留在各自受测版本 |

第一次仓库自检在稀疏检出且交接文件尚未写入时出现缺文件和本单 `blocked_by` 字段格式错误；已补齐必要只读目录、交接并修正字段。常规自检最终仅保留上述四项原有流程错误；基线任务快照检查另有上表所列限制。未修改旧任务 JSON 来消除它们。UE 构建与游戏测试对本次文档改动未运行。

本机 `gh issue list` / `gh pr list` 返回 HTTP 401；公开仓库页面、Git 主干和发行页仍可只读核对。2026-09-25 用户指定 TASK-041 的 Owner 与 Reviewer 都是本人 `XLingyyy`，并授权提交、推送本任务分支。真实 Issue 未登记；同一人不能作为自己的独立审查人，故 JSON 保持 `Blocked`，不伪造流程通过。本单未获合并授权，无资产锁操作。

## 后续

1. 本单提交与推送已获授权；交付提交以远端 `codex/TASK-041-baseline` 的 HEAD 为准。核对远端分支与本地干净状态后按 WORKFLOW 第6.2节清理本任务工作树，实际结果在本轮交付回执记录。
2. 后续登记真实 Issue 和可计入流程的独立审查；若要进入主干，另走 PR 审查与合并。合并后以新 main SHA 单独记录集成状态。
3. TASK-042 及以后按用户指定的规划稿 `原编号 − 2` 建立各自任务，不把历史集成标签 041/042 误作新任务成果。
