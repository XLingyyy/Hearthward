# Hearthward TASK-084—103｜交付说明

本包为用户要求编写的Agent执行任务单，非游戏更新包。2026-10-06，参考main `6fcf5c22e965f0f7409438f19bc7b09e96ffb058`。

## 内容

20份任务Markdown、20份同名schema_version=1 JSON、20份待接手handoff；共142条细化验收用例。另含任务索引、执行顺序、公共写窗口、共享契约草案、证据/资产/缺陷模板、来源与静态校验材料。

[任务索引](docs/planning/TASK-084-103/README.md) · [完整合订版](TASKS-084-103-FULL.md) · [Agent执行指南](docs/planning/TASK-084-103/EXECUTION_GUIDE.md)

## 导入方式

先将ZIP解压到仓库外的临时目录。核对真实Hearthward仓库已有TASK-084—103是否冲突；有同号文件时停止覆盖，不重编号或替换对方任务。确认后将本包`docs/`新增文件按原相对目录纳入游戏仓库。不要把临时目录当游戏工程，不要覆盖现有根README或000—083任务。

根`DELIVERY.md`、`TASKS-084-103-FULL.md`、`VALIDATE_PACKAGE.py`、`VALIDATION.json`、`MANIFEST.sha256`是交付辅助，可留在仓库外；真正仓库增量是本包docs/。本包没有包含/改写原根README和旧设计/测试矩阵。

目录结构：

```text
docs/
  tasks/TASK-084.md ... TASK-103.md
  tasks/TASK-084.json ... TASK-103.json
  handoffs/TASK-084.md ... TASK-103.md
  contracts/CT-TASK-085-presentation-readmodels.md
  planning/TASK-084-103/
    README.md
    EXECUTION_GUIDE.md
    DISPATCH.md
    EXECUTION_ORDER.json
    TEST_CASES.csv
    EVIDENCE.template.json
    ASSET_REGISTER.template.csv
    DEFECTS.template.csv
    SOURCES.md
```

## 激活与状态

任务文件当前Backlog；Owner沿项目记录为XLingyyy，Reviewer/Issue未指派，建议分支未创建。只获得编写/交付请求，不继承旧任务的代码修改/提交/推送/合并/发布授权。Owner派单时可一次确认本批/子批范围并记录本次权限，Agent无需重复询问已授权事项。

先把获准任务快照与精确资产包纳入真实基线，再运行任务范围检查；旧main没有新任务，不能直接对旧SHA证明新单范围通过。资产包不默认授予整Content；拟新增代码/测试文件先检查现有同义实现。

## 检查

在包目录可运行`python VALIDATE_PACKAGE.py`，仅做任务包静态检查，无网络、无Git写入、无游戏执行。若带`--repo <真实仓库根>`，额外只读检查任务号冲突、全局测试编号和文件导入冲突，不复制/覆盖任何文件。

导入完整仓库后由执行Agent按EXECUTION_GUIDE运行原validate_repo、工具自测、真实任务范围检查和相应UE/人工验收。静态文件通过不等于游戏测试通过。本次未运行UE、未创建远端分支、未提交/推送或发布。
