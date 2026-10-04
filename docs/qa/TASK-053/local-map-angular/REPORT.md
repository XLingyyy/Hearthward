# 历史：地图局部折角边缘精修 · 2026-10-04

本报告仅绑定当轮资源；最新水色与测试结果见[当前报告](../local-map-water/REPORT.md)。当轮将整圈偏圆滑的边缘精修为局部折角、尖突和内凹切口，与其余自然曲线混合。折角附近的雾边收窄，保持烟丝流动，使角点在默认近景中清楚可见。[当轮预览](verify_639ec35e/map-1600x1000.png)保持，根目录测试入口已继续更新。

## 实现与面积

48个径向控制点中，15段（31.25%的角度范围）采用射线与二维直线段相交计算，其余为连续曲线。雾层384段网格包含全部控制点，凹口角点不会被跨接成圆滑桥。角点处羽化约为原来的28%—40%，其余过渡保持柔和；几何边界固定，人物可见与雾层共用同一函数。

内部连通无孔洞，全部落在原出生点250米半径内，面积保留90.9798%（约178,638平方米），仅比前版少0.85%。M默认2倍近景、(0,30)平移、出生中心及区域名坐标保持；四份地图美术原件未改，见[保留指纹](preserved-data.json)。[几何方案](outline-plan.json)、[生成脚本](outline-design.py)及[15份前快照](before/)保存。

## 当前验证

- [Development构建](build_3fb31c6c/build.json)：成功、无诊断，经GameFactory UEClient公开API指定本工程。
- [最终独立游戏](verify_639ec35e/report.json)：56/56，包括角段配置、M打开／关闭／重开与默认首屏、真实ActorXY、边界／凹口隐藏、缩放／平移和暂停。
- [图像／当前指纹](final-audit.json)：29/29。四画幅显示区连续，四画幅及缩放／平移时，雾下替换测试层对轮廓外每个像素影响均为0；当前241项源码／DLL／资源指纹匹配，人工Profile59文件、其他UI页面／布局保持。角段数据与实机状态相同。
- [当前场景比对](scene-comparison.json)：9/9，高程、水体和物体位置保持。[工具测试](tool-tests.txt)：33/33；[仓库检查](repo-validation.txt)：0错误；[差异格式检查](diff-check.txt)：通过。[完整交付指纹](delivery-integrity.json)绑定当前素材、文档、预览和结果。
- [根CMD启动](root-launch-smoke.json)：8/8，`preview_9d00c348`／PID19352完成自然加载，三次可见窗口响应通过；观察后窗口留给用户。

已目视核对[默认近景](verify_639ec35e/map-1600x1000.png)。另有[参考尺寸](verify_639ec35e/map-reference-size.png)、[1920×1080](verify_639ec35e/map-1920x1080.png)、[720p](verify_639ec35e/map-1280x720.png)、[超宽](verify_639ec35e/map-ultrawide.png)、[缩放／平移](verify_639ec35e/map-zoom-pan.png)和[人物分开](verify_639ec35e/map-separated-flames.png)。

首次 `verify_15a7cc65`在构建返回成功前启动，其启动清单仍绑定前版DLL，虽56项通过也不作为当前证据；[重叠记录](initial-overlap.json)保留。构建成功、该轮结束后重新运行，最终以 `verify_639ec35e`及其匹配的241项指纹为准。

## 使用与限制

双击根入口加载独立新档，M／Esc返回、WASD移动、X跟随／Z等待；M再开恢复参考首屏，滚轮／方向键调整。普通TestClient同样使用当前Development模块，自动测试只用TestClient/Runs独立配置／GUID池。

构建／回归：`python -X utf8 scripts/ui/launch_map_test.py --build` 和 `--verify`。图像复验用NumPy／Pillow工作区Python运行 `scripts/ui/audit_local_map.py docs/qa/TASK-053/local-map-angular/verify_639ec35e`；游戏不依赖这些库。

分支 `codex/TASK-053-title-wheel`，HEAD `4db5789184fe38e041d62a1e68c8517338ea0b01`，未提交、推送、合并或发布。仅二维显示层Development修改，未改Content、三维地形、玩法或存档，未Shipping打包或Windows物理键鼠完整实玩。正式[基线范围](baseline-scope.txt)仍缺TASK-053获批快照，[本轮增量范围](incremental-scope.json)单独核对，不能替代正式检查；[前版首屏报告](../local-map-entry/REPORT.md)仅历史绑定。
