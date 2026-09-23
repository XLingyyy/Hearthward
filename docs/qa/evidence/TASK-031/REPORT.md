# TASK-031 验证记录

2026-09-24，Windows 11、UE 5.8.2、RTX 4060 Laptop 8 GB。任务基线 `be29573`，实施计划提交 `ea15602`；最终测试代码提交见同目录 `tested-revision.json`。本次使用用户明确提供的两个 ZIP，没有重新生成模型。

## 交付

- 弟弟：`Resource/Tripo/弟弟/dark+fantasy+armor+3d+model.zip`，完整 61 骨骼，包含 Idle、Walk、Run、Dig、Chop、Wait、Jump、Climb 八段 24 fps 动作。替换此前缺失腿部骨骼的候选导入，角色显示高度 160 cm。
- 主角：`Resource/Tripo/主角/medieval+knight+3d+model.zip`，完整 61 骨骼，原包没有动画。使用 UE 原生 IK Rig / IK Retargeter 重定向上述新动作，角色显示高度 180 cm。
- 两套新材质接入 base color、normal、roughness、metallic，限制运行纹理为 2048，并保存 Skeletal Mesh usage。模型本身朝 +Y，角色组件旋转 -90°。
- 两者的 FBX 根骨骼具有 100 倍单位缩放。重定向时使用单位归一化代理和动作，再恢复目标骨架单位，避免手臂动作幅度缩小。原始模型骨骼不作破坏性替换。
- 主角：待机、行走、冲刺、起跳、下落、落地、攻击、采集接原生动画代理。攻击从 Chop 裁切，跳跃分拆为起跳/空中/落地，位移由角色胶囊控制。
- 弟弟：待机、移动、等待、真实工作计时与已提交的战斗攻击驱动动画。Jump、Climb 作为已导入动作保留，未赋予 NPC 跳跃或攀爬能力。

## 结果

| 验证 | 结果 | 证据 |
|---|---|---|
| HearthwardEditor Win64 Development | PASS | build.json |
| Hearthward.Companion 原生测试 | 3/3 PASS | native.json |
| Python 工具测试 | 31/31 PASS | python-tests.txt |
| 主角真实 PIE、Enhanced Input 与玩法 API | 26/26 PASS | hero-pie.json |
| 自然地图新游戏、弟弟执行与八段动作 | 22/22 PASS | brother-pie.json |
| 新骨架重定向、动作采样、材质使用标记 | PASS | retarget.json、motion.json、materials.json |

主角检查包含真实行走关节位移、支撑脚高度、冲刺、起跳/下落/落地、采集暂停/移动打断/一次结算、重复攻击、协攻伤害与动作、死亡清理。弟弟通过结构化委托确认后实际取得并入库两份木材，资源余量从 16 减为 14；这次没有重新测试 LLM 自然语言识别。

已人工查看同目录四张实际渲染截图，确认两种外形、材质和采集/攻击蒙皮变形。图中的灰盒是开发验证目标或原有伙伴资源点。未将本轮结果表述为整个 demo、全地图或 Shipping 验收。

## 调试记录与限制

首次新材质未保存 Skeletal Mesh usage，导致运行时回退为灰材质；已修复并重新运行实际渲染。FBX 根骨缩放造成初版重定向幅度过小，归一化后修复。新建动作默认 30 fps 与原始 24 fps 时长不兼容触发压缩断言；最终动作通过复制 24 fps 模板保留正确采样率，再写入裁切轨道，后续 Editor/PIE 启动均通过。

早期验证脚本存在截图 API 不匹配、读取未暴露属性以及按墙钟固定延迟判断下落的问题，已修正后复测。采集复测记录最终最近目标和真实结算反馈；较早一次选择了邻近仓储目标，不能作为采集通过证据。旧失败记录留在本机 Saved，提交的结果均为最终运行。

公共导入器的组合结果曾因没有生成 PhysicsAsset 报失败，实际 SkeletalMesh/Skeleton 和动画已导入。当前沿用角色胶囊碰撞，不提供布娃娃和逐骨物理。披风随骨骼蒙皮，没有布料模拟；采集和攻击尚未挂接持握工具，动作与灰盒目标可穿插，未做手脚 IK 接触修正。自然地图没有战斗敌人，攻击在独立开发夹具验证。未执行本轮物理键鼠验收、Shipping 打包、第二台机器或全地图长时性能验证。

仓库校验结果见 `repo-validation.txt`：既有 TASK-027 元数据缺项单独记录，不为本任务修改旧任务归属。用户本地 EngineAssociation、IDE 生成文件不包含在提交中。

## 复现

使用 GameFactory Python 环境，从 GameFactory 根目录运行 `Hearthward-ai-npc-fix/docs/qa/evidence/TASK-031/run_validation.py build|native|hero|brother`；每次 PIE 使用隔离存档池，运行前将脚本提示的旧结果目录归档。主角与弟弟实机验证按顺序运行，避免争抢 GPU。脚本通过公开 UEClient 管理自身编辑器。资产重定向与裁切实现保留于 `prepare_characters.py`、`finalize_motion.py`；这些是编辑资产的工程脚本，运行前遵守资产锁和关闭编辑器要求。

实现参考 Epic 官方 [FBX 动画导入](https://dev.epicgames.com/documentation/unreal-engine/importing-animations-using-fbx-in-unreal-engine)、[IK Rig 重定向](https://dev.epicgames.com/documentation/unreal-engine/ik-rig-animation-retargeting-in-unreal-engine) 与 [UE 5.8 重定向操作栈](https://dev.epicgames.com/documentation/unreal-engine/retargeting-operation-stack-in-unreal-engine-5-8)。
