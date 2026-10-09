# 验证报告与证据索引

原始结果只证明对应报告列出的源码、环境与运行路径。合并到main不会自动使历史PASS成为当前提交的PASS。

2026-10-09 候选9 `0.2.0-preview.20261009.1` 已完成Shipping Build/Cook/Stage/Archive，源码 `2d55f0c9bb6d3cc305d6aa2f3bcdfa281f834a2f`，总289.83秒，Cook 0错误/1条MCP许可提示。目录 `F:/HearthwardDemo/iteration-084-103-20261009-9/Windows`，包含两草种接触阴影修复。恢复测试已实际看到标题版本、鼠标新游戏进入卧室、重开后明确鼠标继续恢复自动档，以及F6自动节点列表。窗口模式1280×720和临时限帧对照均未解决持续捕获故障：打开背包、点击手动保存后的捕获与一次重新绑定重试仍超时。手动保存结果、其余页面、双启动器实机、Vulkan及完整路线尚未验证。三个自有进程均通过UEClient关闭，最后确认无Hearthward进程残留。没有修改正式画质、性能门槛或玩法；无ZIP，未发布，整批未完成。 [本轮实测记录](TASK-103/candidate9-resume/result.json)。

基础入口：[构建与测试](BUILD_AND_TEST.md) · [测试矩阵](TEST_MATRIX.md) · [项目状态](../PROJECT_STATE.md)。

当前本地迭代：[TASK-084—103执行记录](../planning/TASK-084-103/EXECUTION_STATUS.md)。各单 `TASK-084` 至 `TASK-103` 的 REPORT 区分工程/原生/正常输入/真实模型/性能/人工层级；UE连接见 [MCP](MCP/20261007/SETUP.md)。声音工程Editor构建和联合原生28/28通过，其中[TASK-099为23/23](TASK-099/REPORT.md)，所选测试0警告/0错误；启动13条frame0 Smoke错误与1条MCP告警单列，原RED保留。099为17个声音事件/12个独立运行时WAV，`NoSound/NullRHI`不提供实际听感信用。

[087 Source.2完整原60](TASK-087/REPORT.md)CPU原始33/60、Vulkan32/60，语言均FAIL；边界各20/20、未观察到白得物品。独立辅助后的60暖组件p95 CPU22.656秒/Vulkan8.672秒为局部PASS，Paint/联合性能不继承。[102稳定单场](TASK-102/REPORT.md)CPU/Vulkan 1%Low分别51.3488/49.0052 FPS，均未达60；完整六场与最终同版Shipping性能仍未验收。

历史内部候选5 `0.2.0-preview.20261007.4` 已完成 Shipping Build/Cook/Stage/Archive，运行树 `F:/HearthwardDemo/iteration-084-103-20261007-5/Windows`；ZIP已生成：`F:/HearthwardDemo/iteration-084-103-20261007-5.zip`，4,645,167,974字节，166个运行时文件，文件名/大小清单匹配，旁有 `.zip.sha256`。17个声音事件/12个独立WAV及9份必要许可文件存在，完整资产来源/许可验收仍见094。局部OS检查通过：CPU新档卧室、F6保存1→2；Vulkan显式鼠标继续恢复同卧室/Main01和原手动/自动共2节点，F6仍2；两路径中性交流提示及正常退出通过。该轮没有模型请求、Unicode/IME、原档兼容或完整路线信用；Explorer双击未验。ZIP内BUILDINFO为ZIP创建前的实际快照，最终状态以[候选报告](TASK-103/REPORT.md)和[最终整理记录](TASK-103/CANDIDATE5_FINALIZATION.json)为准。该候选仅本地内部交付，未发布，整批未取得正式验收。

两处正式空间节点结束逻辑修复分别完成定向原生回归；[Nav12](TASK-103/NAV12_GROUNDED_FINAL_FIRST_BLOCKER.json)实际到达卧室门、楼梯顶、楼梯底、院门四节点，并通过一段1976.913883cm的grounded普通PathFollowing。第二段NavPath `valid=true/partial=true`，严格停止在移动前；正式目标稳定，院门route再次显示已记录。完整撤离、自然营地checkpoint、首次救援及OS连续路线未通过。候选位于角塔/底座XY覆盖，但Nav层归属及partial原因仍UNKNOWN，未自动改游戏几何或玩法。

[UE MCP最终配置与检查](MCP/20261007/SETUP.md)已完成：正常Editor检查时HTTP在线、三个元工具握手、Bootstrap只读查询及RC/Python/CLI启用状态通过。服务依赖该Editor进程。当前Codex聊天的原生工具目录未热挂载；需要在Hearthward项目中新建聊天加载配置，实际新聊天挂载仍未验。

候选4/候选2的启动、模型单例或原档兼容结果保留各自历史绑定，不迁移到候选5。087 Source.2完整语言质量矩阵及102帧门槛仍FAIL；7项设计确认与后续技术项见[本批执行记录](../planning/TASK-084-103/EXECUTION_STATUS.md)，真人和二机验收未完成。

