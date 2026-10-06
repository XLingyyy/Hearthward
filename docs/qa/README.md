# 验证报告与证据索引

原始结果只证明对应报告列出的源码、环境与运行路径。合并到main不会自动使历史PASS成为当前提交的PASS。

基础入口：[构建与测试](BUILD_AND_TEST.md) · [测试矩阵](TEST_MATRIX.md) · [项目状态](../PROJECT_STATE.md)。

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
- [TASK-083 当前 Demo 发行与验证](../releases/demo-20261006-2/REPORT.md)

## 保存规则

- 新临时运行默认写到 `.agent-local/qa/<任务>/<运行>/`。交付时把必要报告、复现脚本及其引用的关键证据提升到本目录。
- `evidence/`保留早期任务与AI历史记录；已有目录和结果文件不批量移动，避免破坏引用及提交绑定。
- 报告注明实际受测提交、命令、结果和未覆盖项；保留证明已知失败的必要证据，不以整理为由删去失败结果。
- 打包客户端、完整部署镜像、重复试跑输出不作为新的QA源码归档；正式包放约定的制品存储，发布说明放 `docs/releases/`。
- 当前main的TASK-053表示通行玩法；原UI分支的同号历史记录见[分支整合清单](../planning/BRANCH_INTEGRATION.md)，禁止覆盖到该目录。
