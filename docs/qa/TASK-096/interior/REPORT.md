# TASK-096：石堡卧室木构，2026-10-09

本轮优先美术，完整实玩后置。Blender MCP制作4320三角形的原创木构，适配现有12×10米卧室。包括三跨窗框、打开的百叶、墙脚板、立柱、天花横梁、挂物钩及壁架。新增一个网格、两个材质实例；旧木复用既有096 PBR。两张床复用098新绳床，旋转90度，保留原床占地与碰撞。

制作源：[BedroomJoinery.blend](../../../../art_source/TASK-096/BedroomJoinery.blend)、[重建脚本](../../../../art_source/TASK-096/author_bedroom_joinery.py)、[规格](../../../../art_source/TASK-096/bedroom-joinery.json)。均为原创几何，无第三方下载输入。资产锁保留至集成交接。

## 检查记录

- UEClient公开API导入，结果在`import/`。新增Interior目录纳入AlwaysCook；尚未执行新Shipping打包。
- Editor构建及`Hearthward.Hometown077.BedroomAndEscapeClearance`定向原生通过，0警告/0错误。证据：[build.json](build.json)、[native.json](native.json)。测试检查新网格可加载、两床实际包围盒处于原碰撞占地内、装饰不参与碰撞，以及卧室、门口、廊道和后门净空。
- 初次新增测试使用TObjectPtr时未取底层指针，并与已有Beds变量重名；已修正。原始失败：[first-build-error.json](first-build-error.json)。
- 正式自然地图、独立新档、诊断相机，使用正式夜间光照。首次画面揭示FBX导入的Y轴反射使南墙立柱出现在门洞前；这是无碰撞装饰，碰撞测试不能代替视觉检查。已在导出阶段补偿Y轴，Blender源保留游戏坐标。窗侧背墙过暗，沿用原灯具补一盏床侧壁灯，未改全局曝光。
- 最终局部画面和结果登记在`visual/`；首次方向错误画面保留为`visual-first/`。不把诊断相机视作完整步行验收。

## 边界

装饰没有新增拾取、奖励、危险、交互规则或导航障碍。主地图未保存诊断相机。当前候选11未包含本轮资产；完整实玩、Owner视觉、新Shipping包保持未验。正午背阴廊道、场景更多近景细节仍属于后续美术工作，TASK-096保持Active。

准确范围快照679b5bc3，最终实现SHA在提交后登记。未合并main、未发布。
