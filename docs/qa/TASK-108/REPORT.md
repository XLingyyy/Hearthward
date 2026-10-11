# TASK-108 · M3 实现与验证

日期 2026-10-11；分支 `codex/TASK-108-m3-homecoming`；基线 main@695240e6f00a84337ac0d694a3f2bc3d91fbe5a5。证据绑定实现提交 749a272a52221ec711563a892748d26671c876ce 的相同代码/数据工作树，本轮已获提交、推送及合并授权；没有发布新版本。使用本机 UE 5.8.2、RTX 4060 Laptop 8 GB、16 GB 内存；引擎构建/测试/生命周期均通过 UEClient。

## 实现范围

- QN05 修灯：第二营地门槛、现场观察、共享仓储木材 2/绳索 2 原子消耗、修复事实、600 XP 唯一奖励。
- QN06 餐桌：整理桌面、原有合法用餐与口粮 5 点消耗、玩家实际位于故乡才记录新事实、600 XP 唯一奖励。旧 public_meal 不自动完成新任务。
- 三档生活区：1–3 / 4–6 / 7–8 阶；储物、床、工作与照明由共享等级、每营地实际设施和任务事实推导。人物复用既有 protected_03，不增加救援人口。没有新增存档字段、战斗单位或数值加成。
- 固定回应和真实成功后的既有 repair 声音事件；道具使用实际地面高度与统一桌面支撑。夜间灯修正为显式流明单位。

## 保存故障及修复

首次窗口完成两条支线并领取奖励后，F6“保存当前进度”返回“拒绝写入无效快照”，见 [失败界面](save-fixture-rejected.jpg)与[诊断快照](failed-scene.txt)。快照中胜利、驻军死亡和旗帜一致，未发现先前怀疑的驻军状态回写冲突。

实际原因：Gameplay.Discovered 包含 m3_home_lamp / m3_home_table，但这两个 ID 只存在于 campaign.locations。Gameplay.ValidateSnapshot 使用顶层 locations 校验，因此拒绝写入。修复将两个地点加入顶层目录，并在距离自动发现阶段跳过 homecoming 地点，继续由现场交互解锁，避免提前揭示和增加探索经验。保留严格存档校验；两个既有 M3 用例增加 Gameplay.SaveSnapshot 校验，覆盖真实失败点。

## 工程验证

- 最终 Editor Development 编译通过，见 [build-save-fix.json](build-save-fix.json)。初始编译失败证据保留，不计作通过。
- 首轮 29/29 原生通过，见 [native.json](native.json)，范围为 M1/M2/M3、战役、TASK-100 夹具、库存、口粮及 TASK-099 暂停/时间线生命周期。进程诊断另有 13 条 Condition failed，不属于测试断言失败；不写成“整进程零错误”。
- 最终保存修复后，M1/M2/M3 定向复测 13/13 通过（含两个新增 Gameplay 快照断言），见 [native-save-fix.json](native-save-fix.json)。进程仍报告上述 13 条 Condition failed，见 [运行诊断](native-save-fix-result.json)。
- 12 表 ID 检查通过：顶层 locations 46、quests 40；正式战役 29 条任务（8 主线/21 支线）、64 地点、80 基础敌人。两个 M3 本地生活锚点不增加 P01–P12 兴趣点数量。

- 仓库元数据检查 109 个任务、0 错误；该检查不代表 UE 玩法验证。

## 实际窗口验证

使用 `E:/HearthwardQA/TASK-108/profile` 和隔离池 `295a8607-d9d5-44cc-8d27-3aa4bddca700`。从既有 QA 旧档开始，开发夹具设置胜利/第二营地及所需材料，测试跳转到灯架/餐桌。通过 Computer Use 正常 E 完成交互、J 领取两条奖励、F6 保存。测试夹具不构成连续夺回故乡的证据。

上次已观察两条支线可完成、固定反馈、餐桌和族人、600 XP 各一次。相关截图：[修灯前](lamp-before.jpg)、[旧光照修灯后](lamp-repaired-before-light-fix.jpg)、[灯任务领奖](lamp-claimed.jpg)、[用餐](home-meal.jpg)、[两任务领取](both-claimed.jpg)。

保存修复后的实际操作结果：

- 在同一隔离旧档重做 E 查看/修灯、E 整理/用餐、J 各领取一次。夜间灯下地面与周围道具可辨，餐桌器具保持桌面接触、族人站在椅旁，无新增动作资源。[最终灯光](lamp-final-lighting.jpg)、[用餐后](meal-final.jpg)。饱食由 48 增至 88。
- F6 点击“保存当前进度”显示“快照已保存”，存档位 1/50 → 2/50，新增手动节点时间 2026-10-11 09:02（游戏 UI 显示）。见 [保存成功](save-success.jpg)。
- 通过 UEClient 关闭 PID 40080 并启动新进程 PID 41992，主菜单正常选择“继续游戏”。在第二营地餐桌前恢复，灯、器具和族人回应恢复；日志两条新支线仍显示已完成。见 [恢复场景](restarted-home.jpg)、[恢复日志](restarted-quests.jpg)。最终测试进程已关闭。
- 重启前后诊断中，四个 M3 事实、两个地点发现记录、两条 claimed 记录一致；经验保持 1350（旧有 150 + 两次 600），共享仓储木材/绳索仍为 0/0。没有重启补发奖励或退还材料。对照见 [restart-comparison.json](restart-comparison.json)；完整只读现场见 before/after-restart-scene.txt 和 before/after-restart-camp.txt。
- 两次诊断口粮半点数均为 187；两次采样处于不同世界时间，未断言动态时钟/饥饿等完全相等。5 点用餐成本与重复扣费边界由原生用例直接断言。


## 未覆盖范围

未完成连续新档→四区战斗→夺回→第二营地的整段游玩；未运行本轮 Shipping；未完成设备声音混音、全部动作接触、真人盲测和第二台实体机器验收。旧档验证只覆盖此次所用 QA 存档及原生边界用例，不是完整兼容矩阵。M1/M2 既有路线和体验缺口保留，TASK-108 保持 Active。

## 提交前检查

2026-10-11 用户授权提交、推送并合并。基于 origin/main 的 --task 范围检查无法执行：基线尚无新增 TASK-108 任务快照；未修改或跳过脚本。已按本轮授权任务单逐项核对暂存路径，全部位于 allowed_paths，旧 GDD 的无关本地删除未暂存。仓库元数据与 GitHub repo-policy 独立执行。只读诊断文本清除了行尾空格，未改变状态数据。

提交前工具自测 85/85 通过（python -m unittest discover -s scripts/tests -q），普通仓库检查 109 任务、0 错误。

源码提交已推送，通过 [PR #73](https://github.com/XLingyyy/Hearthward/pull/73) 整合；后续提交只同步 PR 链接与交接状态。合并状态以该 PR 为准。
