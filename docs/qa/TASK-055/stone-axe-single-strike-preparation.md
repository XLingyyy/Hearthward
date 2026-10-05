# TASK-055 石斧实际单次挥击证据准备

2026-10-04。仅独立 QA 脚本与技术调查；未 UE/build/Git/Content/生产修改。根独占执行、资产登记与正式评审。

## 当前证据与代码

- 现有 `Saved/Task055/motion-hero/results.json` 只覆盖 slash/hit_to_side/fall/swim 四候选真实 Hero 单节点姿态变化与截图；未包含石斧、Chop、当前Attack的刃接触时点。既有 held-axe-review.md 的 Native GREEN覆盖装备GUID／破损／ranged隐藏，没有握点或连续挥击验收。
- 根现 Character.cpp95–103 的石斧是真实 `/Game/Hearthward/Assets/TASK-028/props/stone_bone_axe/SM_stone_bone_axe`，hand_r挂接，roll -90、absolute Scale .7、NoCollision。Hero mesh是真实 `/Game/Characters/Hero/UE5/SK_Hero`，component Scale 180/97.869893、relative Z-90/yaw-90。RefreshHeldTool146–152已按本人weapon GUID／耐久／ranged控制可见，较旧 formal-motion-binding-audit 的早期观察已修正。
- 本次使用正式当前 `/Game/Characters/Hero/AnimationV2/A_Hero_Attack` 和其已有 `/Game/Characters/Hero/AnimationV2/A_Hero_Chop`，不借用新武器或新055候选同名动画，不改任何 uasset。
- 031 `motion.json` 和 `inplace-fix/adaptation.json` 均记实际Hero裁窗 `[1.3333333655248714,2.2500000543232206]`。finalize_motion.py61–67按前半段hand_r最高点，从peak前0.5s取起点、随后hand_r最低点取终点；此算法的peak推算约1.833333s，位于裁窗6/11处。脚本publish用24fps和round(.9*24)帧；应读取实际AnimSequence.get_play_length，不能硬编码资产长度恰好.9。
- 当前 HeroAnimInstance::PlayCombat147–150设置 CombatRate=实际AttackLength/Duration；Proxy PreUpdate74–79设置同一整段rate并reset0，图层进/出混合0.12s。石斧轻击规则来自CombatRules blunt light `.35/.18/.47`，有效段`.35–.53`，总1s。因此高度峰值的旧算法投影约action .54545s，只说明高度峰值晚于有效段终点，未证明刃接触已发生或缺失。
- Combat::Sweep222–253仍从ActorLocation+Z10按200cm reach、±55°扇形扫sphere5，按有效窗和.015子步、HitIds、ChargedWear结算；没有实际axe端点。替换计划须保留其ActionId/epoch/GUID、墙阻挡和一次结算。

## 公开实际采样接口

本机 SkeletalMeshComponent.cpp4365–4376 的 SetPosition仅调用SingleNodeInstance.SetPosition。AnimSingleNodeInstance.cpp359–394只写time并处理可选notifies，没有动画tick/骨刷新。因此不能在SetPosition后立即把get_socket_transform当新姿态。

草稿采用公开 `set_update_animation_in_editor(True)` + registered component正常editor ticks；SingleNode play_rate0、set_position(t,False) 保持指定时间。对每次采样先用公开 `AnimPoseExtensions.get_anim_pose_at_time(clip,t,options)`，设置 COMPRESSED、optional_skeletal_mesh实际Hero、should_retarget=true、extract_root_motion=false；`AnimPoseSpaces.WORLD` 在此API明确指component space。把实际Socket RTS_COMPONENT的hand_r/head/pelvis位置、rotation quat与scale和独立压缩pose对比，等待符合再采世界变换，并读回get_position。若editor实际没有更新或pose不符，写当前时间和误差并停止，不用旧骨数据冒充。

接口源级凭据：SkeletalMeshComponent.h1302 SetPosition UFUNCTION，1472 SetUpdateAnimationInEditor UFUNCTION；AnimPose.h170/192/287 的IsValid/GetBonePose/GetAnimPoseAtTime反射；SceneComponent.h673的K2_GetComponentToWorld ScriptName GetWorldTransform公开world transform。没有调用非反射 TickAnimation/RefreshBoneTransforms，没有新增Runtime入口或引擎扩展。

