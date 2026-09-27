# TASK-049｜实现验证

环境：Windows，UE 5.8.2，Development Editor，RTX 4060 Laptop 8GB。工作目录为独立 `Hearthward-task049`，分支 `codex/TASK-049-narrative-layout`。Owner／Reviewer为XLingyyy；Owner已批准设计、施工和提交推送。真人验收尚未完成，任务保持Active，不创建Issue、不合并main。

当前实现完整SHA：`9374cadbf2eeb5c0f4b54b5b955861b6f723b5dc`；后续文档补录不改变被测源码、运行数据和资产。交付说明见[交接](../../handoffs/TASK-049.md)。共享地形、水体和角色通行能力未修改。引擎构建、原生测试、编辑器启动与停止均由GameFactory的公开UEClient执行。

## 原生与静态验证

- Editor构建PASS，见[final-fixed-build.json](final-fixed-build.json)。
- Campaign 3项＋Save 7项，共10/10 PASS，见[原生报告](final-fixed-native.json)与[Automation索引](final-fixed-automation-index.json)。包括80／84／88分母、严格95%阈值、清场与增援同帧顺序、schema7实际文件迁移和旧档保留、非法救援身份拒绝，以及成长属性浮点上限回归。
- 相关未再修改模块：Camp／Nature／Save旧集合17/17、Combat／Campaign／Save旧集合11/11，分别见[natural-third索引](natural-third-automation-index.json)、[world-fourth索引](world-fourth-automation-index.json)。当前存档修改以final-fixed的10项覆盖，旧报告不替代本次存档回归。
- UE适配器日志中的`Condition failed`来自负向校验用例；Automation逐项结果与entries是测试成败依据。final-fixed的10项均Success。
- 内容关系37/37 PASS，见[content-audit.json](content-audit.json)。覆盖8主线、15支线、10名稳定救援身份、20初始人口、80驻军、增援与奖励关系，以及营地局部坐标契约。
- 源地形检查180/180点位PASS，见[terrain-audit.json](terrain-audit.json)。路线约4803.14m，13个发现节点；按3.5m/s计算22.87分钟、平均发现间隔114.36秒。这些数字来自几何计算，实测行走单独记录。
- 仓库工具自测33/33 PASS；最终L0／批准基线范围检查0错误，暂存区diff检查PASS，见[local-scope.json](local-scope.json)。

## 运行验证

[完整世界流程](final-fixed-pie.json)78项通过，最后一项标题续玩失败；据此修正入口捕获后，[胜利节点重开回归](final-continue-pie.json)14/14 PASS，M08完成。最新修正的[Editor构建](final-epilogue2-build.json)PASS。[尾声集成](final-epilogue2-pie.json)84/84 PASS，10人报到、30人口、唯一弓与重复拒绝、剩余支线实际交付／建造／种植／用餐，以及全23任务完成态保存均通过。未将失败报告改写为通过，也未重复执行与UI修复无关的清场过程。新游戏、夺回与后续救援使用真实地图与现有组件入口。必须区分正常操作与测试夹具：

