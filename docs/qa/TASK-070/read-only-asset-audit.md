# TASK-070 现有资产与动作接入审计

2026-10-04。canonical 070 对应原规划 072。依据已批准的 `docs/planning/TASK-053-074/TASK-070.md`、原规划 072 的 740—750 行和 2026-09-24 审计的资产尾项核对。Owner / Reviewer 均为 XLingyyy，无 Issue。本文是施工材料调查，未启动 UE、构建、导入、下载、采购、生成、修改 Source / Content / art_source 或执行 Git。当前只获 QA / 资产候选事实记录窗口；正式二进制和源码必须由根登记准确路径后才能施工。

读取对象为根集成树 `G:/GameFactory/Hearthward/.agent-local/task051`。以下相对路径均以该树为根；源二进制的已有完整镜像为 `G:/GameFactory/Hearthward/.agent-local/task056`，没有复制或拉取。055 的 PhysicsAsset 与动作施工仍由根推进，本文对其报告的引用保留各自版本和覆盖范围。

## 已确认的范围与结论

- 070 可以复用双角色 61 骨架、Guard / Heavy、14 种 R3 动物和 303 段动作、8 个房屋部件、桌 / 床 / 箱 / 提灯 / 石骨斧 / 静态火堆。没有重做全部动物动作的依据。
- 当前 Content 中查到真实二进制：Hero UE5 8 包、Brother UE5 8 包、Campaign 34 包、Animals/MotionR3 471 包、Nature 14 包、TASK-028 77 包、Demo 24 包。各目录均无 LFS 指针、无小于 1 KiB 的包。这只证明磁盘有包；LOD、碰撞、材质是否正确须消费已有引擎报告或由根实际读取。
- `animal_motion.json` 14 个 mesh 引用和 303 个 AnimSequence 引用全部存在；骨架配对由运行代码检查。051 历史报告的 199 项运行检查通过属于其受测版本，不转换成当前所有玩法、光照或完整动作通过。
- 当前实质 TEMP：射手仍用短刀兵和同一攻击动画；族人仍用弟弟；野猪在两个加载入口都用 pig；设施多类复用同桌 / 同火堆 / 同床；鱼竿有源但无持握；四鱼有独立包但没有正式渔获模型展示；作物已有三个几何尺度阶段，但三个作物仍同一立方体植株；刀枪、鱼、种子等 UI 图标复用斧头 / 肉 / 草药。
- 缺独立盾、腿甲、专用射手、族人、野猪、冶炼炉 / 锻造 / 烹饪模型的已批准正式候选。现有源可以支持本地派生候选，派生完成后仍须真实尺寸、连续动作、许可台账和 Owner 视觉验收。
- 固定录音继续暂缓：54 个 cue、28 个 audio_group 均 `UNPRODUCED`，`Resources/Audio` 下实际 WAV 数为 0。未听录音、未产录音保持未验收；不使用 AI / 静音文件替代。

批准设计要求先给人物、建筑 / 营地、地表 / 故乡三组样图检验一致性。当前尚无070三组真实review材料；D04获批不等于现有资产已符合。下列旧图仅用于定位差距，本轮已实际核看，没有将其当成070验收图：

- 根 `Saved/Task055/motion-hero/slash-front.png`、`motion-brother/slash-side.png`：当前深色穿甲 / 披风轮廓，属于灰盒单节点姿态检查，Brother图有Widget覆盖身体。朴素织物、绳结 / 皮革及三角色剪影区别尚未在这些图得到证明，完整slash的工具接触也未证明。
- 完整镜像 `docs/qa/TASK-049/screenshots/guard-runtime.png / heavy-runtime.png / camp-after-prologue.png`：旧正式地图里可见深色角色、木构房屋、草地树木；普通 / 重兵图处于阴影并受HUD覆盖，营地图多个人物沿同一重甲 / 披风轮廓。木构材料可以复用，不能由一张图宣布角色辨识、石房基、营地暖光 / 故乡冷光和整套D04材质已通过。
- 完整镜像 `docs/qa/TASK-051/runtime_8ccedf902b8e/native/screenshots/pig_natural.png`：旧演示灰色家猪外形、无可辨独立獠牙 / 背鬃，表面亮斑明显。单帧无法区分贴图烘入高光和动态反射；应在中性可移动光源下复核粗糙度 / 色图，不能直接判材质根因或宣称D04粗糙毛皮达标。

070最小样图材料须由根真实UE串行生成：人物组用同光照 / 尺寸基准并排主角、弟弟、族人、普通 / 重兵 / 射手候选，附主要动作连续片段；建筑 / 营地组展示一组设施辨识、床 / 治疗、动态火与日夜；地表 / 故乡组在现有正式落点展示木石房基、地面 / 草木、通路和冷暖光。样图注明候选 / 旧包 / 新派生与仍保留TEMP，Owner先审这三组，随后才批量统一资产。动物 / 三作物 / 图标的后续针对性核看继续保留。

## 来源、许可与读取口径

