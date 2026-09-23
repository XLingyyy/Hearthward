# TASK-040 交接

> **对外发布/验收口径：TASK-029 AI NPC 完整交付。** TASK-040 是内部返工与证据编号，TASK-027～040 原始历史继续保留。

## 基线与工作分支

- AI NPC vNext 集成基线：`04239f542ee99c1a735a380a66faa6ec99dc614b`
- 返工来源：`origin/codex/ai-npc-vnext-rework-01@97f8818`
- 当前修复分支：`codex/ai-npc-vnext-rework-01-fix`
- 目标 main：`73bb10ec4c19260cb72112c7e282a2c29f6c2432`
- 用户主 checkout `D:\Dev\Hearthward` 的未提交改动始终未被 reset/switch/覆盖。

## 本轮实际修复

### 1. Unity build collision

原 clean-tree 默认 Unity build 在 `AgentInteraction.cpp` 与 `NPCContextProjection.cpp` 的匿名 `Json` helper 上 C2084/C2264。

已改为模块前缀 helper，并继续审计匿名 helper；Workshop / Save 重复通用 `Counts` 也完成前缀收口。

结果：默认 Editor Development build PASS。

### 2. CTX-02 压力下真实模型误判

真实 Qwen 在压力状态曾把明确“新采四份木材”误判为缺数量。定位到两类上下文污染：

- Full tier 无差别注入无关 camp beliefs。
- 通用 capability prompt 永久带所有 crafting recipe 数量。

修复后：

- Full = full detail for relevant facts，不再等于 dump all facts。
- collect 不读取无关营地库存 belief。
- inventory query 判定收紧，避免“带回仓库”误命中。
- recipe prompt 只保留能力/物品身份，不再永久注入材料/产出数。
- system prompt 明确中文数词与 batch 语义。

### 3. inventory_report 中文数量

M08 “十份木材”原先会被只认阿拉伯数字的 guardrail 拒绝。新增显式中文数量识别（0～99 + 合法量词），仍不允许报告修改真实仓库。

### 4. Context budget

真实三档计数已验证：

- normal / CTX-02 pressure 均可在预算内单次生成
- CTX-03：full 超预算后降到 compact，2832 tokens，限制保留
- CTX-04：required-minimal 4020 tokens，generation=0，明确失败

### 5. Schema 2 → 3

新增真实磁盘文件级 automation：

- 生成 Schema 2 `.hws`
- 正常 `HearthwardSave::Read` 迁移
- 写出 Schema 3
- 二次读取并严格 Validate

1/1 PASS。

## 当前最终证据（同步 main 前）

- Unity Editor build：PASS
- native：**41/41 PASS**
- real Qwen matrix：**32/32 safety PASS**
- core M01～M10 raw contract：**20/20 PASS**
- generation：32 cases / 32 calls / 0 normal overflow
- CTX-03：PASS
- CTX-04：PASS，0 generation
- Schema 2→3 file migration：1/1 PASS
- TASK-028：49/49 PASS
- TASK-034：16/16 PASS
- TASK-036：16/16 PASS
- TASK-038：26/26 PASS
- repo validator：0 errors
- Python：31/31

证据目录：`docs/qa/evidence/TASK-040/`。

## Raw model 与 guardrail 分开报告

M11/M12/M14/M16 在 clean/pressure 下的 raw JSON 仍可能给出较宽的 collect/repair intent。最终 deterministic guardrail 均正确拒绝/澄清，不产生禁止的世界写。该差异保留在 `model-results.json[l]`，不会把 guardrail 成功写成 raw model 成功。

## 对外 TASK-029 说明

最终 PR/验收不写成“只完成 TASK-040”，而应写：

**TASK-029 AI NPC complete delivery — internally implemented across TASK-027～040.**

并说明实际包含：

- perception/safety
- executor
- suggestions
- combat/directives
- recovery
- belief
- initiative
- episode memory
- tactical cooperation
- coordination prior
- camp routine
- componentization
- bounded context
- real Qwen + guardrail
- save migration

## 下一步

仅剩集成/发布流程：

1. 同步 main `73bb10e+`
2. 解决 README 冲突
3. 最终 validate/build/native smoke
4. 更新最终 SHA
5. commit/push
6. 创建 PR
7. **不 merge**
