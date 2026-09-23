# TASK-027 人物与动作来源

## 既有输入

本次从Hearthward远端main `414dc70481746875c8bd2b36d8b6140b475fcace` 取回已有Tripo人物资产。使用 `Content/Characters/Hero/Tripo/SK_Hero_Tripo.uasset`、同目录骨架、材质和纹理。没有重新调用云生成、购买或下载第三方动作。

这些资产由用户已有仓库提供；本任务没有新增关于源资产公开再分发资格的证明。原有源许可台账缺项继续保留，不能将接入或资产存在解释为许可审核通过。

## 派生动作

| 动作 | 来源与处理 |
|---|---|
| Idle | UE 5.8 安装包自带 ThirdPersonIdle，经原生 IK Retargeter 烘焙 |
| Walk | 同一模板的 ThirdPersonWalk，经原生 IK Retargeter 烘焙 |
| Sprint | 同一模板的 ThirdPersonRun，经原生 IK Retargeter 烘焙；用于冲刺 |
| JumpStart / Fall / Land | 本任务在既有骨架上本地编制关键帧 |
| Attack | 本任务本地编制挥击；未使用预览不合格的原slash片段 |
| Dig | 本任务本地编制蓄力、下压、回收循环 |

待机、行走、冲刺由 `scripts/characters/TASK-027/retarget_template.py` 制作；其余五段由 `author_motion.py` 制作。原始 Tripo 包保留。

模板来源为本机 `G:/UnrealEngine/UE_5.8/Templates/TemplateResources/High/Mannequin/Content`，是 UE 安装包所附的 UE4 Mannequin 第三人称动作，不是 Manny/Quinn 动作包。所需 25 个原始包及依赖保留于 `Content/Mannequin`；它们属于 Epic 提供的引擎内容，本任务未赋予额外独立分发许可。

使用 UE 原生 IK Rig/Retargeter 的链映射和自动姿态对齐，再烘焙为项目骨架的 30 fps AnimSequence。没有配置运行时脚部 IK、布料模拟或工具握持约束。参考 [Epic 双足角色重定向](https://dev.epicgames.com/documentation/unreal-engine/retargeting-bipeds-with-ik-rig-in-unreal-engine)。

## 骨架与单位

UE导入网格高99.802912 cm；实例统一缩放至180 cm，角色胶囊半高90 cm。四向预览中，鼻尖与鞋尖位于+X，Z向上；角色控制也以+X为前方，无附加转向补偿。

源动画根骨含约100倍缩放。原生重定向会去除比例，直接处理会压缩该骨架，因此使用单独的 `SK_Hero_RetargetProxy` 网格将局部位移换算为厘米、缩放归一，并将 +X 朝向旋转至模板的 +Y。输出时恢复原骨架单位和朝向。Tripo 大腿直接连接 Root，绕过骨盆链；烘焙时按参考脚踝高度修正最低支撑脚对应的根骨高度，使平地循环落在地面。这是离线高度校正，不是运行时脚部 IK，冲刺腾空的精修仍有限；原始网格和 Skeleton 不修改。

其余五段本地关键帧动作：地面动作的正向骨架计算包含父骨旋转和缩放，以网格参考姿态的最低脚踝高度校正根骨Z。它只校正整身高度，不做地形射线、双脚锁定或脚掌旋转IK。

骨名来自Tripo，不能按名字推断人体部位：`Head_0`位于一侧肩带，`bone_4`位于头颈链，`bone_17`/`bone_21`为大腿起点。制作依据实际层级和空间位置。

## 边界

不导入武器或采矿工具。挖掘片段接到现有资源交互作动作验证，目前开发夹具的产出仍为木材；本任务没有新增正式矿点、矿石收益或工具消耗。自然地图左键为动作预览；原战斗场景的攻击伤害与耐久结算沿用既有逻辑。