## 任务与专题

| 目录 | 报告入口 |
|---|---|
| [evidence](evidence/) | 保留原目录结构；从任务交接查找对应报告与受测版本 |
| [fix1](fix1/) | [REPORT.md](fix1/REPORT.md) |
| [fix2](fix2/) | [REPORT.md](fix2/REPORT.md) |
| [project-progress-20261002](project-progress-20261002/) | [REPORT.md](project-progress-20261002/REPORT.md) |
| [TASK-044](TASK-044/) | [REPORT.md](TASK-044/REPORT.md) |
| [TASK-045](TASK-045/) | [REPORT.md](TASK-045/REPORT.md) |
| [TASK-046](TASK-046/) | [REPORT.md](TASK-046/REPORT.md) |
| [TASK-047](TASK-047/) | [REPORT.md](TASK-047/REPORT.md) |
| [TASK-048](TASK-048/) | [REPORT.md](TASK-048/REPORT.md) |
| [TASK-049](TASK-049/) | [REPORT.md](TASK-049/REPORT.md) |
| [TASK-050](TASK-050/) | [REPORT.md](TASK-050/REPORT.md) |
| [TASK-051](TASK-051/) | [REPORT.md](TASK-051/REPORT.md) · [使用说明.md](TASK-051/使用说明.md) |
| [TASK-052](TASK-052/) | [REPORT.md](TASK-052/REPORT.md) |
| [TASK-053](TASK-053/) | 保留原目录结构；从任务交接查找对应报告与受测版本 |
| [TASK-053-074](TASK-053-074/) | [REPORT.md](TASK-053-074/REPORT.md) |
| [TASK-054](TASK-054/) | 保留原目录结构；从任务交接查找对应报告与受测版本 |
| [TASK-055](TASK-055/) | 保留原目录结构；从任务交接查找对应报告与受测版本 |
| [TASK-058](TASK-058/) | 保留原目录结构；从任务交接查找对应报告与受测版本 |
| [TASK-059](TASK-059/) | 保留原目录结构；从任务交接查找对应报告与受测版本 |
| [TASK-061](TASK-061/) | 保留原目录结构；从任务交接查找对应报告与受测版本 |
| [TASK-062](TASK-062/) | 保留原目录结构；从任务交接查找对应报告与受测版本 |
| [TASK-063](TASK-063/) | 保留原目录结构；从任务交接查找对应报告与受测版本 |
| [TASK-064](TASK-064/) | 保留原目录结构；从任务交接查找对应报告与受测版本 |
| [TASK-066](TASK-066/) | 保留原目录结构；从任务交接查找对应报告与受测版本 |
| [TASK-067](TASK-067/) | 保留原目录结构；从任务交接查找对应报告与受测版本 |
| [TASK-068](TASK-068/) | 保留原目录结构；从任务交接查找对应报告与受测版本 |
| [TASK-069](TASK-069/) | 保留原目录结构；从任务交接查找对应报告与受测版本 |
| [TASK-070](TASK-070/) | 保留原目录结构；从任务交接查找对应报告与受测版本 |
| [TASK-071](TASK-071/) | 保留原目录结构；从任务交接查找对应报告与受测版本 |
| [TASK-072](TASK-072/) | 保留原目录结构；从任务交接查找对应报告与受测版本 |
| [TASK-074](TASK-074/) | 保留原目录结构；从任务交接查找对应报告与受测版本 |
| [TASK-076](TASK-076/) | [新版UI与玩法整合报告](TASK-076/REPORT.md) |
| [ui-fix](ui-fix/) | [REPORT.md](ui-fix/REPORT.md) |


## 最新整合与发行

- [TASK-077 报告](TASK-077/REPORT.md)
- [TASK-078 报告](TASK-078/REPORT.md)
- [TASK-079 报告](TASK-079/REPORT.md)
- [TASK-080 报告](TASK-080/REPORT.md)
- [TASK-081 报告](TASK-081/REPORT.md)
- [TASK-082 报告](TASK-082/INTEGRATION-REPORT.md)
- [TASK-083 已公开历史 Demo 发行与验证](../releases/demo-20261006-2/REPORT.md)

## 保存规则

- 新临时运行默认写到 `.agent-local/qa/<任务>/<运行>/`。交付时把必要报告、复现脚本及其引用的关键证据提升到本目录。
- `evidence/`保留早期任务与AI历史记录；已有目录和结果文件不批量移动，避免破坏引用及提交绑定。
- 报告注明实际受测提交、命令、结果和未覆盖项；保留证明已知失败的必要证据，不以整理为由删去失败结果。
- 打包客户端、完整部署镜像、重复试跑输出不作为新的QA源码归档；正式包放约定的制品存储，发布说明放 `docs/releases/`。
- 当前main的TASK-053表示通行玩法；原UI分支的同号历史记录见[分支整合清单](../planning/BRANCH_INTEGRATION.md)，禁止覆盖到该目录。
