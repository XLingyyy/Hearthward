# 湖岸与河谷统一水色 · 2026-10-04

湖水与河谷水面统一为湖岸原有的灰蓝色，消除交汇处模型边界造成的深蓝色带。保留原水波纹理与实际水域形状。根目录[地图测试版](../../../../地图测试版.cmd)与[预览](../../../../地图测试版_预览.png)已同步；上层XLingGame同名入口与预览也采用当前资源。

## 实现与保留内容

原着色根据各水面模型高度减地形高度计算深度，湖面与河道模型在交界处深度突然变化，形成色差。现在所有水域共用湖岸灰蓝基色[53,70,75]，波纹沿原全图采样坐标连续变化；实际模型水面高度仍用于判定覆盖，不改变湖岸轮廓或河流走向。制图脚本增加独立报告目录参数，避免覆盖以前的制图证据。

[水色审计](water-audit.json)逐像素比较原图：593,404个改变像素全部属于水域，2,463,324个非水域RGBA及整张Alpha保持一致。湖面世界坐标(790,465)米、25米半径参考区平均RGB与原图最大差1.68级；全部安全水域共享相同色相，水波仍有明暗细节。629对真实模型交界像素的RGB最大通道差中位数由8降至0，当前95%分位为3，属于原波纹变化。[制图清单](cartography.json)绑定当前PNG SHA256、原高程、水域及对象指纹。

原地形凹陷／坡峦、区域名称、红蓝火焰、烟雾材质、48点混合折角轮廓与90.9798%面积、M默认2倍近景及(0,30)平移均保持。新鲜场景与前轮[9项比较](scene-comparison.json)完全一致；[保留清单](preserved-data.json)核对其余237项源码／资源／配置指纹。人工Profile59文件保持，回归和预览各用TestClient/Runs独立配置与GUID新档。

## 当前验证

- 复用前轮成功且无诊断的[Development构建](../local-map-angular/build_3fb31c6c/build.json)。本轮230份编译源码／DLL均未改变，未重复编译。
- [新鲜独立游戏](verify_47a9ea03/report.json)：56/56，包括真实GameViewport的M打开／关闭／重开、默认近景、实际ActorXY、边界与凹口隐藏、缩放／平移及暂停状态。
- [图像审计](final-audit.json)：29/29，四画幅不透明、人物标记可见、轮廓外完全遮蔽、连续内部、动态烟雾、当前241项指纹与其他UI保持。
- [水色审计](water-audit.json)：12/12，保留水域与非水域、匹配湖岸颜色、消除模型色差接缝并保留纹理。
- [根CMD启动](root-launch-smoke.json)：8/8，`preview_b3cdfa7c`／PID38424，自然世界加载结束，三次Development窗口响应；观察后留给用户预览。
- 工具测试33/33、仓库检查0错误、差异格式检查通过。正式基线范围检查仍因基线缺少TASK-053获批任务快照而FAIL；当前用户授权的[增量范围](incremental-scope.json)另行核验，不替代该正式检查。

实机默认首屏：

![统一灰蓝水色](verify_47a9ea03/map-1600x1000.png)

## 复现与初次结果

在Hearthward根目录，使用包含NumPy与Pillow的工作区Python运行：

```text
python scripts/ui/draw_local_map.py docs/qa/TASK-053/local-map-angular/verify_639ec35e/scene.json --report-dir docs/qa/TASK-053/local-map-water
python -X utf8 scripts/ui/launch_map_test.py --verify
python scripts/ui/audit_local_map.py <新verify目录>
python docs/qa/TASK-053/local-map-water/water-audit.py
```

首轮`verify_d844fab8`的旧日志控制台处于“选择”状态，启动日志停在EOS插件加载，300秒未生成地图报告。原UEClient已停止自有进程，失败记录留在[startup-timeout.json](startup-timeout.json)及原运行目录。改用本机UE 5.8源码确认的`-NewConsole`异步日志窗口后，最终运行完成。初次像素审计把旧接缝恰好为8的中位数排除在严格`>8`基线阈值外；[原结果](water-audit-initial.json)保留，纠正为包含8的条件后12项通过，两次审计间产品图像没有变化。

仅本地Development测试版；没有改动三维模型、玩法或保存格式。未提交、推送、合并、Shipping打包或发布；Windows物理键鼠完整实玩与Owner视觉验收仍待进行。
