# TASK-030 交接

## 基线

本分支从 `79c1147 feat: add contextual NPC suggestions` 派生。

本地 AI 提交链：

1. `25e8f13` — authoritative NPC perception/safety
2. `d186dc1` — typed Goal→Plan→Action executor
3. `df12965` — executor compatibility / retained cargo
4. `79c1147` — contextual explicit-refresh suggestions

当前分支：`codex/TASK-030-companion-combat-policy-stack`。

## 已实现

建立“LLM只给高层战斗意图、UE决定战术”的窄边界，第一版只复用已有 wait/follow/attack，不增加伙伴血量、敌人、地图资产或新战斗数值。

- 新增 `companion_order`：只接受 hold / follow / assist。
- 自然语言仍走 model → candidate card → player confirm；确认前不改当前 order/委托。
- confirm 时重新验证当前交流距离、暂停与玩家玩法状态。
- explicit directive 确认后可以通过既有 Cancel 语义替换当前委托，实际携带物资不被静默删除。
- 新增纯函数 `HearthwardCompanionCombatPolicy`：玩家中心 command leash、稳定 target tie-break、无目标 fallback。
- Gameplay 继续负责实际导航、LOS、attack cooldown、hit/damage。
- legacy Z/X/C 复用同一 directive 边界，不依赖 LLM。
- filtered context 增加真实 requested order / tactical intent / target / reason。
- Development Editor 增加 `-HearthwardAIBundlePath=`，隔离 worktree 可复用已有本地模型包；Shipping 不接受此覆盖。

上一轮未提交 WIP 的 Context/Decide/Tactic 与 Observation/Evaluate/Intent 两套 API 已统一；`companion_order` 也不再误走 workshop `PreviewGoal`。

## 用户实机复验修正

用户在 UE 中实际验证后反馈两项问题：

1. 现代对话页顶部的“刷新建议 / 记忆与约定”和静态“弟弟”标题区域发生重叠。
2. “帮我采集两份木材带回营地”确认后伙伴没有开始移动。

已修复：

- 对话页把建议刷新和记忆入口收进 y=240 的第二行工具栏，静态标题 y=290 区域不再有动态按钮 hit region；澄清结束按钮独立放到更上方。
- 手动任务卡轮播仅包含 collect/craft/repair，不把 `companion_order` 误显示成采集卡。
- 根因是 TASK-030 `TickCompanion` 在 typed task active 时调用 `StopNavigation()`，每帧抢掉 executor 的 MoveTo；现改为只标记 `TASK_OWNS_COMPANION` 并直接 return，让 collect/craft/repair executor 独占导航。

## 验证

- HearthwardEditor Development build：PASS。
- repo validation：0 errors。
- Python repo tests：31/31 PASS。
- full native `Hearthward.`：33/33 PASS。
- targeted `Hearthward.NPCAgent.CompanionCombatPolicy`：1/1 PASS。
- deterministic combat PIE：29/29 PASS；UE target=`guard_1`，reason=`ASSIST_NEAREST_PLAYER_THREAT`。
- real Qwen3.5-4B combat directive PIE：14/14 PASS：
  - “跟着我。” → `companion_order/follow`
  - “帮我对付附近的威胁。” → `companion_order/assist`
  - 两者确认前 world unchanged，assist 的具体敌人仍由 UE 选择。
- 用户反馈定向 PIE：11/11 PASS；确认顶部标题区域无动作覆盖、刷新/记忆/手动任务入口仍可点击，并验证 collect 确认后产生物理移动与最终2/2交付。
- TASK-028 executor PIE：49/49 PASS；采集、资源不足、retained cargo、制作、维修与存档恢复均未被战斗策略修正破坏。
- 用户原句真实 Qwen 复验：11/11 PASS；“帮我采集两份木材带回营地。”实际输出 `collect/wood/2/additional_acquired/S1`，确认前世界不变，确认后移动并完成2/2交付。

证据见 `docs/qa/evidence/TASK-030/VALIDATION.md`；人工 UE 验收路径见 `MANUAL_UE_VALIDATION.md`。

## 远端状态

GitHub读取可用；创建 TASK-029 Issue 时返回 HTTP 403 Resource not accessible by integration，故 TASK-030 也不虚构远端 Issue/Reviewer。push/PR 继续按用户要求延后到统一交付阶段。
