# 石斧真实几何、握点和轻击phase候选

2026-10-04，只读分析Root实际 `Saved/Task055/stone-axe-single-node` 结果。本人仅写QA候选与离线计算，未运行UE、构建、Git，未修改Source/Content。候选须Root审查和后续真实渲染/碰撞验证，未作正式验收。

## 真实证据范围

`pose-and-axe-facts.json` captured_utc=2026-10-03T23:58:31.544769+00:00，sampling_complete=true、error未出现；Root确认closed=true。当前Attack实际长度0.9166666865秒、63个姿态；原Chop长6.5833334923秒、160个姿态。两片段均notify_events=[]。最大压缩pose与真实socket位置误差：hand_r=0.00005971cm、head=0.00003419cm、pelvis=0.00001383cm。

三轴、当前Attack六时点full/grip、原Chop三时点full/grip共21张唯一PNG已查看。facts有23条image记录，Chop 1.833333秒的full/grip各重复记录一次，同名文件仍各一张。没有裁剪、修改或重生成这些图片。

真实构造读回：hand_r父socket；held relative translation=0、quaternion=(.7071067812,0,0,.7071067812)、world/absolute scale=.7，NoCollision。Hero mesh scale1.8391764；hand_r父世界scale约183.9176。现动作采用single-node，没有测NativeGraph的0.12秒blend、正常输入/事务、转向或真实敌人碰撞。

## 三轴和semantic选点

实际LOD0有47,631个local vertex、95,278 triangle。坐标包络min=(-34.696587,-25.124842,-49.953133)，max=(37.546192,26.548822,49.868290)cm。柄由低Z/正X/负Y的尾部指向高Z/负X头部，石刃沿+Y伸出。local-x图清晰分出石质外切缘、头部绑带和下柄握持包裹；local-y显示石刃较薄和曲柄；local-z确认切缘侧向，未套用raw FBX轴向/单位。

`stone-axe-measured-endpoints.json` 的三个具体候选为：

| Point | UE mesh local cm | 选点证据 |
| --- | --- | --- |
| Grip | (22.647987604,-17.993159771,-27.5) | 下柄皮革包裹中段。实际Z[-28,-27]截面476顶点；取X/Y的5%/95%包络中点，内部轴心候选。 |
| BladeBase | (-7.649685860,14.690569878,10.305906296) | 真vertex41603，石刃+Y外缘下端，处于石刃与较窄头部连接的切缘尾端。 |
| BladeTip | (-26.873825073,23.822595596,39.805477142) | 真vertex26958，石刃+Y外缘上端；未选木柄顶端、角/骨饰或皮革绑带。 |

真实三轴camera/ortho参数精确投影核对：Grip在local-x/y/z为(641.96,702.77)/(659.45,702.77)/(382.04,364.55)px；BladeBase为(414.88,440.10)/(448.95,440.10)/(609.12,575.05)px；BladeTip为(351.43,235.14)/(315.39,235.14)/(672.57,708.61)px。候选JSON保留完整精度、vertex ID及图像投影证据。Grip是内部轴心，未冒称表面顶点。

两刃端在.7scale下构成25.46294cm弦。对Z10.3至39.8的+Y外缘每.5cm取真实顶点，实际曲刃距弦最大4.21127cm（世界尺度）。这支持先用现5cm半径覆盖该候选切缘弦；它没有证明其他轮廓、扫掠连续性或所有墙体通过。

## 当前握点的确证缺口及最小候选

actual held origin到hand_r为0，但所选包裹Grip到hand_r为27.938114cm。三轴图原点位于未包裹中柄段；当前Attack 0/.320833/.485833等近景可见手落在该中段，原Chop对应时点相同。末帧斧头被身体遮挡/穿入身侧，当前持握尚未闭合。

`hand_r`是骨socket pivot，尚未采手指/掌心中心，不能把Grip重居中到hand_r后的数学零误差当完整掌握姿态PASS。第一步最窄操作是仅在Root无保存QA fixture保留现relative quaternion与.7absolute scale，把所选Grip放到实际hand_r（或Root根据真实近景确认的palm offset），重拍同三时点。方向、掌心偏移及衣袖穿插由实际图片审查，不凭离线投影调整任意欧拉角。

若只把Grip放到hand_r，给定每个实际axe世界TF，候选世界点为 T+QRotate(.7×(P-Grip))。此为已知TF的精确几何投影，未模拟真实新component状态。第一帧parent scale下所需relative translation为(-.0861994292,-.1046664338,.0684828849)cm；公式为 -Qrelative.Rotate(.7×Grip)/HandSocketWorldScale。父骨100倍与Hero1.839倍使relative数值小，不能把localGrip直接赋component RelativeLocation。固定relative值在真实UE重新读回Grip世界误差后才能写生产；不改骨缩放或mesh尺寸。

## .35–.53与真实切缘轨迹

`stone-axe-endpoint-trajectories.json` 对全部223姿态用实际scale/quaternion/translation计算三点世界坐标；CSV提供当前Attack两种绑定的完整63姿态。当前绑定与只平移Grip候选分开标注，没有修改原facts。

