# TASK-055 石骨斧重击实际 Native RED → GREEN

2026-10-05，Root 独占 UEClient；UE 5.8.2 / Windows 11 / D3D12 / RTX 4060 Laptop。HEAD `67fb0784ca8c6d488173e587e7f95c4be0d9092a` 加当前未提交批准施工，全部报告绑定该工作树的对应补丁阶段，未提交推送。

## 原始错误与修复

先只加入 `StoneAxeHeavyUsesCurrentMoveAndPhysicalBlade`，实际 render Native RED 有 7 个业务错误：重击 `.65s` 准备边界 hand 与 `.6L` 源姿态差 18.429017cm/0.494468rad；`.70s` 尚未接触物理刃时已扣目标至 45.400005HP、当前斧磨损2；刃够不到的 150cm 真实 box 被旧200cm扇形命中至48HP并磨损2。其余恢复错误是同一错误命中的后续断言。重击测试零警告；同行 Crafting 的1警告独立记录。

最小生产改动仅四个现有文件：原 phase 公式接收本次捕获 FMove，石斧轻/重共用已测旧 Attack 的 `.6L–.8L` 前挥，PlayCombat 立即复制本次 Move，native graph 和历史刃扫使用各自实际 `.35/.18/.47` 或 `.65/.20/.65` 时钟。移除重击的真实刃排除，保留既有 sockets、历史姿态、低帧子步与墙阻挡。费用、倍率、strong 技能、伤害来源、epoch、GUID 与当前实例两点一次磨损均沿原结算链，没有新片段、握姿或资源。

## 验证

Editor Development 实际编译 PASS。相同 render Native 命令仅追加现有轻击项：

```text
run_native.py --filter Hearthward.Crafting057.WarehouseMaterialsPresentation+Hearthward.Equipment055.StoneAxeHeavyUsesCurrentMoveAndPhysicalBlade+Hearthward.Equipment055.StoneAxeLightUsesClipPhaseAndPhysicalBlade --label crop-target-heavy055-green --render
```

三项全部 PASS、零错误。重击和轻击各零警告；Crafting 仍有一个既有 EnhancedInput standalone fixture 警告。重击三例覆盖 30fps/实际公开 Learn(strong)、1fps 跨有效窗、真实旧 reach 阻挡而实体刃不可达的负对照，并检查准备/早期零伤害、重击倍率、当前/备用 GUID、一次耐力和正常恢复。

原始 `stone-axe-heavy-native-red.json` / `stone-axe-heavy-native-green.json` 保留2项RED、3项GREEN整体结果，各准确 fullTestPath 与 errors 可分离。完整报告在 `Saved/Task053/crop-target-heavy055-red/index.json` 和 `crop-target-heavy055-green/index.json`。正式持握、独立重击美术质量和普通键鼠战斗仍未验收；已拒绝的 Grip 候选保持未绑定，新 motion 许可和 Owner 观感仍未验。
