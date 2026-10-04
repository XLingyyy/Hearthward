# 连续不规则地图范围测试 · 2026-10-04

历史证据：本报告仅绑定当时源码／资源。最新M地图参考近景首屏与验证见[当前报告](../local-map-entry/REPORT.md)。

本轮将地图圆形裁成具有不对称凹口、边缘不规则的连续形状，内部没有分岛或孔洞，保留原圆形约91.8%的几何面积。根目录[地图测试版.cmd](../../../../地图测试版.cmd)以独立新档启动并自动打开M地图。[本轮历史预览](verify_ba7eadec/map-1600x1000.png)与[移动后红蓝火焰](verify_ba7eadec/map-separated-flames.png)来自本轮真实Development游戏渲染；上层根目录同名预览现以最新首屏报告为准。

## 实现与范围

24个非对称径向控制点通过周期三次插值形成连续闭合边界。半径始终为正，且不超出原250米，内部连通；保留率91.75798%，面积约180,166平方米，边界距出生中心约214.87—248.87米。黑雾在边界及外侧完全不透明，软过渡和流动烟丝位于内侧；几何边界固定。人物可见判定与烟雾共用边界函数，实际XY的等比例投影保持，进入裁掉的凹口时隐藏。

出生中心、原500米上限、五区域名坐标、地形／地表／烟雾／透明火焰原件保持；没有重新生成美术、修改Content资产、三维地形、Gameplay或存档格式。[保留数据与素材指纹](preserved-data.json)及[场景9项比对](scene-comparison.json)可复核。任务前[15份快照](before/)保存既有未提交成果。

## 验证

- [Development构建](build_904f2616/build.json)：成功、0诊断，经GameFactory UEClient公开API并显式指定本工程。
- [独立游戏](verify_ba7eadec/report.json)：49/49，包括真实出生、高空观察、63,001个高程点、动态XY、缩放／平移、暂停及返回。新轮廓边界上显示、外侧1厘米隐藏、原圆边隐藏及原圆内部凹口中的弟弟隐藏均通过。
- [图像与当前指纹](final-audit.json)：28/28。四画幅显示区全部连通；四画幅及缩放／平移中，雾下替换紫红测试层后，轮廓外每个像素差均为0，内侧大量可见测试层证明对照有效。当前241项源码／DLL／资源指纹匹配，人工Profile59文件、其他UI页面和布局保持。
- 同组烟雾对照冻结时钟；偏移12秒后外侧1,099,609像素发生变化，平均绝对RGB差5.323，暂停世界仍流动。放大湖面穿过真实视口留边，7,456个对应水面像素可见。
- [根CMD启动记录](root-launch-smoke.json)：PID39820／`preview_f287db09`已启动可见游戏并完成自然世界加载，无致命错误，随后正常关闭窗口；7项启动／日志核对通过。窗口在观察前关闭，连续响应样本明确记为NOT_RUN，没有重新打开窗口或声称Windows物理键鼠实玩。
- [工具测试](tool-tests.txt)：33/33；[仓库自检](repo-validation.txt)：0错误；[差异格式检查](diff-check.txt)：通过。[交付完整性](delivery-integrity.json)绑定当前预览、验证、资料和文档指纹。

已目视核对当前根预览。另可查看[1920×1080](verify_ba7eadec/map-1920x1080.png)、[1280×720](verify_ba7eadec/map-1280x720.png)、[超宽](verify_ba7eadec/map-ultrawide.png)和[缩放／平移](verify_ba7eadec/map-zoom-pan.png)。首次[图像审计](initial-audit.json)误报4项连通性失败：Pillow从数组产生只读图像，填充未执行；显式复制为可写图像后，保留全部检查并验证填充种子及全部连通像素，最终28项通过。

## 使用、复现与限制

双击根目录入口，M／Esc回世界，WASD移动，X跟随／Z等待，再按M看位置；滚轮缩放、方向键平移。普通TestClient也使用当前Development模块。自动回归只用TestClient/Runs独立配置／GUID池，人工资料不覆盖。

构建／实机验证：`python -X utf8 scripts/ui/launch_map_test.py --build` 和 `--verify`。用具备NumPy／Pillow的工作区Python运行 `scripts/ui/audit_local_map.py docs/qa/TASK-053/local-map-irregular/verify_ba7eadec` 可复验图像；游戏运行不依赖这些库。

分支 `codex/TASK-053-title-wheel`，HEAD `4db5789184fe38e041d62a1e68c8517338ea0b01`，仍未提交、推送、合并或发布。仅本地Development显示层测试；Shipping重新打包及Owner完整实玩／视觉验收未执行。正式[基线范围检查](baseline-scope.txt)因缺TASK-053获批快照失败，[本轮增量范围核对](incremental-scope.json)单独记录，不能替代正式检查。此前[地形／暗雾报告](../local-map-relief-fog/REPORT.md)继续保留历史证据。
