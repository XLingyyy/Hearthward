# TASK-070／055 装备导入与持握只读准备

2026-10-04。基线为根集成树 task051 当前文件；制作源从同基线 task056 的实际 FBX／JPEG 只读解析。未启动 UE、导入、修改 Source／Content、复制制作源、计算 checksum 或运行 Git 写命令。本文列待登记的施工窗口，未授权生产施工。根保留引擎、锁、正式审阅及 Git 权限。

## 当前接入事实

- Source 没有 Held3D 类型或组件。`Source/Hearthward/HearthwardCharacter.cpp:95–103` 的 HeldAxe 是唯一现有持握组件，挂在真实 `hand_r`，旋转 `(0,0,-90)`，绝对 Scale、`.7`、NoCollision。`RefreshHeldTool:146–151` 仅查旧 `Gameplay.Equipment[weapon]==axe` 与类型库存数量，未消费本人实际装备 GUID／耐久或 ranged 选择。
- 原石骨斧已实际导入：`Content/Hearthward/Assets/TASK-028/props/stone_bone_axe/{SM_stone_bone_axe,Color,Normal,Roughness,tripo_mat_5ce74ca3}.uasset`。当前 Content 未找到下表十件 UUID／短 UUID 或枪、刀、弩、护具语义包；本表全部为未导入制作源。
- `HearthwardInventoryComponent.h:52–58` 已有 FindInstance／EquippedInstance／EquippedItem／WearInstance。显示可以直接消费这些接口，保持装备、磨损、转交、读档的唯一状态。远程选择读 `CombatComponent::RangedSelected()`；现有 GameplayComponent.cpp:177 先更新 SelectRanged，随后沿 OnChanged 通知角色，不需给 Combat 新造显示事件。
- `Companion/HearthwardCompanionFixture.cpp:98–114` 已有实际 Brother Mesh、Bag，但没有持握组件。先完成主角单实例闭环，再给弟弟自己的手骨添加一件持握组件，避免在主角另造并行 Held3D。
- 正式数据当前武器类为 shortblade／longblade／spear／blunt／bow／crossbow。axe、blunt_2／3 的名字和 toolKind 均是战斧／axe；钉木棒不能未经批准直接替代伐木战斧。crossbow 仅有 crossbow_2／3，没有基础 crossbow ID。不得为贴模型新加道具、改数值、改存档或把不存在的 ID 当装备。

## 十件 FBX 实物

完整逐文件路径及元数据见 [equipment-fbx-facts.json](equipment-fbx-facts.json)。路径根为 `G:/GameFactory/Hearthward/.agent-local/task056/art_source/TASK-004/Tripo/`，六武器在 `武器/outputs/<UUID>/<UUID>_pbr.fbx`，四护具在 `护具/outputs/<UUID>/<UUID>_pbr.fbx`。根来源相对路径完全相同；使用现有镜像原源，原文件保持只读。

每件 FBX7400、一个 Mesh Model、一个 Geometry、零蒙皮 Deformer、一个 UV 层和一个 Phong 材质。GlobalSettings 为 Y up，UnitScaleFactor=1（厘米），Model Lcl Scaling≈100；X Rotation≈-90，另有各自 Y 旋转。原始 AABB 中心均接近 `(0,0,0)`，这不是握柄中心。

下表“主轴／长度”来自未应用 Model 旋转的原始控制点 PCA；三轴长度保持原始数值。考虑节点100倍 Scale 后，第一长度乘100可作为厘米长度估算，不能冒称 UE 导入后的轴或 Bounds。三角面列为逐 polygon 的 n−2 fan 估算，实际 UE triangulation／welding 仍待引擎报告。

