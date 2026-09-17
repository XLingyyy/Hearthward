# 构建和测试入口

## 当前状态

本机 UE 5.8.1 和工具版本已核验；本游戏 UE工程、Editor Target、真实测试组尚未创建。下列仓库工具可运行；
游戏构建／引擎自动化仍为 **NOT_RUN**。TASK-000负责把真实成功命令写回本文件。

## 已提供的仓库检查

```bash
python scripts/validate_repo.py
python -m unittest discover -s scripts/tests -v
python scripts/validate_repo.py --launch-ready
```

最后一项在初始未配置状态应失败，不要为了绿色删掉门槛。
提交前可在真实Git任务分支运行：

```bash
python scripts/validate_repo.py --task TASK-010 --base origin/main
```

此命令用基线中已合入的任务范围检查已提交、已暂存、未暂存和未跟踪改动；
新任务尚未合入基线时会拒绝自动范围认证，先进行任务审批。

## 待M0填充的项目命令

| 项目 | 真实命令 / 值 | 最近有效SHA / 结果 |
|---|---|---|
| 工程.uproject路径 | 待定 | NOT_RUN |
| Editor Target编译 | 待锁UE／编译器后填 | NOT_RUN |
| 蓝图编译与资产加载 | 待选择测试地图后填 | NOT_RUN |
| 自动化测试组与非零用例数 | 待实现注册后填 | NOT_RUN |
| 目标平台打包 | 待定 | NOT_RUN |
| 可执行包启动及闭环 | 待定 | NOT_RUN |

Epic官方有编辑器与命令行自动化入口；锁定版本核对后，测试命令须导出报告，并确认实际执行用例数、
通过／失败和异常退出。不能只看进程返回或没有报错。参考 [S14](../references/OFFICIAL_SOURCES.md)。
本包不提供一个假装有工程路径的build.bat，不把不存在的`TribeGame.Tests`声称已接通。

## 证据要求

使用 [测试报告模板](../templates/test-report.md)，包括tested_commit、环境、前置数据、命令、
实际执行用例数、结果、日志/产物地址和SHA。模块测试、PIE与打包运行分别记录。
无UE环境的Agent标NOT_RUN，并把验证交给具备环境的人，不伪造截图与日志。

## 检查器的覆盖边界

`validate_repo.py`检查必需文件、JSON任务结构、依赖循环、R/测试编号引用、本文使用的行内相对文件链接、
原设计归档哈希或LFS指针OID。它不完整解析所有Markdown语法、不验证外部URL可用性、不自动证明角色审批。
可选任务范围检查从指定base读取已审批范围，覆盖已提交、暂存、未暂存及未跟踪路径；基线必须由团队指定，
不能自己挑一个更宽松的base。CI默认只执行结构检查和工具自测，**未自动接入每个PR的任务范围核验**；
该核验先由评审者按真实任务、可信基线执行。后续接入CI属于单独工程任务。
