# TASK-029 交接

## 2026-09-23 对外总交付口径

Owner 指定：AI NPC vNext 最终对外验收统一使用 **TASK-029**。内部 TASK-027～040 继续作为实现分解与可追溯证据，不删除、不改号。最终 PR 应将以下内部工作作为 TASK-029 的完整能力增量说明：authoritative perception/safety、deterministic Goal→Plan→Action executor、contextual suggestions、hold/follow/assist/routine、adaptive recovery、typed Belief、event-driven initiative、grounded Episode、tactical cooperation、coordination prior、camp routine、组件化收口、bounded context/token budget、真实 Qwen guardrail 与 Schema 2→3 migration。

当前最终证据汇总见 `docs/qa/evidence/TASK-040/`；该目录保留内部返工编号，仅作为 TASK-029 总交付的底层证据来源。

## 基线

- TASK-027：`25e8f13 feat: add authoritative NPC perception safety layer`。
- TASK-028 主实现：`d186dc1 feat: add typed NPC action executor`。
- TASK-028 兼容收尾：`df12965 fix: preserve companion executor compatibility`。
- 当前分支：`codex/TASK-029-contextual-suggestions`。
- 目标是保持第四个独立可审查 commit，之后与 027/028 一起统一形成 GitHub PR。

## 已实现

本单完成 GDD 第11章“三条建议、只主动刷新、点选才传给弟弟”的最小闭环：

- 新增纯 deterministic `HearthwardNPCSuggestions` generator。
- 建议只存在于 AI subsystem 的 ephemeral cache；不会自动刷新。
- 显式刷新最多生成3条：
  - 空闲且安全时的 collect 建议；
  - 当前营地木材事实；
  - 能力/状态交流；有活跃委托时优先 progress。
- 刷新本身不写 memory、clarification、model input、filtered context、candidate 或世界。
- 每条建议绑定 timeline epoch + memory revision；camp/progress 额外绑定观测数量或 command id。
- 点击时重新验证 camp count、timeline、safety、active command。
- 选中的建议才以 `quick_suggestion` 来源进入现有 `SubmitPlayerText` 推理路径；仍必须经过 candidate + 玩家确认才能执行世界写入。
- Modern Screen 和 `-HearthwardLegacyUI` 共用同一 suggestion cache，没有第二条知识通路。

## 验证

详见 [TASK-029 VALIDATION](../qa/evidence/TASK-029/VALIDATION.md)。

当前结果：

- HearthwardEditor build：PASS。
- full native `Hearthward`：32/32 PASS。
- Modern suggestion PIE：40/40 PASS。
- Legacy suggestion PIE：8/8 PASS。
- TASK-028 executor PIE：49/49 PASS。
- TASK-025 workshop regression：129/129 PASS。
- TASK-012 historical companion PIE：45/45 PASS。
- real-model generation：本隔离 worktree 无 GGUF，NOT_RUN；缺模型路径已验证不会直接执行建议。

## 认知边界

“全局建议层知道”不等于“弟弟知道”。

未点击的营地数量：
- 可以显示在玩家侧建议按钮；
- 不会出现在 `LastInput`；
- 不会进入 `LastFilteredContext`；
- 不增加 memory revision；
- 不触发模型或 candidate。

只有玩家点击后，该文本才作为一次 `quick_suggestion` 玩家输入传给弟弟。

## R20 / R21

本单只解决 suggestion cache 自身的 stale revalidation。

仍未定义：
- 对话关闭后迟到的模型文本如何呈现；
- 慢回复是否进入历史；
- 三日主动闲聊及其计时重置条件。

因此 R20/R21 继续保持 OPEN，不将本单描述为“完整主动 NPC”。

## 下一步

TASK-029 完成验证后形成独立本地 commit。下一小步可以进入 TASK-030，但应保持同样分层：优先做 deterministic combat intent/policy boundary，而不是让 LLM 做逐帧战术决策。GitHub 通路恢复后再把 027/028/028-fix/029（以及经验证的下一小单）统一推送为一个可审查 PR，不自动 merge。
