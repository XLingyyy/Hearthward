# TASK-026 自然地图重建

更新：2026-09-22。工作目录 `G:/GameFactory/Hearthward`；分支 `codex/TASK-026-natural-world-rebuild`；基线 `origin/main 4114556`（PR #25 已合并）。当前用户授权接手并允许重构，本轮状态 Active，A1—A8 尚未全部验收。本轮已获用户授权提交并推送任务分支；不包含合并，任务保持Active。

## 打开

推荐从 GameFactory 根目录运行：

```powershell
.venv/Scripts/python.exe -X utf8 Hearthward/scripts/world/TASK-026/open_rebuild.py
```

该入口通过公开 UEClient 启动独立地图，以进程参数启用 DX12/SM6，未修改项目 Config 或其他工程。首次切换 SM6 需要编译着色器。普通 SM5 启动会显示 Nanite 的替代网格，树冠外观失真，不能用作本图视觉验收。加 `--game --medium` 可启动未打包的 Standalone 浏览模式并应用Medium设置。该设置已在独立运行日志中确认，尚未形成正式性能报告。

地图：`/Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds`。编辑器 Play 后 WASD 移动、鼠标转向。地图复用原浏览 GameMode 和现有角色，默认速度350 cm/s。Bootstrap默认入口保持原状；正式游戏流程尚未接入新地图。

## 地形与布局

- World Partition，2017×2017高度采样，2米间距，4032米边长，64个Landscape分区及根Actor。
- 高度范围 -26.08 至712.91米；西部林地山脊、中部河谷、东部开阔台地、北部山峰、东岸海崖。
- 两座不规则岸线湖泊，16段连续河道、台地跌水段与海面；两处天然跨岸连接位于Y=-1120米与680米。
- 营地候选空地 CAMP_A=(-980,-750)米、半径110米；故乡候选空地 HOMELAND_B=(1090,450)米、半径160米。仅整备自然地面，无人文设施。
- 按2米采样、坡度≤32°、排除水域、四邻接最大连通域估算，可通行连通地面约9.975 km²。这个数字是几何分析，实际通行以角色测试为准。
- 8米采样A*规划主环线11.264 km及两条支路；清除路线附近树干和岩石，路线仅用于验证，没有绘制人工道路。

总体图：[rebuild-masterplan.png](rebuild-masterplan.png)。坐标采用UE的厘米单位；图中以米标注，北向+Y在图下方。

## 场景资产

21,387棵树、5,779组灌木、6,127组岩石、99个树桩、56,111个离散草簇；另由Landscape Grass在镜头附近生成地表草。上述为生成记录中的实例数，不表示所有实例同时常驻。树、岩石等按252米单元拆分HISM批次，保留World Partition流送。

蓝花楹树、灌木、树桩和地表素材源自已交付的TASK-004；所需FBX交换文件取自同一Poly Haven资产官方API。岩石使用TASK-004的Sketchfab中岩石OBJ。新增灰色岩面贴图rock_3、任务专用水面/草卡/碰撞网格和派生源归026。详细归属见[资产台账](ASSETS_REBUILD.md)。蓝花楹仍为TEMP_VISUAL候选植物，不确定最终植物群。

树木源网格约386万三角面，使用Nanite Preserve Area；未按完整源网格逐棵常规渲染。原常规LOD减面耗时约27分钟且提交内存超过20GB，因此改用Nanite。贴图常规上限2048，数据贴图关闭sRGB；树干为不可见简单碰撞代理，岩石/树桩为简单碰撞。

陡坡使用三向岩面投影；河湖使用Single Layer Water材质。草源图是纹理图集，草卡UV只选其中一簇，避免把整个图集贴成碎片。远距Alpha贴图保留覆盖率。

## 已定位并修正的问题

1. Open World模板的额外“Flat Middle”编辑层覆盖新高度图，造成局部高度与源数据相差173米。关闭新图继承的额外层后，导出高度与源场的最大差为1.56厘米，符合16位量化误差；9个碰撞下射线采样与放置高度一致。
2. 直接移动外部PlayerStart未标记其包为修改状态，重开后恢复原点。移动前调用modify并保存外部包，重开与角色落地通过。
3. GLB交换Y/Z改变手性，水面三角形绕序同步翻转，修正水面朝向。
4. 默认SM5使用Nanite替代网格；独立入口改用DX12/SM6。SM6复核还发现细叶透明纹理的远距采样丢失，叶片遮罩Mip Bias=-2、Clip=0.2后远景树冠恢复。六三角面草卡关闭自动启用的Nanite，使用向上法线和双面植被着色。
5. 岩石扫描模型横向跨度较大，中心点贴地会令下坡端悬空。最终按旋转后足迹的九点地面最低值，再嵌入高度15%；重放脚本与增量修复一致。
6. GLTF导入默认给水面启用Nanite，与SingleLayerWater不兼容；19个水面网格已改为常规网格，生成入口同步修正。
7. 模板的旧平坦Landscape HLOD已从新地图删除；Landscape保持常载以保证远山连续，植被和岩石保留空间流送。新的远景HLOD尚未制作。

