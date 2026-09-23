# TASK-040 clean-tree branch review

日期：2026-09-23

## 受测对象

- 仓库：`XLingyyy/Hearthward`
- 远端分支：`origin/codex/ai-npc-vnext-rework-01`
- 受测源码提交：`6d1ca5e35050566408f69329cedf2bd2b526d4a4`
- 受测前本地验证 worktree 干净；验证后只增加文档与工具链口径更新。
- `origin/main`：`73bb10ec4c19260cb72112c7e282a2c29f6c2432`。候选分支比 main 落后 11 个提交、领先 4 个提交；最新 `git merge-tree --write-tree origin/main HEAD` 预演发现 `README.md` 内容冲突，`docs/tasks/TASK-004.json` 和 `docs/tasks/TASK-004.md` 可自动合并。未执行合并。

## 工具链

- 统一锁定目标：UE 5.8.2，Installed Build changelist `56702186`。
- 实际引擎：UE 5.8.2 changelist `56702186`，`G:/UnrealEngine/UE_5.8`。
- UBT 检测到 MSVC 14.44.35228、Windows SDK 10.0.22621.0。

## 验证结果

| 检查 | 结果 | 详情 |
|---|---|---|
| `scripts/validate_repo.py` | PASS | 0 errors |
| `python -m unittest discover -s scripts/tests -v` | PASS | 31/31 |
| `HearthwardEditor Win64 Development` | FAIL | UEClient 默认 Unity 构建报 `C2084` / `C2264` |
| 当前受测提交原生 `Hearthward.*` 自动化 | NOT_RUN | Editor 构建失败，自动化测试未启动 |
| 当前源码真实 Qwen CTX-01～04 | NOT_RUN / BLOCKED | 未进入 PIE 模型验证 |
| 16 类样本 × 干净/压力进度 | NOT_RUN | 至少 32 次请求未执行 |
| TASK-028/034/036/038 runtime PIE 回归 | NOT_RUN | 本轮未执行 |
| 真实 pre-TASK-040 Schema 2 文件迁移 | NOT_RUN | 当前缺少对应真实存档 |

Editor 构建命令通过 GameFactory `UEClient.build.project(target="HearthwardEditor", configuration="Development", timeout=1200)` 执行。失败发生在生成的 `Module.Hearthward.cpp`：[`HearthwardAgentInteraction.cpp:21`](../../../../Source/Hearthward/AI/HearthwardAgentInteraction.cpp#L21) 与 [`HearthwardNPCContextProjection.cpp:11`](../../../../Source/Hearthward/AI/HearthwardNPCContextProjection.cpp#L11) 在匿名命名空间中定义相同签名的 `Json`，触发 `C2084`；其后 `Result.Json=Json(Facts)` 产生 `C2264`。

下一步应让两个 Unity 编译源文件中的辅助符号名称唯一，然后从干净提交运行默认 Editor Development build，再运行全量原生测试。该复验完成前，先前工作树记录的 40/40 native PASS 不作为当前 clean-tree PASS。

## 远端与合并状态

- 当前文档差异的 `git diff --check`：PASS。
- GitHub CLI 查询因 HTTP 401 未能读取 PR / Reviewer 实时状态；TASK-040 任务记录中 Issue 和 Reviewer 均未登记。
- 当前分支未创建 PR，未合并。当前代码构建失败，且仍缺真实模型、相关 PIE、旧档迁移、独立评审与 Owner 验收证据，暂未达到合并条件。
