# TASK-032｜AI NPC 自适应重规划与执行恢复

状态：Blocked（用户已授权本地继续研发；远端 Issue / 独立 Reviewer 尚未补齐，不执行 Agent 侧合并）。

## 目标

在 TASK-028 typed executor 和 TASK-027 authoritative perception 之上增加确定性的 Recovery Policy：

```text
Goal → Plan → Action
              ↓
        action failure
              ↓
      Recovery Policy
       ↙      ↓       ↘
 retry   rewind move   hard fallback
              ↓
          continue plan
```

LLM 不生成恢复脚本。UE 根据当前 action、失败原因、真实物资和当前世界可用性决定恢复。

## 第一版能力

- 采集过程中资源点位置变化：回退到 `MoveTo(Source)`，重新靠近后继续采集。
- 短暂去程 / 工作台路径失败：在有界预算内重试当前移动或回退到对应 `MoveTo`。
- 已取得本任务真实物资：任何恢复都优先保货返营；营地不可用时安全持有。
- 连续自适应恢复最多 2 次，之后降级到原有返营 / 等待语义。
- 成功采集、取料、制作 / 维修后清空恢复预算。

## 明确不做

- 不凭空选择第二资源点；当前世界模型只有已知 Source S1。
- 不把资源耗尽、目标销毁、安全失败、材料不足、权限冲突伪装成可恢复。
- 不让 LLM 输出路径、坐标、重试次数或逐帧动作。
- 不改变存档 schema；adaptive recovery 是 runtime execution state。
- 不修改 TASK-026 地图 / Content 资产。

## 验收

1. Recovery Policy 纯测试覆盖 source relocation、route retry、workshop rewind、cargo priority、预算耗尽、hard block。
2. Editor C++ 实际编译新 `HearthwardAgentRecovery.cpp`。
3. PIE 在 Gather 进行中移动真实 Source，NPC 自动进入 `Recovery:Replan`，重新导航并完成 2/2 入库。
4. 重规划期间不增加 acquired / delivered，不产生虚假成果。
5. 真实 Source -2、Camp +2；无需玩家调用 `ResumeBlocked`。
6. repo validator 与 Python 工具测试保持通过。
