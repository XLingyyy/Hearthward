# TASK-026 Rebuild资产台账

所有UE派生资产位于 `/Game/Hearthward/Assets/NaturalWorld/Rebuild`，源文件归 `art_source/TASK-026/Rebuild`。TASK-004源文件只读，旧自然地图只读。

| 输入 | UE派生/用途 | 许可与状态 |
|---|---|---|
| TASK-004 蓝花楹纹理；Poly Haven jacaranda_tree官方FBX | Meshes/SM_Tree；树干/树枝/叶片材质；Nanite Preserve Area | [Poly Haven](https://polyhaven.com/a/jacaranda_tree)，CC0；TEMP_VISUAL |
| TASK-004 shrub_01纹理；同资产官方FBX | Meshes/SM_Shrub；Masked灌木 | [Poly Haven](https://polyhaven.com/a/shrub_01)，CC0 |
| TASK-004 tree_stump_01纹理；同资产官方FBX | Meshes/SM_Stump；简单碰撞 | [Poly Haven](https://polyhaven.com/a/tree_stump_01)，CC0 |
| TASK-004 Sketchfab中岩石source/stone assets.obj与d_m.jpg | Meshes/SM_Rock；Nanite、NDOP26碰撞 | 沿用004来源记录；原页面/作者缺项仍待补齐，未宣称发行许可审查完成 |
| TASK-004 grass_ground、dirt、grass_medium_01纹理 | Landscape草地/泥土、草卡图集 | Poly Haven CC0；法线方向与颜色空间已适配 |
| Poly Haven rock_3 2K纹理 | 三向投影岩壁 | [官方来源](https://polyhaven.com/a/rock_3)，CC0；下载URL在源目录source.json |
| 026程序生成高度/权重/草密度/水面法线 | Textures/T_HeightRG、T_Biomes、T_GrassDensity、T_Water_N | 任务自制；2017²数据源保留 |
| 026程序生成16段河道、2湖、海面 | Meshes下各GLB对应网格与Single Layer Water材质 | 任务自制；纠正轴交换后的三角形绕序 |
| 026三片交叉草卡、隐藏树干碰撞代理 | GrassCards、TrunkCollision | 任务自制；草卡引用004图集的单簇UV；碰撞代理不作可见场景 |

官方FBX直接下载避免额外安装Blender或GPU模型环境，无付费生成。未修改Source、Plugins、Config、uproject或004原件。

Nanite网格依赖DX12/SM6。此任务通过独立启动脚本设置进程级参数，不改变旧玩法的渲染配置。SM5替代网格仅用于最初通行诊断，不能作为最终植物外观。

最终适配补充：叶片遮罩Mip Bias=-2/Clip=0.2，草卡关闭Nanite并使用双面植被着色；岩石九点足迹贴地。草卡、河湖面由任务脚本生成，不由ImageGen生成。相应调整只在Rebuild派生资产内。
