# TASK-034｜Event-driven NPC Initiative 主动交流

状态：Blocked（用户已授权本地继续研发；远端 Issue / 独立 Reviewer 尚未补齐，不执行 Agent 侧合并）。

## 目标

让 NPC 在真实事件发生后主动交流，而不是只在玩家发起 prompt 后说话，也不允许后台轮询 LLM 决定“要不要说”。

```text
real event / belief correction
          ↓
deterministic Initiative Policy
          ↓
dedupe + bounded queue + cooldown
          ↓
player in communication range
          ↓
existing HUD / dialogue surface
```

## 第一版触发

- task completed
- adaptive replan
- hard blocked
- player_report 被回营 firsthand 纠正

普通 partial delivery 不触发主动台词，避免碎片化刷屏。

## 行为约束

- 不调用 LLM 生成主动台词。
- 玩家超出伙伴交流范围时消息排队，不远距离显示。
- 正在模型回复、任务卡待确认或澄清未结束时不插话。
- 同一 evidence / correction 使用 dedupe key，不重复播报。
- 队列有上限，主动消息有显示时长和 cooldown。
- 复用现有 HUD AI 状态面板，不新增第二套通知 UI。

## 验收

1. pure policy 覆盖 completed / replanned / blocked / belief correction / ignored delivery。
2. Editor build PASS。
3. 原生 NPCAgent 测试增加 Initiative Policy 且全绿。
4. PIE：完成2份木材任务后，无新玩家输入，HUD 主动显示完成提醒。
5. belief report 99 被 firsthand=2 纠正后主动说明。
6. replan 发生时玩家在30m外不得显示；重新靠近后显示 queued initiative。
7. initiative 全程不启动模型请求。