- 序章：[final-natural3](final-natural3-pie.json)16/16 PASS。正常新游戏、遗物、弟弟跟随、实际角色移动、后巷出口、20人口营地、M01奖励与保存。
- 夺回：采集／建造／派工／领取／交付／占旗／仓储／保存走真实事务；物资补给、跨场景定位、缩短护送的营地边缘定位及驻军清场是明确夹具。真实处决单独验证；清除88人夹具不证明完整战斗难度。
- 救援：接触、等待／跟随、安全CampAt报到和奖励使用真实组件；营地边缘定位缩短护送距离，不把接触人数直接计入人口。
- 路线：**PARTIAL，未通过全程通行验收**。[首轮](final-route-pie.json)从营地实际行走1261.54m，到索引133的局部导航目标返回部分路径；[导航投射重试](final-route2-pie.json)复现。[局部分段](final-route-segment-pie.json)从上次位置夹具继续步行201.94m，正常跨过133缺口，到索引156、约(160m,-1040m)处再次受阻；[侧向尝试](final-route-segment2-pie.json)仍未到达该采样目标。正常碰撞、重力、步行速度始终启用，未用飞行或穿墙通过。后续约3.34km和弟弟全程自动跟随尚未验证，不能把源地形PASS或场景定位夹具写成全程可达。导航目标投射采用[Epic公开API](https://dev.epicgames.com/documentation/unreal-engine/BlueprintAPI/AI/Navigation/ProjectPointtoNavigation)。手动绕行与地形／障碍碰撞需要Owner在完整路线验收中复核。

## 已复现与修正

1. 部分初版点位位于水下或陡坡：调整同区内点位、房屋位置和南侧路线，未改共享地形。
2. 上坡门口使房屋有效门洞高度不足：地基以入口地面高度定位，实际序章出门已通过。
3. 全局营地位置原本使用局部坐标，候选数据误写绝对坐标会造成双倍偏移：恢复局部坐标并增加内容约束检查。
4. 完整进度保存被拒绝：29级＋二级营地的合法满体力，在双精度校验上限前因float舍入略超限。先运行回归得到[0/1 FAIL](save-regression-before-automation-index.json)，再统一为运行时float上限，[修复后测试](final-fixed-automation-index.json)PASS，同时仍拒绝真正超上限数值。未放宽整个快照校验。
5. 胜利存档的标题续玩入口在读档期间被切回HUD，导致M08缺少续玩事实：读档前捕获标题入口，使用已保存的实际胜利节点做针对性重开回归。
6. 第7名未接触救援者原始落点与场景碰撞，Actor生成失败：只在未接触人物首次落位碰撞时投射到附近导航地面，保存实际位置；不移动已接触／护送人物。修复前[尾声报告](final-epilogue-pie.json)在第7人处失败，修复后84/84通过。
7. QA收集器的Guid格式、相机API、仓储页面动作和处决隔离夹具曾有错误；这些已按运行API修正，不作为产品缺陷计数。

## 资产与边界

34个Campaign资产已导入，8段动画重定向，模型／骨架／材质在PIE中检查。来源、导入记录、截图和LFS锁见[ASSETS.md](ASSETS.md)。所有锁保留到集成交接。

角色与建筑复用、Guard／Heavy、旗杆均为TEMP_VISUAL；弓兵没有最终持弓动作。固定配音、完整演出、Shipping、十小时不间断流程、全路线战斗平衡、真实本地模型推理和Owner独立验收均未由本报告证明。

## 复跑入口

从本工作树运行，使用GameFactory的UTF-8 Python；每次仅启动一个049编辑器。先正常序章生成营地节点，再跑世界流程；后续路线和尾声从胜利节点续玩。脚本中的物资／定位夹具仅用于QA，不写入运行规则。历史开发阶段的备用节点回退已移除，世界脚本要求最新节点为序章后的营地。

```powershell
G:/GameFactory/.venv/Scripts/python.exe -X utf8 docs/qa/TASK-049/run_engine.py --label repro-natural --script docs/qa/TASK-049/verify_campaign_natural.py --map /Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds --result Saved/Task049/natural/results.json --timeout 900
G:/GameFactory/.venv/Scripts/python.exe -X utf8 docs/qa/TASK-049/run_engine.py --label repro-world --script docs/qa/TASK-049/verify_campaign_world.py --map /Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds --result Saved/Task049/world/results.json --timeout 900
G:/GameFactory/.venv/Scripts/python.exe -X utf8 docs/qa/TASK-049/run_engine.py --label repro-route --script docs/qa/TASK-049/verify_campaign_route.py --map /Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds --result Saved/Task049/route/results.json --timeout 1200 --null-rhi
G:/GameFactory/.venv/Scripts/python.exe -X utf8 docs/qa/TASK-049/run_engine.py --label repro-epilogue --script docs/qa/TASK-049/verify_campaign_epilogue.py --map /Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds --result Saved/Task049/epilogue/results.json --timeout 900
```

原生／构建入口：`run_engine.py --label repro-native --build --native 'Hearthward.Save+Hearthward.Campaign' --script ''`。静态入口：`docs/planning/TASK-049/audit.py`、`docs/qa/TASK-049/check_terrain.py`、`scripts/validate_repo.py --task TASK-049 --base e2f3834efb29bd0d3a3339d92a934033cfaa8876`。路线不会保存中途位置；开发时可用未提交的`.agent-local/route-resume.json`提供分段起点（index/x/y/z），报告会明确标记此定位夹具，完整复跑应移走该文件；尾声会保存完成状态；复跑全套前需重新从正常新游戏开始。

证据绑定说明：final-epilogue2构建与PIE、final-natural3运行上述完整源码。final-fixed原生用例所覆盖的存档、成长及Campaign状态／任务规则此后未改；后续UI入口与未接触人物落位修改分别由final-continue、final-epilogue2和final-natural3实际运行覆盖。早期失败报告及路线PARTIAL保留原结论。