| 候选 | UUID | 多边形 / 三角面估算 | PCA 三轴长度（原始） | 原始主轴 XYZ | Model Rotation XYZ |
|---|---|---:|---|---|---|
| 枪 | 29322e3e-a4e3-4d17-9223-80d1f2d65f99 | 48422 / 96818 | 1.373, 0.173, 0.072 | -0.003, -0.701, 0.713 | -90.00, 0.00, 0.00 |
| 弓 | 84cf4884-8d18-4737-a4f5-1b581e7eec13 | 49907 / 99796 | 1.141, 0.199, 0.075 | -0.452, -0.172, 0.875 | -90.00, 0.00, 0.00 |
| 刀 B | 8dd7cafa-fdd7-40cd-9724-a0fa113e72e9 | 49151 / 98299 | 1.280, 0.254, 0.179 | -0.058, -0.654, 0.755 | -90.00, -30.13, 0.00 |
| 刀 A | b39bb9c5-8ee6-484e-8ab1-e6c1ff5046e6 | 51284 / 102563 | 1.237, 0.285, 0.152 | -0.074, -0.602, 0.795 | -90.00, -27.20, -0.00 |
| 钉木棒 | c31fb95d-d93d-467b-bfa6-5e1e9a4bb2e3 | 44999 / 90000 | 1.363, 0.388, 0.385 | 0.633, 0.482, -0.605 | -90.00, -8.84, 0.00 |
| 弩 | c49a60bf-f3bf-4ee9-a854-145882c4ebdd | 48420 / 96836 | 1.090, 0.966, 0.256 | 0.877, 0.003, -0.481 | -90.00, -34.15, 0.00 |
| 双靴 | 64c15994-0225-4d07-96b4-7402c74acb94 | 47305 / 94596 | 1.142, 1.002, 0.844 | -0.310, 0.042, 0.950 | -90.00, -42.75, -0.00 |
| 胸甲 | 76f88975-3cc8-410c-9842-2ddd8765164b | 48216 / 96410 | 1.031, 0.760, 0.501 | -0.175, -0.032, 0.984 | -90.00, -16.95, 0.00 |
| 兜帽 | a42d2caa-03d0-4d3e-b57b-88ae746453e2 | 46139 / 92260 | 1.112, 0.809, 0.777 | -0.386, -0.097, 0.917 | -90.00, -34.07, 0.00 |
| 双护臂 | afdffd2b-08be-462f-880b-e0c83454b953 | 47563 / 95105 | 1.097, 1.006, 0.427 | 0.352, 0.871, -0.343 | -90.00, 1.67, 0.00 |

枪主轴长度约137.30cm，弓114.13cm，刀B128.02cm、刀A123.68cm，木棒136.33cm，弩主轴109.00cm、横向96.58cm，均为源尺度估算。模型自身含斜轴，直接用 raw Z 或石斧 `(0,0,-90)/.7` 套六件会错误缩放或错握。长枪和短刀最终可读尺寸应由实际 Hero180cm／Brother160cm 身体及批准动作对比校准；刀A预览同属单手曲刀，当前手柄不足以直接通过双手长刀验收。

每件四张 JPEG：Color／Normal／Roughness／Metallic，均4096×4096；FBX还嵌入四张非空 Video Content。FBX RelativeFilename 指向 `tripo_pbr_model_<UUID>.fbm/`，实际外部目录为 `<UUID>_pbr.fbm/`，所以相对外部路径不匹配；嵌入图像完整，不能据此报源贴图缺失。导入须真实检查导出的 Texture2D，必要时用既有外部 JPEG 显式导入到该件专属目录，避免跨件同名 Color 覆盖。

