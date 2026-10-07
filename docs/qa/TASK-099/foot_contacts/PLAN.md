# TASK-099 触地事件调查与实际资产写入登记

当前只读证据：`.agent-local/qa/TASK-099/animation-contacts-20261006T225948Z-dcdcc6b1/metadata.json` 在本机 UE 5.8.2 实读四段 locomotion，全部 `notify_count=0`，全部已有 `foot_l`、`foot_r`。Walk 长 2.333333 秒、56 帧区间；Run 长 1.25 秒、30 帧区间。root 的公开 API 实采已完成，原始输出为 `.agent-local/qa/TASK-099/foot-poses-20261007-01/data/poses.json`，四段全部 READ；原字节副本见 `poses-20261007-01.json`。

实际轨迹支持接触候选，仍不能单独证明鞋底触地。`contact-candidates-20261007-01.json` 保留各次主 swing 下降后的首个踝部刹停低点及位置：HeroWalk 右3/32、左18/46；BrotherWalk 右3/31、左18/46；两段 Run 左8/23、右0/15（seam 算一次，不再写末帧）。Walk 的 stance 段存在多处细小低点，不能全部写通知。Run 的右脚跨 seam 已处于低位，以第0帧作为同一候选接触 episode；第29或末帧不能再加一条。

候选踝关节相对默认胶囊底为10.52–14.97厘米，Walk 的 toe 在候选帧仍可翘起，与 heel-strike 相容；该解释是轨迹推断，尚无鞋底或地面实测。20厘米向下查询可作为覆盖已测踝高的技术搜索范围，实际阻挡、walkable floor、材质和运行状态仍必须成立；短射线命中不能自动把未知材质映成某种地面。

## 采样入口

`docs/qa/TASK-099/foot_contacts/sample_foot_poses.py` 是 Editor 内执行脚本。由 root 复用公开 UEClient Python 操作；脚本不启动、关闭或控制 UE，不调用 asset setter/save。无参数时输出新的 `.agent-local/qa/TASK-099/foot-poses-<UTC>-<unique>/poses.json`，日志前缀 `TASK099_FOOT_POSE_REPORT=`。可在执行环境注入 `TASK099_FOOT_OUTPUT_DIR` 指定一个尚不存在的 QA 目录。

使用真实 runtime character CDO 的 mesh、相对变换、胶囊尺寸，`AnimPoseEvaluationOptions` 选择压缩数据、实际 mesh 和 retarget，按 `AnimationLibrary.get_time_at_frame` 的运行采样率读取每一帧，包含末尾 loop seam。`GetAnimPoseAtFrame` 包装器使用 source DataModel 的帧率；本脚本使用明确采样时间配合 `get_anim_pose_at_time`，避免源帧率和运行采样率混用。

输出 `foot_l/r`、存在的 `ball_l/r` 与 root 的完整组件空间坐标、实际默认 mesh 变换后的 actor 坐标、相对默认胶囊底的高度。`AnimPoseSpaces.WORLD` 在此 API 中表示组件空间，未包括 actor 世界变换。缺骨或无效 pose 记录失败，不能把返回 identity 当数据。

所有局部低点只标 `CANDIDATE_ONLY`。胶囊底是作者参考平面；脚骨为关节位置，未采鞋底和碰撞地面。完整原帧与 loop seam 保留，不按移动速度、固定周期或等分相位生成脚步。最终时间需依据本轮实际轨迹审阅，再用 runtime 的实际骨位置与地面碰撞验证。

## 最小生产方案与写锁

