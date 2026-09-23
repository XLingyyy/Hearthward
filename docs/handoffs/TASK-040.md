# TASK-040 交接

## 基线与分支

- AI NPC vNext 基线：`04239f542ee99c1a735a380a66faa6ec99dc614b`
- 来源分支：`codex/ai-npc-vnext-pr`
- 当前本地分支：`codex/ai-npc-vnext-rework-01`
- 基线已包含 TASK-032～039；本任务没有回退到 TASK-029 脏工作区。
- 原始 `D:\Dev\Hearthward` checkout 的未知未提交改动没有被清理、覆盖或纳入当前 worktree。

## 完成内容

### R1｜ContextProjection

新增 `HearthwardNPCContextProjection` 深模块。

- UE 只捕获一次可信快照。
- full / compact / minimal 三档都对同一 snapshot 做纯投影。
- 当前能力相关 belief、episode、记录优先；硬规则、玩家原话和 unresolved 限制不进入普通 Top-K。
- 不再把 `observed_camp_inventory / last_seen_camp / camp_knowledge` 作为并行模型事实源重复注入。
- 每档都通过实际 llama.cpp `/apply-template` → `/tokenize` 计真实模板 token。
- 保持 `max_input_tokens=3328`、输出 256。
- 超限只尝试下一档，不生成；三档都超限时 `CONTEXT_OVERFLOW` 且 generation=0。
- 只有通过计数的同一个 Body 才进入唯一一次 `/v1/chat/completions`。
- 诊断保留 `context tier / dropped fields / input tokens / generation calls`。

### R2｜能力契约单一来源

- `Capabilities()` 继续作为能力枚举唯一事实来源。
- `CompanionOrderPrompt()` 从 registry 自动生成指令集合。
- prompt 不再硬编码 hold/follow/assist 而漏掉 routine。
- schema 测试改为结构化逐 branch 对照 registry。
- hold/follow/assist/routine 全部走相同 `Validate` 边界。
- `inventory` 与 `inventory_report` 的 read-only / cognition-write 语义仍严格分离。

### R3｜Belief 时效

`FHearthwardNPCBelief` 新增 `LastEvidenceAt`。

- `RecordedAt`：最后一次 value/source 语义变化。
- `LastEvidenceAt`：当前 value/source 最近一次有效证据。
- 同值同来源刷新只推进 evidence time，不推进 `Memory.Revision`，因此不会仅因重新观察相同事实使待确认卡失效。
- 来源/数值变化继续视为语义变化。
- 旧档缺字段时保守继承 `RecordedAt`。

### R4｜Episode 完整性

新增 persistent bounded command coverage：

- `Complete`
- `Truncated`
- `Unknown`

新任务只在真实 `SubmitGoal -> Accepted` 后登记 Complete。事件从 128 ring 被淘汰时相应 command 降为 Truncated；后续 completed/cancelled 不能恢复完整性。旧档没有 coverage 证据时迁移为 Unknown。

Episode 的终态与 coverage 分开表达：任务可以“已完成但记录已截断”。

### R5｜Save / 恢复

- Save schema：3
- NPC cognition state version：3
- 明确的 Schema 2 → 3 迁移：
  - `LastEvidenceAt = RecordedAt`
  - coverage = Unknown
- current-format 损坏字段严格拒绝，不自动伪装成旧档迁移。
- migration / validation 在 Restore 修改世界状态前完成。
- `RestoreMemory` 不再二次猜测迁移。

## 当前验证

PASS：

- `python scripts/validate_repo.py`：0 errors
- repository Python tests：31/31
- UE Editor Development build：PASS
- full native `Hearthward.*`：40/40 Success
- 关键新回归：
  - `Hearthward.NPCAgent.CapabilitiesAndLimits`
  - `Hearthward.NPCAgent.BeliefStateProvenance`
  - `Hearthward.NPCAgent.BoundedContextProjection`
  - `Hearthward.NPCAgent.GroundedEpisodeProjection`
  - `Hearthward.Save.FileIntegrityAndSnapshot`
  - `Hearthward.Save.NPCMemoryCompatibility`
  - `Hearthward.Save.PoolProtectionAndSafety`

构建环境说明：

- 统一目标与锁定版本：UE 5.8.2 CL 56702186；本机安装版本一致。
- 在干净提交 `6d1ca5e` 上通过 UEClient 执行默认 Editor Development 构建，失败于 `HearthwardAgentInteraction.cpp` 与 `HearthwardNPCContextProjection.cpp` 的 Unity 编译单元 `Json` 重定义（C2084/C2264）。
- 本轮 repo validator 0 errors、Python 31/31；当前提交的原生测试因构建失败未运行。此前日志中 40/40 成功属于历史工作树记录。

真实模型当前源码：

- 模型 bundle 存在于 `D:\Dev\Hearthward\Runtime\LocalAI`。
- GGUF SHA256 与 `config/local-ai.lock.json` 一致。
- Unreal built-in Python Remote Execution 能发现当前 worktree 的 UE process。
- 本轮使用 GameFactory `engine_adapters.ue5` UEClient 完成默认 Editor 构建；真实 Qwen e2e 本轮未执行，也未验证当前 Game WorldContext 的触发路径。此前工作树记录的 WorldContext 阻塞属于历史验证状态。
- 因此当前源码真实 Qwen CTX-01～CTX-04：**BLOCKED / NOT_RUN**。
- TASK-039 的历史 `CONTEXT_OVERFLOW` 不作为 TASK-040 当前源码结果复用。

详见 [TASK-040 当前验证](../qa/evidence/TASK-040/CLEAN_TREE_REVIEW.md)、[历史工作树验证](../qa/evidence/TASK-040/VALIDATION.md) 与 [ADR](../decisions/ADR-TASK-040-npc-context-and-cognition.md)。

## 尚未验收完毕

- 当前源码真实 Qwen CTX-01～04 与 16 类样本 × 干净/压力进度至少 32 次请求尚未执行；现有 PIE 脚本只准备了 collect 正常/压力和 routine 局部场景，不能替代整套验收。
- TASK-028/034/036/038 的本轮相关 runtime PIE 回归尚未逐套重跑。
- Save 测试读取了更早的真实旧档，但尚无真实 pre-TASK-040 Schema 2 文件或对应旧版序列化路径的迁移证明。
- 当前源码默认 Unity 构建失败，原生测试未运行；真实模型 e2e、相关 PIE、Schema 2真实存档迁移、独立 Reviewer、Owner 体验验收和技术合并条件仍未闭合。
- 原 TASK-040 实现范围为 28 个文件，Content/ 为零修改；Owner 随后明确授权同步仓库 UE 目标版本口径并提交推送验证记录，扩展范围已写入 TASK-040 allowed_paths。

## 流程状态

TASK-040 保持 `Blocked`：

- 真实 GitHub Issue：未登记
- 独立 Reviewer：未登记
- commit：用户已于本轮授权提交到独立任务分支；提交标识以 Git 历史为准
- push：用户已于本轮授权推送独立任务分支；远端状态以 GitHub 分支为准
- PR：未执行
- merge：明确未授权，未执行

当前代码在 UE 5.8.2 默认 Unity 编译下存在可复现的 C++ 编译错误；repo validator 与 Python 31/31通过，当前干净提交的原生测试未运行。TASK-040 保持 Blocked，见 clean-tree review。
