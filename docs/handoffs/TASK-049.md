# TASK-049｜接手与设计交接

## 当前目标和来源

真实TASK-049＝用户规划稿TASK-051：叙事、四区和主支线施工稿。Owner／Reviewer XLingyyy；用户明确不提Issue。原稿043对应真实041的偏移已按TASK-041映射核实。

## 工作区与权限

- 原目录 `G:/GameFactory/Hearthward` 留在旧TASK-027分支，HEAD bdfc0bced35cab4015b5a0d752077b4838c66f9f；Config/DefaultEngine.ini、Hearthward.uproject与未跟踪历史图片等原有改动均保留。
- 本单独立目录 `G:/GameFactory/Hearthward-task049`，分支codex/TASK-049-narrative-layout，基线 `origin/main@efb275b65e771b24ea01b120b733efca9a234d6c`，已取回并核对048／PR50合入。仅检出文档、配置来源与源码用于阅读，LFS未下载，不作为可启动UE工作区。
- 管理worktree工具在本轮工具目录不可用，采用Git worktree的文档隔离目录。未复用有任务依赖的旧工作树，未切换原工程分支或改动原配置。
- 首次agent_context因049任务单不存在返回ERROR；随后按本轮任务授权补建MD／JSON，再执行接手与L0检查。
- 可写范围见049 JSON；当前仅设计文档及README，本单不写二进制资产，因此不申请地图锁。提交／推送／合并未授权，当前未提交、未推送。

## 交付与确认

[D1—D6](../design/DSGN-R17-narrative-layout.md)、[23张任务卡](../planning/TASK-049/QUESTS.md)、[数据](../planning/TASK-049/candidate.json)、[关卡约束与图](../world/TASK-049/LAYOUT.md)、[实施接口与迁移](../planning/TASK-049/IMPLEMENTATION.md)。新设计全部DRAFT，R17/R24保持OPEN，README及设计索引同步记录。

待Owner确认D1—D6后才能把新剧情／人数布局／救援返回／奖励／增援配置视为批准规则；施工源码和地图范围届时具体协调。Owner与Reviewer同人按本次指定登记，不伪称独立审查。没有创建Issue或PR。

## 发现与验证

现有代码可复用Camp营救／故乡、047唯一奖与048自然物；正式人类关卡尚未布置。Gameplay旧任务kind／category不一致、领奖先写Claimed再执行世界变更是代码阅读事实，已列后续实现清单；本轮未修改或声称运行复现。

设计关系检查34/34 PASS；证据和边界见[报告](../qa/TASK-049/REPORT.md)。UE及实际通行均NOT_RUN。仓库L0检查PASS，0错误；git diff --check通过。本地候选allowed_paths对照无越界，结果见docs/qa/TASK-049/local-scope.json。正式--task/--base路径检查仍BLOCKED：主干基线尚无TASK-049任务快照，不能伪称已通过审批基线路径检查。初次稀疏检出缺少旧链接所指文件，补检出.github、Runtime和art_source的已有文件／LFS指针后L0通过，未改这些来源。SVG结构有效；未做UE布局验证。
