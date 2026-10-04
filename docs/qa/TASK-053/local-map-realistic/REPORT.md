# 历史：灰暗写实地图与细长火焰测试 · 2026-10-04

本报告只绑定 `verify_25725b6d` 时的源码和美术。当前根目录入口已更新为[河谷凹陷、坡峦起伏与松散暗雾版](../local-map-relief-fog/REPORT.md)；下述纯黑范围外检查及旧PID均为历史结果。

已按用户最新两幅参考图更新同一根目录[地图测试版.cmd](../../../../地图测试版.cmd)：地图改为低饱和、整体灰暗的写实地表质感，以实际高程表现连续坡面，人物标记改为细长、尖底、带细碎火舌和火星的真实火焰。玩家红橙、弟弟蓝青；原出生中心、直径500米、简短区域名、真实位置动态更新、缩放／平移与范围外黑雾保持。

该轮[出生预览](verify_25725b6d/map-1600x1000.png)和[人物分开后的截图](verify_25725b6d/map-separated-flames.png)来自真实游戏渲染。当时上层 `E:/AiAgent/XLingGame` 的同名入口及预览亦已替换，另有 `地图测试版_移动验证.png`。当时根入口启动的可见Development游戏PID21100三次响应与自然世界加载完成检查通过。

分支 `codex/TASK-053-title-wheel`，HEAD `4db5789184fe38e041d62a1e68c8517338ea0b01`。当前实现未提交或推送，证据以[239项文件指纹](verify_25725b6d/source-manifest.json)绑定本地源码、DLL、素材及绘图／启动脚本。

## 最终美术与生成记录

- [实际地形底图](../../../../Resources/UI/Art/map-local-terrain.png)：1800×1800，灰绿枯草、灰岩、暗色水面、真实建筑屋顶和植被概貌；取消首版显眼的彩色色块与线状高程台阶。
- [地表材质原件](../../../../Resources/UI/Art/map-surface-materials.png)：1536×1024，六种写实表面纹理，内置 `image_gen` 生成。只给原地形和实际物体提供表面质感。
- [透明红蓝火焰原件](../../../../Resources/UI/Art/map-flames.png)：1254×1254，内置 `image_gen` 以用户图二为形态参考生成，两种颜色共用一个透明图集。未以Python重绘或改色，原件字节和Alpha保留。
- [完整最终提示词](imagegen-prompts.json)、[生成素材原件指纹与锚点](generated-assets.json)均归档；使用内置工具，无CLI/API替代调用。

地形绘制沿用[实测场景](../local-map/verify_afbee402/scene.json)的63,001个2米间距碰撞高程点、四件水面模型三角形和实际Actor／实例。当前世界重新导出的[场景](verify_25725b6d/scene.json)与原来源在高度、水面、建筑、树、岩石等[八项比对](current-scene-comparison.json)完全一致。AI材质不决定地理边界或地标位置。中心仍为 `(92000,53500)` 厘米，半径25000厘米；地表高度约201.75至247.28米，不在出生范围内新增高山。

绘图记录[cartography.json](cartography.json)含来源、图像、地形高度与水面遮罩指纹。地表平均亮度由157.4降至81.3，平均饱和度由0.355降至0.141，保持地形与暗色水体可辨。

火焰按透明纹理的真实尖底校准UV，使根部始终对应标记的 `(.5,.9)`。显示框15×44设计单位，实际火焰更细；轻微摆动与红蓝相反的上方倾斜围绕根部进行，原XY锚点不偏移。圈外人物继续隐藏，最终雾罩遮住所有超出圆形的像素。地图文字仍只有故乡、草原、坡地、河谷、湖岸。

## 当前验证

- [Development构建](build_f6a84f24/build.json)：成功、0诊断，通过GameFactory UEClient公开API；首次构建[UV兼容错误](build_cf0062ce/build.json)已以明确FBox2f转换修复，原失败记录保留。
- [真实独立游戏验证](verify_25725b6d/report.json)：40/40，保留原真实出生点、相同比例、实际人物位置更新、缩放／平移、圆形250米边界、缺失／出界人物、关闭地图后输入及关闭菜单暂停时定位检查。
- [图像与当前文件审计](final-audit.json)：13/13，四画幅范围外最大RGB为0；出生时贴近的红蓝火焰均有可见像素；239指纹匹配、其他页面／布局保持、人工Profile59文件未变。
- [灰暗风格与火焰素材核对](style-audit.json)：12/12，涵盖亮度／饱和度变化、原观察范围及名称坐标、水面遮罩、高程来源、圈外黑色、真实透明Alpha、原件指纹、细长比例与尖底锚点。
- [当前场景几何比对](current-scene-comparison.json)：8/8。[根入口启动观察](root-launch-smoke.json)：8/8，正常可见游戏、实际进程、三次窗口响应及加载完成，无致命错误；不是Windows物理键鼠完整实玩。
- [工具测试](tool-tests.txt)：33/33；[仓库自检](repo-validation.txt)：0错误；差异格式检查通过。

已目视核对[出生画面](verify_25725b6d/map-1600x1000.png)、[分开后的两簇火焰](verify_25725b6d/map-separated-flames.png)及地图明暗。另有[1920×1080](verify_25725b6d/map-1920x1080.png)、[1280×720](verify_25725b6d/map-1280x720.png)、[超宽](verify_25725b6d/map-ultrawide.png)、[缩放／平移](verify_25725b6d/map-zoom-pan.png)原生截图，四画幅均完成像素核对。

## 使用与边界

双击根目录入口，新的独立GUID测试档加载完成后自动打开地图；M／Esc返回世界，WASD移动、X跟随／Z等待，M再次查看。滚轮缩放、方向键平移。普通TestClient使用当前Development模块，也包含新版地图。

构建与验证为 `python -X utf8 scripts/ui/launch_map_test.py --build` 和 `--verify`。使用具备NumPy／Pillow的工作区Python运行 `scripts/ui/draw_local_map.py <scene.json>` 可从相同实际场景与已保存材质复现底图；`scripts/ui/audit_local_map.py <验证目录>`核对当前渲染证据。游戏入口不依赖NumPy／Pillow。底图仍为当前场景的静态制图；以后修改三维地形或建筑需要重新绘制，位置标记持续实时更新。

没有Content资产、Gameplay规则、保存格式或人工测试资料变更。当前仅Development测试，Shipping打包、Windows物理键鼠完整实玩和Owner视觉验收未执行。正式[基线范围检查](baseline-scope.txt)仍缺TASK-053获批快照；此限制没有伪报通过，本轮用户授权和实际写入路径另行核对。未提交、推送、合并或发布。