参考：[SkeletalMeshComponent public Python](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/SkeletalMeshComponent)、[AnimPoseExtensions public Python](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/AnimPoseExtensions?application_version=5.2)、[Get Anim Pose at Time 5.8](https://dev.epicgames.com/documentation/en-us/unreal-engine/BlueprintAPI/Animation/Pose/GetAnimPoseatTime)、[AnimPoseSpaces](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/AnimPoseSpaces)。当前5.8本机UFUNCTION及实现确认方法可用，脚本尚需根真实执行验证。

## 根执行稿及输出

`sample_stone_axe_hero_pose.py`：288行，AST通过。根用既有公开 `UEClient.runtime.launch_editor` 的 `-ExecutePythonScript=<absolute script>`，真实RHI；只在fresh独立editor执行。NewBlankMap(False)丢弃当前scene，因此勿附着人工未保存编辑现场。脚本不PIE、不正常装备或攻击；仅临时把production ctor的HeldAxe可见并切SingleNode，以隔离资产姿态及挂接问题。

采样当前Attack按60个均匀action子步，加精确`.35/.53/6÷11/.70/1`；原Chop按自身24fps全段，另含旧裁窗起点、峰值投影及终点。记录两片段真实长度、notify触发时间/持续时长、压缩pose及实际component/world骨TF、actualHeldAxe world/relative TF、Hero mesh TF和实际position、待核对误差。通知仅读取，sampling关闭notify dispatch。

真实照明复用现031/070 SceneCapture2D→1024² RTF_RGBA8→下一Slate tick export，Directional key/fill+既有HDR SkyLight、fixed manual exposure；当前mesh/material不替换。输出21张：石斧identity三轴3张；Attack六时点全身/握点近景12张；Chop三时点全身/握点近景6张，实际合计21张，以脚本images字段为准。每张摄像机/目标/orthographic width有实际记录，不代表正常输入验收。脚本将 `sampling_complete` 与图片 `review=pending` 分开。

输出 `Saved/Task055/stone-axe-single-node/pose-and-axe-facts.json`、PNG及 `stone-axe-ue-local-vertices.json`。MeshDescription LOD0 actual UE local所有点从公开GetStaticMeshDescription/GetVertexPosition读出，逐个确认稠密有效ID，不套FBXraw尺度。石斧原预览已实际查看：长柄、石刃位于头部一侧；此预览未提供UElocal数值，无法单凭截图准确指定Grip/BladeBase/BladeTip。

首次运行不提供猜测的semantic点。根查看真实三轴和全顶点后，选择柄握持中心、刃根、刃端，提供QA文件 `docs/qa/TASK-055/stone-axe-measured-endpoints.json`，要求mesh_path为上述准确package、coordinate_space=`UE mesh local cm`、selection_evidence和三个候选向量。该文件不是资产配置。已有时脚本使用公开MathLibrary.transform_location(实际axeTF,local)记录世界候选；不存在时明确not selected。已存全部actual axe quaternion/scale/translation，选点后可直接离线计算各时刻真实世界轨迹，避免重复跑相同姿态。

## 依据结果再定生产窗口

首轮必须核对根真实图片、pose误差、实际axe长度、局部柄/刃方向及world握点误差；不把hand/axe origin恰好重合当柄握点正确。手骨100倍单位和Hero约1.84scale会影响relative translation；现absolute weaponScale只隔离尺度继承，校准需用实际父socket世界变换求inverse。

有真实semantic端点后，分别绘制BladeBase/BladeTip随时间的三维轨迹和速度、下降段及actor forward方向变化，按实际石刃穿过具体目标表面的区间提出接触窗。空中高度峰值、最大速度、手骨最低点分别是几何指标，均不能单独当命中时点。可以对实物目标/固定QA目标平面作几何相交调查，但要记录目标位置、范围、方向及真实刃边；不把有利拟合的平面称正常敌人碰撞验收。

源窗与接触区间确认后再给三段SourceTime映射：准备、有效、恢复分别映射到唯一FMove的Windup/Active/Recovery，保持既有时长、伤害/耐力/耐久值域；PlayCombat/Proxy只消费该动作真实时钟，取消/旧epoch关闭。当前整段rate/SequencePlayer需要局部变化，暂未写patch。真实Sweep需采源时间对应刃根/刃端和owner变换，沿现.015跨有效窗样本/目标去重/墙阻挡/ChargedWear；不能仅在同一当前pose重复扫actor扇形冒称真实低帧轨迹。

预期精准Source候选仍是HeroAnimInstance.cpp/.h、CombatComponent.cpp/.h及Character必要握点校准，但根须先根据QA决定实际最小窗口。当前阶段不生产、不Socket author、不改新武器、不扩其他动作。最窄回归应先真实刃段+可控目标RED（现窗口实际错位、窗外零伤害、低帧跨窗/墙/取消及正确GUID一次耐久），已有CombatTests的事务/旧epoch样本继续复用，避免重复测试。

## 2026-10-04 当前冻结稿

Root实际070 SceneCapture确认10000/2000 lux在EV10参数写入后仍纯白，30/10 lux+Sky1已取得可判断图片。055完整采样稿现固定30/10 lux+Sky1，同Manual physical=true f4/64/ISO100/bias0并读回实际值，IN_TICK再入保护保留；300行AST通过，未执行UE。旧fixed-exposure-ev10.patch为历史候选，执行以当前完整sample_stone_axe_hero_pose.py为准。EV10真实作用未验证，暂不继续追曝光原因，优先取得真实geometry/pose/PNG。完整源级事实与限制见../TASK-070/fixed-exposure-evidence.md。
