# 石斧轻击 Native RED 与手掌实测

2026-10-04，实际构建通过；`Saved/Task053/budget-recovery-green055-blade-red/index.json`。

`StoneAxeLightUsesClipPhaseAndPhysicalBlade` 四个正常本人 GUID Equip→Attack 子场景，七处实际错误：

- logical .35 对比实际 source .6L=.55，hand 世界位置偏差 31.044213cm、rotation angular distance 1.041503rad。
- logical .40 刃尚未触及固定真实 Query body，当前 Health 已从100变70、当前斧耐久80变79。
- 一次1秒跨窗，同时 Actor 从原地/yaw -45 移到 Y+120/yaw+45：固定窄 body 预期受30伤害，实际100且当前斧未磨损；恢复阶段同一缺失结果产生重复断言。
- 该运行未应用 phase/真实 blade 生产修复。静止窄 body 对照和其他未列错误的断言通过。

原 `HeldAxeFollowsCurrentInstanceAndMode` 的显式 `-Task055PalmSkinCapture` opt-in 在真实 RHI 下 Success：公开 GetCPUSkinnedVertices，实际 LOD0 13409 顶点、所选掌/指169顶点、hand_r及15指骨共16骨、205个完全在选区内的三角形；morph target count=0。实际 JSON 为 `Saved/Task055/stone-axe-palm-skin/actual-hand-skin.json`，保留原LOD0 ID、实际权重/section BoneMap与世界坐标。掌心位置及握持贴合尚未据此验收。

## 实际Runtime GREEN

`revision071-blade055-green-no-once068-red` 中StoneAxeLightUsesClipPhaseAndPhysicalBlade Success、0 Error/Warning；五子场景含30fps phase/接触前零伤害、1fps完整跨窗、前后Mesh世界变换移动命中、静止窄body零命中和真实薄墙blocking正控。当前GUID仅实际接触磨损一次、备用不变，恢复无额外伤害；同运行Combat及另两Equipment测试通过。

生产仅石斧轻击按既有.35/.18/.47时长映actual clip .6→.8有效段；读取实际BladeBase/BladeTip socket和当前relativeTF，按上一/当前真实Mesh world TF插值采样GetAnimationPose；现原落地/坠落/死亡业务仅一份。普通输入影片、掌握、真人体验仍未覆盖；本次保留原握持位置/方向/尺度。

无保存palm候选真实渲染在单节点source0初始化后，source.55和压缩pose匹配（位置误差<.00004cm）。四PNG完成、根逐张检查：minimum-align改善方向，但实际手皮肤中心轴交叉仍存在，不能验收无穿掌。首非零采样初始化失败的原记录另存stone-axe-palm-candidate-first-position-red。
