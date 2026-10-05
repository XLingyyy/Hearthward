# TASK-071｜成熟两域实际跨进程往返记录

2026-10-04。本记录依据根执行的两个真实 Native automation 进程、原始 index 与该隔离池 manifest。子代理只读核对并整理 QA；构建、引擎执行、正式审阅及任务验收归根。本记录不宣称 TASK-071 全部完成。

## 实际执行与证据

测试入口：`Hearthward.Save.ActualLoadPointInvalidatesOldCommandTicket`，显式 `-Hearthward071RestartPhase=write/read`，同一 fresh pool `7c0ef8ac-2b09-4d90-a9bf-8bde2f179657`。

| 阶段 | PID | 原始报告 | 071 结果 |
| --- | --- | --- | --- |
| write | 20324 | `Saved/Task053/restart071-real-movement-write-guard068-mapwheel069/index.json` | Success，0 Error，0 Warning |
| read | 25716 | `Saved/Task053/restart071-mature-two-domains-read/index.json` | Success，0 Error，0 Warning |

read 原始 Info 明确记录 `writer=20324`、`pid=25716`、相同 SaveId 与 `two actual LoadPoint passes`。两个进程之间由根串行启动和退出；测试另断言 write/read PID 不同。write 的同进程组合还包含地图用例的 1 项 EnhancedInput Warning，该警告归地图测试；071 自身为零警告。

实际保存节点 `7E25958246B3A48259895E978F19FC60`；manifest `Saved/Task071/7C0EF8AC2B094D90A9BF8BDE2F179657/manifest.json`，池中两个节点。manifest 为本隔离测试由真实 SavePoint 发布的数据，未使用私人存档，未手填存档快照。

## 实际业务前置与写边界

夹具使用正常 Standalone GameInstance、真实 LocalPlayer/Controller、InitializeActorsForPlay、NotifyBeginPlay、已注册 tagged WorldStatic QueryOnly 地板。一次正常 World Tick 执行组件前置；后续真实 public Movement.TickComponent 推进自然落地。同步 World Tick 使用同一 GFrameCounter 的去重问题已由根实际修复并取得本次 write 成功，原始失败只属于夹具前置。

现有 Gameplay.EnableAdventure / Nature.EnsureWorld 创建首营与真实生态。三个活 PrototypeEncounter Actor 只在初始准备时通过公开场景位置 API 放到首营安全半径外，保留生命、身份与 Opponents；LoadPoint 通过真实 Combat 快照恢复其保存位置，两次 read 不重新定位。该场景准备属于显式 Native fixture。

正常 SelectBuilding / ConfirmPlacement 与真实五秒 TimedAction 完成一个一级工作台，实际 Paid 为 wood72。真实 SelectProduction(rope) / AssignWorker(0) / SetProduction 后，Clock.Tick(.5) 投入 wood2 并留下未完成批次。ReadMap 消费一张真实地图，生成实际藏宝点及未领取实例奖励。

实际保存边界：`A = W = 12.525000000372529`，包含真实落地和建造准备耗时，未归零。player wood0、shared wood0、brother wood2、source wood8。A/W 本轮相等；设施跳时造成 A/W 分离仍未测。

| 代表状态 | 磁盘节点 / manifest 实际值 |
| --- | --- |
| 工作台 | GUID `20813215492FD303310AF9AE80678B17`，kind workbench，level1，camp，Paid wood72 |
| 生产区域 | `facility_20813215492FD303310AF9AE80678B17`，worker `[0]`，job rope，Enabled/Safe true |
| 已投入批次 | Active true，Work0.5，Required360，Inputs wood2，Outputs rope1，Completed0，BatchStopAt0 |
| 藏宝点 | GUID `4BF1A945484BEA7F62F7D88D09293FA0`，treasure_map_1，实际位置 `(-7270.467030178787,31163.124188102243,0)` |
| 未领取奖励 | metal_ingot8；bow_2 实例 `BDCB9DA5446FA691AE743191ED367669`，Durability80，UniqueClaim None |
| 图箱账本 | Maps/Rewards 含 treasure_map_1，Opened 不含该图，Pending 非空 |

## read 必须核对的当前业务断言与实际结果

本次 read 的两次 public LoadPoint 和随后 public SavePoint 均执行到函数成功出口。现有断言覆盖：

- 先标准读取池，根据 manifest SaveId 定位真实磁盘节点，逐项确认 campaign/command/axe/memory/workbench/region/treasure 身份与写进程一致；磁盘 CampEconomy/Nature 与 manifest 的实际保存字符串相同。相同池节点数及非空投入、Pending 前置成立。
- 每次 LoadPoint 更新 live epoch；实际工作台 Actor 由 ResolveFacility 恢复，facility identity/level/camp/Paid 保留。区域 worker、queue enabled、Completed、BatchStopAt、投入 Inputs、未结算 Outputs、Work、Required、ToRations 保留；不重复扣输入、不提前发 rope。
- 实际藏宝 Actor 与保存点 GUID、Key、Definition、Position 保留。Maps/Rewards/Opened 账本及 Pending 所有 stack、实例 GUID/Definition/Durability/UniqueClaim、装备引用逐项比较。未发生 claim 重放。
- 完整 player/brother/shared inventory 的 Version、BackpackRank、Stacks、Equipped、Instances 数量及逐实例内容保留；另比较 source 库存和真实命令 requested/acquired/carried/delivered。未来 read 准备库存被加载覆盖，无额外库存增益。
- A/W 精确保持写边界，按保存 Origin 还原显示日期和分钟；NPC agreement 原文/ID/campaign/RecordedAt，revision、event identity/payload、command coverage、conversation flags/last-contact 和原话 exchange 保留。
- 最后正常 SavePoint 仍能捕获；CampEconomy/Nature 与原点相同，NPCOperations 未增加，NPCReceipts 数量未增加，Origin 与 KnowledgeRevision 保留。

“零增益”信用限于这两次实际 LoadPoint / resave 的上述完整断言。不包含之后真实 batch completion、Pending claim 和 Due settlement 的一次结算验证。

## 当前边界

Due、非默认 Generation2、双营地、正式地图标题 new/quit/continue、正常键鼠输入、真实 HTTP/UI 迟到回调、完整旧档代表矩阵仍未测。本次材料是当前开发夹具的真实磁盘跨进程恢复，不能称真人成熟旧档或正式游玩重启验收。

两域本轮确有非空代表状态与实际原生结果；后续扩展方案见 `restart-due-generation-two-camps-scope.md`，该方案仅调查，不提供未执行信用。
