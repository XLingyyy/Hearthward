# 来源与核对范围

参考仓库：XLingyyy/Hearthward。规划SHA：`6fcf5c22e965f0f7409438f19bc7b09e96ffb058`。读取日期：2026-10-06。

本次通过GitHub连接读取当前main、任务模板/已有JSON、目录/分支记录、校验脚本、测试矩阵、验收规则及相关模块目录；沿上一轮已读取的设计/发行/源码内容细化任务。没有执行UE、没有真实试玩，没有复制游戏源码/模型/字体到交付包，也没有提交/推送远端。

## 格式与校验依据

|文件|核对内容|读取到的Git blob SHA|
|---|---|---|
|docs/templates/task.json|schema_version 1、Backlog/权限/空Reviewer等字段|beb357679a7812cdd45d326f83f8953333ebe1bc|
|docs/tasks/TASK-083.json|最新任务编号与现有权限/范围模式|6f03cac622c7f1773606b13c9eb2a980c953ebd4|
|scripts/validate_repo.py|任务字段、范围、依赖/契约、全局T/R引用与base快照规则|fc1bc48fe2beb53dd1d2417cf7017a4ab2e97b21|
|docs/qa/TEST_MATRIX.md|有效全局测试索引T-001—026；历史索引不替代新批准规则|13e4662b8045d681dc311436d9379b3b9b619052|
|docs/planning/TASK-053-074/ACCEPTANCE.md|真实模型/联合性能/真人/二机既有门槛|a6cac17cb0a8b25e7149cbe5c90d9949376aa3a3|

编号查询中当前main的TASK-084.json不存在。读取到的分支命名未显示本批建议分支；没有逐个递归审计全部历史分支内容或实际远端LFS锁，因此不承诺本地下载任务包就已经“占用”远端编号。实际导入/派发需再次检查同号冲突。

## 内容来源

`AGENTS.md`、`WORKFLOW.md`、`docs/START_HERE.md`、`docs/PROJECT_STATE.md`、`docs/design/CURRENT.md`、`docs/design/DSGN-003-first-release-slice.md`、`docs/releases/demo-20261006-2/REPORT.md`、`docs/qa/TASK-077/REPORT.md`、`docs/qa/TASK-081/REPORT.md`、`docs/planning/TASK-053-074/TASK-070.md`、`docs/REPOSITORY_LAYOUT.md`、`art_source/README.md`及各任务source_refs列出的读取入口。

源码抽查/目录核对包括UI制作/地图/任务引导/对话与仓储、CompanionFixture读取接口、AI/Campaign/Camp/Gameplay/Experience模块、Resources/UI配置目录。不是全仓代码审计；本包对拟新增文件和执行时需定位的包有明确说明，不宣称已经存在。

## 静态检查边界

随包校验器检查20份JSON格式、现有全局T索引引用、局部用例编号、连续任务编号、同包依赖无环、路径语法/允许禁止冲突、契约存在、Markdown相对链接与初始状态/权限。它按已读取规则做任务包检查，**不是完整游戏仓库的validate_repo.py，也不是UE构建/游戏验证**。

当前容器无法建立完整Git克隆（外网DNS不可用）；GitHub连接的只读内容查询已完成。没有在伪造的仓库上声明全仓校验通过。完整仓库/任务基线/LFS/构建/实机检查由执行Agent在真实环境按指南运行。