## 真实验证边界

SM5首次新图测试：默认速度连续步行424.68米/120秒通过。植被增密后的复测：连续步行424.64米；北浅滩178.22米/51秒、南浅滩178.25米/51秒均通过，无卡死或穿底。每个跨岸测试先独立定位起点，跨岸期间使用现有Enhanced Input连续步行，无传送。六个固定观察点使用明确标记的观察传送，不计入路线里程。

地图重开、单一玩家输入、出生点地面高度、正常落地、空仓储、不启动本地模型、无伙伴夹具、自然批次加载通过。证据：[首次步行](../../qa/evidence/TASK-026/rebuild/first-walk-425m.json)、[增密后步行与双浅滩](../../qa/evidence/TASK-026/rebuild/sm5-movement-and-fords.json)。这些结果仅证明受测版本的局部通行和隔离。

SM6复测：424.68米/120秒通过，13项局部检查通过；[报告](../../qa/evidence/TASK-026/rebuild/sm6-movement.json)。随后调整叶片/草地材质、降低岩石贴地高度、修复水面渲染；最终35秒局部复测走过124.98米，13项检查通过，运行截图可见林地草簇。见[最终局部复测](../../qa/evidence/TASK-026/rebuild/final-local-walk.json)与[运行林地截图](../../qa/evidence/TASK-026/rebuild/runtime-forest.png)。

完整主环线、两支路、Standalone跨区往返、1080p Medium平均FPS/P95和Owner视觉验收尚未完成。PIE Slate帧间隔含编辑器与截图开销，不换算成Standalone性能PASS。当前不标记TASK-026完成，不把004部分适配等同于004整单交付。

## 复现与继续工作

先在宿主Python运行 `prepare_rebuild.py`、`prepare_dressing.py`；引擎内按 `rebuild_terrain.py` → `rebuild_assets.py` → `rebuild_dressing.py` 执行。已有新图增量修改不必重复导入高度。草卡/水面网格变更后使用对应重导入脚本。最后执行 `polish_rebuild_surface.py` 保存Alpha覆盖率与海面修正，再执行 `refine_rebuild_foliage.py` 保存细叶采样、草材质及岩石足迹贴地修正。`fix_rebuild_water_rendering.py` 修复已导入水面网格的Nanite设置；`rebuild_grass_maps.py` 重新赋予Landscape材质并保存其状态（该步骤未证明原生GrassMap问题已消除）。`finalize_rebuild.py` 固化Nanite材质用法，`capture_rebuild.py` 留存原始编辑器截图。所有脚本在 `scripts/world/TASK-026/`，仅操作Rebuild路径。

原生编辑器控制台示例：

```text
py "G:/GameFactory/Hearthward/scripts/world/TASK-026/verify_rebuild.py"
```

测试参数在 `art_source/TASK-026/Rebuild/verification_options.json`，完整路线在同目录routes.json。Saved下报告为临时结果，交接时复制确认过的JSON与截图到docs证据目录。

接手时的7份任务文档和API_GAP本地改动已保存在Git stash `task026-pre-handoff-local-documents`。旧L_NaturalWorld和violet-sept持锁资产未修改。新图和派生资产的远端锁记录放在rebuild证据目录，外部Actor包与材质/网格连带修改一起管理。真实Issue与独立评审仍缺，不伪造流程字段。

## 工程参考

采用Epic的[Landscape尺寸建议](https://dev.epicgames.com/documentation/unreal-engine/landscape-technical-guide-in-unreal-engine)、[World Partition](https://dev.epicgames.com/documentation/unreal-engine/world-partition-in-unreal-engine)与[Nanite内容工作流](https://dev.epicgames.com/documentation/unreal-engine/working-with-naniteenabled-content)。参考这些技术约束不代表场景已经通过性能或视觉验收。

## 当前视觉与流程限制

地形比例与自然分区已落地，仍有大尺度山体轮廓偏圆、树种单一、岸线细节和瀑布泡沫简单的问题，未达到参考图的最终美术密度。最终PIE林地截图可见草簇；出生空地草密度较低。原生Grass组件返回实例计数0，尚不能据此确认Landscape Grass运行生成状态，独立运行的草地覆盖仍需专项复核。

仓库校验检查了本轮1509条变更路径，未发现任务范围越界；整体返回FAIL，原因仅为Active任务缺独立reviewer和真实Issue URL。保留这两项缺口，不填造数据绕过校验。
