# 归火项目测试端

双击仓库根目录[启动测试版游戏.cmd](../启动测试版游戏.cmd)，从标题页新建或继续游戏。当前入口使用 TASK-076 整合版本的 Development Editor 编译结果；代码变化后需重新构建。整合报告见 [TASK-076](../docs/qa/TASK-076/REPORT.md)。这份源码入口需要 UE 5.8.2、Python、GameFactory 和本地模型环境，独立发行包尚未更新。

本机路径从被忽略的 `.agent-local/environment.json` 读取；`HEARTHWARD_PYTHON`、`HEARTHWARD_FACTORY_ROOT` 和 `HEARTHWARD_UE_ROOT` 可覆盖对应路径。克隆后先取回 Git LFS 资源。运行 `python -X utf8 scripts/ui/verify_input_client.py --build --label first_build` 编译。

普通启动的存档和设置位于 `TestClient/Profile`，日志在 `TestClient/Logs`。重复启动复用原测试档。`python -X utf8 scripts/ui/launch_test_client.py --fresh-profile` 使用新的独立档案。自动验证写入 `.agent-local/qa/TASK-076/`，使用独立 GUID 档池及配置，不覆盖人工测试档。

## 操作

| 功能 | 默认操作 |
|---|---|
| 背包、技能、日志、地图 | Tab、K、J、M |
| 建造、交流、存读档 | B、T、F6 |
| 暂停、返回 | Esc / P；打开页面的快捷键可再次关闭 |
| 地图视图 | 默认新版局部地图；右上“世界地图”进入探索、路标和传送；“局部地图”返回 |
| 地图缩放、平移 | 滚轮、方向键；世界地图右键设路标 |
| 四个道具栏 | 1 药品、2 食物、3 弓箭、4 投掷；再次按已选药品／食物／投掷栏使用 |

普通页面遵从菜单暂停设置；标题、暂停、存档页始终暂停，交流页保持世界运行。设置需要应用，返回时放弃未应用更改。界面方向键、翻页、Home/End、Esc、Enter 保留用于导航。

背包分装备、材料、消耗品、工具。鼠标左键拖放可移动或交换同类格子；装备拖入对应槽穿戴，拖出卸下，丢弃仍使用 R。消耗品和工具页可把物品拖入对应快捷栏，拖出清空；库存、耐久和动作预留均保持真实状态。Esc 取消拖动，滚轮可在拖动期间翻页。

“行装管理”保留我的背包、弟弟背包和营地仓储；转交、维修、升级继续检查原有距离、脱战、设施、营地等级和材料条件。返回按钮及 Esc 回到原背包分类。

进食和用药需要 3 个有效游戏秒，暂停冻结计时，完成后消费库存；使用期间和持续药效期间遵从道具互斥。伙伴自动进食保持原玩法。弓箭栏只选择弹药，装备可用弓后按住左键拉弓、松开射箭；切栏取消拉弓。投掷可再次按 4 或左键使用。

新版局部地图展示地形与实时兄弟位置。世界地图保留探索雾、已发现地点、任务指引、路标、驻军筛选和符合原条件的传送。局部地图预览入口为[地图测试版.cmd](../地图测试版.cmd)，使用独立预览档。

## 验证入口与历史

- 输入／加载：`python -X utf8 scripts/ui/verify_input_client.py`
- 背包拖放／行装返回：`python -X utf8 scripts/ui/verify_drag_client.py --suite equipment-return`
- 四栏道具：`python -X utf8 scripts/ui/verify_quick_client.py`
- 提示时限：`python -X utf8 scripts/ui/verify_feedback_client.py`
- 原生存档、库存、生存、战斗：`python -X utf8 scripts/ui/verify_drag_native.py`

图形验证逐组运行。以上是可复验入口，当前实际结果以 TASK-076 报告为准。原 UI 分支的历史测试、另一台电脑路径及桌面清理记录保留在[原分支交接](https://github.com/XLingyyy/Hearthward/blob/fa1828ed/docs/handoffs/TASK-053.md)，不作为当前机器的验证结果。
