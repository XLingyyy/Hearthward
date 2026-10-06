历史地图／尖端版本证据；最新简约UI与程序见 [SIMPLE-UI-REPORT](SIMPLE-UI-REPORT.md)。

# TASK-078 红蓝火焰三角尖端

2026-10-06，接续用户要求将玩家和弟弟火焰指示朝向的尖端稍微加粗，形成小三角形。分支 `codex/map-1000m-heading`，已同步的远端main基线 `ca21120cef4421d039c44248a0cfa5043c248feb`。当前成果保存在本地，未提交、推送或公开发布；任务仍为Active，不代填人工验收。当前结果绑定 [地图Source／DLL指纹](flame-tips/source-manifest.json)、[传送Source／DLL指纹](flame-tips/travel-source-manifest.json)及[Shipping程序指纹](flame-tips/shipping-build-info.json)。

在原火焰方向端增加宽11、长8设计单位的小三角形，红蓝各用原标识颜色。三角形绘在火焰纹理下，保留原亮芯、纹理、闪动和整体长条火焰外形。增粗范围限定在前端的一小段。三角尖点保持在原角色地图坐标，尖头和纹理共用一次旋转变换，跟随玩家与弟弟各自Actor朝向；尖点不随闪动位移。

实现只修改 `Source/Hearthward/UI/HearthwardScreenPaint.cpp` 的火焰绘制。没有修改位图、投影、地图范围、地名、输入、地点分类、传送逻辑、存档或主地图资产。此前全屏无黑雾矩形地图和传送修复保留。

![火焰尖头细节](flame-tips/flame-detail.png)

上图来自实际1600×1000 Widget渲染的局部裁切，4倍最近邻放大以便查看尖端；未重绘或生成标识。两个人物的位置由独立验证夹具分开，以便同时查看红蓝火焰。[完整截图](flame-tips/map-separated-flames.png)、[对应地图坐标与朝向](flame-tips/map-separated-flames.json)及[修改前同区域](flame-tips/flame-detail-before.png)。

## 当前验证

环境为Windows、UE5.8.2、VS14.44、Windows SDK10.0.26100.0，引擎操作使用显式指定本工程的GameFactory `UEClient` 公开API。本次视觉改动没有新增镜像实现的测试；复用现有定向地图检查并观察实际渲染图。

| 检查 | 实际结果 | 证据 |
|---|---|---|
| Editor Development构建 | PASS | [editor-build.json](flame-tips/editor-build.json) |
| Win64 Shipping重新构建 | PASS | [shipping-build.json](flame-tips/shipping-build.json) |
| 原生地图、字体、拖动、地点与反馈回归 | 4/4通过，0错误；各1项夹具警告 | [native-index.json](flame-tips/native-index.json) |
| 实际地图Widget与角色朝向检查 | 98/98通过 | [runtime-report.json](flame-tips/runtime-report.json) |
| 新游戏剧情交互、路标和往返传送检查 | 37/37通过 | [travel-report.json](flame-tips/travel-report.json) |
| 图像、地理与当前Source／DLL指纹审计 | 44/44通过 | [rectangular-audit.json](flame-tips/rectangular-audit.json) |
| 仓库工具测试 | 33/33通过 | [tools-tests.log](flame-tips/tools-tests.log) |
| 独立Shipping启动 | 6次窗口响应采样通过 | [shipping-startup.json](flame-tips/shipping-startup.json) |
| ZIP全部152个成员CRC检查 | PASS | [zip-integrity.json](flame-tips/zip-integrity.json) |
| 普通仓库、差异、当前路径及最终指纹绑定 | PASS；基线批准快照检查前置条件不满足 | [repository-checks.json](flame-tips/repository-checks.json) |

实际绘图检查覆盖玩家yaw0／90／180／270／45度，弟弟各自使用相差90度的朝向；火焰与三角头共享真实旋转，原实际坐标锚点检查通过。截图见[0度](flame-tips/heading-0.png)、[90度](flame-tips/heading-90.png)、[180度](flame-tips/heading-180.png)、[270度](flame-tips/heading-270.png)、[45度](flame-tips/heading-45.png)。普通屏、超宽屏、放大平移和最小倍率保持等比例、实际地形覆盖屏幕四边。

当前651个分布式地面碰撞采样匹配高度源，P99误差0.0266米；137项建筑组件轮廓仍匹配制图输入。见[当前地理](flame-tips/current-geography.json)与[制图输入](flame-tips/atlas-geography.json)。底图和火焰纹理未修改。

原生回归各保留隔离LocalPlayer缺少有效PlayerInput的初始化警告。运行使用Development真实新游戏、Widget、碰撞和导航，以及独立GUID档池的播种／重定位夹具；不能等同物理键鼠、玩家存档、完整真人通关或第二机器验收。Shipping仅独立验证启动和窗口响应。

本次原始目录：Editor `rect_build_3330a87e`、原生 `rect_native_ea3d6d3a`、地图 `rect_verify_4f3b08d2`、传送 `rect_travel_be6f6edb`，均位于 `.agent-local/qa/TASK-078/`。复验入口 `scripts/ui/launch_map_test.py --build / --verify / --travel`，图像入口 `scripts/ui/audit_rectangular_map.py <运行目录> --atlas-scene <制图输入>`，原生过滤器 `Hearthward.Map064+Hearthward.UI069.MapScaledGuidance+Hearthward.Map078`。

## 本地交付

当前目录 `E:/AiAgent/XLingGame/Hearthward-Playable-20261006-FlameTips/Windows`。根目录三个启动脚本已更新：`启动正式游玩版.cmd`、`启动地图更新版.cmd`、`启动矩形地图版.cmd`。随包包含模型与CPU／Vulkan运行库，无需UE或Python。便携包 `Hearthward-Playable-20261006-FlameTips.zip` 需完整解压后运行其中“启动游戏.cmd”。旧TravelFix、FullMap、RectMap、Map1000与Latest包保留。

只有Paint.cpp相对已验证TravelFix源码发生变化，UI资源逐项匹配原包；本次重新编译Shipping程序，并复制经过验证的原烘焙资源与运行库到独立新目录，没有声称重新Cook。程序、全部Source和UI资源指纹见BUILD-INFO及最终绑定检查。

Editor DLL SHA256：`c418b7b64369aa222e2a9c678278401a1fefa3769bb7d152d0ac37e753cf9766`。Shipping SHA256：`331dd17038ebd6af0c8a458754661bff9c8b633e03b534c395c5c3dc4e1b5b1b`。ZIP为4911458725字节、152个成员，全部CRC通过；SHA256：`7759bca1f53c3d87ac4ae6b49c39168c3df9c5b2c9280d3672d5b96d0941eca4`，见[交付记录](flame-tips/delivery.json)。当前为可玩开发预览，原项目真人通关、精细美术与正式RC待办继续保留。

本新任务在基线中没有批准快照，基线任务范围检查前置条件不满足；未改验证器或制造批准。普通仓库自检、当前allowed_paths直接比对和最终Source／资源／程序绑定另行记录。本轮未提交或推送。
