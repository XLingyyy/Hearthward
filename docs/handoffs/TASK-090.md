# TASK-090｜本地实现交接

[任务单](../tasks/TASK-090.md) · [元数据](../tasks/TASK-090.json) · [本批指南](../planning/TASK-084-103/EXECUTION_GUIDE.md)

## 2026-10-09 交互提示热修复

2026-10-09 已修复靠近交互点不显示提示：新版 HUD 重新接入遗物/路标/工作台/通用交互目标，提示跟随实际距离和键位，衬板适配多行与 150% 字号。Task090 最终原生 4/4 通过，实际 PIE 遗物靠近显示/离开隐藏已验证。版本 `0.2.0-preview.20261009.2`，候选10 Shipping Build/Cook/Stage/Archive 已成功；运行目录 F:/HearthwardDemo/iteration-084-103-20261009-10/Windows，源码 0e430f5fa740dff35bd1ba8924e031115edb9cae；正常键鼠仍由用户继续验证。整批尚未完成，原有模型/性能/完整路线/真人与二机缺口保留。 新增 read-only Campaign API，保留旧接口行为；源码范围已由 104842d0 快照记录。证据：[prompt-20261009](../qa/TASK-090/prompt-20261009/REPORT.md)。新增用例与原三项同次运行通过，不合并旧分母。

## 2026-10-07 历史状态

2026-10-07，任务Active，根目录`G:/GameFactory/Hearthward`，实际分支`codex/TASK-084-103-iteration`。完整HEAD `6fcf5c22e965f0f7409438f19bc7b09e96ffb058` 加root协调的本轮未提交工作区。用户本会话授权逐单本地实施和验证；root拥有生产源码与公开UEClient生命周期，mcp_setup补急救回归及证据文档。Owner XLingyyy；正式Reviewer与人工签收未指派。

## 实际产物

只读PreparationView消费原Campaign条件/阶段，HUD与日志显示一致的行动与手动领奖入口，修复D084-001开场地点。主线与材料保持原ID；任一人物倒地时隐藏普通准备与材料目标/详情，保留急救文本及材料tracker。main_03 waiting取实际Actor或已Located Position，不沿旧出发fork；日志目标文案与地图定位复用Resolver.World，所选任务查询不改TrackedQuest。没有改主线条件/成本、自动领奖、未知地点解锁或Save schema。

## 实施与证据

实际构建`.agent-local/qa/TASK-090-100/build-green-repair-20261007/result.json` ok=true。过滤`Hearthward.Iteration.Task090.`实际3项均Success：AuthoritativeStagesAndReadOnly为0 warning/error，BrotherDownedSuppressesMaterials与PlayerDownedSuppressesMaterials各1条Standalone EnhancedInput warning、0 errors。最新子集`docs/qa/TASK-090/native-green-20261007.json`保留原test对象、设备和报告时间；只计本单3项。

两条150%真实伤害倒地Widget回归首轮均Fail，6 errors/2 warnings全部保留于`native-urgent-red-20261007.json`；生产修复后原断言GREEN。AuthoritativeStagesAndReadOnly新增真实journal当前目标、map组件随Position更新、显式所选任务Resolver不改tracking等断言已随最新构建运行，旧`native-first-20261007.json`不覆盖。测试解析实际visible components/动作，不把布局配置存在当已显示。

## 交付与权限

README/PROJECT_STATE由root统一收尾。原生已具本单3项成功证据；正常OS/中文IME、最终渲染、Owner视觉/真人、二机、性能、发行未验。现有dirty状态下含任务快照的--base范围检查仍受真实基线约束，不改验证器或伪填SHA。未提交、未推送、未合并、未发布。

## 下一位Agent

Source全部释放且处于root Package冻结，不再写入。原生未出现新相关改动时无需重复同一用例。后续以正常新档/中期兼容旧档、OS页面链路、150%长提示及多比例实际绘制、Load展示组合分别补证据；具体T090-C01—C07的已验子范围及剩余见REPORT。