- 新增 `Source/Hearthward/Experience/HearthwardFootContactNotify.h/.cpp`：一个原生 `UAnimNotify`，只携带 `FootBone`（左/右脚）；覆写 UE 5.8 的三参数 `Notify(MeshComp, Animation, EventReference)`。不新增保存字段或全局事件总线。
- 扩充 `Source/Hearthward/Experience/HearthwardPresentationComponent.h/.cpp`：接收实际角色、脚骨和当前 epoch，重用现有音效播放/去重/空间衰减路径；弟弟共用玩家的 Presentation，不给弟弟挂一个要求自身 Gameplay 的新 Presentation。
- 补一个定向 native 测试文件，验证四资产真实 Notify、左右脚与采样时间一致，以及实际脚下阻挡/空中/暂停/读档/未知材质的消费边界。实际循环播放和试听另由 root 执行；仅调用测试入口不能声称已听到脚步。
- 内容写锁严格限定 `Content/Characters/Hero/AnimationV2/A_Hero_Walk.uasset`、`A_Hero_Run.uasset`、`Content/Characters/Brother/Animation/A_Brother_Walk.uasset`、`A_Brother_Run.uasset`。不用改 Skeleton、SkeletalMesh 或 locomotion proxy。root 已核验现有 LFS 锁并授权该窗口，随后实际保存四包16条候选，见[实际写入原报告](asset-write-20261007-01/write.json)。
- `Resources/Data/experience.json` 的足音事件和声音文件由 root 协调已有音效 writer。root 已批准通用 `movement.footstep`；默认或未知 PhysicalMaterial 可以播放通用候选，不能推断为石、木或土。正式声音风格和真人试听仍未验收。

运行门槛：实际 Character 正在地面移动、CurrentFloor walkable、Alive、未暂停、未恢复存档；实际 `MeshComp->DoesSocketExist(FootBone)`；读取 `GetSocketLocation` 的已评估脚骨位置，向下作排除人物自身的真实阻挡查询，拒绝穿透起点、不可行走法线和人物阻挡。20厘米短查询覆盖本轮已测踝高，声音位置来自实际 `Hit.ImpactPoint`。回执保留实际 PhysicalMaterial；通用足音不猜表面类别。每次合格接触独立临时成功 ID，并按 epoch 消费；跳跃和无地面不发声。

现有 Hero/Brother 的 walk/run 已在同名 `Locomotion` sync group。Notify 使用 leader-only（`trigger_on_follower=false`），权重门槛需明确设置，避免 blend 两段同时触发。具体门槛和左右相位需结合实际轨迹、现有组同步和运行验证确定，不能用常数周期替代接触。

本机 `SkeletalMeshComponent.cpp:5177–5186` 在 `FinalizeBoneTransform()` 后 dispatch queued AnimNotify，以允许最新 socket 位置；两角色又配置 AlwaysTickPoseAndRefreshBones。常规路径可以在 Notify 内读取实际脚位，没有证据要求额外延迟队列。未评估/未知状态仍由实际地面检查拒绝。

## API 依据与当前状态

本机 `Engine/Source/Editor/AnimationBlueprintLibrary/Public/AnimPose.h`、`Private/AnimPose.cpp`、`Public/AnimationBlueprintLibrary.h` 为 5.8 接口的主要依据。官方 [AnimPoseExtensions](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/AnimPoseExtensions?application_version=5.7)、[AnimationLibrary](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/AnimationLibrary?application_version=5.7) 描述对应 Python 绑定；[Animation Notifies](https://dev.epicgames.com/documentation/en-us/unreal-engine/animation-notifies-in-unreal-engine) 说明通知与同步 follower/权重行为。网上 Python 页目前为 5.7，本机 5.8 header/implementation 优先。

脚本语法检查PASS；root公开API实际pose sampling四段READ，原报告归档。新生产已实际Editor build成功；writer前Native06的四段零Notify产生ActualClipDispatch预期RED，GroundGuards硬编码100伤害前置已修。root公开writer实际保存四包16条候选并成功停止Editor；随后07既有Foot两项实际Success/0warning/error，限真实guard和四段SingleNode一个左候选。Run接缝原请求0秒被UE规范化并保存为9.999999747378752e-05秒，没有在动画末端追加重复事件；不能将持久化时间写成精确0。07之后获准扩既有fixture，增加两Run实际右接缝正向短区间和真实Brother来源，目前新断言NOT_RUN，不能继承07。实际循环混合、设备听感及Owner试听继续未验，写入结果不作为运行触地PASS。
