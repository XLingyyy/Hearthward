# TASK-078 全屏地图与悬浮地点入口

当前火焰尖端更新、Source／DLL指纹和本地包以 [三角火焰尖端报告](FLAME-TIPS-REPORT.md) 为准；本文件保留此前版本证据。

本文件保留此前地图布局版本的历史指纹与结果；当前用户四项bug修复、源码验证与可玩包以 [地图与传送修复报告](TRAVEL-FIX-REPORT.md) 为准。

2026-10-06，接续用户截图要求：保留右上“地点与传送”，删除四周深色背景和其他外围文字描述。分支为 `codex/map-1000m-heading`，基线为 `ca21120cef4421d039c44248a0cfa5043c248feb`。成果保存在本地，未提交或推送。旧矩形和圆形报告保留为历史，本次结果绑定 [当前Source／DLL／资源指纹](fullscreen/source-manifest.json)。

地图地形现在铺满整个屏幕，移除四周留边、细边框、左上地图尺寸标题、左下“Esc 返回”及右下拖动／缩放提示。右上“地点与传送”成为悬浮入口，点击仍能展开和收起地点面板。地图内部的地名、任务标记和红蓝角色火焰保留。M／Esc返回、左键拖动、滚轮缩放及方向键平移继续可用。

地图区域仍为3000米×2000米，横纵等比例投影。最小缩放倍率改为覆盖整个屏幕，屏幕比例不同时显示矩形的一部分，可拖动查看剩余部分；拖动边界不会露出底幕。该调整响应最新截图要求，取代旧版“缩小到整幅矩形并留出空白”的显示方式。地图材质、水域颜色、实际地形及建筑分布保持前一版数据。

![全屏地图](fullscreen/map-preview.png)

[最小缩放预览](fullscreen/map-minimum-zoom.png)、[超宽屏预览](fullscreen/map-ultrawide.png)、[地点面板](fullscreen/map-locations.png)及[拖动后预览](fullscreen/map-dragged.png)均来自本次真实Widget渲染。

## 验证与指纹

环境为Windows、UE5.8.2、VS14.44、Windows SDK10.0.26100.0。引擎操作通过GameFactory `UEClient` 公开API，显式指定本工程。图像审计使用本机包含numpy/Pillow的Python。

| 检查 | 实际结果 | 证据 |
|---|---|---|
| Editor Development构建 | PASS | [editor-build.json](fullscreen/editor-build.json) |
| 探索／任务地图、字体缩放、原生拖动与入口回归 | 3/3通过，0错误 | [native-index.json](fullscreen/native-index.json) |
| Development Standalone新游戏运行检查 | 98/98通过 | [runtime-report.json](fullscreen/runtime-report.json) |
| 图像、地理与当前指纹检查 | 44/44通过 | [rectangular-audit.json](fullscreen/rectangular-audit.json) |
| 仓库工具测试 | 33/33通过 | [tools-tests.log](fullscreen/tools-tests.log) |
| Win64 Shipping Build/Cook/Stage/Archive | PASS | [package-result.json](fullscreen/package-result.json) |
| 最终独立Shipping启动 | 6次窗口采样均正常响应 | [shipping-startup.json](fullscreen/shipping-startup.json) |
| 便携ZIP全部152个成员CRC检查 | PASS | [zip-integrity.json](fullscreen/zip-integrity.json) |
| 普通仓库／差异／当前路径范围和最终指纹绑定检查 | PASS | [repository-checks.json](fullscreen/repository-checks.json) |

原生三项各保留隔离LocalPlayer缺少有效PlayerInput的初始化警告。没有隐藏警告或宣称零警告。运行验证覆盖角色实际位置和朝向、各地图入口、左键捕获／拖动／释放、边界夹取、缩放、M／Esc返回、暂停及缺失弟弟情形。这些检查采用真实Widget／Slate事件，并非Windows物理鼠标输入或真人通关。

1280×720、1600×1000、1920×1080、2560×1080、放大平移和最小缩放六组状态完成原图／纯色地形探针成对渲染。地图内部99.59%—99.70%的像素受到地形探针影响，屏幕四边覆盖率99.07%—100%，其余像素为前景地名和标记。视口边界与实际屏幕四边一致；没有用于四周留边的底幕、边框或外围描述元素。最小倍率也覆盖两个屏幕方向。

地理检查重新采集了整个矩形内651个分布式真实地面碰撞点，全部命中，原高度源比对P99误差0.0266米。当前137项建筑组件轮廓与图集输入一致。必要场景摘录和完整原始场景SHA见 [当前地理记录](fullscreen/current-geography.json)及[图集输入记录](fullscreen/atlas-geography.json)。地形图集SHA256仍为 `62e3e14ede4abeb731edd50ba3a883752d52efe341ee64447ad71743e949832f`；没有重画地形、移动建筑或修改Content。

当前Editor DLL SHA256为 `188586bac40c9daac4960bf17729ea12410dcdf09bde7532e32715513e7bd0aa`。最终Shipping程序SHA256为 `75cd17bf12327f7302db2a310c7afcd0b07d14386c62d4d113e625ce323c73e1`。程序、全部Source和打包UI指纹见 [shipping-build-info.json](fullscreen/shipping-build-info.json)。

复验入口：`python -X utf8 scripts/ui/launch_map_test.py --verify`。本次原始运行目录为 `.agent-local/qa/TASK-078/rect_verify_09951766`，原生目录为 `.agent-local/qa/TASK-078/rect_native_ced432f2`，Editor构建目录为 `.agent-local/qa/TASK-078/rect_build_9ca54f4d`。随后执行 `scripts/ui/audit_rectangular_map.py <运行目录> --atlas-scene <制图输入场景>`。

## 本地交付

最新版目录为 `E:/AiAgent/XLingGame/Hearthward-Playable-20261006-FullMap/Windows`，包括游戏、本地模型、CPU和Vulkan运行库，无需UE或Python。上层项目根目录“启动正式游玩版.cmd”“启动地图更新版.cmd”“启动矩形地图版.cmd”均指向本版本；旧RectMap、Map1000与Latest目录和ZIP保留。

便携包为根目录 `Hearthward-Playable-20261006-FullMap.zip`，需完整解压后运行其中“启动游戏.cmd”。共4911451908字节、152个成员，全部CRC检查通过。ZIP SHA256为 `cd0a1060bebab9c6e39b381cc51faf0c4f8128c8b801b30c8f21bb5fcdf40014`，记录见 [delivery.json](fullscreen/delivery.json) 和 [zip-integrity.json](fullscreen/zip-integrity.json)。正式RC、长期性能、完整真人通关与第二台机器验收仍未完成；当前交付为可玩开发预览。

普通仓库自检、差异检查、当前allowed_paths直接比较及最终源码／程序／资源绑定均通过，实际结果见 [repository-checks.json](fullscreen/repository-checks.json)。基线提交没有本新任务的批准快照，基线任务范围校验前置条件不满足；实际失败记录保留，此项未标记PASS，未改验证器或伪造批准。
