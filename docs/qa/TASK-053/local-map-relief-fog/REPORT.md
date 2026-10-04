# 河谷凹陷、坡峦起伏与松散暗雾测试 · 2026-10-04

历史证据：本报告仅证明本轮当时的源码／资源。最新不规则范围、根目录预览和验证见[当前报告](../local-map-irregular/REPORT.md)。

已按用户最新参考继续更新根目录[地图测试版.cmd](../../../../地图测试版.cmd)。河谷采用低处暗色、谷底遮蔽和坡壁明暗，坡地增加顺着实际高程弯曲的坡脊／坡面层次。四周改为松散、细薄、缓慢流动的灰黑烟丝与少量暗红余烬，边缘柔化；出生点直径500米以外仍完整遮蔽。地图文字仅原五个区域名，红／蓝细长火焰仍随真实人物XY更新。

[本轮历史预览](verify_9fb13427/map-1600x1000.png)与[移动后人物分开](verify_9fb13427/map-separated-flames.png)为本轮最终版真实游戏渲染，当时上层 `E:/AiAgent/XLingGame` 的同名入口、预览及移动验证图也已更新。根入口当时启动可见Development游戏，PID33608三次窗口响应及世界加载检查通过；该记录不代表窗口当前仍运行。

分支 `codex/TASK-053-title-wheel`，HEAD `4db5789184fe38e041d62a1e68c8517338ea0b01`。源码未提交、推送或发布；本轮原件[19份快照](before/)保存，不覆盖此前UI工作。

## 地形与暗雾

- [地形底图](../../../../Resources/UI/Art/map-local-terrain.png)仍取63,001个实际碰撞高程点、原水面三角形和实际树／岩石／建筑位置。平面投影与500米范围保持；4.5倍垂直强调只用于制图光照、地形遮蔽和细轮廓，没有修改三维模型或XY比例。
- 实测河谷小区域平均222.07米，故乡高地239.13米；坡地区域高度218.66—236.80米。新版谷底与高地有更清楚的明暗差，坡地亮度10—90百分位差由34.71增至49.06，配合弯曲坡壁刻画起伏。数值辅助目视核对，不代替Owner视觉验收。
- [新烟雾素材](../../../../Resources/UI/Art/map-dark-fog.png)：1536×1024，内置 `image_gen` 依用户参考生成，原件字节保留。[完整提示词](imagegen-prompt.json)、[原件路径及SHA256](generated-asset.json)归档；未采用CLI/API替代生成。既有地表材质与红蓝透明火焰原件保持。
- 烟雾按整个真实视口统一采样，缓慢漂移；内侧多层渐变和轻微不规则烟边衔接地图。圆形250米边界及其外侧为完全不透明的烟雾。底图圈外Alpha为0，避免旧黑色方形盖住烟丝。暂停世界时继续刷新烟雾，位置边界保持固定。

## 当前验证

- [Development构建](build_a84b26f1/build.json)：成功，0诊断，经GameFactory UEClient公开API；最终构建包括缩放时视口裁切修正。
- [最终独立游戏](verify_9fb13427/report.json)：46/46，包括既有40项出生、人物真实位置、圆形边界、缩放／平移、暂停与返回检查，以及6组雾层对照图导出。使用显式Development夹具及独立GUID测试池。
- [图像及当前资源审计](final-audit.json)：20/20。四种画幅和缩放／平移时，雾层下面替换成醒目的紫红测试层后，圈外每个像素差值均为0；圈内测试层有大量可见像素，验证测试有效。所有画幅为不透明烟雾视口，出生时贴近的两簇红蓝火焰仍可见。
- 烟雾对照捕获冻结同组时钟，另用时钟偏移12秒捕获流动效果；圈外1,058,423像素发生变化，平均绝对RGB差5.580。此项验证纹理时钟采样变化，不声称物理键鼠实玩。
- 放大时改为按实际视口裁切，消除16:10设计画布边缘截断地形的直线。修正前已观测到顶部画幅留边内水面像素为0，最终同一位置12,611个湖面像素可见；范围外仍通过完整遮蔽核对。
- [地形与风格核对](relief-style-audit.json)：16/16，原高程、水体遮罩、Actor轮廓、中心／范围、区域名坐标和火焰原件保持；新版地形图整体灰暗低饱和，范围外Alpha为0，新雾原件与生成输出指纹一致。
- [当前场景比对](current-scene-comparison.json)：9/9；[241项源码／DLL／制图／资源指纹](verify_9fb13427/source-manifest.json)均与当前一致。人工 `TestClient/Profile` 59文件及其他UI页面、布局保持。
- [根入口启动观察](root-launch-smoke.json)：8/8；[工具测试](tool-tests.txt)：33/33；[仓库自检](repo-validation.txt)：0错误；[差异格式检查](diff-check.txt)：通过。

已目视核对[出生截图](verify_9fb13427/map-1600x1000.png)、[人物分开截图](verify_9fb13427/map-separated-flames.png)与地形图。另有[1920×1080](verify_9fb13427/map-1920x1080.png)、[1280×720](verify_9fb13427/map-1280x720.png)、[超宽](verify_9fb13427/map-ultrawide.png)、[缩放／平移](verify_9fb13427/map-zoom-pan.png)及全部烟雾对照原生图。

首轮 `verify_bdb62872` 的46项和[首次图像审计](first-render-audit.json)保留；第二轮 `verify_a4b5873f` 调整了谷底明暗，其[19项审计](second-render-audit.json)保留。目视缩放画面后修正设计画布直边截断，并新增湖面穿过画幅留边的检查，最终证据以 `verify_9fb13427` 为准。首次像素审计遇到NumPy布尔值不能序列化的问题，已明确转为Python bool，检查未删减。此前[灰暗火焰版本](../local-map-realistic/REPORT.md)只证明其历史源码。

## 使用、复现与限制

双击根目录入口，新建独立测试档后自动打开M地图。M／Esc回世界，WASD移动，X让弟弟跟随／Z等待，再按M查看；滚轮缩放、方向键平移。普通TestClient也使用当前Development模块。没有C盘安装或人工测试资料覆盖。

构建／实机验证：`python -X utf8 scripts/ui/launch_map_test.py --build` 和 `--verify`。使用具备NumPy／Pillow的工作区Python运行 `scripts/ui/draw_local_map.py docs/qa/TASK-053/local-map-realistic/verify_25725b6d/scene.json` 可复现最终底图；`scripts/ui/audit_local_map.py <最终验证目录>`核对渲染，[地形核对脚本](relief-style-audit.py)可复验形状与来源。[制图记录](cartography.json)含输出、原始采样、水面及实际物体指纹。游戏运行不依赖NumPy／Pillow。

本轮仅地图显示与测试美术，未改Content资产、三维地形、Gameplay或存档格式。底图是当前场景的静态制图，世界地形／建筑改变后需重新生成；人物标记实时更新。Shipping重新打包、Windows物理键鼠完整实玩及Owner视觉验收未执行。正式[基线范围检查](baseline-scope.txt)仍因缺TASK-053获批快照失败；[本轮增量范围核对](incremental-scope.json)单独记录，不能替代该正式检查。未提交、推送、合并或发布。
