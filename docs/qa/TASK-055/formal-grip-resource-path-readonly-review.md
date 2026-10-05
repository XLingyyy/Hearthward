# TASK-055 正式持握资源与施工边界只读审阅

2026-10-05。只读 Root `task051` 当前设计、meta、Source、资源目录和来源记录；仅写自身 QA。未生成 mesh/pose，未运行 UE/build/Git，未重复蒙皮复算。

结论：055 仍有可施工工程，当前 `status=Active`、`blocked_by=[]`、整批设计和施工均已授权。失败的有限持握组合只证明这些组合不可用，没有证明必须由 Owner 另购/提供资源，或现有骨架不能制作合格握姿。

| 当前资源/链 | 已确认事实 | 剩余工作的性质 |
| --- | --- | --- |
| 石骨斧 | `/Game/Hearthward/Assets/TASK-028/props/stone_bone_axe/SM_stone_bone_axe` 已有实测 Grip/BladeBase/BladeTip socket。Character.cpp95–103 仍为 hand_r、原 relative rotation、absolute `.7`；没有把未验 palm 候选写入生产。 | 正式持握姿态和 relative transform 尚未通过。既有 socket 可复用，不必重新采购模型或重算源拓扑。 |
| 当前 Hero Attack | `/Game/Characters/Hero/AnimationV2/A_Hero_Attack` 是当前实际使用片段；HeroAnimInstance.cpp100 加载 AnimationV2，Combat.cpp244/282 只有石斧轻击使用已对齐 source phase 与真实刃/socket。重击/其余近战仍走旧角色中心路径。 | 当前真实刃/phase 工程可保留，继续正常输入、动作切换/取消与完整挥击验证；重击/其他武器还需各自真实动作和轨迹接入，不能把已有轻击测试扩大成全部战斗 PASS。 |
| 实际手部骨与轨迹编辑 | 已采到 hand_r、15 右指骨及完整实际影响骨数据，没有独立通过的 grip/finger PoseAsset；本轮目录检索也未见独立握姿命名资源。既有五02半展组合已被真实表面交叉否定。 | 仍具备在已有 AnimSequence 派生资源中制作骨轨迹的工程能力；现有资料不保证某个微调可成功。当前停止候选扫描，后续是否制作正式握姿由 Root 安排，不自动列成外部资源阻塞。 |
| 新 055 Motion | Hero/Brother 各12件已存在于 `/Game/Hearthward/Assets/TASK-055/Motion/{Hero,Brother}/knight61_SK_*_Skeleton_Anim<name>`。四片段单节点加载/骨架/关节 QA 完成；现 Hero/Brother 构造器仍加载旧 AnimationV2/Animation。 | slash 的局部打击窗、hit_to_side 的受击窗、fall 的实际用途和状态接入仍属工程/资产作者验证。没有现成已验弓 draw/release、弩 reload/fire、格挡/翻滚/执行专用段，不可按名称宣称覆盖。 |
| 其他装备 | 070 已实际导入 `.../Equipment/spear/SM_Spear` 与 `.../Equipment/shortblade/SM_ShortBlade` 首批样板；meta 明确尚不生产绑定。其余现有六武器/四护具源及缺盾/腿甲事实见既有审计。 | 样板尺寸、握点/动作、装备实例显示和动态穿戴适配可继续工程验证。生产绑定先由 Root 登记准确共享窗口；缺盾/腿甲属于内容制作缺口，不代表其已有玩法事务代码不能验证。 |
| 装备事务/正常入口 | Character.cpp146 已按本人 weapon GUID、耐久和 ranged 选择显示石斧；gameplay 的 axe/bow/shortblade/longblade/spear/shield 仍沿 APPROVED_TASK_047 数据。Combat draw/reload/projectile 有现成真实状态链。 | 正常键鼠近战/远程、格挡耗尽、破损、转交、付费维修和保存恢复仍可独立推进。当前 native/UI 结果不替代这些正常入口验收。 |

若后续制作正式石斧握姿，最小资源路线为现有 Attack 的有来源派生 AnimSequence，沿当前 Hero 骨架、斧 mesh/socket 和一个通过的持握变换；正式图与 Sweep 必须消费同一片段。本报告没有创建或指定新姿态。现引擎公开 [IAnimationDataController::SetBoneTrackKeys](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/IAnimationDataController/SetBoneTrackKeys) 可编辑既有骨轨迹，支持保留位置/尺度并更新旋转；没有新增 IK 框架、购买服务或新 runtime 类的既定必要性。最终贴合能否成立仍须作者资源和实际接触/近景证明。

授权/provenance 须按资产分别记录：

- 石骨斧有具体来源授权事实：`art_source/TASK-004/Tripo/妙妙道具/SOURCE.md` 记录 2026-09-22 Owner 提供/授权四张输入、付费 Tripo 身份及生成输出公开入库，四件明确包含石骨斧。官方 [Tripo 商用说明](https://www.tripo3d.ai/help/privacy-policy/how-to-use-tripo-models-commercially) 列付费生成模型的使用、修改与分发权；不能把这份四 props 证明扩展给其他武器或人物 ZIP。
- 新 knight61 的 `art_source/TASK-055/motion/knight61_motion_source.json` 只确认已有本地 ZIP、FBX member、转换 ID、目标骨架与 no_new_purchase；`license_status` 明确未核实。当前人物/动作工程接入有 TASK031 用户指令与本批055施工授权，输入权利/生成时账户事实等发行 provenance 不由工程测试推导。该台账归074补齐；不据此将本地候选工程适配全部暂停。
- 新武器十件源的旧 task.json 成功、task ID/PBR/面数不能证明各自输入权利和付费身份；070已有来源审计明确缺口。登记标准 meta 或公开成功导入不会补上许可事实。

必须 Owner 的部分为：070三组正式风格和实际资产视觉验收；073真人体验/1v3；个人输入权利和生成账户等无法从仓库证实的事实确认；最终字幕版发行范围及发布授权。录音沿用户指令暂缓。Root 的精确 allowed_paths 登记、LFS 锁、串行 UE 作者/构建和正常功能验证是当前施工流程，不能包装成需要用户再次批准整套设计。

证据：`docs/planning/TASK-053-074/TASK-055.md`、`DECISIONS.md` D04/D06、`docs/tasks/TASK-055.json`；当前 Character.cpp95/146、HeroAnimInstance.cpp94/169、BrotherAnimInstance.cpp75、CombatComponent.cpp244/600；`formal-motion-binding-audit.md` 的资产清单（早期源码状态已被当前上述文件取代）、`MOTION_QA.md`、`stone-axe-half-section-combination-review.md`；070 `first-weapon-actual-import-review.md`、`equipment-import-hold-preflight.md` 与 task meta。
