# TASK-071｜完成全系统存档迁移和时间线回归

2026-10-03，设计Approved／施工已授权，实施按依赖推进。原稿073，阶段D，P0；Owner／Reviewer：XLingyyy，无Issue。

依赖：TASK-052、TASK-054、TASK-055、TASK-057、TASK-058、TASK-059、TASK-060、TASK-061、TASK-062、TASK-063、TASK-066、TASK-067。基线、权限和共同规则见[BASELINE](BASELINE.md)、[DECISIONS](DECISIONS.md)、[INTERFACES](INTERFACES.md)、[ACCEPTANCE](ACCEPTANCE.md)。

## 当前基础与目标

当前schema9、Compatible-v8档池及更新兼容、备份和52时钟迁移已存在。各领域新增字段必须随所属任务保存，本单对最终实现做真实旧档迁移、全系统一致性和迟到时间线验收，不能等到071才补基本存档。

## 快照与旧档矩阵

最终快照同时包含玩家／弟弟位置与生命、饥饿due／半药／疗程、A／W／ClockVersion、装备实例及耐久、背包／共享仓储／地面物、建物变换／投入／等级、唯一人物分配、生产输入／小数工时／公共池、资源Due、动物代次／尸体／掉落、鱼成功序号／图箱／待领取、作物照料／个体家畜／蛋奶、正式任务／救援／增援／旗帜／Victory、伙伴事实／权限／约定／事件。以当时实际字段为准，不保存重复派生摘要或Actor指针。

取可追溯的真实旧Demo档、批准043—052各schema代表、当前待发行档，覆盖序章／切片／成熟生态／夺回前／永久Victory／双营地，逐档迁移核对关键事实。仅支持已有批准迁移链；缺字段默认值不得发装备、材料、经验、蓝图或免费设施。旧时代W保留，ClockOrigin迁移沿052，不按墙钟离线算收益。

## 恢复与故障

先停输入／旧异步写入，创建新epoch，恢复各领域同一快照，再重建场景和交互。取消旧模型／导航／动画／UI请求及长任务结果；同档重复读取不取得未来产物。保存稳定边界、危险自动存档延后一个待办、50全保护不覆盖、写临时后成功替换沿CT003。

原池／备份保留；未来schema和未知损坏只报原因，禁止自动删档。升级兼容冲突逐项预览，取消不变，确认先备份。schema头与payload不一致拒绝，部分领域无效不得悄悄接受半世界。只在实际新增持久字段需要时升级schema，并协调全部消费者，编号本身不触发升版。

## 最小验证与出口

定向原生用每类代表档／同刻领域事件测保存恢复、旧档迁移、保护满池、失败写不破原件、未来档拒绝；真正运行保存→退出→启动→继续，涵盖后台作业、图箱待领取、生态Due和双营地。真实异步模型回复与UI回调跨读档一次，结果0结算。输出字段／迁移清单、原档来源、预期和实测；不得提交个人档或把虚构fixture称真实玩家旧档。

## 建议施工范围

- `Source/Hearthward/Save/`
- `Source/Hearthward/Update/`
- `Source/Hearthward/Tests/SaveTests.cpp`
- `Source/Hearthward/Tests/WorldClockTests.cpp`
- `领域快照消费者仅限复现缺陷所需精确文件`
- `docs/qa/TASK-071/`

Owner已确认本方案并授权施工；上述范围在任务激活时按准确文件登记到allowed_paths。契约沿用：[CT-003-save-load](../../contracts/CT-003-save-load.md)、[CT-TASK-043-world-time-persistence](../../contracts/CT-TASK-043-world-time-persistence.md)、[CT-TASK-052-clock-refresh](../../contracts/CT-TASK-052-clock-refresh.md)。Shared Save／gameplay.json／主地图变更先在所属单登记准确边界；不因列入建议路径而自动授权。
