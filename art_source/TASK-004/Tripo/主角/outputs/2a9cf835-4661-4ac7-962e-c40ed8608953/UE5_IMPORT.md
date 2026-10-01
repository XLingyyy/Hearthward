# UE5 导入与角色使用

## 文件

- 骨骼模型：`2a9cf835-4661-4ac7-962e-c40ed8608953_model.fbx`
- 动画合集：`../4599d259-9a84-4382-b77f-63fe7f05c20d/hero_animations_idle_walk_run_slash_hurt.fbx`
- 动画：`idle`、`walk`、`run`、`slash`、`hurt`，均请求为原地动画。

## 1. 导入骨骼模型

将骨骼模型拖入 UE 内容浏览器，建议放到 `/Game/Hearthward/Characters/Hero/`：

- Skeletal Mesh：开启
- Import Mesh：开启
- Skeleton：首次导入选择 None，让 UE 创建新 Skeleton
- Import Animations：关闭
- Import Morph Targets：按需，首轮关闭
- Normal Import Method：Import Normals and Tangents
- Convert Scene Unit：开启
- Create Physics Asset：开启
- Material Import Method：Create New Materials（后续整理为项目材质实例）

导入后检查参考姿势、根骨缩放、脚底高度、左右手朝向、披风/刀鞘权重和材质槽。

## 2. 导入动画合集

将动画合集 FBX 拖入同一目录：

- Skeletal Mesh：关闭
- Import Mesh：关闭
- Skeleton：选择上一步创建的 Skeleton
- Import Animations：开启
- Animation Length：Exported Time
- Use Default Sample Rate：开启
- Import Custom Attribute：开启

文件中包含 idle、walk、run、slash、hurt 动画栈。若 UE 导入器只选中一个 Take，可对同一 FBX 重复导入并分别选择对应 Take，或先在 Blender/Maya 中拆分动画栈。

## 3. 移动

- 创建 Animation Blueprint，Skeleton 选择本角色骨架。
- 建立 `Speed = VectorLengthXY(Velocity)`。
- 用 1D Blend Space 组织 idle、walk、run。
- Character Blueprint 使用 `CharacterMovementComponent` 驱动位移；动画为原地版本，不依赖 Root Motion。

## 4. 战斗

- 用 slash 创建攻击 Animation Montage。
- 在有效伤害帧添加 Anim Notify，用于开启/关闭武器碰撞或执行命中检测。
- 用 hurt 创建受击 Montage，并设置高于移动状态机的 Slot。
- 刀具目前属于生成模型外观的一部分；若要真正拔刀和命中，需将武器拆成独立 Static/Skeletal Mesh，再挂到手部 Socket。

## 5. UE5 Manny/Quinn 动画复用

当前为 Tripo/Mixamo 兼容人形骨架，并不是原生 Manny/Quinn 骨架。若需要复用 UE5 动画：

1. 为本角色创建 IK Rig，设置 pelvis 为 Retarget Root。
2. 建立 spine、neck、head、左右 arm、左右 leg 等 Retarget Chains。
3. 使用 Manny 的 IK Rig 创建 IK Retargeter。
4. 对齐 A/T Pose 后批量导出重定向动画。

## 已知风险

- 披风、长袍、链甲、刀鞘与腰带可能在大幅跑动/攻击动作中发生穿插。
- 自动权重需要在肩、髋、肘、膝和披风根部人工检查。
- 物理资产需要删除多余碰撞体，并为披风另建 Chaos Cloth 或简化骨骼方案。
- 自动生成的 PBR 材质可能需要在 UE 中重新打包 ORM，并校正法线贴图类型和纹理色彩空间。
