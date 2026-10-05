# 双营地与生态的独立进程保存验证

2026-10-04，独立pool `BD9C227EEB6148FB8D20FADFF541CB2E`。

- WRITE `Saved/Task053/restart071-two-camps-equipped-write/index.json`：Success，0 Error，0 Warning；writer PID18236，SaveId5745A3BA4B3CEEF8328BBFA66776E328。
- READ `Saved/Task053/restart071-two-camps-equipped-read/index.json`：Success，0 Error，0 Warning；reader PID31432，与writer分离；实际两次LoadPoint与再SavePoint检查通过。
- 真实公开剧情Start/Relic/Directive/旅行入口，实际80个稳定base敌方identity经本人已装备斧AttackPower的HitTarget结算514次；正常Tick、四次Interact和每面5A计时完成四旗，四件实际赠品、唯一hearth_blade及胜利状态。未用 cleared/victory 字段注入。
- 本Native是明确场景放置与伤害夹具，不代表普通输入、真实刃接触、导航或真人剧情体验已验收。
- 两营地设施GUID/配方/变换/原材料投入、族人唯一归属/真实迁移、资源identity/remaining/due、奖励identity和Experience及RewardFacts，经独立进程实际还原、两次Load、再保存核对。
- 合并生态路径实际reader将 nature_hare0 刷新至Generation3，camp_048_herb_patch0只刷新一次，原已投入rope批次只完成一次，重复等待无额外刷新或免费产物。
- 第一夹具失败要求所有资源Camp必须camp/hometown，违反正常CampAt半径下营外资源None契约。第二失败重复Equip当前斧触发现有卸下toggle。两项均在测试夹具按实际契约修正；无生产Save变更。对应失败原报告完整保留。
- 命令r>1部分领料的实际Load defect另有独立RED；本两营地命令revision1路径不覆盖它。正常菜单/HTTP异步回调、目标机器性能及真人验收仍未覆盖。
