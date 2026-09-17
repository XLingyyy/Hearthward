# Hearthward（归火）· 开发仓库入口

当前交付是**工作流与工程启动骨架**，不是已完成的UE工程。游戏设计基线为GDD v0.3。
已阅读 GDD v0.3，并拆出首个工程任务：[TASK-003 第三人称灰盒行走基线](docs/tasks/TASK-003.md)。
当前交付包含工作流和任务单；尚未创建游戏工程、未实现玩法。

仓库：[XLingyyy/Hearthward](https://github.com/XLingyyy/Hearthward)。本地已独立初始化 Git、配置 origin 和 Git LFS。
本次经用户授权建立初始 `main` 基线；后续开发使用独立任务分支与 PR。
本机准备结果与剩余事项见 [TASK-000 交接](docs/handoffs/TASK-000.md)。

从 [START_HERE](docs/START_HERE.md) 开始。完整协作规则见 [WORKFLOW](WORKFLOW.md)。
开发Agent先读 [AGENTS.md](AGENTS.md)，成员上手见 [CONTRIBUTING](CONTRIBUTING.md)。

| 记录 | 入口 |
|---|---|
| 当前已集成状态 | [PROJECT_STATE](docs/PROJECT_STATE.md) |
| 工具链与验证 | [ENVIRONMENT](docs/ENVIRONMENT.md) |
| 设计与未定规则 | [CURRENT](docs/design/CURRENT.md) / [OPEN_QUESTIONS](docs/design/OPEN_QUESTIONS.md) |
| 人与模块职责 | [OWNERSHIP](docs/OWNERSHIP.md) |
| 初始任务 | [BACKLOG](docs/planning/BACKLOG.md) |
| 验收与构建 | [TEST_MATRIX](docs/qa/TEST_MATRIX.md) / [BUILD_AND_TEST](docs/qa/BUILD_AND_TEST.md) |

本仓库骨架未给游戏素材、第三方模型或商业插件授予新许可。导入已有仓库前逐文件比较，
不要直接覆盖既有权限、忽略规则、Agent指令或CI。工具链已记录本机实测版本；真人Owner与团队采用仍待确认。
