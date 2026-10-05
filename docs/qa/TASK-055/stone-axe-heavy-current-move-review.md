# TASK-055 石骨斧重击：有界 Native RED 与生产提案

2026-10-05。基于 Root `task051` 实际 Source、已授权石骨斧与旧 Hero Attack，以及既有真实 clip/socket 采样。只写自身 QA 提案，未修改 Root/Source/Content，未生成握姿/mesh，未 UE/build/Git。C++ 未编译，Native 尚未运行，当前不能登记为真实 RED/GREEN。

## 已确认行为与动画资源窗口

`CombatComponent.cpp99` 的 `!Heavy` 排除重击的分段动画，`cpp244` 的 `!HeavyAttack` 排除重击的真实刃扫。重击仍使用 Actor+Z10、reach200、±55°的扇形碰撞；旧 Attack 按总时长均匀播放。

当前唯一规则 `Move(blunt,true)` 是准备 `.65` /有效 `.20` /恢复 `.65`、总 `1.50s`、成本30、倍率2.6。其有效段 `.65–.85s` 被均匀映到旧 Attack 源 `[.3972222308,.5194444557]s`。既有实际63姿态采样中，该区间11个刃样本的最大X为 `53.036992cm`，距真实测试box的X80前表面明显不足（扫刃半径5cm）。这是静态时序与实际采样的组合证据，不冒称已执行重击。

当前轻击已实际使用同一个旧片段的 normalized `[.6,.8]`，即源 `[.5500000119,.7333333492]s`，包含已测切缘前挥/下降与目标面穿过。若按重击本次 FMove 分段映射，这四个既有真实目标面交点落在逻辑 `.766667/.783333/.800000/.816667s`，处于重击自己的有效窗。无需引入新 motion 或修改动画资产就能验证这个工程候选；直接移除两个排除条件仍会使用 Light helper 的错误时钟，因此不足。

唯一源资源仍为 `/Game/Characters/Hero/AnimationV2/A_Hero_Attack`，模型为已有 `SM_stone_bone_axe` 的实际 BladeBase/BladeTip socket。该方案复用一次下劈路径并改变准备/有效/恢复播放速度；尚不证明独立重击动作质量、正式掌握、blend、正常输入或所有目标通过。不绑定已拒绝 Grip 候选，不继续握姿扫描。

`strong` 当前只提供 `heavy_damage` 的5/10/15%被动效果，没有 `heavy_speed` 或重击技能解锁门；已有 Combat 原生也验证无需 strong 解锁。实际伤害依旧为捕获 ActionPower × heavy/light Move倍率比 ×当前 heavy_damage，费用依旧通过 SpendStamina 应用装备承重与 cost，接触后磨损当前 GUID 两点一次。受伤来源、ActionId/epoch、实例校验、去重和原 CommitOpponentHealth 调用均不变。

## 未应用的 Native RED 提案

`stone-axe-heavy-native-red-proposal.patch` 仅新增既有 `EquipmentPresentationTests.cpp` 的一项：

`Hearthward.Equipment055.StoneAxeHeavyUsesCurrentMoveAndPhysicalBlade`

复用该文件真实 Hero/AnimInstance、生产 EquipInstance/HeavyAttack、真实 Clock、QueryOnly Visibility box 与实际斧 socket；没有直接设置完成/伤害/耐久。三例限定为：

1. 30fps近目标：在生成目标前通过现有公开 Learn 正常消费初始预算学习 strong，伤害数值从实际 effect/power读取；在重击Windup边界比较真实 native hand pose与源`.6L`，准备及早期active `.70s`必须零伤害/磨损，之后真实刃命中按现有重击倍率和技能结算。当前均匀图与扇形预计会使pose/早期伤害断言失败，须Root实际运行确认。
2. 1fps近目标：一次低帧跨重击active，仍需正确一次伤害/耐久，不缩成轻击时序。
3. 30fps远负对照：box `(150,0,100)`/extent3真实阻挡旧reach穿过中心；现已测实体刃最大X约87cm，不能靠200cm扇形打到它。正常重击成本仍支付，HP/武器耐久必须保持。当前旧扇形预计会错误命中，须实际运行确认。

各例都有当前/备用两个真实斧 GUID，断言正常1.5s完成、恢复不重复伤害/磨损/耐力支付。没有直接设置Skills/Experience；Learn使用实际初始Level1的2点预算，调用不计为桌面技能UI验收。未复制既有epoch/取消/事务测试，未添加更多候选或无关装备检查。

## 未应用的最小生产提案

`stone-axe-heavy-production-proposal.patch` 只涉及四个现有文件，基线真实行尾均LF：

- `Combat/HearthwardCombatRules.h`：同一现有phase公式接收本次捕获的 `const FMove&`，取消固定Light Move；`.6/.8`源分段保持。
- `Combat/HearthwardCombatComponent.cpp`：石斧轻/重均启用真实刃链，将现捕获Move传动画；历史姿态采样也传同一Move。保持当前 sockets、实际relative/world变换插值和低帧子步/墙阻挡代码。
- `Animation/HearthwardHeroAnimInstance.h/.cpp`：石斧标记改为覆盖轻/重，PlayCombat接收可选Move指针并立即复制；图消费该副本与Combat Elapsed。执行/其他攻击调用保持默认无指针，PlayAttack/StopCombat、死亡/落地/坠落优先逻辑保留，没有新公共getter/runtime类/模块/依赖。

指针只用于调用时复制私有Combat已捕获的值，不长期存储。未新增第二张时间或伤害表，没有用Duration猜轻重，没有改FMove/skills/gameplay参数；Light副本就是原Move，原轻击phase计算保持同值。

Root应先审阅/登记并只合Native补丁、实际取得RED，再审阅/合入生产补丁。最窄GREEN范围为新增重击项＋现有 `StoneAxeLightUsesClipPhaseAndPhysicalBlade`，检查共享phase没有破坏轻击。正常重击与持握视觉继续单独验收。若真实Native显示该旧clip窗口不适用于重击，保留失败证据并收回此映射，不用只开bool或换未授权动画绕过。

生成入口 `prepare_stone_axe_heavy_patches.py` 使用实际Root文件和精确单窗替换生成未应用diff；当前已执行生成成功。参考既有 `stone-axe-endpoint-trajectories.json`、`stone-axe-geometry-phase-candidate.md`、当前 `CombatRules.h19`、`CombatComponent.cpp86/239/412`、`HeroAnimInstance.cpp65/116/169`；采样接口主源见既有 [Epic GetAnimationPose](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UAnimSequenceBase/GetAnimationPose?lang=en-US)。
