# TASK-029 + TASK-030 主干组合复验

日期：2026-09-24。操作环境：Windows、UE 5.8.2、Editor Development、隔离 `-HearthwardSaveTestPool`。合并候选从 `codex/integrated-latest-20260923@cbf0ebe50cb120dafd9e497c7ff1d7d36659737a` 引入 `main@be9286fdc09beac8cf10ec21126472b0ce5bfbcb`；验证源码提交为 `1f8e4dd2c9c2e7b921c4aeb36e49ff5419f5ff6a`。测试在提交前的相同源码树运行，提交只归档经验证的代码与结果。

## 冲突与代码范围

- `HearthwardNaturalCamp.cpp` 同时保留 TASK-030 仓库箱体网格/碰撞和本分支的场景树绑定；伙伴木材点绑定到树时不再绘制方块标记。
- `README.md` 采用当前 main 的 TASK-030 玩法与资产说明，加入本次 AI/UI/攻击/跟随集成状态。交互组件保留等距稳定排序并接入 TASK-030 的 HarvestSubsystem；HUD 保留情境提示并接入小目标提示。
- TASK-030 的 `.uasset` / `.umap` 由 main 原样继承，没有手工二进制合并或修改 LFS 资产。

## 当前组合结果

| 检查 | 结果 | 证据 |
|---|---|---|
| UE 5.8.2 Editor Development | PASS | `Build.bat HearthwardEditor Win64 Development`，合并源码 19 actions |
| Python | 31/31 PASS | `python -X utf8 -m unittest discover -s scripts/tests -v` |
| 原生 `Hearthward.*` | 43/43 PASS | `Saved/Task029Integration/merge-native.log`，SHA256 `B5305E55B88EED62F0D3E5937E012A1737611F7A3AF36814B65A6AD195D3E68C` |
| TASK-030 正常地图 Demo | 70/70 PASS | [task030-demo.json](task030-demo.json)：实景采集、建造、四配方、床、篝火、存档与跨地图加载 |
| 跨页 UI / 焦点 / 确认 | 62/62 PASS | [ui.json](ui.json)；HUD API / PIE，非物理键盘 |
| 无控制台授物的自然路线 | 51/51 PASS | [natural-route.json](natural-route.json)：走到场景树采集、回营建工作台、走近工作台制作、仓储、地图/日志、存档 |
| 疾跑跟随 | 8/8 PASS | [follow.json](follow.json)：玩家 3576 cm，伙伴间距 193 cm |
| 攻击结算 | 10/10 PASS | [attack.json](attack.json)：空挥动画不磨损，开发敌人夹具中的真实命中扣耐久 |
| TASK-029 runtime smoke | 23/23 PASS | [runtime-smoke.json](runtime-smoke.json)，显式开发夹具 |
| 全仓 repository validator | 2 errors | canonical TASK-027 缺 reviewer 和 Issue URL，合并前同样存在 |

首次直接启动 TASK-030 脚本因未预建被忽略的 `Saved/DemoValidation` 目录而提前退出；目录就绪后重跑 70/70。原自然路线在新的箱体和工作台摆位下，先后因离工作台过远、箱体比工作台更近而拒绝制作入口。脚本修为实际走近工作台并验证有效工作台 ID 后，最终 51/51；没有放宽工作台距离或改变游戏逻辑。失败尝试日志保留在本地 `Saved/Task029Integration/`，没有计入 PASS。

## 合并与验收边界

以上是合并候选的自动化复验。真人键鼠、自然地图实体敌人/可见斧头模型、跳跃手臂动画、真实 Qwen 本轮耗时、Shipping 包及独立评审仍为 NOT_RUN。GitHub PR 与独立 Reviewer 按仓库工作流单列；合并后须在 main 的新 SHA 上记录集成结果，不能把当前分支 PASS 直接称为 main 已验证。
