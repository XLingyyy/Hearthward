# Windows Demo 0.2.0-preview.20261006.2

基于最新 main `af08e1ab`，包含 TASK-078—082。用户授权整理并发布更新。

状态：Editor、Shipping Build/Cook/Stage/Archive、定向整合与独立包检查已通过，已准备发布附件。采用独立 QA 存档目录，现有用户档与前版包保留。

流程参考 [Epic 打包文档](https://dev.epicgames.com/documentation/unreal-engine/packaging-your-project)。完整构建由公开 UEClient.build.package 执行，成功后实际运行归档程序。

## 文件整理

旧根目录 Resource、ui pic 与日期目录下的18份文件与规范归档重复，经逐字节对照后移除旧副本，规范制作源保留。详见[清单](file-cleanup.json)。源码、UE运行资产和用户存档未搬迁；README、最短阅读入口、目录说明与QA索引已更新。

## 当前整合验证

测试源码为 `f7984b273cebbf2aa3215a83e821ccc269cc6369`。随后 `0be7c5393f75789ea4cb5d3dace50dd3966844cf` 仅整理文档与旧素材副本，Source、Resources、Config、Plugins 和发行脚本无差异；Shipping包以该干净提交构建。

| 检查 | 结果 | 证据 |
|---|---|---|
| Editor Development | PASS | [构建](editor-build.json) |
| 伙伴、仓储、地图、任务指引原生回归 | 10/10 PASS | [结果](native-result.json) |
| 正式地图对话显示和4:3布局 | 10/10 PASS | [结果](dialogue-visual.json)、[画面](dialogue-world.png) |
| 弟弟真实到岗、队伍暂停续接、真实模型状态回复 | 26/26 PASS | [结果](dialogue-final.json) |
| 仓储、简约页面与字号布局 | 77/77 PASS | [结果](storage-result.json)、[仓储](storage.png)、[大字体](storage-large-font.png) |

原生测试有3项既有夹具初始化警告（CrowdManager/RecastNavMesh及LocalPlayer/PlayerInput），无失败；未读取可选用户存档诊断，因此没有把空操作计为第11项行为检查。灰盒族人测试使用受控资源和岗位夹具，真实模型输出的旧入库量由UE实时状态正确替换。

复验沿用 scripts/ui/verify_main_integration.py 对应的公开UEClient路径及原081脚本；本轮仅改写QA输出目录，断言和夹具保持。仓储使用现有 HearthwardStorageVerify 入口。所有测试使用隔离存档目录。

范围检查记录：首次以 f7984b27 的任务快照检查时，快照尚不含刚发现的18个旧副本路径，报告 OUT_OF_SCOPE；用户本轮文件整理授权和随后更新的任务单覆盖这些路径，未修改验证器或代填人工审查。

## 独立包与交付

完整 Shipping 构建／烘焙／部署／归档通过，耗时约3分50秒，见[打包结果](package-result.json)和[源码标识](build-info.json)。

通过实际 Windows 键鼠操作完成15项检查：新游戏进入卧室、任务标记、新右侧对话、族人表单与非安全营地拒绝原因、地图打开／拖动／缩放、背包、弟弟跟随、手动保存、关闭后重新启动并继续。恢复卧室、跟随状态及任务标记。见[实机记录](packaged-smoke.json)、[对话](packaged-dialogue.png)、[地图](packaged-map.png)、[保存](packaged-manual-save.png)、[恢复](packaged-restored.png)。

测试存档位于独立 QA/Profile，未纳入发行ZIP；未修改用户原档。原生场景录像位于本地 `F:/HearthwardDemo/20261006-2/QA/packaged-session.mp4`，310帧、2fps，不含UI，仅记录卧室与跟随，已检查代表帧，不代表完整通关。

- 可运行目录：`F:/HearthwardDemo/20261006-2/Windows/启动游戏.cmd`。
- 完整ZIP：`Hearthward-Demo-0.2.0-preview.20261006.2-Windows.zip`，4,882,366,341字节，150个文件。
- GitHub附件：三个小于2GiB的分片、Combine-Demo.cmd 和 DOWNLOAD-README.txt。文件清单和大小见[交付记录](delivery.json)。
- ZIP文件目录检查通过；分片总大小与原ZIP一致。未重复计算全包散列或做无证据需求的全量校验。
- 包含本地AI模型、CPU/Vulkan运行库；不需要UE或Python。旧包保留，建议新版解压到新目录。现有游戏更新检查只查正式Release，本次预览需从发布页手动下载。

本次没有重新验收精细美术、完整夜袭演出、全游戏语言理解矩阵、完整通关或第二台机器。族人产出与真实模型状态回复在PIE验证；独立Shipping检查覆盖上述明确列出的流程。
