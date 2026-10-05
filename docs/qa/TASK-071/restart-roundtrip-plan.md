# TASK-071｜实际退出与独立进程恢复计划

2026-10-04。仅调查与最小方案；未运行UE、构建或Git，未改生产存档/个人档。根负责授权脚本路径及串行执行。

## 已有证据边界

- 根报告 integrated071-skin055：现有Save 11/11通过，包括真实HWS2旧池四点迁移、当前重写/重读、HWS7包裹显式schema8拒绝。私人原池保持，来源不能推定为真人Demo进度。
- 新增 ActualLoadPointInvalidatesOldCommandTicket 仅验证实际LoadPoint、epoch和公开提交边界；尚无执行结果。根已将误用私有State.Shared的库存准备修正为公开Adjust，并补未来库存变化后ticket仍current的正控制。
- TASK016/017/047/048脚本和报告覆盖同一编辑器进程重开PIE。TASK020/verify_ui_reload.py声明并使用独立新编辑器恢复，可复用操作顺序。
- TASK052/launch.py通过同一UEClient持有并停止自己启动的PID，现有接口足够两次串行启动，无需新增生产接口或依赖。
- 048旧PIE脚本调用advance_calendar，当前WorldClock的非Shipping AdvanceCalendar没有UFUNCTION，不直接重跑旧脚本。正式Python可调用Camp.WaitAtCampfire、Camp.Sleep或WorldClock.RequestTimeAdvance。

## 最小两进程路线

建议仅增加QA071两个引擎脚本prepare_restart_pie.py、verify_restart_pie.py；主机启动按052现有UEClient模式串行调用，无需引入新的runner框架。先登记精确路径再写。两次均传同一个新-HearthwardSaveTestPool=<GUID>，预期文件保存在Saved/Task071/restart-<GUID>/，不使用个人档。

1. 进程A打开正式自然图，使用HearthwardGameMode、标题页new和正常序章交互。明确记录测试材料补给/定位；建物由实际SelectBuilding/ConfirmPlacement完成并等待稳定边界，不改建物账本或持久状态。
2. 通过一级真实工作台、AssignWorker(普通人物0)、SelectProduction(rope)、SetProduction准备实际后台投入。保存前观测Batch.Active、0<Work<Required和已投入Inputs。用真实use_item(treasure_map_1)留下非空图箱Pending；正常采集耗尽至少一个来源得到Remaining=0、Due>W。用正常床/篝火将W准备到某个未触发Due之前，队列在该准备后开启。
3. 双营地必须从真实Campaign胜利事务产生，或使用同GUID内已经通过实际胜利事务保存的当前测试点。Camp.ReclaimHometown单独调用会使Camp.Hometown与Campaign.Victory不一致，现Save校验明确拒绝，不能作为自然路线替代。若没有现成合法成熟/胜利进度，需要真实清敌/占旗准备，不能写State.Victory、Flags、敌人Health或包装虚构旧档。
4. 在正常save页面冻结并执行save。预期从实际最新SavePoint.World的反射属性及各Describe JSON取得，记录SaveId/CampaignId、A/W/Origin、投入/小数Work/Completed、Pending物品和实例GUID、来源Due/Generation、双方状态/装备/背包/共享仓储/地面物、建物GUID/变换/等级/Paid、双营地/唯一人物岗位，以及实际任务/增援/旗帜/Victory/NPC事实。先assert四个要求的代表状态确实存在，避免空状态往返虚假通过。
5. 输出prepare结果后执行正常quit，主机确认PID A已经退出；必要时通过持有它的同一个UEClient收尾。A未退出不得启动B或记为重启通过。进程B使用同GUID打开自然图，从真实标题页执行continue，确认恢复的SaveId就是A记录的最新节点，不调用new。
6. continue同步完成后在同一Python回调立即open_page(save)，在下一worldtick之前冻结并比较代表状态。当前Campaign.Restore及标题continue会写正常home_continued事实；它是有代码依据的单独预期变化，检查该事实并保留其余Campaign事实/终态比较，不将整个Campaign差异忽略。重复LoadPoint两次必须保持时钟、投入、Pending、库存、奖励与回执无增加。
7. 解冻后使用真正有效设施请求跨过一个准备好的Due和已投入批次边界。第一批仅预留一批原料，避免连续队列完成多批造成断言歧义；核对投入未重扣、一个批次结算、一个资源/动物刷新身份，以及图箱领取一次和再次领取无增益。通过剩余Due与实际W计算等待，墙钟进程退出间隔不增加A/W或收益。最后正常保存、截图、退出并保留两次启动/停止PID证据。

## 公共接口与数据依据

- UI/HearthwardScreenActions.cpp：new/continue/save/load/quit；continue按Created选择最新，自然图OpenSavePoint直接PrepareSession/LoadPoint，非自然图会OpenLevel，需等待完成后重新定位World/UI。
- UI/HearthwardScreenWidget.cpp：自然图默认GameMode来自现052验证；URL加载路径和标题continue都使用真实存档恢复。
- CampSubsystem.h：AssignWorker、SetProduction、SelectProduction、Sleep、WaitAtCampfire、Describe均UFUNCTION；Camp.Describe返回State.Snapshot JSON。
- CampState：工时、Inputs/Outputs、Completed、人物岗位、Paid和Camps均实际快照内容；一级工作台rope为已知配方，输入wood2/输出rope1，工作360。
- NatureActions.cpp：ReadMap由物品使用消费地图，在真实Ground选址后生成含实际装备实例的Pending；无需先开箱制造待领取状态。
- SaveGame.cpp：Campaign.Victory必须等于Camp.Hometown，救援抵达阶段必须对应Camp.Rescued。SaveSubsystem的Capture/WritePoint/Restore保持真实世界同一边界；GetPoints公开提供已保存快照，不需要私有访问或新的保存字段。
- Python读取非Blueprint字段采用实际C++反射名，例如point.get_editor_property("World").get_editor_property("CampEconomy")。UE5.8 PyWrapperStruct.cpp:1023–1089通过ScriptStruct.FindPropertyByName解析，PyGenUtil.cpp:1418–1440转给PyUtil读取；不依赖该字段是否生成snake_case公开属性，不写入快照。

## 仍未覆盖

完整071出口还需要真实HTTP模型回复跨Load一次，以及实际UI旧回调跨Load一次的零结算证据。新Native旧CommandTicket测试不替代这些实际回调。真实旧Demo的成熟生态/夺回前/永久胜利/双营地代表档目前未发现可追溯来源；新建正常测试进度可验证当前往返，应标记为当前隔离fixture，不能称真实玩家旧档。两进程路线自身NOT_RUN。
