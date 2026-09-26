# TASK-048 实现验证报告

2026-09-27；Windows 11、UE 5.8.2、Win64 Development、RTX 4060 Laptop；执行者Codex。Owner／Reviewer XLingyyy已批准D1—D6、临时动物表现及施工提交推送。

- main基线：e95fde185dc38d0b69c2de42ce66db0d1fd93294（047／PR #49）。
- 施工范围基线：2617b7fccc0bd0173905b039437b3bcf69f73e7e。
- **最终受测源码：48a0b8a4a6de4c778a1ff1eb8cd2fa22a809888f**。后续提交只更新证据和文档，不改运行代码、数据、模型或受测脚本。
- 分支：codex/TASK-048-nature-content。未合并main、未执行正式独立PR审查。

## 结果

| 检查 | 结果 | 证据 |
|---|---|---|
| HearthwardEditor Win64 Development | PASS | [构建命令与输出](final-build.json) |
| 相关原生自动化 | PASS，40/40，失败／未运行均0 | [UEClient结果](final-native.json)、[引擎逐项报告](final-native-index.json) |
| 实际PIE操作与重开恢复 | PASS，91/91个具名检查 | [PIE结果](final-pie.json) |
| 正式自然地图 | PASS，19/19 | [自然地图结果](final-natural-pie.json) |
| 仓库及TASK-048路径范围 | PASS，0 errors | [范围检查](scope.txt) |
| Python仓库工具 | PASS，33/33 | [工具日志](tools.txt)；本单未修改工具代码 |
| 原设计候选关系 | PASS，38/38 | [纸面验算](calculation.json)，仅绑定历史设计提交bbbcda9c3eec0decf53368560927b515e1cbe2a6 |

原生进程启动日志在Initialize Engine之前记录了13条“Condition failed”；所选40项测试的引擎逐项报告均Success、errors=0、warnings=0，进程返回0。原始诊断完整保留，不将“测试通过”写成启动日志完全无告警。此记录未修改引擎或过滤测试结果。

## 实際覆盖

PIE使用明确的平地和材料夹具，调用实际UI／组件操作并等待真实动作完成：

- 落枝裸手采集五秒、背包增加与营地同一来源扣减。
- 播种、浇水、施肥；通过已有世界日历入口推进六次八小时，成熟收获6份并返种。跳时为显式测试调用，不冒充这轮实际操作了床睡眠。
- 实际张力按住／放开完成渔获；抛竿后取消耗饵但不减库存／竿耐久；成功减一鱼和一耐久；钓鱼中拒绝保存与跳时。
- 背包使用藏宝图、生成地点、五秒领取批准的弓和金属锭、持久一次性领取事实。
- 野生鹿走现有三秒处决、仅一次30经验、无额外人类清敌事件、拾取6肉；两日后原出生槽生成新GUID。
- 实际建工作台与营地升阶、栏舍准确实付账本；两只家畜捕捉付费、预留栏位、实际跟随入栏；完整饲料窗口、两日出生幼崽、家畜不导致营地停产。
- schema7同节点还原全部自然状态，旧Epoch拒绝；退出PIE后重新开始并“继续游戏”，保持动物身份、栏舍／鱼点／奖励和全部装备实例。

原生覆盖鱼种挣扎上下界、持续按住／放开失败、固定种子2%样本、长跳时只恢复一轮鱼点、缺料暂停／成长／繁殖、真实schema6文件迁移与原字节备份、待领取装备GUID不可跨容器重复，以及现有时间、营地、战斗、伙伴护卫、制作／维修、装备／库存、交互和存档回归。

正式自然地图使用与产品标题页相同的GameMode覆盖，只作用于本次编辑器内存，未保存.umap。新游戏生成8种野生动物、12只起始家畜、4个湖边鱼点及全部批准资源数量（12树／6石／4普通矿／2精矿等）；现场按E采到木材、只存在一个原生交互入口。为了定位观察而移动玩家；未把定位操作当作全图步行可达证明。

## 复现命令

在任务工作树中使用G:/GameFactory/.venv/Scripts/python.exe -X utf8：

```text
docs/qa/TASK-048/run_engine.py --label final --build --native "Hearthward.Nature048+Hearthward.Save+Hearthward.Camp+Hearthward.Combat+Hearthward.Time+Hearthward.Gameplay+Hearthward.Inventory+Hearthward.Actions+Hearthward.Companion+Hearthward.NPCAgent.CompanionCombatPolicy+Hearthward.Resource" --timeout 900
docs/qa/TASK-048/run_engine.py --label final-natural --script docs/qa/TASK-048/verify_natural_world.py --map /Game/Hearthward/World/Natural/Rebuild/L_HearthwardWilds --result Saved/Task048/natural/results.json --timeout 900
scripts/validate_repo.py --task TASK-048 --base 2617b7fccc0bd0173905b039437b3bcf69f73e7e
```

同一个UEClient拥有并停止对应编辑器，停止结果见[PIE停止](final-stop.json)和[自然地图停止](final-natural-stop.json)。详细入口／固定夹具和观察方法保留在[PIE脚本](verify_nature_pie.py)、[自然地图脚本](verify_natural_world.py)；完整失败定位见[首次失败与修复](first-failure.md)。

## 画面及限制

- [钓鱼面板](screenshots/fishing-panel.png)、[渔获结果](screenshots/fish-success.png)、[退出重进后的栏舍](screenshots/disk-restored-family.png)。
- [自然地图采集](screenshots/resource-harvest.png)、[湖边入口](screenshots/lake-fishing.png)、[地形观察位置](screenshots/natural-ground-observation.png)。
- 14件静态模型已导入，野猪复用猪、默认材质、无逐物种骨骼动作，头部为独立简化碰撞区。动物最终视觉、专用动作和水下生态：NOT_RUN／未制作，按Owner指示保留后续精修。
- 全部物种完整实战组合、自然路线长程牵引、4032米全图通行／流送／长期性能、最终经济平衡、Shipping和十小时剧情：NOT_RUN。
- 当前证据为定向玩法验证；不表示主干已集成或Owner已完成最终体验验收。新增内容权威源为Resources/Data/gameplay.json，原38项纸面检查不代替运行验证。

工程依据沿[设计](../../design/DSGN-R15-nature-production.md)中的Epic官方Data Driven Gameplay与Random Streams资料；没有增加插件或依赖。