| 来源组 | 直接证据 | 本轮可复用与真实门槛 |
|---|---|---|
| TASK-004 动物 | `art_source/TASK-004/Tripo/动物/SOURCE.md`：Owner 确认付费 Tripo，2026-09-23 授权输入与输出公开同步；静态 / 骨骼索引保留 15 个任务 ID | R3 继续沿同一源链。最终逐件发布许可仍归 074；不存在 boar 专用来源 |
| TASK-004 敌人 | `敌人/SOURCE.md` 和 `敌人模型与骨骼索引.md`：两张 Owner 输入、付费账户、静态与 rigged FBX | Guard / Heavy 来源可追溯；源中自带武器 / 甲不等于独立装备可拆装。专用 archer / tribesfolk 未找到 |
| TASK-004 家具、房屋、妙妙道具、篝火 | 各目录 `SOURCE.md`、索引与 Task ID：Owner 输入、付费账户、公开同步授权；输入图片权利由 Owner 保证 | 现有派生继续引用这些源。篝火源明确是静态火焰造型；不能算实时火焰 |
| TASK-004 六武器、四护具 | `武器和护具生成清单.md`：2026-09-22、10 个 Tripo ID、ChatGPT Image 输入文件、生成参数与 task.json | 未找到这两个目录独立 SOURCE.md；清单与 FBX 存在无法补齐逐件输入权利 / 生成时付费证明。055 已将最终许可留给 074 |
| 当前双角色 / 055 knight61 | `Resource/Tripo/主角/medieval+knight+3d+model.zip`、弟弟 ZIP；031 首轮 / inplace-fix 报告；`art_source/TASK-055/motion/knight61_motion_source.json` 指向已有本地 ZIP | 来源档案可定位；055 source JSON 的 `license_status` 明确未核实。旧 027 Tripo 人物不同骨架，不能混作当前 61 骨架许可 / 动作依据 |
| UI / 字体 | `Resources/UI/art-provenance.json`：built-in image_gen 产物 ID、Owner UI 参考；Noto CJK 和 LXGW 字体附 OFL | 有生成过程和字体证据；参考 UI 输入权利、逐件发布台账仍须收口。当前新增生活物品的独立图标缺失 |

