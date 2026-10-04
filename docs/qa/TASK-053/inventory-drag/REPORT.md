# TASK-053 · 背包拖放与分类布局（2026-10-03）

当前未提交工作树为`codex/TASK-053-title-wheel`，基线完整SHA为`4db5789184fe38e041d62a1e68c8517338ea0b01`。用户批准Tab背包四页的拖放、即时装备／卸下和属性更新、材料整页、消耗品／工具四栏配置及药品文案更正。后续统一使用项目内TestClient，C盘桌面安装不恢复。

## 当前操作

| 页面／目标 | 左键拖放的结果 |
| --- | --- |
| 所有背包物品 | 拖到本分类空格移位；拖到已有物品格交换位置。空格和排列随存档恢复。 |
| 装备 → 对应当前装备槽 | 立即装备真实实例，攻击力及实际部位减伤同时更新；错误槽位拒绝。 |
| 当前装备 → 背包格子或槽位外 | 立即卸下，物品仍在背包；拖到格子同时更新位置。 |
| 消耗品／工具 → 上方四栏 | 按实际作用配置药品1／食物2／弓箭3／投掷4；类别不符提示无法放置。 |
| 已配置四栏 → 栏位外 | 清空快捷引用，库存不减少；拖回本分类格子还可改变放置位置。 |

普通点击仍选择物品；拖动中可用滚轮跨页，Esc取消此次拖放，页面切换或读档释放鼠标捕获。移动位置与配置都不消费物品，真正丢弃仍使用R。装备变更遵从原有战斗／动作限制，不能绕过正在用药或进食的互斥限制。

材料取消“当前装备”，每页30格填满左侧面板，删除“暂无此类物品”。原食物页改为“消耗品”，与工具页一样在上方显示四个道具配置槽。药草膏、半份药草膏的类型和笼统说明修正为药品，ID、数量、效果、配方和价格保留。装备页补齐手部与工具实际槽位，使这两类装备也能拖入对应位置。

此前四栏规则保持：HUD滚轮不选栏；首次按对应键选择，再次按药品／食物／投掷键使用，药食各3个有效游戏秒且互斥。弓箭栏需可用弓，由左键拉弓、松开射箭；投掷栏左键投掷；药品／食物及无弓的弓箭栏左键使用近战武器，无武器提示。操作、错误和受伤提示两秒自然消失，实时动作进度继续显示。

## 实现与保存

UI仅记录拖动源的物品ID、真实装备实例GUID、时间线代次及指针位置。Gameplay公开命令拥有装备、快捷引用和稳定物品ID的格子位置；移动只更新位置，装备不复制实例、重置耐久或扣库存。界面刷新保持捕获，读档取消旧拖动，旧代次命令被拒绝。

Gameplay存档增加可选`inventoryPositions`对象，严格验证已知物品、数值类型、整数0—499及每分类槽位唯一性；旧档缺字段时沿用内容显示顺序。快捷栏允许空引用，仍校验四个角色的实际作用。真实保存／载入验证了稀疏空格、快捷配置、装备实例、数量和耐久；进食预留也不被移动物品破坏。[CT-004](../../../contracts/CT-004-presentation-gameplay.md)已同步接口及所有权。

## 当前版本证据

| 验证 | 实际结果与证据 |
| --- | --- |
| Development Editor构建 | [build_4c57b867](build_4c57b867/build.json)：成功，零诊断。 |
| 背包真实Slate拖放 | [verify_ca0765fa](verify_ca0765fa/report.json)：120／120检查通过；实际指针按下、移动、释放、捕获及跨页。 |
| 四栏使用回归 | [verify_4ef0008e](../quick-items/verify_4ef0008e/report.json)：65／65检查通过。 |
| UI输入与回登录 | [inventory_drag_final_1565bda9](../input-lifecycle-fix/inventory_drag_final_1565bda9/report.json)：144／144检查通过。 |
| 两秒提示 | [verify_41ccaaca](../timed-hints/verify_41ccaaca/report.json)：17／17检查通过，淡出实采1.804秒。 |
| 原生回归 | [native_5e3f4b68](native_5e3f4b68/summary.json)：生存2、Gameplay6、战斗2、存档10、库存6，共26／26，零失败、零警告。 |
| 当前版本指纹 | [fingerprint-checks.json](fingerprint-checks.json)：上述五轮分别232／232／229／231／229文件与当前源码／DLL匹配，拖放及原生轮次包含gameplay.json。 |
| 仓库、工具及范围 | [repository-checks.json](repository-checks.json)：仓库0错误、工具33／33、差异空白和本地授权路径检查通过；正式基线范围审计的既有限制单独记录。 |

拖放检查覆盖同定义多实例、即时属性、错槽拒绝、拖出卸下、空格移位／占用交换、30格材料、四个固定角色、药品标签、刷新捕获、Esc取消、跨页、设置返回、真实存读档与旧请求失效、数量／耐久／预留保持。原生测试还覆盖无新字段的旧档、错误位置类型／范围／冲突和空快捷引用。

环境及调用方式见[environment.json](environment.json)：Windows、UE5.8.2、显式本项目的GameFactory UEClient公开API；独立Development游戏真实渲染1600×1000，原生组使用NullRHI。回归配置与GUID存档池均在TestClient/Runs，不覆盖人工测试档。

两秒提示首次与拖放并行运行时，采样已到2.122秒，错过1.65—2秒淡出窗口；文字消失检查通过，淡出采样检查失败。[原始失败报告](../timed-hints/verify_ae580840/report.json)保留。单独重跑17项全部通过，没有修改提示时限或删除检查。此前构建与夹具修正记录保留，但当前PASS只绑定上表最终版本。

## 四页画面

最终四张真实视口PNG已目视核对：

- [装备拖放与即时属性](verify_ca0765fa/equipment-drag.png)：12个实际装备槽与部位减伤。
- [材料整页30格](verify_ca0765fa/materials-full-page.png)：上方装备区域已移除。
- [消耗品与药品说明，150%字号](verify_ca0765fa/consumables-text-150.png)：四栏数字、药品类型／说明无遮挡。
- [工具与四栏配置](verify_ca0765fa/tools-quick-slots.png)：箭矢／投掷物配置及移动后的空格。

装备截图左上有开发引擎短暂的资源准备状态，不属于游戏背包文案；其余截图准备状态已结束。

## 复验与边界

双击[TestClient/启动测试端.cmd](../../../../TestClient/启动测试端.cmd)继续试玩。项目根目录执行：

```text
python -X utf8 scripts/ui/verify_drag_client.py --build
python -X utf8 scripts/ui/verify_drag_client.py
python -X utf8 scripts/ui/verify_quick_client.py
python -X utf8 scripts/ui/verify_input_client.py --label inventory_drag_final
python -X utf8 scripts/ui/verify_feedback_client.py
python -X utf8 scripts/ui/verify_drag_native.py
python scripts/validate_repo.py
python -m unittest discover -s scripts/tests -v
python scripts/validate_repo.py --task TASK-053 --base 4db5789184fe38e041d62a1e68c8517338ea0b01
```

图形回归逐组运行，避免资源竞争错过短时采样窗口。最后一条正式基线范围命令仍返回“No usable approved task snapshot at base: TASK-053”；基线没有此工作树新增任务单的批准快照。本地allowed_paths逐项审计通过，不替代该正式检查。

本轮没有Windows物理键鼠实玩、Shipping重新打包、C盘部署、提交、推送、合并或发布。Owner视觉／玩法实玩仍待进行，任务保留Active。此前桌面包及旧轮次报告只证明当时版本。
