# Windows Demo 0.2.0-preview.20261006.1

用户要求先完成开场地图并交付当前可玩的Demo。目标为本地Win64 Shipping便携发布包，打包目录 `F:/HearthwardDemo/20261006`；已按用户后续授权发布GitHub预览Release，未上传商店。

源码基线 `53a1efd0ddf169a36265930aea63b58cabfaf061`，分支 `codex/TASK-077-hometown-stonehold` 加当前未提交改动。随包BUILD-INFO.json记录版本、分支、基线和改动路径；保留打包时源码状态；随后按用户授权提交同一实现至main，提交后未重新打包。

## 本次开场范围

- 独立石堡建筑群：双床卧室、回廊、楼梯、庭院、侧门、城墙、角塔、主堡和六栋石屋。
- 第二版Marble仅裁出103680三角形的立面，保留原UV与贴图；原生背墙、基座和收边供给完整结构，游戏碰撞独立于生成网格。
- 源模型使用无光照材质。公开材质绑定增加显式unlit选项与Tint参数，原生建筑Actor按既有世界时钟调整昼夜颜色。此立面不会独立响应局部灯火；可行走区域仍使用原生受光材质。
- Cook显式收录TASK-077及已使用的自然石材目录。随包部署原有新版UI、玩法数据、本地模型与CPU/Vulkan运行库。

## 验证状态

| 检查 | 实际结果 |
|---|---|
| Development Editor构建 | PASS，见 [构建记录](../../qa/TASK-077/demo-build.json) |
| 卧室、通路、家乡相关原生检查 | 4/4 PASS，见 [原生记录](../../qa/TASK-077/demo-native.json) |
| 自然地图新游戏、取护符、跟随、7段实际输入行走、撤离 | 16/16 PASS，见 [路线记录](../../qa/TASK-077/demo-walk.json) |
| 昼夜总览与开场画面 | 已检查，见 [夜间](../../qa/TASK-077/demo-night.png)、[日光检查](../../qa/TASK-077/demo-daylight.png)、[回廊](../../qa/TASK-077/demo-gallery.png) |
| Win64 Shipping Build/Cook/Stage/Archive | PASS，约10分钟，见 [打包结果](package-result.json) |
| 独立发布包实际启动、菜单、保存与恢复 | PASS，见 [实机记录](packaged-smoke.json)、[保存](manual-save.png)、[重启恢复](restored.png) |

路线检查使用Pawn移动输入和实际导航，未沿路线传送；不等同于物理键鼠通关。日光总览是临时检查条件，正式开场仍为第1日夜间。

## Demo边界

本版为可玩开发预览。主堡、角塔和石屋没有完整可探索内室；精细建筑美术、街巷细节、完整夜袭过场和火烟演出仍未完成。现有夜袭流程、战斗、伙伴与营地系统保留。模型自由语言能力、完整游戏节奏及第二台机器兼容性未在本次完整验收。

本轮没有新增Marble积分消费。原始生成文件与失败/对照素材保留在art_source和QA目录，未作为整堡直接加入发布包。实际交付仅包含经过裁分的三项新增uasset；LFS锁保留待集成。

## 本地交付

- 可直接运行：`F:/HearthwardDemo/20261006/Windows/启动游戏.cmd`；CPU入口位于同目录。
- 便携ZIP：`F:/HearthwardDemo/20261006/Hearthward-Demo-0.2.0-preview.20261006.1-Windows.zip`。完整解压，保留全部目录。
- 实际启动的Shipping包完成新游戏、Tab背包、M地图、X跟随、F6手动存档、关闭重启继续；恢复位置、跟随状态与1/3进度。测试存档位于独立QA/Profile，未装入发布包。
- [标题](title.png)、[卧室](bedroom.png)、[背包](inventory.png)、[地图](map.png)。低帧率场景记录位于交付目录`QA/packaged-session.mp4`，不含UI且仅记录卧室会话，不代表完整通关视频。
- 当前包为本机验证的开发预览；精细开场地图尚未完成最终美术验收。本轮工作按用户后续明确授权提交并集成main；Git记录为最终依据。

## GitHub发布

用户明确要求同时发布可执行Demo。已公开为[预览Release](https://github.com/XLingyyy/Hearthward/releases/tag/v0.2.0-preview.20261006.1)，标签`v0.2.0-preview.20261006.1`对应游戏实现提交`e34a51864e2fb84a0541e0ca2fc7f33dc765444f`。

GitHub单附件限制小于2GiB，原始4876019470字节ZIP按1887436800、1887436800、1101145870字节拆成三个分片，附`Combine-Demo.cmd`和`DOWNLOAD-README.txt`。下载五个文件到同目录，运行合并脚本后解压。发布前核对全部附件状态与大小，原始游戏二进制未改动。

包内BUILD-INFO及RELEASE-NOTES保留打包时记录，发布页明确说明后续main集成与公开状态。此次仅发布分片并同步文档，没有重新编译或替换游戏资产。
