# TASK-110 高清物品图标与制作图示

日期：2026-10-11。工程 G:/GameFactory/Hearthward；分支 codex/TASK-110-item-art。
受测基线：34908e89c095de50b9283441ef88198deea20819，加本任务未提交工作区补丁。UE 5.8.2 / Windows / RTX 4060 Laptop。没有提交、推送、合并或打包发布。

## 实现
- 77 件物品全部绑定 item.<id>，使用 58 张 1254×1254 透明 RGBA 图。相同实物模型的等级变体共用对应图片。资产约 94.8 MiB。
- 背包、详情、行装、仓储、快捷栏使用同一物品映射；详情及格子保持原图比例，取消石斧详情独用旧图的分支。
- 工作台配方列表、每项材料与产出，营地加工投入／产出，建造材料，行装维修费用和背包升级材料增加图片；名称、数量、可用／缺额仍保留。
- 大字号可读布局原先丢弃图像；现在保留对应材料／产出／配方列表图片，随文本一起滚动。
- gameplay.json 的语义差异仅为 icon / quickIcon 字段；没有改变配方数值、物品属性、世界模型或存档格式。
- Resources 在现有 Build.cs 中递归打包，新文件沿用已有部署路径。本轮没有运行 Shipping 打包。

## 外观依据
[逐项映射](../../../art_source/TASK-110/catalog.json)、[生成与实物说明](../../../art_source/TASK-110/README.md)记录来源。
武器以当前角色生产模型的 UE 预览为参考，鱼类以既有动物模型截图为参考，草药修正为无红浆果的叶束。
服装、盾、镐、钓竿等没有随物品切换的独立可见角色模型，按现有材质、用途与旧图设计；没有宣称这些物品已逐件完成三维对应验收。
参考脚本的光照实际为 directional 60/30、sky 2、f2；早期 references.json 文本仍保留旧光照描述，已修正脚本后续报告文案。

## 验证
| 检查 | 结果与范围 |
|---|---|
| 77 项图片引用 | PASS；58 文件存在、RGBA、1254×1254、完整图片引用，无旧 atlas UV。见 ASSET_AUDIT.json |
| Editor Development | PASS；最终 gate-04 经 UEClient.build.project 构建 |
| Hearthward.UI.Task110.ItemArt | PASS；最终 gate-04，77 项详情绑定，100%／150% 制作和营地图标、维修及建造离屏渲染 |
| 原有制作交易回归 | PASS；gate-02 的 Hearthward.Crafting057.WarehouseMaterialsPresentation，实际仓储投入／制作与材料状态检查 |
| 原有维修入口回归 | PASS；gate-02 的 Hearthward.Equipment055.RepairEntryKeepsSelectedInstance |
| 工具测试 | PASS；88/88，python -m unittest discover -s scripts/tests -v |
| 仓库自检 | PASS；python scripts/validate_repo.py，0 errors |
| 基线任务路径自检 | NOT_PASS；基线尚无新建 TASK-110 任务快照，工具拒绝。当前任务允许路径另行逐项核对，无越界任务改动，见 SCOPE.json |

最终入口：
```
../.venv/Scripts/python.exe -X utf8 scripts/qa/run_engine_gate.py --output E:/HearthwardQA/TASK-110/gate-04 --render --tests Hearthward.UI.Task110.ItemArt
```
该目录已经存在，重跑须选新输出目录。UEClient 使用隔离 UserDir 和临时存档池。截图来自真实 Slate/UMG 渲染与原生夹具，未操作玩家存档；不等同完整键鼠游玩、Shipping 或全路线验收。

## 实际画面
- [背包草药放大](screenshots/inventory-herb-100.png)
- [石斧对应外观](screenshots/inventory-axe-100.png)
- [鱼类](screenshots/inventory-fish_carp-100.png)
- [种子](screenshots/inventory-seed_grain-100.png)
- [工作台材料与产出](screenshots/crafting-100.png)
- [150% 字号制作图文](screenshots/crafting-150.png)
- [150% 字号维修与背包升级](screenshots/equipment-150.png)
- [营地加工](screenshots/camp-100.png)
- [150% 字号营地图文](screenshots/camp-150.png)
- [建造材料](screenshots/building-150.png)

## 失败记录与修正
gate-01 首次编译暴露 UE 5.8 JSON 共享字符串转换及 FVector2D 窄化转换，已修正。
gate-02/03 的新测试未正确进入工作台：夹具起初没有工作台，补齐后主角仍位于地板内，视线检查拒绝。最终夹具采用现有交易测试的角色高度和真实工作台入口，gate-04 通过。没有放宽游戏工作台入口条件。
gate-02 的截图还发现大字号丢图和维修费用换行挤压，均在最终截图中修复。
历史完整输出保留于 E:/HearthwardQA/TASK-110；final-* 为最终定向结果，transactions-* 为前一轮两项交易／入口回归证据（该轮整体失败属于新夹具，不能将该轮整体标为通过）。

## 自行验证
打开 G:/GameFactory/Hearthward/Hearthward.uproject，在 UE 5.8 中运行游戏，Tab 背包查看图片；靠近工作台打开制作；营地管理 → 设施管理查看每批投入和产出；行装管理查看维修和升级材料。
本任务仍为 Active，等待 Owner 外观验收。旧 v0.3.0 发行包没有更新。
用户原有旧 GDD 文件删除保持原样，本任务未处理该文件。
