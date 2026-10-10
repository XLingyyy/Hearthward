# 候选14：美术接触修正与补验

2026-10-10，内部候选，未发布，TASK-095—098保持Active。

运行目录：`F:/HearthwardDemo/iteration-084-103-20261010-14/Windows`。冻结源码 `b975b91965c3a2f02fcbe4c2858097d1bc6ad236`，构建前工作树干净，Shipping Build/Cook/Stage/Archive实际成功。内嵌版本仍为`0.2.0-preview.20261009.3`，以CandidateID区分。未重复执行候选13的131包容器清点，不迁移其检查信用。

## 本轮交付

- [095步态](../locomotion/REPORT.md)：主角/弟弟按实际片段位移和相位校准；4项原生通过，576次PIE采样。主角350cm/s时支撑脚滑动中位数约238→18cm/s，未修改游戏移动速度。
- [095军需员](../grounding/REPORT.md)：模型与胶囊半高差导致的额外8cm悬空修复；1项原生及恢复后画面通过，NPC胶囊保持。
- [096石堡](../../TASK-096/route-review/REPORT.md)：卧室外走廊增加灯具，昼夜近场复核及净空回归通过。
- [097路线](../../TASK-097/walking/REPORT.md)：九节点昼夜18段局部贴地移动、72样本、36截图留档；接近视角检查完成。局部限帧对照不能证明整体性能达标。
- [098营地](../../TASK-098/closure/REPORT.md)：16份实际Slate尺寸/字号渲染、两营地保存恢复13项检查、联合原生6/6通过。联合原生有2条旧夹具警告，未隐藏。

本轮合计11项原生通过；工具测试33/33，仓库验证0错误。详见各报告及[evidence.json](evidence.json)。

## 独立Shipping开局

使用公开UEClient启动CPU后端和隔离UserDir，经Computer Use真实键盘Return从新游戏进入石堡卧室。已观察角色、弟弟、HUD、遗物距离标记与室内美术，窗口在当前桌面内。证据：[观察](observation.json)、[启动](launch.json)、[截图](opening.png)、[关闭](stop.json)。自有进程由原UEClient正常关闭。启动器已准备，本轮未验Explorer双击。

## 保留事项

完整剧情实玩按用户要求后置。坡地连续动作、节点间连续通行、水岸/分区连续检查、旧档兼容和独立重启恢复、OS DPI矩阵、最终性能和第二台机器仍未完整验证。Owner美术签收及来源账户/输入权利证据仍待闭合。因此整单仍Active，不将本次局部成功记为完整验收。