当前light为.35准备/.18有效/.47恢复，总1秒；整段均匀播放把有效窗映到clip[.3208333403,.4858333439]。该区间actual BladeTip：X45.859→50.821、Y-64.262→-39.605、Z153.503→192.053cm；仍在举斧。绑定重居中候选同区间X47.944→52.667、Z151.891→208.695，也处准备。此证据来自真实切缘位置/方向及图片，未把hand高度峰值当接触。

另选明示几何QA目标前表面X=80，Y[-60,-10]，Z[80,170]cm。该面可对应QA box centre(110,-35,125)/extent(30,25,45)，用于根进一步真实collision RED；这里未生成box、未声称真实enemy命中。当前binding刃弦仅在已采uniformAction .716667/.733333/.75/.766667穿过该面（clip .656944/.672222/.6875/.702778，交点Z156.01→110.12）；Grip平移候选在已采.716667/.783333穿过（clip .656944/.718056，Z156.79/91.05）。中间两个端点都可能已入目标面后方，面交点不应被误解为连续碰撞只发生两次。

当前.35–.53所有刃点X至多约53cm，现5cm半径仍触不到X80目标面。现Combat::Sweep在Owner位置+Z10处延长到Reach200扇扫，可能在尚举斧时对同目标造成伤害；几何和既有权威扫描来源存在确证错位。整段actual BladeTip最大X86.966cm，Grip重居中候选最大X104.820cm；后续不能把实体刃段再拉长到Reach200来掩盖差异。

## 最小phase/trace设计草稿

保留唯一FMove的.35/.18/.47、伤害/耐力/耐久及1秒总时长；用单一源时间映射把clip normalized .6→.8作为当前石斧light候选有效段，即source[.5500000119,.7333333492]，旧Chop source[1.8833333774,2.0666667148]。该段包住已测前挥下降及明确目标面穿过，Root实际collision决定最终边界。准备映0→.6L，active映.6L→.8L，恢复映.8L→L。对应播放率1.57143/1.01852/.390071；这组值只适用于已测石斧轻击，未扩重击/其他武器/其他动作。

现source精确窗口候选：`Animation/HearthwardHeroAnimInstance.cpp/.h` 的PlayCombat/PreUpdate；`Combat/HearthwardCombatComponent.cpp/.h` 的Attack/Sweep；单一共享phase公式可局部放既有`CombatRules.h`避免动画/扫描复制公式；`HearthwardCharacter.cpp`仅已核准Grip挂点。Root仍需登记实际最小范围再施工。Anim proxy应消费Combat Elapsed与该Move的同一源时间，显式设置attack player时间并停自动积分，保留现revision/StopCombat与0.12blend；不继续按独立DeltaSeconds做三段不同步计时。

真实Sweep仍沿已有clamp到有效窗、.015分段、ActionInstance GUID、epoch/ActionId、HitIds和ChargedWear。各跨窗历史样本必须在对应source time抽真实Hero hand组件姿态，再应用已核准Grip/刃端和mesh世界TF。只读取本帧socket并在整个低帧窗重复使用不能闭合历史轨迹。Engine公开Runtime `UAnimSequence::GetAnimationPose(FAnimationPoseData&,FAnimExtractContext)` 支持对RequiredBones抽样并进行实际retarget；本机Editor AnimPose.cpp:636–701现QA正是设置实际mesh RequiredBones/retarget并调用此Runtime接口。生产不依赖Editor AnimPoseExtensions，不加模块。`GetBoneTransform`单bone未自动等价于当前retarget component pose，不直接拿未验证的单bone拼接替代。

抽样后的实际刃弦/相邻时刻刃端轨迹需用现Visibility球扫，并处理最前blocking wall。保留5cm半径；低帧时需覆盖两个端点的跨时刻运动与实际刃段，真实薄墙/目标回归才能证明连续扫掠。当前离线样本未验证native blend、碰撞或任意曲率下不漏扫，不从数据表生成固定世界轨迹用于生产。

最窄下一步：先Root无保存握点fixture/同图实测；登记一条真实stone-axe light Native回归，以normal本人GUID Equip→Attack→真实Clock驱动目标前表面确认现RED。修复后仅准备/恢复零伤害、有效窗真实刃接触、低帧跨窗同一次GUID耐久及blocking墙对照；既有CombatTests已覆盖事务/旧epoch/取消则复用，避免重复。最终正常输入录制至少准备/active/恢复、同动作角色与刃段位置，另实测Native 0.12blend。若mesh/world握点仍不合格，先校准再让phase成为验收。

主源：[Epic UAnimSequenceBase::GetAnimationPose](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UAnimSequenceBase/GetAnimationPose?lang=en-US)、[Epic UAnimSequence::GetBonePose](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UAnimSequence/GetBonePose)。具体5.8.2签名已读本机Engine/Classes/Animation/AnimSequence.h:427/437/508与Editor/AnimationBlueprintLibrary/Private/AnimPose.cpp；未套旧UE4弃用GetBoneTransform签名。