FBX四张纹理分别连 DiffuseColor、NormalMap、ShininessExponent、ReflectionFactor。后两者不等于 UE 的 Roughness／Metallic 输入。Epic FBX 文档说明自动材质连线覆盖 Color 和 Normal，其余须核对；因此只见导入成功或一个材质槽不能取得 PBR 完成信用。Color 使用 sRGB，Normal 使用真实 Normalmap compression，Roughness／Metallic 使用线性采样并连接对应材质输入，保持原四张图和UV，不把木料整件强制金属化。[Epic FBX Static Mesh Pipeline](https://dev.epicgames.com/documentation/unreal-engine/fbx-static-mesh-pipeline-in-unreal-engine?lang=en-US)

四件护具全部无蒙皮：兜帽／胸甲需要头颈／躯干适配；双靴、双护臂各合在一个 Geometry，需要先分左右并适配足／前臂骨。没有独立盾、腿甲、箭袋源。本轮静态武器闭环不把这些护具当可穿戴交付，也不把护臂冒充 legs。

## 公开导入与现有 QA 可复用范围

`G:/GameFactory/engine_adapters/ue5/assets/client.py:248` 提供 `ue.assets.import_weapon(source, destination=..., options=...)`；其 source 必须是 `{game_id,run_id,task_kind,task_id,artifact_key}`，直接传本地 FBX 字符串违反 v1 契约。`resolve_source` 可先查身份及真实文件，`validate` 目前只验证源扩展名／文件存在，不能当 LOD、材质、骨架、Bounds 或碰撞验收。

本批只有旧 task.json，没有现代 meta.json；当前 `test_data/outputs` 的18个 meta 中也没有这六武器 UUID。正式导入需根在本地 Saved/Task070 的单次 staging 下准备标准 `<game_id>/<run>/assets/3d_object/<UUID>/meta.json` 和相邻真实 FBX（显式 fbx_path）、必要外部 JPEG，设置该导入进程自己的 AAAGF_OUTPUT_ROOT。复用 pipeline.common.paths 的既有目录／metadata函数；不改 factory 全局配置，不改原源，不下载，且不能在 meta 中用超出 task_dir 的绝对路径绕过 resolver containment。此准备仍待根授权执行；本次没有创建 staging。

复用055现有 `import_motion.py` 的 UEClient 构造、正式 launch/stop 和原始结果写法，调用公开 assets.import_weapon；不用 `_service`、`_transport`、私有 dispatcher，也不拷贝 importer helper。本地单件后处理由根拥有的 `runtime.launch_editor(extra_args=[-ExecutePythonScript=...])` 运行真实 unreal API，处理本单新包并写实际 QA。

**导入 options 的真实范围很窄**：GenericImportScriptBuilder 消费 as_skeletal／generate_collision／combine_meshes（后两只在生成碰撞路径）；普通静态FBX没有 asset_name／normalize_scale／target_tris／lods 的实现。不得写这些被忽略的 options 后宣称已生效。`import_generated_mesh` 是仓库另一路既有工具，能报告真实统计和请求LOD，但 UEClient武器导入不路由到它；它的 target_tris 约束仅尝试生成后续LOD，LOD0仍保留源面数，不能称其已经给LOD0减面。

- 新持握网格使用 StaticMesh，as_skeletal=False，不引入骨架。不要请求 generate_collision=True 或 ComplexAsSimple；武器视觉组件保持 NoCollision／不影响导航，命中仍由055权威事务执行。引擎实际如果自动产生碰撞，使用 StaticMeshEditorSubsystem.remove_collisions 去掉，并读取 simple count／convex count／BodySetup，记录真实结果。
- 可直接复用本机 `StaticMeshEditorSubsystem.h` 现有 SetLods／SetLodReductionSettings／GetLodScreenSizes／RemoveCollisions／GetSimpleCollisionCount。建议先一个刀或枪验证，三项reduction_settings明确对应LOD0=1、LOD1=0.25、LOD2=0.08，LOD0保留源，后两比例作为初始候选，必须读取真实每LOD triangles／vertices，近远景检验柄／刃轮廓、UV、跳变；这两个比例是待验参数，不是已达性能目标。
- 当前真实 StaticMesh.h:2146–2200 的 BlueprintPure GetNumVertices／GetNumTriangles／GetNumLods／GetBounds 支持Python统计；获取实际包、bounds origin/extents、每LOD三角面、UV、material slots，Texture2D尺寸／sRGB／compression、材质真实纹理引用，不能仅以文件存在或输入 request 作通过。相关 UE5.8 本机公开函数已只读核对；在线 Python 文档当前为5.6，对照本机5.8声明后使用。[Epic StaticMeshEditorSubsystem](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/StaticMeshEditorSubsystem?application_version=5.6)

## 最小内容包、锁与源码窗口候选

新目录拟为 `Content/Hearthward/Assets/TASK-070/Equipment/<piece>/`，逐件导入隔离，确保同名贴图互不覆盖。根登记准确子目录后，公开导入返回实际 imported_paths；随即选其唯一 StaticMesh 并使用正常 EditorAssetLibrary.rename_asset 归到下表最终名称，读取实际依赖后再记录最终逐文件锁。公开 import_weapon 不接受命名选项，不先假定它的自动命名。暂存自动名／redirector由根受控清理并记录，避免遗留。

| 件 | 制作源 UUID | 最终 StaticMesh package 候选 | 可绑定条件 |
|---|---|---|---|
| spear | 29322e3e-a4e3-4d17-9223-80d1f2d65f99 | /Game/Hearthward/Assets/TASK-070/Equipment/spear/SM_Spear | 先基础spear；金属层级有独立材质／Owner外观验证后记录 |
| shortblade | 8dd7cafa-fdd7-40cd-9724-a0fa113e72e9 | /Game/Hearthward/Assets/TASK-070/Equipment/shortblade/SM_ShortBlade | 基础单手刀先校准长度、柄中心、刃端；源图看似金属刃，不能自动标所有石／金属层级正式外观 |
| blade_a | b39bb9c5-8ee6-484e-8ab1-e6c1ff5046e6 | /Game/Hearthward/Assets/TASK-070/Equipment/blade_a/SM_BladeA | 第二刀只作比较候选；未通过双手柄／长刀动作前不绑longblade |
| bow | 84cf4884-8d18-4737-a4f5-1b581e7eec13 | /Game/Hearthward/Assets/TASK-070/Equipment/bow/SM_Bow | 基础bow显示；静态弦、拉弦／放箭动作未完成单列 |
| blunt_candidate | c31fb95d-d93d-467b-bfa6-5e1e9a4bb2e3 | /Game/Hearthward/Assets/TASK-070/Equipment/blunt_candidate/SM_BluntCandidate | 先导入评审；当前axe／blunt_2／3都是斧，不直接把木棒变成伐木斧 |
| crossbow | c49a60bf-f3bf-4ee9-a854-145882c4ebdd | /Game/Hearthward/Assets/TASK-070/Equipment/crossbow/SM_Crossbow | crossbow_2／3现有类，保持装填和箭耗规则；金属层级外观／装填动作待验 |

以上每个 mesh 的磁盘锁为对应 `.uasset` 路径；每个专属目录还登记 `Color.uasset`、`Normal.uasset`、`Roughness.uasset`、`Metallic.uasset`、`tripo_mat_<UUID前8位>.uasset`，按公开导入实际结果逐文件补齐并核对。可将正式材质统一改为同目录 `M_<piece>.uasset`，但只有现有导入材质无法修正确连线时才创建，避免无意义重复。首次闭环只锁该一件6包，不先锁其他四护具／全部Content。已有石斧如需追加 Grip／刃端 socket，仅登记和锁 `Content/Hearthward/Assets/TASK-028/props/stone_bone_axe/SM_stone_bone_axe.uasset`；不锁／修改 Hero/Brother SkeletalMesh、Skeleton、PhysicsAsset 或Motion。

主角最小 Source 窗口：

1. `Source/Hearthward/HearthwardCharacter.cpp`：复用现有 HeldAxe 单实例；constructor维护本单实际mesh硬引用（必要成员用UPROPERTY保留引用）／已验证握点绑定，RefreshHeldTool改用本人装备slot→GUID→有效Instance／Durability，按既有Combat ranged选择决定当前一件显示。分类消费现有combatClass；不得把不存在、毁损或转出实例继续显示。保留现有装备／库存／读档订阅与取消订阅。骨挂接和model grip需要各件实际校准，武器switch后更新同一组件。
2. `Source/Hearthward/HearthwardCharacter.h`：仅为已确认多mesh硬引用和当前绑定缓存增加必要私有成员；已有HeldAxe名称可以保留，单纯改名没有实现收益。不新建装备系统／公有setter／第二持握组件。
3. `Source/Hearthward/Tests/EquipmentPresentationTests.cpp`：根登记后局部增加真实 Character 持握断言，验证GUID→mesh、耐久0／转交／卸装／读档隐藏与恢复、ranged选择、手骨及可见实例唯一。当前文件已有UI装备测试，不重复数值／事务测试。

先保留原axe绑定并完成一个spear或shortblade闭环，再扩bow／crossbow；木棒与第二刀按上表门槛决定真实映射。constructor硬引用可让最终Cook依赖发现新mesh和其材质／贴图，不需先把未绑定候选整目录AlwaysCook；根最终074仍实查cook依赖。纯字符串软引用则额外登记 `Config/DefaultGame.ini` 的准确单件目录；最小实现优先沿现constructor硬引用，避免扩大窗口。

弟弟后续精确窗口为 `Source/Hearthward/Companion/HearthwardCompanionFixture.cpp` 与 `.h`，沿本人 Bag 的GUID／OnInventoryChanged及现有读档恢复，不读主角装备状态；该角色当前无持握组件，新增其一个实例有实际需要。此阶段不动 `Source/Hearthward/Animation/`、Combat数学／伤害窗口、sharedSystem、Gameplay数据或存档。弓正常握持常需hand_l，近战与弩优先hand_r；只有055实际源姿势明确后才登记左右手绑定，不能拿一段Chop假作射箭已完成。

## 握点、尺度与实际验收顺序

1. 根先单件真实导入，并报告经过节点／坐标轴转换后的UE bounds、原点、每LOD、材质。导入相对源单位×100已在Model上体现，不另凭raw extent小于1再乘100。保持现有 SetAbsolute(false,false,true) 防手骨继承Scale把武器放大；角色实际身高180/160cm已确定。
2. 在该StaticMesh定义真实 Grip socket（柄包裹中心、朝向按055实际手掌／手指），近战另定义 BladeBase／BladeTip，弓／弩按实际发射模型定义ArrowOrigin。沿Epic socket流程，不改人物Skeleton或创建虚构碰撞端点。socket坐标需用导入后的mesh实物选点，PCA只提供轴候选，不能据原始PCA直接宣称握点数值准确。[Epic FBX Sockets](https://dev.epicgames.com/documentation/unreal-engine/fbx-static-mesh-pipeline-in-unreal-engine?lang=en-US#sockets)
3. 以真实 hand_r／hand_l 骨变换和Grip的逆变换计算同一组件位置／方向。绝对Scale只取消比例继承，RelativeLocation仍会被手骨比例变换；须用实际父hand世界Scale换算Grip偏移，不能简单把厘米GripOffset直接写入相对位置。连续动作抽帧核对柄中心和手骨世界位置，避免静止一帧贴手、动作后漂移。此为根据当前骨100倍单位和SceneComponent变换规则的实现约束，最终误差需真实UE测量。
4. 真实渲染至少覆盖持握近景和全身远景、已批准动作准备／有效／恢复三个时间片及装备切换。每件记录hand bone、mesh／Grip世界坐标、尺寸、角度、材质、LOD，Owner审阅柄贴手、刃朝向、双手支持／身体穿插。明确实际PIE正常输入与临时测试fixture；截图不得混记为正常键鼠验收。
5. 只做持握显示时保持视觉NoCollision，不借本轮声明055真实武器扫掠完成。命中端点接入应由根另登记权威Combat路径，复用现ActionId／epoch／有效窗口／一次结算；此处为下一阶段提供真实socket数据。弓弦变形、弩装填、实际箭外形、盾／腿甲／箭袋缺源，分别保留未验收条目。

## 来源许可边界

十件的 `武器和护具生成清单.md` 和逐件 task.json 真实记录2026-09-22、v3.1-20260211、50k面、PBR、quad、任务success及输入图名；各武器／护具目录没有 SOURCE.md。现有 `妙妙道具/SOURCE.md` 确认其四props使用付费账户及Owner输入／公开入库授权，`房屋建筑部件/SOURCE.md`确认另八件建筑；其确认对象不能自动扩展成这十件的独立台账证明。

公开Tripo付费模型商用条款允许使用、修改、分发，但不授予输入图权利；因此最终发布台账应把这十个UUID对应Owner输入图授权和生成时付费身份补成具体事实。当前可进行工程适配调查，最终074来源验收保持未闭合，不能把任务success或webp存在当许可证明。[Tripo商用说明](https://www.tripo3d.ai/help/privacy-policy/how-to-use-tripo-models-commercially)

本轮没有付费生成、下载、依赖安装或音频制作；固定54提示／28录音组仍暂缓，与装备导入分开保留真实状态。
