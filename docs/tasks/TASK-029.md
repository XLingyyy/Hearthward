# TASK-029｜AI NPC 上下文快捷建议与显式刷新

状态：Blocked（实现与技术回归已扩展完成；独立 Reviewer / Owner 最终验收与 main 合并仍待完成）。

> **对外交付口径（2026-09-23 Owner 指示）**：最终 AI NPC vNext 总验收、PR 与项目状态统一对外称为 **TASK-029 AI NPC 完整交付**。仓库内部仍保留 TASK-027～040 的原始任务编号与证据，用于追踪 perception/safety、executor、suggestions、combat、recovery、belief、initiative、episode、coordination、routine、componentization 与 context/cognition rework；不重写历史任务编号。

## 目标

把当前对话页里的静态/占位快捷建议改成真正的**上下文建议层**，但保持玩家控制：

```text
UE authoritative facts
        ↓
deterministic suggestion generator
        ↓
3 ephemeral suggestions
        ↓
玩家主动刷新才更换
        ↓
玩家点击某一条
        ↓
revalidate current world/timeline/memory
        ↓
submit as quick_suggestion player input
        ↓
existing LLM/parser → candidate card → player confirm → executor
```

建议本身绝不直接写世界。

## 认知边界

GDD允许“全局建议层”看到最新营地数据，但弟弟不因此自动全知：

- 建议刷新时可以生成“营地现在有 X 份木材……”；
- 未点击：这条信息不会进入NPC memory/context；
- 点击：该建议文本才作为一次来源为 `quick_suggestion` 的玩家输入发送；
- 如果营地数量、timeline、memory规则或安全状态已经变化，旧建议拒绝执行并提示重新刷新。

## 第一版建议类型

不引入新经济阈值，仅用现有事实：

1. **可执行采集建议**：没有进行中委托且当前 collect 安全时，给出“采集4份木材并送回营地”。
2. **营地事实建议**：显示当前共享仓储木材数量，并可选择把这条事实带进交流。
3. **状态/能力建议**：
   - 有进行中委托时优先“问一下当前委托进度”；
   - 空闲时提供“说说你现在能帮我做什么”。

不足3条时用安全的通用交流建议补齐，不泄露未知世界信息。

## R20边界

本单只完成“建议陈旧”的保守处理：点击时重新验证，失效就拒绝并提示刷新。

不在本单定义：
- 已关闭对话后迟到的LLM文本如何展示；
- 迟到文本是否保留历史；
- 三日主动闲聊与什么交互重置计时。

因此R20/R21仍保持OPEN。

## 验收

- refresh前不自动出现新建议或触发模型。
- 每次显式refresh最多3条。
- unselected suggestion不进入AI filtered context/memory。
- stale camp-count / timeline / safety / active-goal变化能拒绝旧建议。
- suggestion click只提交输入；collect仍必须经过已有候选卡+确认。
- 现代Screen和Legacy dialogue都有入口。
