# TASK-031 — AI NPC用户复验修正：任务导航仲裁与对话工具栏

## 背景

用户在 TASK-030 后直接进入 UE 实机复验，发现：

1. 现代对话页顶部动态入口与静态“弟弟”标题发生视觉重叠。
2. 自然语言“帮我采集两份木材带回营地。”形成并确认任务后，伙伴没有开始移动。

## 目标

只修复上述两个已复现回归，不增加新能力。

### 导航仲裁

TASK-030 的 combat policy 在发现 collect/craft/repair active phase 时必须完全让出伙伴导航控制权。它可以更新只读 tactical reason，但不得调用 StopNavigation 或发起新的 MoveTo。

### 对话工具栏

把建议刷新、记忆入口和已有手动任务入口组织到标题上方的固定工具栏行，保证静态标题区域无动态 action hit region。

手动任务卡只轮播 collect/craft/repair；高层 combat directive 继续通过 Z/X/C 或自然语言 candidate 进入，不混入资源任务卡。

## 验收

- Editor build PASS。
- 用户反馈定向 PIE：标题区域无按钮覆盖；collect 确认后实际移动并完成2/2。
- 用户原句真实 Qwen 复验 PASS。
- TASK-028 executor PIE 保持全通过。
- TASK-030 deterministic combat PIE 保持全通过。
- full native Hearthward Automation 保持全通过。
