# fix1 修复与定向验收

2026-09-29，在 `codex/ui-fix` 分支完成；当次验证时未提交或推送。测试工程为 UE 5.8.2 的 `Hearthward-ui-fix/Hearthward.uproject`，自然地图 `/Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds`，独立测试档池。

| 文档现象 | 修复与验证 |
| --- | --- |
| 房内低头时摄像机穿屋顶 | 两块屋顶补面只阻挡 `Camera` 通道，保留地面可见性追踪。修复前七个屋顶组件均为 `NoCollision`，低头时相机 Z 为 23120、屋顶中心 Z 为 22863；修复后相机 Z 为 22856，停在屋顶下方。 |
| HUD 黑框遮挡、文字难读且位置靠中 | HUD 的 `notice` 不再绘制大面积面板；文字按实际行宽绘制半透明弱黑底。战役提示、战斗提示、当前武器和操作说明移到边缘，空生存提示不创建元素。见[屋内截图](inside.png)。 |
| 猎弓图标和右键行为不符、远敌仍提示处决 | 右下角显示当前选中的武器；近战显示攻击／可用格挡，远程选中时才显示瞄准／射击。仅有有效偷袭目标时提示 F 致命暗杀和 R 非致命击晕。R 的动作标签与目标终态记为“击晕”，F 保持暗杀；两者依照已批准规则共用 3 秒动作、清敌、奖励和刷新结果。见[近敌提示](execution-cue.png)。 |
| 开幕夜景无法看清路线 | 保留低强度夜间太阳以保持暗色天空，另加不参与天空大气的 1.5 lux 冷色月光照亮地面。敌人的 `Region->Lighting=.12` 及感知计算未改。见[屋外夜景](night-house.png)。 |
| 石骨斧悬空 | 斧模型挂到角色 `hand_r` 骨骼，并关闭对导入骨骼异常缩放的继承。修复前斧中心距右手约 39 cm；修复后两者位置重合，世界缩放为 0.7。专用握持与击晕动画仍使用既有片段。 |

Development Editor 构建通过。`verify_pie.py` 在真实自然地图 PIE 中完成 12/12 项检查：新游戏、屋顶相机阻挡和低头相机位置、斧挂点／位置／缩放、夜间双光源、序章敌人、近敌提示，以及 R 击晕的启动、标签和清敌终态。原生自动化 `Hearthward.Combat.ExecutionActionsAndSnapshot` 为 1/1 成功，并覆盖 F 致命结果、R 击晕结果及后者的快照恢复。PIE 脚本从编辑器执行，结果和原始截图写入 `Saved/Fix1Reference/verify.json` 与同目录 PNG。

本轮未覆盖真人完整键鼠操作、不同画质与窗口比例、专用动作动画的美术验收。当前正在运行的旧编辑器进程不会自动载入新编译的模块，人工验收前需关闭后重新打开本分支工程。

实现参考了 UE 5.8 的 [Spring Arm 相机碰撞通道](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/USpringArmComponent)、[骨骼挂点](https://dev.epicgames.com/documentation/unreal-engine/skeletal-mesh-sockets-in-unreal-engine)和[定向光与天空大气交互](https://dev.epicgames.com/documentation/unreal-engine/directional-lights-in-unreal-engine)文档。

成果已归入 `Hearthward-ui-fix` 的游戏提交 `7eeecf52b4f2b1a72d7aa45d1f8d1e65e78a0486`。本报告的阶段检查早于 fix2；组合源码的当前验证及限制见 [fix2报告](../fix2/REPORT.md)。
