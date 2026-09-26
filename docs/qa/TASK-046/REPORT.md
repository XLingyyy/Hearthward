# TASK-046 营地经济实施验证

2026-09-26，Windows 11、UE 5.8.2、Win64 Development Editor、RTX 4060 Laptop。Owner已批准D1—D6；沿canonical046实现，原稿编号048。任务分支已同步main@2dab1f86223bd71890e20f6ed45eef644d0729ca，未合并main。

## 受测版本与结果

- 最终运行源码：`d9ba86d44007783ad7d74069695fd79f55d1c9e8`。Editor构建通过；实际PIE **65/65通过**。后续只更新测试录像观察器、文档和证据，无玩法源码或数据变化。
- 原生相关 **25/25通过**，0失败、0测试警告，受测源码`d3441673b5025911fb13c4f700da03051947874f`。随后d9ba86d仅清空启停队列后的缓存显示文字，不改变这些测试覆盖的状态、库存、保存或时间逻辑，因此未重复运行原生套件。
- 仓库工具 **33/33通过**；源码与工具未因后续文档整理改变。仓库结构和允许路径自检通过，详见`repo-validation.txt`。范围批准基线`482cc48681ce7f309eb36eb8421a379d5c8f2229`。
- 测试存档全部使用独立随机`HearthwardSaveTestPool`；没有使用或覆盖用户档。夹具不保存Content资产。

证据：[构建](build.json)、[原生命令](native-final-command.json)、[25项报告](automation.json)、[PIE启动](pie-launch.json)、[65项结果](results.json)、[PIE脚本](verify_camp_pie.py)。

## 实际验证范围

原生覆盖有限来源60游戏日收支、整餐扣点、无源停产、分帧与八小时结算等价、耗尽刷新／占地暂停、批次恢复、人物跨营地唯一分配、五身体岗位、兄弟实际工效与睡眠停止、II级速度、真实投入和缺料等待、救援去重、累计退款、畸形快照、个人／共享建造预留；新增成长存档回归验证三阶弟弟120生命／110耐力合法、超上限拒绝。现有库存、存档、时钟、生存、建造／加工逻辑回归一并通过。

PIE使用真实角色、建造组件、仓储、营地系统、时钟、保存和UI命令：

- 工作台72木材、五秒建造、移动中断释放预留、施工中禁止保存；通过真实仓储转移提供明确测试材料。
- 救援事实去重、面板提交材料升二阶、烹饪设施和累计40点捐粮解锁三阶；材料按实际表扣除。
- 即时木2→绳1、独立工作台队列、投入只扣一次、工作台II升级期间暂停并保留批次。
- 配置三组有限食物点并分配4人；床睡眠一次推进480W且A不变，完成5份采食和II级加工，兄弟饥饿推进、不自动吃饭；公共餐实际扣5点。
- 实际搬迁保留账本和投入；取消活动批次须确认，已投入不退；拆烹饪设施返木192／石96，即实付材料各80%向下取整。
- 保存后修改再回档，拒绝旧epoch，准确恢复设施实付、源点、批次与口粮；故乡胜利回调夹具生成仓储1、床2、篝火1，赠送账本为零。
- 结束第一轮PIE、重开世界、从磁盘继续，恢复两营地设施、同一阶位与获救人口。

本轮通过游戏UI处理函数和组件公开接口驱动，不声称物理键鼠操作或正式自然地图路线验收。65项检查列表以results.json为准。

## 画面与录像

已目视检查[发展](growth.png)、[分工](workers.png)、[设施](facilities.png)、[口粮](food.png)及[世界画面](world.png)，营地四页文字与操作项在当前视口可见，无相互遮挡。沿用已有皮革面板、家具和角色资源，没有新增专用设施模型或劳动动画。

[实际PIE录像](camp-pie.mp4)采用引擎Shot SHOWUI约每0.5秒以上采一帧，并按采样时刻生成可变帧率视频；包含编辑器窗口，非高帧率动作质量或性能证据。采样数、时长和编码命令见[录像记录](video.json)。

