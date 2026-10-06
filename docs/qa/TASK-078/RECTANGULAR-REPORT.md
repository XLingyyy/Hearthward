# TASK-078 无黑雾矩形地图与左键拖动

当前火焰尖端更新、Source／DLL指纹和本地包以 [三角火焰尖端报告](FLAME-TIPS-REPORT.md) 为准；本文件保留此前版本证据。

本文件保留此前地图布局版本的历史指纹与结果；当前用户四项bug修复、源码验证与可玩包以 [地图与传送修复报告](TRAVEL-FIX-REPORT.md) 为准。

本报告是移除四周留边之前的矩形版本历史记录。当前实现、可玩包与重新验证结果见 [全屏地图报告](FULLSCREEN-REPORT.md)；下文“当前”均指本历史报告受测版本。

2026-10-06。用户在已交付半径1000米地图之后追加五项要求。本轮在同一本地分支 `codex/map-1000m-heading` 继续实现，基线仍为 `ca21120cef4421d039c44248a0cfa5043c248feb`；代码未提交、推送或公开发布。当前结果以本报告及 [Source／DLL／资源指纹](rectangular/source-manifest.json) 为准，首次圆形版本的报告与证据保留为历史记录。

## 当前行为

地图取消黑雾底幕与圆形遮挡，地图区域为3000米长、2000米宽，即此前2000米直径的1.5倍与1倍。地图面积为6平方公里，边界是完整长方形，四角不会再被圆形裁切。

初始视图靠近玩家，默认缩放1.15，仅显示矩形的一部分。按住鼠标左键拖动，滚轮以鼠标所指位置为中心缩放，也可用方向键平移；缩小到最小倍率可查看整个矩形。控件点击仍执行地点选择、传送或返回，不启动拖动；释放左键、失去鼠标捕获或切换页面都会结束拖动。缩放与平移均有边界约束。

地图在同一屏幕坐标系内按横纵等比例投影，依据窗口尺寸扩大可用视口，支持16:10、16:9与超宽屏幕，保持实际距离比例。M、菜单、任务定位和地点传送继续共用当前地图；红蓝火焰仍分别对应玩家与弟弟实际位置和朝向。地图底图直接可见；原100米地点发现、任务未知信息和传送资格规则继续适用。

保留原灰暗地形、灰蓝水域、山地纹理、地名与暖灰描边。地图范围限制在原始4.032公里地形图集内；开场位于地形东侧，因此矩形中心向西约束至X=516米，而不是向世界边缘外补画地形。初始视图仍靠近玩家；在其他营地打开或定位时，可使用同一图集内的相应矩形区域。

![初始局部视图](rectangular/map-preview.png)

![缩小后的完整矩形](rectangular/map-full-region.png)

拖动、地点面板与超宽屏预览分别见 [拖动截图](rectangular/map-dragged.png)、[地点截图](rectangular/map-locations.png)、[超宽屏截图](rectangular/map-ultrawide.png)。

## 地理对应检查

沿用现有2400×2400完整地形图集，未重画或移动主地图资产。底图高度来源、实际水域GLB和制作方法见历史 [制图参数](cartography.json)。本轮在整个3000米×2000米矩形内分布31×21个真实碰撞射线采样，651个全部命中地面；与实际使用的原始高度源对齐后的P99误差为0.0266米，整体高度偏移约−0.00049米。该检查覆盖扩大后的矩形，区别于此前仅开场500米方形内的密集采样。

当前场景137项建筑组件的位置与横纵包围盒均与底图制图输入吻合，比较精度为0.0001米。底图SHA256保持 `62e3e14ede4abeb731edd50ba3a883752d52efe341ee64447ad71743e949832f`，水域仍使用原位置及三角形。没有为了填满新区域虚构地形或建筑。完整检查与指纹见 [rectangular-audit.json](rectangular/rectangular-audit.json)，必要采样及建筑轮廓见 [当前地理记录](rectangular/current-geography.json) 和 [图集输入记录](rectangular/atlas-geography.json)。这些记录是完整原始场景的必要摘录，附有完整场景SHA256。

