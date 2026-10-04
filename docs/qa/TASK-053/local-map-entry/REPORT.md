# M地图参考近景首屏 · 2026-10-04

历史证据：本报告只绑定当时源码／资源。当前局部折角边缘精修与验证见[新报告](../local-map-angular/REPORT.md)。

已把用户[附图](reference.png)的近景构图设为玩家从世界按M打开地图后的默认首屏：2倍缩放，出生中心在中央略偏下（平移0、30设计单位）。滚轮／方向键仍可调整；关闭再按M恢复默认首屏。从设置等子页面返回时保留手动视图。实际地形、不规则揭示范围及红蓝人物标记继续实时工作。

[根目录测试版](../../../../地图测试版.cmd)仍复用同一入口；[本轮首屏预览](verify_201de502/map-1600x1000.png)保留，可查看与参考同为2395×1453的[实机渲染](verify_201de502/map-reference-size.png)。原500米上限、出生中心、约91.8%面积、不规则边界及四份地图美术原件保持，见[保留数据](preserved-data.json)与[场景9项比对](scene-comparison.json)。本轮前[13份快照](before/)保存；根目录预览现以最新报告为准。

## 当前验证

- [Development构建](build_71454392/build.json)：成功、无诊断，经GameFactory UEClient公开API指定本工程。
- [独立游戏](verify_201de502/report.json)：55/55。通过GameViewport输入路径投递M，验证从HUD打开、关闭和再次打开；初次及缩放／平移后的重开均得到1700设计单位底图、(836,500.5)中心。覆盖四画幅、参考尺寸、实际ActorXY、凹口／边界、动态刷新、暂停和返回。
- [图像／面积／连通性审计](final-audit.json)：28/28；四画幅及手动缩放／平移时，雾下替换测试层对轮廓外每个像素影响为0，四画幅显示区连通，红蓝火焰可见。缩放后的顶部留边仍有3,022个水面像素，没有直线截断。
- [241项源码／DLL／资源指纹](verify_201de502/source-manifest.json)与当前一致；人工Profile59文件及其他UI页面／布局保持。[交付完整性](delivery-integrity.json)绑定当前说明、预览和审计脚本。
- [根CMD启动观察](root-launch-smoke.json)：8/8，`preview_f0f68d9b`／PID2796完成自然加载且三次窗口响应正常；观察结束时窗口留给用户。[工具测试](tool-tests.txt)：33/33；[仓库自检](repo-validation.txt)：0错误；[差异格式检查](diff-check.txt)：通过。

初次[像素审计](initial-audit.json)在720p及超宽软雾边缘各报告1个孤立像素。原因是将渐变对照差值截在80，低于80的可见连接被排除；[0—80阈值诊断](connectivity-diagnostic.json)证明所有实际受测试层影响的像素是连续的。最终连通性使用差值大于0的完整可见集合，中心强色有效性和外侧零差遮蔽检查保持，没有修改雾边或删除检查。

## 复现与限制

双击根目录地图测试版，新游戏加载后自动打开同一默认首屏；M／Esc回世界，WASD移动，X跟随／Z等待，再按M看位置。普通TestClient亦使用当前Development模块；自动测试只用TestClient/Runs独立配置／GUID池。

构建／实机回归：`python -X utf8 scripts/ui/launch_map_test.py --build` 和 `--verify`。图像复验用工作区NumPy／Pillow Python运行 `scripts/ui/audit_local_map.py docs/qa/TASK-053/local-map-entry/verify_201de502`；游戏不依赖这些库。测试为引擎原生输入及渲染，未执行Windows物理键鼠完整实玩或Shipping打包。

分支 `codex/TASK-053-title-wheel`，HEAD `4db5789184fe38e041d62a1e68c8517338ea0b01`。未提交、推送、合并或发布。正式[基线范围检查](baseline-scope.txt)因缺获批TASK-053快照失败；[本轮增量范围](incremental-scope.json)另行核对，不能替代正式检查。此前[不规则范围报告](../local-map-irregular/REPORT.md)仅证明历史绑定版本。
