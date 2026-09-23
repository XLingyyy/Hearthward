# TASK-026｜新营地局部与游戏入口

更新：2026-09-23。引擎：UE 5.8.2。地图：`/Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds`。本轮仅精修 CAMP_A 出生点和周边；远区沿用已有底座，未按营地质量重做。

## 场景与资源身份

- CAMP_A 中心约 `(-980,-750)m`；现有 PlayerStart 约 `(-97600,-75200,16195)cm`。保留中央自然空地和朝湖方向的视线，避开既有 S1 步行路线与出口。
- 在已有 S1 批次上增补 14 棵树、17 块小石和 28 组灌木。前 10 棵塑造营地边缘；后 4 棵在朝湖方向两侧形成前景。范围和精确位置见 `art_source/TASK-026/Rebuild/ReworkV2/camp-detail-manifest.json`、`camp-frame-manifest.json`。
- 从 `origin/main` 的 `73bb10ec4c19260cb72112c7e282a2c29f6c2432` 选择性取回 TASK-004 的 Poly Haven 冷杉、松树源文件（2 个 `.blend`、36 张 4K 贴图，源文件只读）。两份 `.blend` 的 LFS 锁属于 `violet-sept`，026 分支不重复提交这些源包，重建时从该 main 提交获取。用源模型派生两种树形各两件网格，并在营地周边四个 Generated 树木批次替换可见树；原实例编号、变换和成对的树干碰撞保持。冷杉源 LOD 的 UV 原为通用角点属性，已在派生 FBX 中转换为标准 UV 层；新 UE 网格名以 `_CampUV` 结尾。36 张源贴图中使用了 26 张，另有 8 个专用材质；资产清单见[台账](ASSETS_REBUILD.md)。
- 在营地半径 34—112 米的八处林缘增补 48 棵针叶树（冷杉 29、松树 19），保留朝湖方向中央 38 米宽的视线走廊。每棵树有一对稳定实例 token；位置、树种、高度见 `art_source/TASK-026/Rebuild/ReworkV2/camp-conifer-grove-manifest.json`。树干仍是独立 `BlockAll` 代理，可见树冠为 `NoCollision`；落地、水域、坡度、既有树石和步行路线均在生成时检查。
- 可见树与不可见树干碰撞使用成对的稳定实例 token；石块仍是独立的 `BlockAll` HISM 实例。两份 manifest 记录木材/石材的预留资源身份。后续可从命中组件及实例序号映射到 token，再接砍伐/挖掘、掉落和持久化。本轮尚未加入这些玩法，不把普通碰撞误称为可采集。
- 对现有 Generated 批次进行完整批次替换；Authored 批次及 S1 外实例受现有 plan/apply 合同保护。第一轮保存 16 个外部 Actor 包，前景第二轮保存 2 个，均沿用本人的 LFS 锁。计划、应用结果与回滚备份见 `docs/qa/evidence/TASK-026/rework-v2/camp-detail-*`、`camp-frame-*` 和 `Saved/Task026/ReworkV2/backups/`。

## 游戏入口与存档

- Bootstrap 保持默认入口。主菜单“新游戏”打开自然地图，并用 URL `game` 选项选择 `AHearthwardGameMode`、`HearthwardNewGame=1`。地图加载后才创建自然世界进度；玩家从现有 CAMP_A PlayerStart 出生。
- 主菜单“继续游戏”和“载入存档”先读取节点索引。自然地图节点先切换地图，再按 `HearthwardLoad=<SaveId>` 恢复。自然世界与旧伙伴测试进度共享原有存档池，但 `FHearthwardWorldSave.NaturalWorld` 区分模式；旧节点默认值为 false，当前文件格式为 schema 3，schema 2 通过已有认知迁移读取。
- TASK-029 当前工作分支已接入运行时营地伙伴、有限木材点和仓储。自然快照同时保存伙伴位置、携带物资、资源余额、执行委托、认知与已建建筑；`NaturalCompanion` 标记区分旧的无伙伴自然存档，旧档保留玩家物资并补建伙伴。HUD 显示伙伴状态，开放对话、记忆、仓储、建造、制作/维修与技能；旧开发场景路标和敌人仍不自动生成，地图/任务日志继续关闭。最新验证见[AI 接入报告](../../qa/evidence/TASK-029/natural-camp-integration/REPORT.md)。

## 定向验证与边界

- `HearthwardEditor Win64 Development` 构建通过。`Hearthward.Save.FileIntegrityAndSnapshot` 与 `Hearthward.Save.NPCMemoryCompatibility` 两项定向 Automation 测试各 1/1 通过，覆盖自然世界节点同池序列化和旧文件兼容读取。
- 在隔离测试档池中实机完成“主菜单新游戏 → 营地出生 → F6 手动存档 → 返回主菜单 → 继续游戏 → 营地恢复”；存档页同时显示初始自动节点与手动节点。测试池 ID 保存在 `Saved/Task026/CampIntegration/test-pool-id.txt`，避免后续实测继续写入默认档池。
- 首次排查入口时曾在默认 `pool.hws` 创建一个新营地自动节点；原有节点未删除。之后的闭环实测均使用上述隔离测试池。
- 第二轮前景树应用后重新打开独立游戏窗口，出生视角可见左右树群，中央仍保留向湖方向的开口；未发现新树悬空或出生点被阻挡。
- 最终独立游戏出生视角截图：`docs/qa/evidence/TASK-026/rework-v2/camp-final-spawn.png`。此图直开自然地图，仅用于检视地形和植被；主菜单闭环另在隔离档池中实测。
- 新针叶树的 38 个 UE 资产包已落盘，四个既有树批次完成材质与网格切换，新增 48 对实例的 plan/apply 检查通过。材质复用了项目已有叶片远距 Alpha 覆盖率与 Mip Bias 设置；当前独立游戏截图见[营地针叶树出生视角](../../qa/evidence/TASK-026/rework-v2/camp-conifer-grove-spawn.jpg)。此图直开自然地图，未代替主菜单闭环测试。
- 清理无引用的初版冷杉网格后，使用隔离档池实机从 Bootstrap 标题页点“继续游戏”，加载到有新树群的营地，HUD 显示距营地 4 米，截图见[继续游戏画面](../../qa/evidence/TASK-026/rework-v2/camp-conifer-continue.jpg)。此轮只复测继续游戏；新游戏、手动存档和跨菜单恢复的完整闭环见上方先前测试。
- 现有两种源树的树冠较疏，出生视角仍达不到参考图里的密针叶林轮廓。下一轮若要求接近样图，需要更密树冠的常绿树或幼树自然资产；本轮暂停继续堆放同一种稀疏树形。
- 自然地图当前仍使用既有灰色胶囊角色；004 人物资产尚未完成本图接入。远山与远水、全图路线和流送、全图性能、Owner 视觉验收尚未完成，不能据此关闭 TASK-026。
