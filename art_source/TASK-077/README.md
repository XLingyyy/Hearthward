# 山地石堡聚落：制作方向

用户于2026-10-05确认山地石堡风格，后续要求优先交付当前可玩的Windows Demo。当前采用原生石堡布局，并将第二版Marble裁出的103680面立面接到实心主堡。整份生成世界未直接使用；首版及第二版试导入证据保留。用户已授权使用订阅积分，本轮本地裁分、导入和打包未新增积分消费。

## 空间与现有玩法

开场住区的建筑群约112×90米，位于既有住区内；这不是整个480×400米家乡四区的面积。卧室12×10米、层高5.4米，设两张床、遗物包和朝外的窗。2.8米门洞接带顶回廊，4米宽楼梯通向自然地形庭院。城墙保留正面入口、侧向开口和撤离侧门。主堡最高塔约32米，配较低的角塔和回廊形成层次。

建筑采用独立原生 Actor，使用已有石材、木材和家具；不修改营地房屋类和主地图二进制资产。四区据点、巡逻与撤离目标维持原有数据。楼梯、门洞与路线必须在实际地形中验证，不能依据平面示意直接认定可玩。

美术方向：粗砌灰色山石、深色旧木梁、克制的铁件、暖色灯笼；层叠增建的居住翼楼围绕坚固主堡，带生活痕迹。避免整齐对称的宫殿、欧式大教堂、东方宫殿和夸张魔法装饰。夜袭火光、烟雾和破坏应单独制作，保留日后夺回家乡时的正常建筑。

## Marble 首批制作

先试作一个内庭与主堡立面的外观区域。卧室作为另一批独立室内场景，避免让一次重建同时处理封闭室内与整座聚落。上传经验证的白模至 Chisel，保持门洞、楼梯、地面与侧门位置；生成结果只作为视觉资产，经检查后搭配 UE 中明确的碰撞和导航。

外观提示词：

> A grounded mountain stonehold inhabited by a small clan, an irregular fortified settlement built up over generations. Rough grey fieldstone masonry, weathered dark timber galleries, deep-set narrow windows, square watchtowers, a tall asymmetrical keep, layered roofs, covered cloisters and an open lived-in inner court. Restrained iron fittings and warm lanterns. Human-scale residential details beneath an imposing defensive silhouette. Follow the supplied blockout layout and preserve its doorways, stairs, open court and postern exit. Clear traversable ground, no rubble blocking passages. Overcast daylight for readable materials. No people, no text, no heraldic logos, no magic, no cathedral, no palace, no active fire or smoke.

卧室提示词：

> A shared bedroom for two brothers inside a mountain clan stonehold. Two simple separate timber beds, worn blankets, personal belongings, a small wooden chest and a warm metal lantern. Rough grey stone walls, dark exposed timber ceiling beams, a deep window looking toward the courtyard, a broad open doorway leading into a covered gallery. Modest, inhabited and intimate, not royal luxury. Preserve the supplied room dimensions, window, doorway and clear central walking area. No people, no text, no extra doors, no stairs inside the bedroom, no rubble or fire.

导出选择高质量带纹理 GLB，保留原文件与生成页面来源。生成、导出及重试的积分预算以账户页面确认；Pro 网页积分与 API 不互通。不会为了此任务申请 API 付费。

参考：

- https://docs.worldlabs.ai/marble/create/chisel-tools/chisel-basics
- https://docs.worldlabs.ai/marble/export/mesh
- https://docs.worldlabs.ai/marble/support/account-billing

生成页面：https://marble.worldlabs.ai/create/5d40afdc-f4dc-4444-a4a3-6139ccf0717c
可见性：Private。白模保持原比例；全景机位(25,10,-35)米。外观提示词另加Rugged mountain slopes beyond the walls.，用于交代山地环境。

## 导出检查结论（2026-10-06）

修正版世界：https://marble.worldlabs.ai/world/a59e2b3c-e9aa-432a-933e-0baf864f863a 。3500积分高质量网格导出已完成，原始带纹理GLB为130352464字节、592899三角形、8192²内嵌纹理。未重新支付该次导出或购买额外积分。

`prepare_mesh.py` 通过标准任务产物路径裁出296794三角形的建筑区域，并经公开UEClient导入。`bind_material.py` 重新绑定贴图，使用公开接口新增的 `used_with_nanite=True` 选项。框架补丁见 `ue-nanite-binding.patch`，仅影响显式开启该选项的材质绑定，运行游戏不依赖此脚本。

Nanite默认材质警告消除后，实际画面仍有严重几何问题。原始GLB的独立投影同样出现碎裂和天空附着，见 `docs/qa/TASK-077/raw-mesh-projection.png`；此图按三角形中心采样颜色，只用于几何问题定位，不能代表完整贴图渲染质量。UE完整渲染证据见同目录 `rejected-mesh-source-view.png`。

试导入二进制资产隔离在被忽略的 `.agent-local/qa/TASK-077/rejected-content/`，当前游戏未引用。原始下载文件保留在本目录用于追溯，不当作最终美术。碰撞参考 `stonehold-collider.glb` 包含大范围背景地形，未用于游戏碰撞。

游戏当前使用可控的独立建筑组件和原有资源，六栋石屋围绕主堡布置，主线通路独立验证。模型与当前几何均不能声称为最终完成的城堡美术。

## 第二版试导入

第二版 `courtyard-v2-textured.glb` 与 `courtyard-v2-vertex-colored.glb` 已保留，来源见 `marble-courtyard-v2.json`。带纹理版声明 `KHR_materials_unlit`，内庭在无光照视图下可读；外侧仍需裁去天空附着面并补全建筑，未用于正式运行。

复现两组对照时顺序执行：`prepare_courtyard_v2.py` → `import_courtyard_v2.py` 用于5cm聚类顶点色版；`prepare_courtyard_textured.py` → `import_courtyard_v2.py --textured` 用于原始拓扑与UV的带纹理版。两组复用同一标准任务产物和试验资产路径，后一组会覆盖前一组，需先保存对照证据。导入脚本生成默认受光材质，仅用于对照；配合 `verify_fortress_pie.py` 的 `-HearthwardCourtyardInspect` 检查无光照视图，不能直接当作正式夜间材质。

框架公开材质绑定增加显式 `use_vertex_color=True` 支持无纹理顶点色，默认False；与Nanite选项一起保存在 `ue-nanite-binding.patch`。实际引擎导入与绑定成功。当前试验uasset已移到 `.agent-local/qa/TASK-077/courtyard-v2-evaluation-content/`；运行Content不保留未接受资产。截图、格式差异与验收边界见 `docs/qa/TASK-077/REPORT.md`。

## Demo采用的立面

执行 `prepare_facade.py` 后再执行 `import_facade.py`。脚本保留带纹理原文件，仅按区域提取103680面，使用标准任务标识 `TASK-077-facade`。公开绑定选项 `unlit=True` 保留源材质的无光照方式，生成Tint参数供游戏世界时钟调整昼夜。该材质不会独立响应局部灯火；可行走的卧室、回廊、庭院和灯笼周围使用原生受光材料。

运行资产仅为 `SM_StoneholdFacade`、`M_StoneholdFacade_PBR`、`T_StoneholdFacade_base_color` 三项。原生主堡提供背墙、基座和收边；生成网格NoCollision，通路以原生建筑碰撞为准。整合后的原生4项、完整撤离16项再次通过，最新日夜截图和发布包记录见 `docs/releases/demo-20261006/REPORT.md`。
