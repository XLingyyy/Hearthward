# TASK-105 M0 工程整合验证

2026-10-11。基线 `ff5f6f985b326011b9dc968ebe9ace55f4582088`，本地分支 `codex/TASK-105-engineering-optimization`。用户选择先完成 M0 工程优化，M1–M3 保留提案。本报告记录本机真实执行，包内上游 403 与 Linux 工具结果仅为历史背景。

## 实现

- 整合一般/战役目录索引，保留字符串比较、首次匹配、原行对象和未索引表的原错误路径。
- 战役总数和清除数直接遍历当前数组，保留原有敌人组、生命条件、增援分母及胜利规则。
- 修复首次 UE 5.8.2 编译发现的 JSON 字段键类型差异：以 `FString(Field.Key)` 显式转换。
- 修复 OBJ 在 Windows 检出为 CRLF 后不符合源资产字节清单的问题：仅为该 OBJ 固定 LF，不改几何、不改预期摘要、不重新导入资产。
- 修复已发布 v0.3.0 导致原版本测试失效的夹具：预览/稳定比较使用固定输入，模拟更新使用高于当前版本的动态主版本，并检查相同发布版本不提示更新。
- 用户在本地删除了根 docs 的旧 GDD；四处引用改为仍保留的归档。本次提交保留远端原件，不纳入该已有删除，也不更改设计规则。

未更改游戏数据、存档格式、模型参数、画质和 Content 二进制。UI 调用关系见 [静态审查](UI_AUDIT.md)，尚无帧率收益证据。

## 已执行验证

| 项目 | 结果 | 证据与范围 |
|---|---|---|
| 正式目录 ID | PASS，12 表 | [生产检查](catalog-production.json)；77 items、29 skills、58 craftingRecipes、38 repairRecipes，含兼容/内部目录行，不等同设计物品配额 |
| 全仓 Python 工具 | 初次 84/85 PASS | [原始结果](tools-initial.txt)；唯一失败是 OBJ CRLF 字节差异 |
| OBJ 修复定向复测 | 8/8 PASS | [复测结果](tools-axe-lf-retest.txt)；其余已通过工具路径未修改 |
| 新增原生索引/战役测试 | 7/7 PASS | [原生结果](native-results.json) |
| 相关原生回归 | 最终 37/37 PASS | 首轮 36/37，旧版本夹具修复后单项 PASS；含战役、库存、装备、存档迁移及药品可变属性 |
| 原生警告 | 保留 | 5 项 Campaign067 测试有 CleanupWorld 未先 EndPlay 的既有夹具警告；不声称零警告 |
| Editor Development | PASS | 首次编译失败、修复后构建及最终构建 JSON 位于本地 `.agent-local/qa/TASK-105/` |
| 仓库结构/文档 | PASS | [结果](repository-check.txt)，断链修复后 0 错误 |
| 基于旧提交的任务范围检查 | 无法通过该前提 | 基线提交尚无 TASK-105 任务单，检查器返回 No usable approved task snapshot；没有修改检查器或伪造基线授权 |
| Shipping Build/Cook/Stage/Archive | PASS | [构建记录](shipping-build.json)；候选目录 `E:/HearthwardQA/TASK-105/m0-candidate-20261011` |
| 候选实际运行 | PASS，局部冒烟 | [运行记录](runtime-smoke.json)；新游戏、背包、地图、手动保存、独立重启继续及可见装备/保存节点一致 |

## 证据身份和边界

候选保留基线提交、分支、生产源码 diff 和新增源码副本；构建时为未提交工作树，不能把旧基线 SHA 单独当作受测实现。构建记录与完整日志留在本地 QA 目录。该受测生产实现随后提交为 `efd1e1ec9137a2179609db3bf49c2ba4cde94fef`；后续交付记录更新仅涉及文档。

新档→首救→回营的完整连续路线、六场景 CPU/Vulkan 同口径性能、真人和第二台机器尚未在本轮完成；既有语言质量与 1% Low 限制继续保留。上述原生夹具与启动冒烟不替代这些验收，也不声称整个规划 M0 的发布退出标准已经达成。用户随后授权提交、推送并通过 PR 合入 main；代码集成状态见 [PR #70](https://github.com/XLingyyy/Hearthward/pull/70)。本轮未发布。

## 实机证据

[保存前装备](runtime/inventory.png)、[重启后装备](runtime/inventory-restored.png)、[恢复场景](runtime/continued.png)、[恢复节点](runtime/saves-restored.png)。画面由正常窗口输入取得；没有完成完整深层库存比对、设备混音或 AI 推理评测。