## 修复与失败历史

1. 首轮原生23/24：资源选择总返回刚刷新的首个点，中断其他点采收，破坏长期轮换。改为优先采完存量较少的可用点，随后6/6营地复测、最终25/25通过。
2. PIE首次推进到存档后被拒绝：兄弟保存上限仍固定100，与营地成长冲突。按存档阶位校验，新增实际完整SaveGame池的上限回归；最终存读档和跨PIE通过。
3. 提交后的unity构建发现Bed参数与其他源文件全局名冲突；改为BedId，最终Editor构建通过。
4. 视觉检查发现启用生产时仍显示旧“已暂停”；启停后清空缓存提示。
5. Python夹具尝试访问Bootstrap营地不存在的仓储组件、调用未暴露的Actor方法；改用仓库既有Development测试入口。录像观察器跨PIE保留失效世界指针，已在结束世界前清空，并保证观察器错误写入报告。

简要记录见[failure-history](failure-history.json)。最终通过统计不包含失败尝试。

## 可复现命令

从GameFactory使用其现有Python环境、公开UEClient，显式指定本任务Hearthward.uproject和`G:/UnrealEngine/UE_5.8`：

```python
u.build.project(target="HearthwardEditor", configuration="Development", timeout=1200)
u.testing.run_automation_tests(
    "Hearthward.Camp+Hearthward.Inventory+Hearthward.Save+Hearthward.Time+Hearthward.Survival+Hearthward.Gameplay",
    report_dir="<task>/Saved/Task046/native-final",
    extra_args=["-NullRHI", "-culture=en",
      "-ini:Engine:[/Script/EngineSettings.GameMapsSettings]:EditorStartupMap=/Engine/Maps/Entry"],
    timeout=300)
u.runtime.launch_editor(map_path="/Game/Hearthward/Bootstrap/L_Bootstrap", extra_args=[
    "-NoSound", "-HearthwardSaveTestPool=<unique UUID>",
    "-HearthwardAIBundlePath=G:/GameFactory/Hearthward/Runtime/LocalAI",
    "-ExecutePythonScript=<task>/docs/qa/TASK-046/verify_camp_pie.py"])
# 等待Saved/Task046/pie/results.json，检查ok及每项checks，再由同一UEClient停止本次PID。
```

仓库内执行`python -X utf8 scripts/validate_repo.py --task TASK-046 --base 482cc48681ce7f309eb36eb8421a379d5c8f2229`及`python -X utf8 -m unittest discover -s scripts/tests -v`。

## 内容和验收边界

- 正式食物物种、自然地图安全源点／容量／路线仍由048配表。046实现有限点位接口；本次三组各16份为显式测试夹具，不在自然地图凭空补来源。四人基本生活尚未构成自然图实玩循环。
- 正式营救、故乡夺回关卡未新增。系统接可信完成回调；不会把Demo击杀或模型文本当正式胜利。
- 普通族人是人口／岗位状态，未新增20个个体模型或日常动画。兄弟需要实际到达岗位且停止其他活动；未新增劳动动作片段。
- 锻造等高级装备和菜单由047等内容单定义；046可建造／升级相应设施，没有发明未批准配方。专用设施美术继续复用现有模型。
- 营地内已登记树木使用两日耗尽刷新；石头／矿物／其他自然点没有擅加刷新白名单。营地外世界完整刷新及旅行不在本单。
- 原生、PIE和录像不等于完整十小时经济平衡、全图流送、独立Shipping安装、第二机器、正式自然地图全流程或Owner体验验收；这些均未运行。
- Owner／Reviewer均XLingyyy；Issue可选。按仓库规则，正式独立PR审查待完成，不自批或合并main。

批准前的成本展开、3分钟／15分钟净采集预算及历史验证保留于[设计阶段报告](DESIGN_REPORT.md)与[原计算器](calculate.py)，不将纸面预算计入运行通过项。
