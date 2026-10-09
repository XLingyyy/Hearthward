# TASK-100 四区建筑制作源

`ZoneArchitecture.blend` 是通过本机 Blender MCP 制作的四类建筑源。模型以米建模，原点位于地面中部，X 轴为穿行方向；源场景为方便检查，四个模型依次沿 Y 轴相隔 13 米摆放，导出时各自在零点。

制作脚本为 `author_zone_architecture.py`。重建时先打开原有 `WorkshopEntry.blend`，再通过 Blender MCP 执行此脚本；它新建独立场景并另存 `ZoneArchitecture.blend`，保留原样板。不要在已有同名 Zone 材质的源文件上重复执行。

FBX 输出采用 GameFactory `task_output_dir('Hearthward', '3d_object', 'TASK-100-zones', run_id='20261009')` 分配目录。每个文件包含一个可见模型和独立 `UCX_<模型名>_<序号>` 凸碰撞块。`meta.json` 记录四个 `SM_*_path`；通过 `UEClient.assets.import_prop` 的源描述符导入，目标为 `/Game/Hearthward/Assets/TASK-100/Zones`，选项 `generate_collision=True, combine_meshes=True`。

`zone-mesh-spec.json` 记录几何尺寸；`zone-placement.json` 记录正式自然地图上的 16 个场景落点。七个落点因坡度调整了展示位置，原游戏配表、旗点、巡逻和任务坐标保持不变。资产使用现有 TASK-096 世界坐标石材／木材；作坊内复用 TASK-098 的设施模型作场景陈设。

材质、模型和碰撞为本次自行制作；不含下载模型、付费生成或新增第三方许可。现有复用资源沿用各自的来源记录。验收与限制见 `docs/qa/TASK-100/ZONE_INTEGRATION_20261009.md`。
