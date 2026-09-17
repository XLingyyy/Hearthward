# Hearthward 项目状态

更新：2026-09-17。当前为 TASK-000 初始仓库基线交付；已阅读设计并拆出 TASK-003，尚无玩法实现。

| 项目 | 实际状态 |
|---|---|
| 开发根目录 | 独立 Hearthward 仓库，与上层 GameFactory 工具仓库隔离 |
| 工作流 | v1.0 启动包已导入；根 WORKFLOW.md 为维护入口；团队采用仍为 DRAFT |
| GitHub | origin 已连接；公开仓库，本次按授权建立初始 main 基线 |
| 当前分支 | 启动内容在 codex/TASK-000-workflow-bootstrap 准备，首次提交后建立 main 并推送 |
| Git LFS | 本仓库启用；锁列表查询成功且为空；推送锁验证已启用 |
| 本机工具 | UE 5.8.1、MSVC 19.44.35228.0、SDK 10.0.22621.0 已核验安装 |
| GitHub CLI | gh 登录返回 HTTP 401；GitHub 连接器可读取仓库并确认管理权限 |
| 设计 | 已读取 v0.3 正文、表格和附录；R01—R25仍为 OPEN，未新增设计决定 |
| 正式游戏工程 | 尚未创建 .uproject、Source、Content |
| UE 编译／打包／试玩 | NOT_RUN |
| 团队保护／双账号 LFS／两机验证 | NOT_RUN |
| 模型与资产生成服务 | NOT_RUN |

本机已具备继续工程初始化的工具前提，完整 M0 尚未验收。
首个工程任务为 [TASK-003](tasks/TASK-003.md)：建立 UE 工程和第三人称灰盒行走场景。
已获首次提交／推送授权；后续仍需确认真人 Owner 与独立评审人，
建立 main 后运行 repo-policy，再配置经确认的远端保护。
游戏工程初始化、实际构建、资产锁和两机验证分阶段执行，保留真实证据。

本次记录见 [TASK-000 交接](handoffs/TASK-000.md)。启动包原始自检记录保留在
[STARTER_VALIDATION](qa/STARTER_VALIDATION.md)，不作为本游戏验证证据。
