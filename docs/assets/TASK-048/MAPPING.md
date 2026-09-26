# TASK-048 自然内容资产对应

Owner 已确认：本单复用现有动物模型与临时表现，后续再精修。

已将 TASK-004 的 14 个既有动物静态 FBX 导入 `Content/Hearthward/Nature/`；野猪临时复用家猪，鹿 B 保留为源文件备用。源文件未修改。导入操作使用独立编辑器，不修改自然地图资产。

| 导入资产 | 源文件 | 当前用途 |
|---|---|---|
| `SM_pig` | [pig](../../../art_source/TASK-004/Tripo/动物/outputs/0059d8b0-37bf-4db1-9336-41c0debd81e0/pig_model.fbx) | 野猪／家猪共用临时外形 |
| `SM_wolf` | [wolf](../../../art_source/TASK-004/Tripo/动物/outputs/4ca01192-6ab4-4df4-bb58-b1928304cd8b/wolf_model.fbx) | 对应物种临时外形 |
| `SM_hare` | [hare](../../../art_source/TASK-004/Tripo/动物/outputs/588471a5-d278-414e-bd5b-de3209def6d4/hare_model.fbx) | 对应物种临时外形 |
| `SM_pheasant` | [pheasant](../../../art_source/TASK-004/Tripo/动物/outputs/59a5036b-7578-46b1-8716-558e6df2b8f6/pheasant_model.fbx) | 对应物种临时外形 |
| `SM_stag_a` | [stag_a](../../../art_source/TASK-004/Tripo/动物/outputs/6cda5dcd-6fcb-4588-a12f-820a1393de76/stag_a_model.fbx) | 鹿 |
| `SM_black_bear` | [black_bear](../../../art_source/TASK-004/Tripo/动物/outputs/7f5206d7-7c0a-41da-a755-7250f789f84b/black_bear_model.fbx) | 对应物种临时外形 |
| `SM_ram` | [ram](../../../art_source/TASK-004/Tripo/动物/outputs/8334bd78-63dd-4981-917c-e2430e97a48d/ram_model.fbx) | 对应物种临时外形 |
| `SM_carp` | [carp](../../../art_source/TASK-004/Tripo/动物/outputs/b2ce3e41-e542-4b01-978c-2ce448eaf8b0/carp_model.fbx) | 鱼类展示源模型 |
| `SM_hen` | [hen](../../../art_source/TASK-004/Tripo/动物/outputs/bcc1c840-fd3e-4e0a-b429-d3b9e27ee67e/hen_model.fbx) | 对应物种临时外形 |
| `SM_goat` | [goat](../../../art_source/TASK-004/Tripo/动物/outputs/d005ba13-ec3b-4bc8-8bd4-dcc4a3c6e639/goat_model.fbx) | 对应物种临时外形 |
| `SM_catfish` | [catfish](../../../art_source/TASK-004/Tripo/动物/outputs/d8fb01be-19a2-4706-88fd-eb16471c8c27/catfish_model.fbx) | 鱼类展示源模型 |
| `SM_crucian_carp` | [crucian_carp](../../../art_source/TASK-004/Tripo/动物/outputs/e193cb21-fa34-4bb1-b522-c0198e186127/crucian_carp_model.fbx) | 鱼类展示源模型 |
| `SM_eel` | [eel](../../../art_source/TASK-004/Tripo/动物/outputs/e473cc70-a885-4fc7-abe4-b64643fb1bbf/eel_model.fbx) | 鱼类展示源模型 |
| `SM_red_fox` | [red_fox](../../../art_source/TASK-004/Tripo/动物/outputs/e976d24b-8ce3-4b1f-9cbd-e23883468b83/red_fox_model.fbx) | 对应物种临时外形 |

## 表现边界

- 动物移动由游戏 Actor 驱动；尚无逐物种骨骼动作、皮毛材质和幼体模型精修。
- 头部使用独立命中碰撞区，正式骨骼与精确体型适配留待后续。
- 鱼点提供张力操作与四种独立渔获；已导入鱼模型，不声称完成水下生态或游泳动画。
- 资源复用既有树、石与灌木模型；田地、围栏及宝箱使用运行时组合几何，成熟与剩余库存由交互界面标示。
- 动物模型本轮未导入嵌入材质；使用引擎默认材质。

导入记录：[import-results.json](import-results.json)。新增资产锁：[locks.json](locks.json)，交接前保留；没有解锁其他任务资产。

源授权和历史来源见 [SOURCE](../../../art_source/TASK-004/Tripo/动物/SOURCE.md)。
