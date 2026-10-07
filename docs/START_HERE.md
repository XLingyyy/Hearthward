# 最短阅读路径

2026-10-07 最新Owner决定：[七项推荐方向已批准](design/DSGN-004-iteration-art-local-ai.md)，按1A—6A、7B继续制作与本地模型对照。以下候选5与测试记录保留其原版本范围；新首件尚未视觉签收，新模型尚未选定。


2026-10-07 当前工作区入口：[TASK-084—103执行记录](planning/TASK-084-103/EXECUTION_STATUS.md) → 对应任务REPORT/handoff → [独立候选报告](qa/TASK-103/REPORT.md)。分支 `codex/TASK-084-103-iteration`，基线 `6fcf5c22e965f0f7409438f19bc7b09e96ffb058`，存在未提交变更。声音工程联合原生28/28、[099子集23/23](qa/TASK-099/REPORT.md)与后续空间节点回归分开登记；[087 Source.2完整质量](qa/TASK-087/REPORT.md)和[102帧门槛](qa/TASK-102/REPORT.md)仍FAIL。

当前内部候选5 `0.2.0-preview.20261007.4` 已完成 Shipping Build/Cook/Stage/Archive，运行树 `F:/HearthwardDemo/iteration-084-103-20261007-5/Windows`；ZIP已生成：`F:/HearthwardDemo/iteration-084-103-20261007-5.zip`，4,645,167,974字节，166个运行时文件，文件名/大小清单匹配，旁有 `.zip.sha256`。17个声音事件/12个独立WAV及9份必要许可文件存在，完整资产来源/许可验收仍见094。局部OS检查通过：CPU新档卧室、F6保存1→2；Vulkan显式鼠标继续恢复同卧室/Main01和原手动/自动共2节点，F6仍2；两路径中性交流提示及正常退出通过。该轮没有模型请求、Unicode/IME、原档兼容或完整路线信用；Explorer双击未验。ZIP内BUILDINFO为ZIP创建前的实际快照，最终状态以[候选报告](qa/TASK-103/REPORT.md)和[最终整理记录](qa/TASK-103/CANDIDATE5_FINALIZATION.json)为准。该候选仅本地内部交付，未发布，整批未取得正式验收。

两处正式空间节点结束逻辑修复分别完成定向原生回归；[Nav12](qa/TASK-103/NAV12_GROUNDED_FINAL_FIRST_BLOCKER.json)实际到达卧室门、楼梯顶、楼梯底、院门四节点，并通过一段1976.913883cm的grounded普通PathFollowing。第二段NavPath `valid=true/partial=true`，严格停止在移动前；正式目标稳定，院门route再次显示已记录。完整撤离、自然营地checkpoint、首次救援及OS连续路线未通过。候选位于角塔/底座XY覆盖，但Nav层归属及partial原因仍UNKNOWN，未自动改游戏几何或玩法。

[UE MCP最终配置与检查](qa/MCP/20261007/SETUP.md)已完成：正常Editor检查时HTTP在线、三个元工具握手、Bootstrap只读查询及RC/Python/CLI启用状态通过。服务依赖该Editor进程。当前Codex聊天的原生工具目录未热挂载；需要在Hearthward项目中新建聊天加载配置，实际新聊天挂载仍未验。

候选4/候选2的启动、模型单例或原档兼容结果保留各自历史绑定，不迁移到候选5。087 Source.2完整语言质量矩阵及102帧门槛仍FAIL；7项已批准方向与后续技术项见[本批执行记录](planning/TASK-084-103/EXECUTION_STATUS.md)，真人和二机验收未完成。

2026-10-06 已公开主干历史入口：受测集成基线为 `main@af08e1ab`（PR #61），含 TASK-078—081 弟弟／对话／任务指引及 TASK-082 地图／传送／仓储界面。该版 Windows Demo 见[发行报告](releases/demo-20261006-2/REPORT.md)，当前源码和已知限制见[项目状态](PROJECT_STATE.md)。

先按[目录说明](REPOSITORY_LAYOUT.md)定位文件；开发证据见[QA索引](qa/README.md)，素材见[制作源索引](../art_source/README.md)。

## 先看游戏与制作源

- [README](../README.md)是玩法、构建和运行入口。正常新游戏从夜袭序章开始；自然世界和各模块原始验证见对应任务报告。
- [动物演示操作](qa/TASK-051/使用说明.md)说明14种动物的近景、弟弟接近、致命伤和重置；专用地图使用独立临时档池。
- [建模与动作制作说明](../art_source/TASK-051/制作说明.md)提供Blender／FBX、贴图、连续预览、指导、工具代码与克隆恢复步骤。先运行`git lfs pull`取回二进制。
- [更新兼容报告](qa/update-compatibility-20260930.md)说明旧档保留、新版进度隔离、冲突清理及正式版更新提示。

## 开始一项开发任务

1. 阅读[AGENTS](../AGENTS.md)、[WORKFLOW](../WORKFLOW.md)，确认实际工作目录。Hearthward是游戏仓库，GameFactory是独立工具仓库。
2. 阅读[PROJECT_STATE](PROJECT_STATE.md)、当前`docs/tasks/TASK-xxx.json`和任务分支的交接；运行`python scripts/agent_context.py --task TASK-xxx`。
3. 核对目录、分支、完整HEAD、未提交文件、Owner、授权范围、相关契约和实际LFS锁。[TASK-053—074](planning/TASK-053-074/REVIEW.md)施工成果已合入main，验收仍以各单记录为准；新任务使用独立分支，不把原`.agent-local/task051`视作默认最新主干。
4. 读取最小相关实现、[设计基线](design/CURRENT.md)和[未定规则](design/OPEN_QUESTIONS.md)。实施后执行匹配验证，更新README和本任务交接，记录真实提交与推送状态。

TASK-041及早期AI标签与canonical任务的对应关系见[映射账本](planning/TASK-041-baseline-ledger.md)。原始测试只证明各报告中列出的版本与路径；没有执行的当前UE／发行检查不得写为PASS。
