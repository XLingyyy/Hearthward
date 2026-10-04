# TASK-053 · 装备腾格、投掷分类与行装管理（2026-10-04）

用户最新批准三项更改，继续只在项目内TestClient验证。当前分支`codex/TASK-053-title-wheel`，HEAD `4db5789184fe38e041d62a1e68c8517338ea0b01`；本轮修改仍在工作树，未提交、推送、合并或发布。此前轮次的PASS只绑定各自当时源码。

## 当前行为

已穿戴的真实装备实例从下方装备网格中排除，原格子留空并可接收其他物品；卸下时重新显示。同种物品有备用实例时，下方只显示未穿戴数量，不再把穿戴实例标成下方装备。实际总库存、重量、实例GUID与耐久保持，不复制或删除物品。装备全部藏起后不再凭空产生后续空页。

投掷物按实际投掷／诱饵作用优先归入工具导航，原有四栏投掷角色和使用效果继续生效。目前投掷火罐位于工具页。药品和食物仍归消耗品，四个固定角色与错误放置提示保留。

行装管理采用与背包相同的全屏不透明黑底、炭灰厚边三栏、金色标题／分隔线／选中光带。左侧是物品实例与分页，中间显示选中实例的说明及实际耐久，右侧是转交、穿戴、维修、放到地面和背包升级。上方可切我的背包、弟弟背包、营地仓储。原转交距离、脱战、结束动作、仓储设施、维修配方和背包升级条件继续由现有Gameplay命令验证。100%字号每页8行，150%每页6行，右侧说明与按钮分离。

Gameplay拥有可用数量、显示槽位和装备命令；UI只读取并展示。[CT-004](../../../contracts/CT-004-presentation-gameplay.md)已同步。保存结构未新增字段；既有可选位置字段、旧档兼容和时间线代次检查继续使用。本轮开始的16份文本快照与指纹位于[before/source-manifest.json](before/source-manifest.json)。

## 当前版本验证

| 检查 | 实际结果与证据 |
| --- | --- |
| Development Editor构建 | [build_38fe7f59](build_38fe7f59/build.json)：成功，零诊断。 |
| 背包与行装管理真实Slate操作 | [verify_0296fd47](verify_0296fd47/report.json)：177／177通过。 |
| 四栏道具使用 | [verify_43c4fea0](../quick-items/verify_43c4fea0/report.json)：65／65通过。 |
| UI输入、设置及退出回登录 | [equipment_management_final_320127ed](../input-lifecycle-fix/equipment_management_final_320127ed/report.json)：144／144通过。 |
| 两秒提示 | [verify_79652b5e](../timed-hints/verify_79652b5e/report.json)：17／17通过。 |
| 原生玩法及存档 | [native_64fd365e](../inventory-drag/native_64fd365e/summary.json)：生存2、Gameplay6、战斗2、存档10、库存6，26／26通过，零失败、零警告。 |
| 当前源码和DLL指纹 | [fingerprint-checks.json](fingerprint-checks.json)：五轮232／232／231／229／229文件均匹配当前工作树和DLL。 |
| 工具、仓库与范围 | [repository-checks.json](repository-checks.json)：工具33／33、仓库0错误、差异空白及3539条本地授权路径检查通过；正式基线快照缺失限制单列。 |

177项检查包含真实指针按下／移动／释放、捕获及跨页、所有四页拖放、错槽拒绝、装备即时属性、穿戴腾格、备用实例数量、腾出的格子放入其他装备、卸下回包、实际存读档、数量／耐久／进食预留、旧请求拒绝。行装管理覆盖普通／大字号三栏、实际耐久和维修报价、分页、堆叠物数量控制、弟弟实际实例转交及装备／卸下／转回、仓储动作配置和返回后恢复HUD输入。维修与升级的设施／支付规则沿用既有实现；本轮UI检查验证入口与真实报价，未声称逐项完成付款操作。

环境和启动参数见[environment.json](environment.json)及各轮launch.json。通过显式本项目的GameFactory UEClient公开API构建与启动，真实Development独立游戏1600×1000渲染，原生组NullRHI。所有回归使用TestClient/Runs独立配置与GUID存档池，人工测试资料未被覆盖。

## 已核对画面

- [普通字号行装管理](verify_0296fd47/management-player-100.png)。
- [150%字号行装管理](verify_0296fd47/management-player-150.png)：转交说明与按钮无重叠。
- [弟弟背包及真实装备状态](verify_0296fd47/management-brother-150.png)。
- [营地仓储](verify_0296fd47/management-storage-150.png)。
- [720p](verify_0296fd47/management-720p.png)、[2560×1080宽屏](verify_0296fd47/management-ultrawide.png)。
- [穿戴装备腾空格子](verify_0296fd47/equipment-drag.png)：截图中的斧为实际持有两件、只穿戴一件，下方显示另一件备用装备。
- [工具页投掷物](verify_0296fd47/tools-quick-slots.png)。

以上最终PNG已目视核对。第一次构建的局部变量与成员重名诊断已修正；[首次UI报告](verify_42f14bcf/report.json)保留175项中的5项失败。调查确认夹具在新页尚未重绘时点击，且弟弟仍有暂停中的当前命令；最终夹具等待Slate重绘、通过现有Cancel API结束指令并满足实际转交安全条件，未放宽生产条件或移除检查。另一次目视核对发现150%字号说明换行遮挡，修正文案和字号后重新构建并跑完上表所有当前版本回归。

## 复验入口与边界

双击[TestClient/启动测试端.cmd](../../../../TestClient/启动测试端.cmd)，按Tab进入背包，再点击底部“行装管理”。项目根目录复验：

```text
python -X utf8 scripts/ui/verify_drag_client.py --suite equipment-management --build
python -X utf8 scripts/ui/verify_drag_client.py --suite equipment-management
python -X utf8 scripts/ui/verify_quick_client.py
python -X utf8 scripts/ui/verify_input_client.py --label equipment_management_final
python -X utf8 scripts/ui/verify_feedback_client.py
python -X utf8 scripts/ui/verify_drag_native.py
python scripts/validate_repo.py
python -m unittest discover -s scripts/tests -v
python scripts/validate_repo.py --task TASK-053 --base 4db5789184fe38e041d62a1e68c8517338ea0b01
```

图形回归逐组运行。最后一条正式基线范围命令仍失败：基线缺TASK-053获批任务快照，本地allowed_paths审计独立记录，不替代该检查。README、TestClient说明、任务单与契约已同步。

当前只完成Development真实渲染与原生事件回归，未执行Windows物理键鼠逐项实玩或重新Shipping打包。C盘原游戏安装保持删除，后续测试统一本项目。任务保留Active供Owner继续试玩。
