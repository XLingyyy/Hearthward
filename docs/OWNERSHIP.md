# 领域、路径与共享资源负责人

> 全部路径为待批准工程划分；没有真实负责人前，不能据此声称写权限已授予。
> 行政角色在toolchain锁文件绑定账号，此表用于领域责任。

| 领域 | 候选独占实现目录 | 契约／主要消费者 | Owner / Backup |
|---|---|---|---|
| 基础角色与交互 | Source/TribeGame/Interaction/；Content/Tribe/Interaction/ | 角色、UI、伙伴 | 未分配 / 未分配 |
| 库存与制作 | Source/TribeGame/Inventory/；Content/Tribe/Inventory/ | CT-001、营地、UI、伙伴 | 未分配 / 未分配 |
| 营地与生产 | Source/TribeGame/Camp/；Content/Tribe/Camp/ | CT-001、时间、资源 | 未分配 / 未分配 |
| 战斗与潜入 | Source/TribeGame/Combat/；Content/Tribe/Combat/ | 状态、警戒、任务 | 未分配 / 未分配 |
| 弟弟命令与知识 | Source/TribeGame/Companion/；Content/Tribe/Companion/ | CT-001/002/003、UI | 未分配 / 未分配 |
| 存档与时间 | Source/TribeGame/Save/；Content/Tribe/Save/ | CT-003、所有可保存模块 | 未分配 / 未分配 |
| UI | Source/TribeGame/UI/；Content/Tribe/UI/ | 只消费状态与命令契约 | 未分配 / 未分配 |
| 关卡集成 | Content/Tribe/World/ | 主流程、全部模块 | 未分配 / 未分配 |
| 基础契约与工程 | Source/TribeGame/Contracts/；Config/；根工程与CI | 跨域共享 | 未分配 / 未分配 |

## 必须预约的共享资源

`.uproject`、Build.cs/Target.cs、核心GameMode/GameInstance/Controller/角色基类、
共享输入映射、全局标签、根UI、主地图、公共声明、保存格式、工具链与GitHub工作流。
领域Owner不因为“自己的功能需要”就自动拥有这些资源的改动权。

## CODEOWNERS

[示例](../.github/CODEOWNERS.example)必须替换为真实有权限的账号再安装。
文件存在不代表要求Owner批准的分支保护已经启用；CODEOWNERS也不是排他写锁。
