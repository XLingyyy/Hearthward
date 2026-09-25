# TASK-041｜统一当前基线、任务编号与验收账本

2026-09-25 用户指定本单为实际 TASK-041；输入规划稿的 TASK-043 对应本单。该规划稿及 `Hearthward_Audit_2026-09-24.md` 在原工作区为本地未跟踪输入。本单从 `main@ee3f4c2eede9f856c4589fd4b85b1b05c82a1b6b` 建立独立文档分支。

## 目标与范围

1. 用主干提交、已合入 PR 和发行记录统一 [README](../../README.md)、[PROJECT_STATE](../PROJECT_STATE.md)、[START_HERE](../START_HERE.md) 的当前口径。
2. 在[基线与验收账本](../planning/TASK-041-baseline-ledger.md)中登记 canonical 编号、历史 AI 标签、规划稿偏移及旧任务尾项。
3. 将实现、主干集成、受测版本、流程字段和 Owner 体验分别记录；保留未完成项。

允许路径以 [TASK-041.json](TASK-041.json) 为准。2026-09-25 后续指令将 GitHub Issue 改为可选，并要求处理 TASK-027/028 的仓库自检错误；本轮同步更新工作流、任务模板、校验器与定向测试。改动不涉及玩法、设计决定、资产、原始证据或旧任务元数据。

## 验收

- [x] 三个当前入口标明同一审计主干 `ee3f4c2eede9f856c4589fd4b85b1b05c82a1b6b`，Shipping 受测源码单列为 `8b54550b5f9d7d01c9e9e0f7444826090667f3f5`。
- [x] 规划稿 043→canonical 041 与历史 AI 041/042 分别解释；旧证据路径保留。
- [x] TASK-004、026—031 的五类状态与待办逐项列出；没有把开发分支 PASS 写成当前主干重跑。
- [x] 修正未合并、未打包、M/J 自然地图入口、房屋陈设等已过时表述。
- [x] Issue 改为可选；`Ready`／`Active` 不强制先指定 Reviewer，进入 `Review` 后仍校验独立 Reviewer。TASK-027/028 的空字段保留原值，仓库自检不再因这四项失败。
- [ ] 独立 Reviewer／正式审查登记。用户指定 Owner 和 Reviewer 均为 `XLingyyy`；同人指定已记录，尚不满足独立评审要求，任务状态保持 `Blocked`。

本单交付文档核对和仓库流程校验规则，不运行 UE 或重新发布；测试结果和限制见[交接](../handoffs/TASK-041.md)。2026-09-25 用户授权本单提交和推送任务分支，未授权合并。
