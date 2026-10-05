# TASK-055 装备来源、持握与动作接入调查

2026-10-04。来源和当前代码只读调查。未导入武器／护甲、改写 Mesh、启动 UE 或实现下述方案；仅写本 QA 记录及 equipment-source-geometry.json。所有候选仍须根登记准确源码／内容路径和 LFS 锁，完成实际渲染及操作验收。

## 可复用资产与直接来源

已经导入且当前角色代码使用的持握模型为 `/Game/Hearthward/Assets/TASK-028/props/stone_bone_axe/SM_stone_bone_axe`；实际包为 `Content/Hearthward/Assets/TASK-028/props/stone_bone_axe/SM_stone_bone_axe.uasset`，随包已有 Color／Normal／Roughness 及 tripo_mat_5ce74ca3 材质。

其他下列候选位于现有 Tripo 来源目录。当前集成树多数为 LFS 指针，可直接只读使用 task056 的完整 LFS 镜像；十项均未在当前 Content 中找到对应 UUID 或语义模型。逐张读取各自实际 rendered.webp 后得到下列外形分类，没有将文件夹名等同于可穿戴功能。

候选实际文件路径统一为 `G:/GameFactory/Hearthward/.agent-local/task056/art_source/TASK-004/Tripo/<分类>/outputs/<ID>/<ID>_pbr.fbx`；其余贴图位于同目录 `<ID>_pbr.fbm/`，预览是 `<ID>_rendered.webp`。根集成来源相对路径为相同的 `art_source/TASK-004/Tripo/...`，无须另造来源。

| 分类 | ID | 实际预览外形 | 后续用途／限制 |
|---|---|---|---|
| 武器 | 29322e3e-a4e3-4d17-9223-80d1f2d65f99 | 木柄石枪 | spear 候选，先核对实际柄端／刃端／握点 |
| 武器 | 84cf4884-8d18-4737-a4f5-1b581e7eec13 | 木弓 | bow 候选；当前静态弓弦无法证明蓄力变形 |
| 武器 | 8dd7cafa-fdd7-40cd-9724-a0fa113e72e9 | 单手刀 | 刀候选，和下一项外形很近；未验长度／握点 |
| 武器 | b39bb9c5-8ee6-484e-8ab1-e6c1ff5046e6 | 单手刀 | 刀候选；不能未经尺寸／动作验证标作双手长刀 |
| 武器 | c31fb95d-d93d-467b-bfa6-5e1e9a4bb2e3 | 钉木棒 | blunt 参考候选；现行 axe 已有石斧，不新增武器类型 |
| 武器 | c49a60bf-f3bf-4ee9-a854-145882c4ebdd | 木弩 | crossbow 候选，仍需实际装填／放箭动作 |
| 护具 | a42d2caa-03d0-4d3e-b57b-88ae746453e2 | 皮兜帽 | hood 候选，须做当前头颈形体适配 |
| 护具 | 76f88975-3cc8-410c-9842-2ddd8765164b | 皮胸甲 | chest 候选，须变形／体型适配 |
| 护具 | 64c15994-0225-4d07-96b4-7402c74acb94 | 一双皮靴 | feet 候选，原文件一整个 Geometry；需分左右并拟合 |
| 护具 | afdffd2b-08be-462f-880b-e0c83454b953 | 一对护臂 | gloves 装饰候选；不能代替腿甲或给 legs 防护 |

直接读取十个 Binary FBX 的 Geometry／Deformer／Vertices／PolygonVertexIndex：每个只有一个 Geometry，Deformer 为零；约 45,000—51,000 控制点／多边形。详见 equipment-source-geometry.json。这里计的是 FBX polygon，不冒称 UE 三角形数。它们均没有现成蒙皮，兜帽／胸甲／双靴不能仅挂到一个骨便作为完整四部位动态穿戴通过。模型尺寸、轴向、pivot、LOD、碰撞均待实际导入检查。

当前来源目录只找到以上六武器、四护具，以及已有石骨斧／鱼竿等 props。没有查到可直接复用的独立盾或腿甲来源。F:/Download 另有用户已有剑／石斧／铁斧／铁锤 ZIP，但尚未核对其模型内容／来源与任务范围，不把它们记作正式已接入。TASK-004 SOURCE.md 仍保留输入图片权利待 Owner 确认的边界，最终许可归074。

## 当前持握与动画事实

`HearthwardCharacter.cpp` 创建 HeldAxe 并挂到实际 hand_r，旋转 (0,0,-90)，设置绝对 Scale 后缩放 .7，无碰撞，初始隐藏。RefreshHeldTool 只判断字面 weapon=axe 和类型数量；尚不覆盖其他武器、ranged 选择、offhand、实际 GUID／耐久。Brother 没有相应持握组件。TASK-028 manifest 仍将最终手部方向／socket 检查留给后续任务，未给出经过验收的全部武器握点表。

玩家 CombatComponent::Sweep 以角色中心、固定 reach、±55° 扇形扫掠；它按现行 Windup／Active 和 .015 秒子步跨窗口采样，却未读取 HeldAxe／真实武器端点。兄弟战术与狩猎分支播动作后直接 DamageOpponent；敌方正式近战也直接调用身体 Damage。上述表现不能作为真实武器接触已经完成。

当前 Hero A_Hero_Attack 从 TASK-031 Chop 源窗裁切，PlayCombat 将整段按动作总时长缩放。Brother 六状态里的 Attack 同样沿旧动作；候选055 slash／hit_to_side／fall／swim 已导入且做静态阶段QA，尚未接正式图或验证完整语义窗口。Slash 整段约6.58秒，不能整段压成一次短击后宣布动作通过。现行 FMove 的准备／有效／恢复时间、费用和倍率仍是唯一规则，不能由动画 Notify 另算伤害。

## 建议的最小施工顺序（尚未实施）

1. 根先完成真实 PhysicsAsset 与敌箭四部位测试。只以实际 BoneName 判断身体部位；源权重／单位尺度详见 body-weight-scale-audit.md。
2. 先闭合已有石斧单条近战路径：确认实际模型的握点、刃根和刃端及连续 Chop／Slash 手轨迹；保持现在绝对尺度约束，避免手部骨的100倍单位 Scale 把武器放大。根登记现有石斧包及必要 character／animation／combat 精确范围后再修改。
3. 持握显示以本人的 Inventory 装备槽和 GUID 为依据；武器磨损、转交、装备切换、读档清理沿现接口。为后续左右手模型各登记准确绑定；缺盾／腿甲的内容需要制作或补齐真实来源并验收，不能用护臂重命名充数。
4. 准备、有效、恢复段按现行 FMove 对齐真实动画源帧窗。碰撞只消费 ActionId／epoch 的有效窗口；真正武器握点／刃端通过骨变换与模型 socket 得到。低帧跨窗口需要真实动画时间采样或对应的实际端点扫掠；仅在同一帧重复扫角色扇形无法证明轨迹。
5. 对石斧单条路径做最小回归：窗口外无伤害、实际刃段接触一次、墙阻挡、同一 ActionId 不重复结算、取消关闭旧窗口；连续实际渲染检查手柄贴手、动作与接触时点。原有1/30/120fps中心扫掠测试只能保护旧窗口结算，不能单独证明新真实武器轨迹。
6. 石斧闭合后再接刀／枪／弓／弩及盾，各自需要经过尺寸、握点、源窗和实际玩法验证。Static弓弦、装填状态、箭／弩箭外形和专用动作缺口保持明确；不默认将共享Chop播给全部动作。材质、各阶段外形和LOD最终统一归070，来源许可归074。