## 验证

环境：Windows、UE5.8.2、VS14.44、Windows SDK10.0.26100.0。引擎操作使用显式指定本工程的GameFactory `UEClient` 公开API。

| 检查 | 结果 | 证据 |
|---|---|---|
| 最终Editor Development构建 | PASS | [editor-build.json](rectangular/editor-build.json) |
| 探索／任务地图、字体缩放与完整目标、左键拖动与释放三项原生回归 | 3/3通过，0错误 | [native-index.json](rectangular/native-index.json) |
| 实际Development Standalone新游戏地图验证 | 97/97通过 | [runtime-report.json](rectangular/runtime-report.json) |
| 图像、地理与当前指纹校验 | 26/26通过 | [rectangular-audit.json](rectangular/rectangular-audit.json) |
| 仓库工具测试 | 33/33通过 | [tools-tests.log](rectangular/tools-tests.log) |
| Win64 Shipping Build/Cook/Stage/Archive | PASS | [package-result.json](rectangular/package-result.json) |
| 最终独立Shipping启动 | 6次采样均有窗口且正常响应 | [shipping-startup.json](rectangular/shipping-startup.json) |
| ZIP全部152个文件CRC检查 | PASS | [zip-integrity.json](rectangular/zip-integrity.json) |

原生三项均保留一项隔离LocalPlayer缺少有效PlayerInput的初始化警告；未隐藏警告或声称零警告。运行验证使用真实Widget／Slate事件、Actor和碰撞，采用独立GUID档池，覆盖M开关、双方五组朝向、实际位置、左键捕获／拖动／释放、边界夹取、缩放、完整矩形四角、暂停／不暂停与缺失弟弟。

1280×720、1600×1000、1920×1080、2560×1080及缩放平移状态均完成原图与纯色探针成对渲染。矩形内部99.49%—99.69%的像素受到探针影响，其余为前景地名与标记；矩形外像素差全部为0。当前图片没有黑雾遮挡，地图裁剪未越界，初始显示局部和缩小显示完整矩形两种状态均通过。

复验命令为 `python -X utf8 scripts/ui/launch_map_test.py --verify`，随后使用包含numpy/Pillow的Python执行 `scripts/ui/audit_rectangular_map.py <当前运行目录> --atlas-scene <底图制图输入场景>`。原始当前运行目录为 `.agent-local/qa/TASK-078/rect_verify_10840325`，原生目录为 `.agent-local/qa/TASK-078/rect_native_ab43a61f`。这不是Windows物理鼠标输入或完整真人通关验收。

## 本地交付

当前包为 `E:/AiAgent/XLingGame/Hearthward-Playable-20261006-RectMap/Windows`，包含完整游戏、本地模型和CPU／Vulkan运行库，无需UE或Python。根目录“启动矩形地图版.cmd”“启动地图更新版.cmd”“启动正式游玩版.cmd”用于启动本包。便携ZIP为 `Hearthward-Playable-20261006-RectMap.zip`，必须完整解压后运行其中“启动游戏.cmd”。旧Map1000与Latest包保留。

最终Shipping程序SHA256为 `fd4cd811b300007dad09aeb6b9efa9768c92b32448dda9335969e3e8c59955fc`。最终程序、全部Source和UI指纹见 [shipping-build-info.json](rectangular/shipping-build-info.json)，包内配置与底图也已与当前资源逐文件比较。最终包独立启动，约30秒内6次窗口采样均响应正常。ZIP为4911451583字节（约4.91GB），152个成员全部CRC通过；SHA256见 [delivery.json](rectangular/delivery.json)。当前定向地图检查及独立启动不代表全游戏通关、长期性能或第二机器验收。

普通仓库自检、差异检查与当前allowed_paths直接比较通过，记录见 [repository-checks.json](rectangular/repository-checks.json)。基线提交不存在本新任务的批准快照，`validate_repo.py --task TASK-078 --base ca21120c…` 的前置条件不满足；实际失败保留，未改验证器或伪造批准。此项未标记为PASS。