官方核对：Tripo 当前官方[商用说明](https://www.tripo3d.ai/help/privacy-policy/how-to-use-tripo-models-commercially)允许付费用户使用、修改、分发及商业使用生成模型，并要求用户自行拥有输入图权利。该页面无法证明某个历史 ZIP 的账户或输入权利，本文保持上述档案边界。

源完整性事实通过文件存在、大小和 LFS 指针头读取取得，没有重新计算 SHA。根树的 TASK-028 FBX、R3 AS / SK、角色 ZIP 多数为 132 / 133 字节指针；完整镜像里的对应 R3 AS / SK 均为 MB 级文件。例：pig `AS_pig.blend` 50,893,254 字节、`SK_pig.fbx` 8,942,636 字节；原始鱼竿 FBX 6,596,300 字节、木弓 FBX 6,114,700 字节，完整镜像可直接读取。原目录仅鱼竿等部分源完整，不能把根指针当 DCC 源。

根追加的克隆准备核对：task051 / task056 HEAD 同为 `67fb0784ca8c6d488173e587e7f95c4be0d9092a`；镜像 tracked Content 的 status / diff 为空。按该 HEAD 的 2,441 个 Content 文件扫描，task051 指针 0、缺失 0，因此本次待 smudge 既有资产数 / 字节均为 0。task056 对应 2,441 文件全为实物，总计 3,624,942,834 字节，无缺失 / 指针。根有修改的 Hero / Brother SK、根新055 Motion / Physics不属于任何恢复集合；本轮没有复制。R3编辑源的LFS状态与运行Content状态分别记录，磁盘包存在仍不能宣称当前全部正式动作运行通过。

## 角色、装备与候选实物

| 对象 | 当前实际 UE 引用 / 尺寸 | 骨架、材质、LOD / 碰撞证据 | 已确认的应用与剩余 TEMP |
|---|---|---|---|
| 主角 | `/Game/Characters/Hero/UE5/SK_Hero`；导入高 97.869893 cm，显示 180 cm | 当前 61 骨；031 报告 Color / Normal / Roughness / Metallic，最大 2048；LOD 未见当前完整检查；055 `PA_HeroCombat` 实际生成，body 轮廓 / 四部位 / 姿态最终依根本次证据 | `HearthwardCharacter.cpp:82`；仅 HeldAxe 挂 `hand_r`、旋转 Z=-90、绝对 scale=.7、NoCollision |
| 弟弟 | `/Game/Characters/Brother/UE5/SK_Brother`；导入高 97.863766 cm，显示 160 cm | 同 61 骨和材质管线；`PA_BrotherCombat` 同上；保持当前单位比例和胶囊 | `CompanionFixture.cpp:98`；没有与 Inventory 装备实例对应的武器 / offhand / 四部位护甲模型组件 |
| 普通兵 | `/Game/Hearthward/Campaign/Guard/SK_Guard_Runtime`；代码按 mesh bounds 适配 160 cm | 原导入 `SK_Guard` 报告高 99.74417 cm；抽样骨记录 22 个，非完整总骨数；34 包含两兵种材质纹理、原始 / Runtime mesh、Skeleton、8 动作及 retarget 依赖；LOD 未有完整当前证据 | `CampaignActor.cpp:51`；短刀兵源可复用，正式轮廓 / 武器连续握持仍 TEMP |
| 重兵 | `/Game/Hearthward/Campaign/Heavy/SK_Heavy_Runtime`；同样 160 cm | 原导入高 99.82482 cm；抽样骨记录 34 个，非完整总骨数；独立 retarget 骨架；当前 body collision 路径 `Target->CreateBodyCollision()` | Heavy 外形和规则独立；厚甲 / 武器变形、LOD、全部姿态仍待实际验收 |
| 专用射手 | 当前仍 `/Game/Hearthward/Campaign/Guard/...` | 源目录只有 short_blade_soldier / heavy_armored_soldier；未查到 archer mesh、箭袋、弓弦 rig 或专用拉弓动画 | `E->Kind != heavy` 均选 Guard；同 Attack 播放。已有 ranged 规则和 projectile，视觉仍 TEMP |
| 普通族人 / 受保护人 | 当前仍 `/Game/Characters/Brother/UE5/SK_Brother`，160 cm | 复用 Brother rig / 动作；没有独立朴素织物形体候选 | `CampaignActor.cpp:46`；`PresentLabor:83` 固定 Brother Dig / Idle。不能用名称或标签宣布剪影区分已完成 |

071 UI 装备页与 055 GUID / 耐久工作均应保留。`RefreshHeldTool()` 当前只判断 weapon 字面 axe 与数量；实际显示必须消费该角色本人的装备实例、槽位与剩余耐久。两角色源根单位约 100 倍，挂接继续保持绝对组件 scale，避免骨单位把工具放大。

已导入石骨斧：`/Game/Hearthward/Assets/TASK-028/props/stone_bone_axe/SM_stone_bone_axe`；源为 `妙妙道具/outputs/5ce74ca3-7123-488b-babd-d69a75aec742/stone_bone_axe_model.fbx`。028 实测未缩放全尺寸 72.24 × 51.67 × 99.82 cm，LOD=1、simple collision=0、Color / Normal / Roughness；运行 Held visual NoCollision。最终握点、刃根 / 刃端、贴手连续运动没有该 manifest 的通过结论。

以下十个源在完整镜像真实读取过，055 `equipment-source-geometry.json` 保存了二进制 FBX 几何事实。共同为一个 Geometry、Skin Deformer=0；不是已蒙皮穿戴件。每行源路径为 `art_source/TASK-004/Tripo/<目录>/outputs/<ID>/<ID>_pbr.fbx`；贴图在同目录 `<ID>_pbr.fbm/`。当前没有对应独立 UE 包；尺寸是源坐标跨度，未换算为 UE cm。LOD / 碰撞 / pivot / 材质连接仍须实际导入。

| 目录 / ID | 实际预览分类（复用 055 核看） | 源跨度 XYZ / FBX polygons | 可直接推进的用途与边界 |
|---|---|---|---|
| 武器 / 29322e3e-a4e3-4d17-9223-80d1f2d65f99 | 木柄石枪 | .07153 / .96540 / .99888；48,422 | spear 候选，核握点 / 枪尖后导入 |
| 武器 / 84cf4884-8d18-4737-a4f5-1b581e7eec13 | 木弓 | .54704 / .28317 / .99907；49,907 | bow 静态候选；弓弦变形 / 左手持弓 / 右手拉弦缺口 |
| 武器 / 8dd7cafa-fdd7-40cd-9724-a0fa113e72e9 | 单手刀 | .18290 / .85649 / .99978；49,151 | shortblade 候选；不默认同长刀 |
| 武器 / b39bb9c5-8ee6-484e-8ab1-e6c1ff5046e6 | 单手刀 | .15853 / .80535 / .99935；51,284 | 第二刀候选；长刀需要尺寸 / 剪影修改和双手完整动作 |
| 武器 / c31fb95d-d93d-467b-bfa6-5e1e9a4bb2e3 | 钉木棒 | .99686 / .83321 / .98040；44,999 | 仅现行 blunt 参考；无依据新增武器类型 |
| 武器 / c49a60bf-f3bf-4ee9-a854-145882c4ebdd | 木弩 | .99950 / .96580 / .59003；48,420 | crossbow 候选；装填 / 弩箭模型与动作缺口 |
| 护具 / a42d2caa-03d0-4d3e-b57b-88ae746453e2 | 皮兜帽 | .60783 / .81939 / .99795；46,139 | head 候选；头颈拟合 / 蒙皮待制作 |
| 护具 / 76f88975-3cc8-410c-9842-2ddd8765164b | 皮胸甲 | .51328 / .76107 / .99102；48,216 | chest 候选；需当前 torso 蒙皮适配 |
| 护具 / 64c15994-0225-4d07-96b4-7402c74acb94 | 一双皮靴 | .87030 / .98995 / .97224；47,305 | feet 候选；整块 Geometry 需分左右 / 蒙皮拟合 |
| 护具 / afdffd2b-08be-462f-880b-e0c83454b953 | 一对护臂 | .86883 / .99795 / .72930；47,563 | 手臂装饰；无法填 legs 装备缺口 |
| 独立盾 / 腿甲 / 箭袋 / 箭弩箭 | 未找到对应候选来源和正式包 | 无可复用实物尺寸证据 | 可在已批准 D04 内本地制作带源的候选；不得把重兵模型上盾或护臂重命名成完整可穿戴件 |

## 设施、床、治疗、储物、火与房屋

以下为 `Resources/Data/gameplay.json:buildings` 当前真正加载的路径，非旧 028 manifest 的预计路径。`BuildingComponent::SpawnBuilding:48–64` 保留表内 Box half extent 做放置 / 阻挡 / 导航，visual mesh 一律 NoCollision。070 调整视觉时先保持这些边界；修改边界才触发定向站位、建造、路径和交互回归。

路径简写 D=`/Game/Hearthward/Assets/Demo/`；所有下列 D 包均存在真实二进制，包含 Color / Normal / Roughness / Metallic / tripo_mat。当前 D 包的 LOD 数、线性 Roughness / Metallic 和纹理实际连接本轮没有 UE 复读；不能把 TASK-028 同源包的旧检查直接移植到 D 包。

| 实际玩法 ID | 当前 UE mesh（D 前缀） | Root half extent cm / visual scale | 来源 / 当前表现与最小替换 |
|---|---|---|---|
| workbench | `table/wood_table_model` | 70,76,36 / 1.3 | 家具 a463c066 table；真实工作台入口。保留桌，增加工具 / 加工面派生后验收 |
| forge | `table/wood_table_model` | 70,76,36 / 1.3 | 与工作台同外形；缺砧 / 锤 / 金属加工辨识。只在当前 footprint 内做独立候选 |
| smelter | `campfire/campfire_model` | 54,56,27 / 1.1 | 同篝火；缺炉体 / 炉口 / 冶炼剪影 |
| cooking | `campfire/campfire_model` | 54,56,27 / 1.1 | 同篝火；可复用现有陶罐源制作架锅候选，须验证完整源、尺度和运行引用 |
| campfire | `campfire/campfire_model` | 54,56,27 / 1.1 | 篝火 source；当前静态火焰，未查到运行实时火 / 燃烧声音路径 |
| bed | `bed/rope_wood_bed_model` | 105,57,18 / 2.1 | 家具 4f3ec881 bed；真睡眠交互，陈设与可交互床独立 |
| medical_area | `bed/rope_wood_bed_model` | 105,57,18 / 2.1 | 与床同形；治疗区可用药罐 / 布料组合形成辨识，不能仅改文字 |
| warehouse_access | `table/wood_table_model` | 70,76,36 / 1.3 | 真实仓储入口但同桌；可复用已有箱模型，保持 Root 和访问组件 |

床 / 篝火 / 治疗实际通过 `HearthwardFurnitureInteractionComponent`；仓储通过 ResourceInteraction + StorageAccess。生活交易、床位 / 治疗增益和生产规则不由美术另算。

箱已存在两套同源包：D `chest/wood_chest_model` 与 `/Game/Hearthward/Assets/TASK-028/furniture/wood_chest/SM_wood_chest`。源是家具 `911b746d-38db-4697-80ff-a48d2fe938c8/wood_chest_model.fbx`。前者被 `NaturalCamp.cpp:104` 用于真营地仓储，独立 Box half extent 61,69,18.5 cm；后者为房内陈设及 `CampaignWorld.cpp:234` 的任务匣。后者 028 实测 101.07 × 114.45 × 30.76 cm，scale .85、LOD1、simple0、陈设 NoCollision。自然藏宝箱当前仍 `NatureActor.cpp:99` 立方体底箱 / 盖 / 金属条，`NatureActions::Commit` 是实际开箱路径。最小闭环只替藏宝箱 visual 为现有箱，保留 treasure ID、奖励 / 开箱账本、交互和碰撞口径；若要开盖，现有单 Geometry 还需真实拆盖源和动画。

房屋前缀 H=`/Game/Hearthward/Assets/TASK-028/house/`。八项 source / UE 包 / Color / Normal / Roughness 已在 028 manifest 与实际 import / reopen 报告登记：

| H 下部件 | 原 TASK-004 source ID / 文件 | 未缩放尺寸 XYZ cm / 当前 scale |
|---|---|---|
| thatched_roof_ridge/SM_thatched_roof_ridge | adc6123b-d9ce-4595-aa1d-491bfb435956 / thatched_roof_ridge_model.fbx | 42.15,100.64,16.52 / 1.5,6.3,2.3 |
| thatched_roof_slope/SM_thatched_roof_slope | 204bb2c0-6cd4-4930-9b75-f0df1167d0f5 / thatched_roof_slope_model.fbx | 123.77,120.47,33.77 / 3.2,3.2,2.3 |
| wood_door/SM_wood_door | 9a0e2767-f1cf-4964-bdbe-2db52b27dfcf / wood_door_model.fbx | 30.66,47.05,99.98 / .5,2.2,2.2 |
| wood_floor_panel/SM_wood_floor_panel | 91c736a2-f678-410a-8865-1e76d3a6be60 / wood_floor_panel_model.fbx | 136.32,136.29,17.40 / 2.4,2.4,2.1 |
| wood_stairs/SM_wood_stairs | 77f7c347-95e6-420c-a558-6024087b8e6a / wood_stairs_model.fbx | 120.62,125.25,43.76 / 1.5,1.5,1.2 |
| wood_wall_doorway/SM_wood_wall_doorway | 6dc110ab-9947-4858-a542-21f5fa074e8e / wood_wall_doorway_model.fbx | 58.51,90.87,76.49 / .5,3,3 |
| wood_wall_solid/SM_wood_wall_solid | 5ab09d9b-b28e-4619-a708-59b175a8ffe1 / wood_wall_solid_model.fbx | 59.15,91.78,77.32 / .5,3,3 |
| wood_wall_window/SM_wood_wall_window | 17b709d0-4ce6-4e92-b83c-4009a0047ef4 / wood_wall_window_model.fbx | 64.60,87.39,75.35 / .5,3,3 |

每项 source 的完整相对目录为 `art_source/TASK-004/Tripo/房屋建筑部件/outputs/<ID>/<文件>`。每项 LOD1、simple collision0；房 Actor 11 个独立 Box 覆盖地板、墙、门框和阶梯，门固定打开。Roof / Floor backing 是已有局部缺面修复，不能删去。自然营地 / 序章及四区 `CampaignWorld.cpp:333–339` 同样实例化 `AHearthwardTask028CampHouse`；当前没有另四套区域房型。最小工作先统一同源粗糙木石 / 草顶材质和有限陈设、保持营地与四区正式坐标 / 门口净空，局部 028 缺面仍回原任务；不重铺整张地图。

## 动物、四鱼、围栏与三作物

每个下列 R3 对象均有完整镜像 `art_source/TASK-051/animal_motion/Hearthward/animal_motion_20260930/assets/motion/<slug>/AS_<slug>.blend`、`SK_<slug>.fbx`、`clips/` 和 `SK_<slug>.fbm/`。实际 mesh / Skeleton / PhysicsAsset / PBR 包在 `/Game/Hearthward/Animals/MotionR3/<slug>/`；mesh 为 `SK_<slug>`，Skeleton 为 `SK_<slug>_Skeleton`，动画为 `Animations/AN_<slug>_<动作>_SK_<slug>_Skeleton_Anim`。每件 import_report 均记录材质和 Color / Normal / Roughness 依赖、PhysicsAsset、骨架匹配、时长；本轮没有重新读取各 LOD / convex 形状或逐纹理精修。

| 正式对象 / slug | R3 导入全尺寸 XYZ cm（旧真实引擎报告） | 骨数 / 动作数 | 实际应用与缺口 |
|---|---|---:|---|
| 鹿 / stag_a | 159.61,72.82,180.37 | 34 / 22 | 正式 wildlife deer→stag_a；保留 R3 头重绑 / 步态 |
| 野兔 / hare | 65.00,37.36,77.06 | 25 / 20 | 正式 wildlife；原动作可复用 |
| 雉鸡 / pheasant | 65.00,29.11,59.61 | 52 / 23 | 正式 wildlife；鸟类独立骨架 |
| 公羊 / ram | 156.25,82.57,163.23 | 27 / 25 | 正式 wildlife；与 goat 两套源 / mesh，Owner 仍须核角形 / 剪影 |
| 野猪 / boar | 当前 pig 160.00,68.21,107.40 | 当前 pig 50 / 27 | NatureActor:46、AnimalMotion:42 都 boar→pig；无专用獠牙 / 鬃毛源或包 |
| 狼 / wolf | 160.00,41.67,101.75 | 34 / 22 | 正式 wildlife；BiteShort 消费攻击序列变化 |
| 黑熊 / black_bear | 220.00,85.92,148.98 | 45 / 26 | 正式 wildlife；保留 R3；源帧差异原档明确未逐角完全一致 |
| 赤狐 / red_fox | 166.34,68.35,128.48 | 39 / 21 | 正式 wildlife；不可从原 Tripo 簇数代替 R3 骨数 |
| 山羊 / goat | 160.00,51.53,140.96 | 34 / 26 | 正式 domestic；幼年 .55，产品与 Calendar 沿062；保留完整 Captured / Lead |
| 家猪 / pig | 160.00,68.21,107.40 | 50 / 27 | 正式 domestic；不能为野猪修改共享 pig 外形导致家猪变野猪 |
| 母鸡 / hen | 65.00,42.08,77.55 | 48 / 26 | 正式 domestic；独立鸟 rig，幼年 .55 |
| 鲤鱼 / carp | 60.00,21.30,32.71 | 16 / 16 | R3 演示 / 水生源和独立 fish_carp；正式渔获显示未接 |
| 鲫鱼 / crucian_carp | 30.00,11.87,19.67 | 16 / 16 | 同上，独立 fish_crucian_carp |
| 鲶鱼 / catfish | 90.00,41.86,39.58 | 22 / 16 | 同上，独立 fish_catfish |
| 鳗鱼 / eel | 100.00,42.09,34.51 | 28 / 17 | 同上，独立 fish_eel |

上表共有 15 个玩法对象，boar 与 pig 当前共享源；唯一源为 14 种、303 动作。原 Tripo 静态 `Nature/SM_<slug>` 14 包仍存在，但正式 Nature 动物在 Motion.Configure 成功后隐藏静态 Shape。不要因 048 的默认材质历史说明而宣称当前 R3 全部是默认材质；当前真正 visible mesh 已有 PBR 依赖。

实际碰撞：Nature 的静态 Shape / 头部 sphere 仍承担部分查询，pawn ignore；AnimatedMesh 为 QueryOnly，Visibility block，NoNavigation。`AlignMesh:207` 消费 Juvenile .55。动物身体部位映射包含 name head / leg 的判断；070 任何 rig / 外形改动须使用真实当前 hit 骨与落地检查，不能按高度补造部位。R3 存在 PhysicsAsset 包不等于每个形状已经通过正式战斗碰撞。

野猪最小派生可在完整 pig R3 AS 上独立增加獠牙 / 鬃毛及独立毛皮材质，保留 pig 骨名、层级、根比例、动作与家猪原包；导出新的 boar mesh，复用同 Skeleton / 27 段动作。两个 boar→pig 入口必须同步修正。为避免扩张，先验证 idle / walk / snout strike / collapse / corpse 五条真实连续段、头部命中和落地，再决定其余复用动作是否需变形修正。新派生的造型和源记录未完成，本轮未制作或认可其视觉。

| 其他对象 | 当前来源 / UE / 几何事实 | 实际路径、保留与最小施工 |
|---|---|---|
| 鱼竿 | 原 TASK-004 `妙妙道具/outputs/411884f6-12c4-466f-b0da-a5939596b967/primitive_fishing_rod_model.fbx` 完整镜像可读；Content 未查到对应 rod / UUID 包；尺寸 / 材质 / LOD / pivot 待导入 | Fishing:17 查 tool 槽 GUID / 耐久，1 秒抛竿耗饵、张力、完成入 bag / 磨损真实；无持竿 / 鱼线 / 收竿动作。先导入现有源、以 FishingId / stage 仅显示正常钓鱼中的工具和线 |
| 四鱼渔获 | 上表 R3 / `Nature/SM_carp` 等独立静态包齐全；静态尺寸见048 import-results | `FinishFishing:88–112` 按 species结算，未 Spawn 捕获模型；可复用对应静态 / R3 pose做短时成功展示，失败 / 满包不展示成功，不建立新生态系统 |
| 围栏 | 无独立正式源 / 包；当前 Engine Cube 角柱、横杆、料槽组合；NoCollision | NatureActor:86；保持 pen ID、360 cm横杆范围、移动 / 送料 / 产品。可复用现有木材材质和本地几何派生候选，不能阻断牵引寻路 |
| 野菜 greens | 当前同一地块 Cube、9 对 stem / leaf，无独立 source / Content 包 | 062 已按 Calendar 消费 .3 / .65 / 1几何尺寸 / 位置三阶段；最小新增萌芽 / 茂叶 / 成熟可食叶的真正剪影，沿同 stage消费，不重写成熟数学 |
| 谷物 grain | 同野菜的通用立方体植株 | 需与野菜区别的细秆 / 穗形三阶段派生；不只换颜色；物品 / seed / yield沿现表 |
| 药草 herb | 同野菜的通用立方体植株 | 需低簇叶 / 花序三阶段派生；无现成独立源，允许本地制作带源的候选并经 Owner 核看 |

## UI、环境与声音的实际缺口

UI 直接加载 `Resources/UI/interface.json` 的 `assets` 和外部 PNG（`ScreenWidget.cpp:114–138`），并由 `ScreenContent` 使用物品表 `icon`。现有原图 / 透明清理图和 provenance 已存在，不须重复生成整套页面。当前 shortblade / longblade / spear / hearth_blade 仍 icon=axe；fishing_rod=rope；三 seed=herb；四 fish / egg / milk=meat；leggings=armor。这是可定位的独立图标缺口。最小范围制作带源的刀、长刀、枪、鱼竿、三种种子、四鱼、蛋、奶、腿甲必要图标，并只替这些现存 ID 的图标键；升级件共享该类外形即可，不改数值或装备槽。

地图已使用 `mapTravelIcon / mapLandmarkIcon / mapCampIcon / mapQuestIcon`（ScreenContent:294 / 334）；marker依真实发现 / 激活 / 任务世界位置，NativePaint有地图裁剪。该路径已存在，先核辨识 / 缩放 / 对比度，不重写地图投影或另造标记账本。字体已附 Noto / LXGW OFL。062 的真实 Native Widget offscreen截图可保护中文和布局，不能替代正常键鼠或 Owner 视觉验收。

世界已有 `WorldPresentation` 的 daylight消费、暖日光 / 冷夜 fill、SkyLight recapture、体积雾和手动曝光。ApplyTime沿 WorldClock::DaylightAt；070天气仅消费时间与表现状态，不改变探测 / 农业 / 战斗的亮度规则或收益。仓库运行源码未查到实时 campfire particle、雨特效或环境 / 交互 / 战斗 SFX 调用；目前可见声音接入是固定 WAV voice 与混音设置。静态火堆和房内已有提灯 PointLight 不构成实时火焰完成。

最小火候选可复用现有木石 / 火堆基础，单独隐藏源的静态火焰，使用引擎内置组件制作动态火 / 烟层，光源保持营地暖色并复核夜间可读。只用现有引擎能力，不加插件。雨 / 雾只作为场景氛围候选，须在 camp / town 日夜实渲染核看；当前没有已确认可直接用的环境、工具接触、击打 / 格挡 SFX 源和逐件许可，本轮不以自动波形或静音补齐声音。

## 动作 / ActionId 与真正结算入口

| 行为 | 当前真实入口 / 身份 | 070 可以消费的事件与工程缺口 |
|---|---|---|
| 玩家轻 / 重近战 | CombatComponent::Start:65 产生 ActionId / ActionEpoch，保存 ActionInstance；Move Windup / Active / Recover；Sweep:222–247 在有效窗对子步采样，HitTarget消费ActionId | 动作同步沿055当前裁定；当前扫掠仍角色中心 ±55° / Move.Reach，未取实际武器端点。动画只统一Attack整段变速，不能据此认定全部武器准备 / 接触 / 恢复完成 |
| 玩家弓 / 弩 / 装填 / 处决 | Start的 draw / crossbow / reload 等已有ActionId；处决调用PlayCombat(3,true) | 当前播放基础Attack不等于拉弦 / 装填 / 处决专用动作。箭本体当前是 Engine Sphere .06、NoCollision，命中仍由Projectile移动查询与Event / Epoch结算 |
| 格挡 / 受击 / 倒地 / 死亡 | CombatTarget / CombatComponent已有guard、Memory.HitRemaining、死亡SetCorpse；055候选hit_to_side / fall已导入 | Hero / Brother正式图未根据这些事件分派对应完整动作；候选fall不能直接当死亡保持；由055闭合该契约后070消费 |
| 弟弟协攻 / 狩猎 | GameplayComponent:474–475、CompanionFixture:1019–1021先PlayAttack后直接DamageOpponent；后者内部另生成Damage GUID | 没有完整同一ActionId准备 / 接触 / 恢复路径；070须复用055完成后的实际攻击事务，禁止动画Notify另计算伤害 / 磨损 |
| 普通 / 重兵攻击 | CampaignActor:135直接Damage(Power,body,...,NewGuid)，随后PlayAttack；冷却2.5秒 | 当前无真实武器端点 / 独立动作阶段。与055正式战斗窗口衔接，不在070另造第二套伤害系统 |
| 射手 | CampaignActor:131生成Projectile Event=NewGuid、Epoch、Velocity；随后同PlayAttack | 真投射物不等于弓弦 / 出箭时点贴合。需使放箭视觉消费该实际发射事件，不能预放箭或改变射击规则 |
| 野生动物攻击 / hit / fatal | NatureActor:236增加AttackSequence；AnimalMotion::ObserveNature:275按序列播攻击；生命下降播Hit，fatal播Collapse→Corpse | 已有真实顺序与R3 clip，不重做303动作。boar派生应沿pig SnoutStrikeShort；接触仍须真实事务证据，不能把动画存在当有效接触已验 |
| 玩家采集 / 建造 / 种植 / 浇水 / 施肥 / 送料 / 收获 | Nature::Act:61记PendingAction / target / option / count / epoch；TimedAction::Start / Complete；Nature::Commit实际预检和发布；Building::Complete实际commit材料 | TimedAction当前只有status / startedAt等计时状态，无统一ActionId；Hero正式Dig只观察Resource / Harvest交互目标，Nature面板五秒动作未统一分派手部动作。可消费领域现成pending事务 / epoch与成功commit；避免给每个Tick新GUID冒充一次事件 |
| 弟弟采集 / 照料 / 生产 | Companion Command / Ticket / Execution / Action计时；BrotherAnim只要任何Action Running就Dig；族人劳动Dig / Idle | 新表现消费CommandId / Ticket / 目标 / epoch及成功交付事件；不能让照料 / 修理都当Dig已验收。只补会出现在正式路径的动作，保留原303动物序列 |
| 钓鱼 | FishingId + ActionEpoch + FishingRod GUID；Cast / Bite / Tension / Done；FinishFishing成功才入袋 / 磨损 / 减库存 | 没有独立模型或动作时点。表现按现阶段消费；满袋、断线、取消、换竿 / 读档终止隐藏效果，不新增第二奖励 |
| 固定语音 | Presentation::PlayFixedCue(Id,Event,SharedKnowledge)，PlayedEvents / Epoch、空间通信、Alive检查；读取28组WAV | Cue触发契约存在；声音缺失返回UNPRODUCED。录音继续暂缓，动态AI回复仍文字 |

055 可复用动作：`art_source/TASK-055/motion/knight61.fbx` 与 `/Game/Hearthward/Assets/TASK-055/Motion/{Hero,Brother}/knight61_SK_<角色>_Skeleton_Anim<clip>` 各12包，共24真实AnimSequence。slash=6.583s、hit_to_side=1.25s、fall=3s、swim=5.708s已有真实单节点姿态QA；尚未正式全部绑定。主角正式八段 `/Game/Characters/Hero/AnimationV2/A_Hero_*`；弟弟正式六段 `/Game/Characters/Brother/Animation/A_Brother_*`。运动候选、动作语义 / 源帧窗、实际伤害窗口必须分开记录。

工程参考沿现架构：Epic [FBX Static Mesh Pipeline](https://dev.epicgames.com/documentation/en-us/unreal-engine/fbx-static-mesh-pipeline-in-unreal-engine)说明静态源可分别设置pivot、socket、LOD与简化碰撞，导入后需检查材质连接；[双足 IK Rig 重定向](https://dev.epicgames.com/documentation/en-us/unreal-engine/retargeting-bipeds-with-ik-rig-in-unreal-engine)要求实际 root / chain映射。当前项目已采用原生IK Retargeter，无需新重定向依赖。源上的多边形数不当作UE三角形数，文件名不当作已实现装备类型。

## 可自主推进的最小真实施工顺序

以下是在根登记精确Source / Content / source路径、取本人资产锁后的施工顺序；本轮没有实施。D04视觉方向已批准，候选完成前无需重复问方向；Owner最终看的是可播放、可操作的具体结果。

0. **先完成三组真实样图**：以现有完整源做最小人物、建筑 / 营地、地表 / 故乡候选，补可播放片段和日夜镜头，标明旧包和派生范围。根串行生成可review材料，Owner审具体结果后才批量统一。下面1—6的首件候选可作为样图材料，不能提前把全套资产当已批准的最终视觉。
1. **先闭合现有源最小引用**：以现有箱替自然藏宝箱visual；导入现有鱼竿及必要贴图，给正常FishingId阶段真实持竿 / 线 / 收竿；成功渔获展示引用现有四鱼。输出准确导入尺度、LOD / 材质 / collision记录；保持奖励和Root footprint。验证一条正常钓鱼、满袋失败 / 取消、开箱读档不重复，再实渲染核握点和四鱼辨识。
2. **设施与三作物辨识**：沿原桌 / 火堆 / 床 / 箱基础在既有footprint内做工作台 / 炉 / 锻造 / 架锅 / 仓储 / 治疗区别；三作物各三阶段真实几何剪影，消费现有Calendar stage。先完成可核看的局部派生源和实际包，再统一木石 / 布料材质；不改变配方、容量、成本、成熟或生产数学。小范围测站位 / 交互 / 建造边界和3阶段真实截图。
3. **专用野猪**：从pig R3派生独立boarmesh / 材质，保留家猪原包及pigSkeleton / AnimSequence，两个映射同步；验五条连续动作、幼体 / 家猪不受影响、头部命中与地面站位。复用源确实齐全，缺的是派生结果及Owner视觉通过，不能记成“必须另购野猪”。
4. **武器和装备表现接055闭合路径**：石斧单条实际挂接 / 源窗 / 接触先通过，再导入现有刀 / 枪 / 弓 / 弩候选。盾、腿甲、箭袋和箭弩箭需本地制作带源候选；兜帽 / 胸甲 / 靴需当前61rig适配。每件按本人装备GUID显示，取消 / 磨损 / 转交 / 换装 / 读档清理均消费原契约。070只做资产 / 手轨迹适配；055攻击窗口仍由根先施工，禁止把共享Chop当全武器完成。
5. **射手、族人和建筑一致性**：沿现有Guard / Brother合法来源制作保持既有比例的独立剪影 / 衣料派生，射手配弓 / 箭袋与真正发射事件；族人去重甲并保留运动rig，重兵保留厚轮廓。营地及四区只更新同源材质 / 陈设，保持正式建筑布局。真实日夜 / 连续动作 / 解救与劳动画面由Owner核看；局部变更再测路径、受保护目标和射击。
6. **UI与氛围收口**：仅补上述缺失物品图标并更新其icon映射；地图标记先复用现有。动态火 / 烟与天气采用既有引擎能力，保持WorldClock亮度规则。环境 / 接触SFX先补合法来源与实际成功事务绑定，再试听；录音仍明确暂缓，不用这一项拖住独立资产施工。

步骤0及1—3的首件可以先做可审阅成果，原source在本机完整镜像已有，且不会等待065真人协作体验；批量统一消费三组样图的Owner审阅结论。步骤4的真实攻防阶段消费依055；步骤5的具体派生候选和步骤6的声音仍要独立证据。范围扩大时只新增当前实际需要的精确路径，不开放全部Content / Source。

## 真实收口门槛

- **源门槛**：盾 / 腿甲 / 箭袋 / 箭弩箭、独立射手 / 族人、boar派生、炉 / 锻造 / 架锅、三作物九阶段当前没有已完成的正式独立源与包。允许沿D04做本地派生候选，须保留可继续编辑的源与准确导出 / 导入记录。十武器护具无skin，不能通过单骨挂接把胸甲 / 靴默认算动态穿戴完成。
- **许可门槛**：各TASK004 SOURCE可支持其对应源链，但双角色ZIP / knight61和六武器四护具的逐件账户 / 输入权利仍有缺项；最终074必须核实。不存在来源证据的SFX不能自动宣布合法。当前调查无采购、下载或新增授权事实。
- **运行门槛**：真实UE读取尺寸 / 骨架 / LOD / collision / 材质连接与实际引用，必须由根串行执行。静态包、骨架配对、单节点姿态、offscreenwidget、源码搜索分别只证明各自事实；未证明完整动作贴手、接触时点、整段轮廓或正常操作。
- **Owner门槛**：D04风格审批已完成；派生资产仍需Owner逐类看完整动作、角色 / 兵种 / 族人 / 动物剪影、日夜场景、界面图标，并对实际声音试听。任何保留TEMP和未听录音均不能在070写最终美术 / 声音验收通过。
- **录音门槛**：用户已暂缓，54逻辑 / 28组口径保留。074如需以字幕交付，须由Owner在实际发布候选范围中明确接受；本轮只保留UNPRODUCED，不虚构新的批准。

可复用证据入口：028 `asset-manifest.json`；048 `MAPPING.md / import-results.json`；049 `ASSETS.md / import-assets.json / retarget-assets.json / content-audit.json`；051 `制作说明.md / source_catalog.json / import_report.json / verification_summary.json`；055 `equipment-animation-gap-audit.md / equipment-source-geometry.json / body-weight-scale-audit.md / MOTION_QA.md`；062真实Native Widget截图由根持有。旧清单status与新导入记录错位时，以实际当前引用加对应引擎证据为准，保留原历史档案。
