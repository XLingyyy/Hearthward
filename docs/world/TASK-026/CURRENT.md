# TASK-026 自然世界当前状态

更新：2026-09-22。本文记录分支 `codex/TASK-026-natural-world` 上的本地灰盒实现，不代表main已集成、Owner已验收或最终地理设计已批准。

## 打开与浏览

在 UE 5.8.1 中打开 `Hearthward.uproject`，加载 `/Game/Hearthward/World/Natural/L_NaturalWorld` 后使用PIE浏览。地图级 `BP_NaturalWorldGameMode` 复用现有 `HearthwardCharacter`，但使用空白HUD，不生成020开发场景、伙伴或仓储内容，也不启动本地模型。

现有Bootstrap与默认新游戏入口没有修改；自然世界尚未接到正式游戏流程。

## 序列化实现

| 项目 | 当前值 |
|---|---|
| 地图 | `/Game/Hearthward/World/Natural/L_NaturalWorld` |
| 模板 | UE 5.8 `/Engine/Maps/Templates/OpenWorld` |
| Landscape | 2017 × 2017采样，XY间距2 m，边长4032 m；验证时加载36个代理，完整创建记录为64个代理 |
| 分区/外部包 | World Partition + OFPA；175个外部Actor包、14个外部对象包 |
| 路线声明 | 4.8 km矩形主环线、北向和东向两条支路 |
| 水系 | 一条弯曲河带、两处36 m宽灰盒浅滩 |
| 地标 | `SentinelRidge`、`TwinFangs`、`RiverCrown` |
| 候选空地 | `CAMP_A`：(-850 m, -650 m)，半径220 m；`HOMELAND_B`：(820 m, 620 m)，半径240 m |
| 重复物 | HISM批次：760树干、760树冠、900灌木、3200草簇、420岩石，以及地形/河床/水面/路线/地标实例 |

`scripts/world/TASK-026/create_natural_world.py` 固定种子260921，生成脚本在目标资产已存在时会主动失败，不能直接作为增量重建工具。后续若要重生成，必须先在独立工作树和明确授权下处理已有二进制资产，不能覆盖当前地图。

## 资产与来源

本任务只读复用以下已入库源贴图并将UE派生资产保存到026专用目录：

- `art_source/TASK-004/polyhaven/地表/草地/textures/grass_ground_diff_4k.jpg` → `T_GrassGround_D`
- `art_source/TASK-004/polyhaven/地表/土/textures/dirt_diff_4k.jpg` → `T_Dirt_D`
- `art_source/TASK-004/polyhaven/地表/岩面/textures/rocky_terrain_diff_4k.jpg` → `T_Rock_D`

材质为 `M_GroundGrass`、`M_Soil`、`M_Rock`、`M_Water`、`M_Foliage`、`M_Bark`、`M_Trail`。树木、灌木、草、岩石、浅滩和地标目前使用Engine BasicShapes，不是TASK-004自然模型的完成适配；不得把本分支表述为004整单交付。

## 已验证

2026-09-21在本机 UE 5.8.1 中通过脚本执行真实PIE，27项布尔检查全部通过：

- 地图和12个保留资产存在，地图可重开；
- World Partition代理、外部Actor目录/包数和项目依赖闭包符合本任务路径；
- 独立浏览场景只有一套玩家输入，仓储为空、本地模型未启动、没有保存伙伴夹具；
- 生成批次在出生点流送；角色可落到地面、使用现有Enhanced Input连续行走超过10 m；
- 西侧浅滩可连续行走超过10 m；
- 5个固定观察点已截图。

原始报告与截图见[验证说明](../../qa/evidence/TASK-026/README.md)。报告生成后资产没有再修改；证据同步只把被忽略的 `Saved/Task026` 输出复制到受版本控制目录。

## 已知问题与未完成验收

- 五张截图只是运行证据。人工查看可见基础形体树冠悬空、树干明显倾斜、植被/岩石重复、方块地表接缝和大面积空白；A4明确未通过。
- A1尚缺比例总体图、五类自然分区边界、可行走面积计算方法与坡度说明。创建报告中的9 km²只是一项脚本声明。
- A3只验证出生点短距离和西侧浅滩；4.8 km主环线、两条支路、东侧浅滩及边界没有连续实走录像。
- A5没有Standalone跨区往返、加载/卸载和明显停顿记录；当前PIE短测不能替代流送验收。
- A6没有RTX 4060 Laptop 8GB、1080p Medium三类场景60秒采样，也没有P95、显存或纹理池数据。
- 未执行Shipping、打包、干净克隆、第二机器、独立评审或Owner验收。
- Git LFS已配置，但2026-09-22远端锁查询因本机HTTPS凭据不可用而失败；提交只能证明指针/对象进入分支，不能证明锁流程完成。

## 建议接手顺序

1. 从本Pull Request检出分支并确认LFS对象完整，再在UE中重开地图运行 `verify_natural_world.py`。
2. 优先替换悬空基础形体、处理地表拼接和自然过渡；每轮只改026路径内资产。
3. 补总体图、五类分区和可行走面积说明，再录制主环线/两支路/双浅滩连续实走。
4. 用Standalone完成流送往返和目标硬件性能采样；最后由Owner做视觉验收。

