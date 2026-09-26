# TASK-044 交接｜生存恢复、倒地救援与环境伤害

## 本轮结果

2026-09-26。Owner／Reviewer：XLingyyy。用户明确执行043、044并自行提交推送；无需Issue，未授权合并main。043产品规则已确认，本单提交完整可审查设计候选，**D1—D6尚待Owner批准，任务Blocked**。没有自批设计或独立PR审查。

- 父分支codex/TASK-043-world-time；源码／文档父基线b7d4df35763bca5dd0bb97ed9ab78eea4f68098a。
- 工作分支codex/TASK-044-survival-rules；本任务范围登记提交165f3921efe2d07c6f81ce0811d48001a242beb7，正式路径校验以该提交为基线。
- 专用目录G:/GameFactory/Hearthward/.agent-local/TASK-044；未触碰原TASK-027工作区的Config、uproject或未跟踪文件。
- main核对为35ac8706e50fe8b7cf099b3b73e803572d5a3ff0。与043为堆叠交付，043须先集成；未包含052提前施工分支。

## 交付

1. [DSGN-R04](../design/DSGN-R04-survival-recovery.md)：GDD与当前Demo事实分栏；D1饥饿／回血、D2药品／治疗区、D3耐力／暂停、D4倒地／环境残伤、D5自动维持／救援权限、D6同时间结算；生命状态图与SV01—SV22。
2. [CT-TASK-044](../contracts/CT-TASK-044-survival-transitions.md)：复用现有时钟、库存、动作和Save；定义候选费用锁定／中断、事件去重、时钟消费和回档身份边界；保持DRAFT。
3. 同步任务JSON／Markdown、CURRENT、OPEN_QUESTIONS、契约入口、README及PROJECT_STATE。R04／R08／R09／R18／R22仍OPEN；半份药、残伤、对话暂停和所有新数值明确是候选。

## 验证

本轮只改10个允许的文档路径，父基线源码未变。

- python -X utf8 scripts/validate_repo.py --task TASK-044 --base 165f3921efe2d07c6f81ce0811d48001a242beb7：PASS，0错误；同时检查仓库结构、字段、链接和任务范围。
- git diff --check：PASS。
- 设计纸面算术／覆盖检查：22个SV编号唯一；480W饱食消耗16.6667；零饱食300W降90%最大生命；三日4320W；12／20秒另加0.5延迟。只证明文本算术和案例齐备，不是运行测试。
- 源码事实核对：UseItem当前仍即时药品；TimedAction固定5秒；家具床仍5秒扣饱食恢复。后续不得把这些视为044正式实现。
- 工具自测未重跑：未修改脚本；本轮采用最窄的文档校验。UE构建、原生生存测试、PIE、Shipping均NOT_RUN。

外部工程参考为Epic Gameplay Abilities与UGameplayAbility官方文档，链接在契约正文。仅借鉴费用提交／取消职责，不引入GAS、依赖或额外联网需求。没有编辑二进制资产，无LFS写锁操作。

## 待Owner决定

见DSGN-R04第10节的六行清单。审批应逐组明确批准或修改；不能用“执行任务”“提交推送”或当前Demo行为替代。收到决定后先同步CURRENT和R项获准子范围，保持未定的药品配方／物品实例、环境伤害表和视觉配表开放。批准不等于运行验证。

按用户顺序本轮止于044，不自动推进045或052。052独立代码保留、不合入当前分支。若之后继续，依实际编号顺序进行。

## 提交与清理

本轮成果按授权提交推送至本任务分支，远端核对后移除本任务工作树；提交号及实际清理结果见交付回执。原主目录、其他任务工作树和052远端分支不删除。

回滚本单仅需撤销044文档提交；无代码、资产、配置和存档格式变更，不需要迁移用户存档。父043已批准规则与历史证据保持不动。

## 2026-09-26 实施接手

Owner已批准D1—D6并明确“沿用044，开始实现游戏代码”。本单转Active；实现期间的代码、验证和交付待继续记录，不以设计批准冒充已实现。
