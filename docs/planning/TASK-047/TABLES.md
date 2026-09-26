# TASK-047 完整候选表

**PROPOSED：待Owner批准，不是运行表。** 单位：重量为抽象单位；表内经验为累计值。所有取得链在自然地图的布置状态见candidate.json sources。

## 逐级成长

|等级|累计经验|下级经验|人物生命增量|人物耐力增量|技能总预算|
|---|---|---|---|---|---|
|1|0|100|0.0|0.0|2|
|2|100|135|1.694915|0.847458|2|
|3|235|170|3.389831|1.694915|3|
|4|405|205|5.084746|2.542373|4|
|5|610|240|6.779661|3.389831|5|
|6|850|275|8.474576|4.237288|6|
|7|1125|310|10.169492|5.084746|7|
|8|1435|345|11.864407|5.932203|7|
|9|1780|380|13.559322|6.779661|8|
|10|2160|415|15.254237|7.627119|9|
|11|2575|450|16.949153|8.474576|10|
|12|3025|485|18.644068|9.322034|11|
|13|3510|520|20.338983|10.169492|12|
|14|4030|555|22.033898|11.016949|13|
|15|4585|590|23.728814|11.864407|13|
|16|5175|625|25.423729|12.711864|14|
|17|5800|660|27.118644|13.559322|15|
|18|6460|695|28.813559|14.40678|16|
|19|7155|730|30.508475|15.254237|17|
|20|7885|765|32.20339|16.101695|18|
|21|8650|800|33.898305|16.949153|18|
|22|9450|835|35.59322|17.79661|19|
|23|10285|870|37.288136|18.644068|20|
|24|11155|905|38.983051|19.491525|21|
|25|12060|940|40.677966|20.338983|22|
|26|13000|975|42.372881|21.186441|23|
|27|13975|1010|44.067797|22.033898|24|
|28|14985|1045|45.762712|22.881356|24|
|29|16030|1080|47.457627|23.728814|25|
|30|17110|1115|49.152542|24.576271|26|
|31|18225|1150|50.847458|25.423729|27|
|32|19375|1185|52.542373|26.271186|28|
|33|20560|1220|54.237288|27.118644|29|
|34|21780|1255|55.932203|27.966102|29|
|35|23035|1290|57.627119|28.813559|30|
|36|24325|1325|59.322034|29.661017|31|
|37|25650|1360|61.016949|30.508475|32|
|38|27010|1395|62.711864|31.355932|33|
|39|28405|1430|64.40678|32.20339|34|
|40|29835|1465|66.101695|33.050847|35|
|41|31300|1500|67.79661|33.898305|35|
|42|32800|1535|69.491525|34.745763|36|
|43|34335|1570|71.186441|35.59322|37|
|44|35905|1605|72.881356|36.440678|38|
|45|37510|1640|74.576271|37.288136|39|
|46|39150|1675|76.271186|38.135593|40|
|47|40825|1710|77.966102|38.983051|40|
|48|42535|1745|79.661017|39.830508|41|
|49|44280|1780|81.355932|40.677966|42|
|50|46060|1815|83.050847|41.525424|43|
|51|47875|1850|84.745763|42.372881|44|
|52|49725|1885|86.440678|43.220339|45|
|53|51610|1920|88.135593|44.067797|46|
|54|53530|1955|89.830508|44.915254|46|
|55|55485|1990|91.525424|45.762712|47|
|56|57475|2025|93.220339|46.610169|48|
|57|59500|2060|94.915254|47.457627|49|
|58|61560|2095|96.610169|48.305085|50|
|59|63655|2130|98.305085|49.152542|51|
|60|65785|0|100.0|50.0|52|

## 29节点逐项映射

全部保留稳定ID；每节点3级，每级1点；前置节点至少1级。rank_values表示该节点累计效果。

|ID／名称|前置|候选效果（1/2/3级）|处理|
|---|---|---|---|
|strong／重击精通|无|heavy_damage：[0.05, 0.1, 0.15]|重击从开场可用；删除随机击晕，防止等效击杀造成随机秒杀|
|vigor／坚韧体魄|strong|health：[8, 16, 24]|保留并明确结算|
|guard／坚守|strong|defense：[0.03, 0.06, 0.09]|保留并明确结算|
|recovery／战后调息|vigor|recovery：[0.15, 0.3, 0.45]|保留并明确结算|
|resolve／坚毅|vigor|health：[5, 10, 15]|保留并明确结算|
|bravery／勇武|guard|attack：[0.03, 0.06, 0.09]|保留并明确结算|
|resilience／坚韧|recovery|defense：[0.02, 0.04, 0.06]|保留并明确结算|
|secondwind／重振|resolve|recovery：[0.1, 0.2, 0.3]|保留并明确结算|
|trail／寻路者|无|discover：[0.2, 0.4, 0.6]|保留并明确结算|
|insight／洞察|trail|sense_duration_seconds：[6, 7, 8]|替换经验加成，避免领任务前免费洗点套利|
|sustain／荒野生存|trail|hunger：[0.08, 0.16, 0.24]|保留并明确结算|
|scout／远眺|insight|sense_radius_m：[18, 21, 24]|替换重复地点发现倍率，基础感应仍15米|
|forager／识途|sustain|discover：[0.1, 0.2, 0.3]|保留并明确结算|
|wisdom／见闻|scout|sense_cooldown_seconds：[18, 16, 14]|替换经验加成，基础冷却仍20秒|
|ration／节粮|forager|hunger：[0.04, 0.08, 0.12]|保留并明确结算|
|runner／轻盈步伐|无|sprint：[0.04, 0.08, 0.12]|保留并明确结算|
|breath／悠长呼吸|runner|stamina：[5, 10, 15]|保留并明确结算|
|endure／耐力训练|runner|cost：[0.06, 0.12, 0.18]|保留并明确结算|
|rest／迅速恢复|breath|staminaRecovery：[0.08, 0.16, 0.24]|保留并明确结算|
|lightstep／轻步|endure|detection_growth_reduction：[0.08, 0.16, 0.24]|替换重复耗耐减免；仅减视觉发现增长，不能免受击确认|
|longstride／疾行|endure|sprint：[0.02, 0.04, 0.06]|保留并明确结算|
|composure／从容|rest|staminaRecovery：[0.04, 0.08, 0.12]|保留并明确结算|
|reserve／余力|longstride|stamina：[3, 6, 9]|保留并明确结算|
|edge／精工刃口|无|attack：[0.06, 0.12, 0.18]|保留并明确结算|
|mend／护甲修整|edge|armor_wear_reduction：[0.1, 0.2, 0.3]|替换全身常驻减伤为四件实际护甲耐久效率|
|cook／善用食材|edge|food：[0.08, 0.16, 0.24]|保留并明确结算|
|durable／经久耐用|mend|durability：[0.1, 0.2, 0.3]|保留并明确结算|
|seasoning／烹调|cook|food：[0.05, 0.1, 0.15]|保留并明确结算|
|reinforce／加固|durable|local_armor_bonus：[0.01, 0.02, 0.03]|替换常驻全身减伤；仅完整命中部位护甲增加百分点|

## 全物品目录

|ID／名称|重量|阶／稀有度|用途／部位|伤害／局部减伤|耐久|取得来源|
|---|---|---|---|---|---|---|
|wood／木材|1|1／common|材料／—|0／0%|0|world:wood|
|stone／石材|1|1／common|材料／—|0／0%|0|world:stone|
|ore／矿石|2|1／common|材料／—|0／0%|0|world:ore|
|meat／鲜肉|0.5|1／common|食物／—|0／0%|0|world:meat|
|arrow／箭矢|0.05|1／common|工具／—|0／0%|0|craft:arrows|
|axe／石／皮战斧|3.2|1／common|blunt／weapon|30／0%|80|craft:stone_axe|
|bow／石／皮猎弓|1.8|1／common|bow／ranged|25／0%|80|craft:craft_bow|
|hood／石／皮兜帽|0.8|1／common|head／head|0／3%|100|craft:craft_hood|
|armor／石／皮胸甲|4|1／common|chest／chest|0／5%|100|craft:craft_armor|
|gloves／皮护腕|0.7|1／common|装饰配件，不提供减伤／容量／免弹药／hands|0／0%|0|craft:craft_gloves|
|boots／石／皮靴|1.2|1／common|feet／feet|0／3%|100|craft:craft_boots|
|belt／兽皮腰带|0.6|1／common|装饰配件，不提供减伤／容量／免弹药／waist|0／0%|0|craft:craft_belt|
|shield／石／皮盾|2.8|1／common|shield／offhand|0／0%|80|craft:craft_shield|
|quiver／箭袋|0.5|1／common|装饰配件，不提供减伤／容量／免弹药／quiver|0／0%|0|craft:craft_quiver|
|amulet／故乡护符|0|1／unique|零重关键任务物，不可丢弃／销毁，无战斗加成／—|0／0%|0|claim:campaign_start_amulet|
|roast／烤肉|0.5|1／common|食物／—|0／0%|0|craft:roast|
|herb／草药|0.2|1／common|材料／—|0／0%|0|world:herb|
|hide／兽皮|0.6|1／common|防具与背包原料／—|0／0%|0|world:hide|
|rope／绳索|0.3|1／common|工具／—|0／0%|0|craft:rope|
|flower／山花|0.1|1／common|资源见闻／收集，无首版生产消耗／—|0／0%|0|world:flower|
|medicine／药草膏|0.5|1／common|食物／—|0／0%|0|craft:medicine|
|firepot／投掷火罐|1|1／common|一次性伤害投掷，基伤60（普通守卫H100的60%），护甲／难度后结算；无分解回收／—|60／0%|0|craft:firepot|
|medicine_half／半份药草膏|0.25|1／common|044受伤中断产生半份，不能直接制作或拼回整份／—|0／0%|0|event:interrupted_medicine|
|wild_food／可食植物|0.5|1／common|食物／—|0／0%|0|world:wild_food|
|cooked_food／熟食|0.5|1／common|食物／—|0／0%|0|craft:cooked_food|
|full_meal／大餐|0.5|1／common|食物／—|0／0%|0|craft:full_meal|
|metal_ingot／金属锭|2|1／common|材料／—|0／0%|0|craft:metal_ingot|
|refined_ore／精矿|2|2／common|高级金属原料；金属镐采集／—|0／0%|0|world:rich_ore|
|steel_ingot／精炼金属锭|2|3／common|高级装备原料／—|0／0%|0|craft:steel_ingot|
|shortblade／石／皮短刀|1.8|1／common|shortblade／weapon|20／0%|100|craft:craft_shortblade|
|longblade／石／皮长刀|3.5|1／common|longblade／weapon|27／0%|80|craft:craft_longblade|
|spear／石／皮长枪|3|1／common|spear／weapon|23／0%|100|craft:craft_spear|
|leggings／石／皮护腿|2|1／common|legs／legs|0／4%|100|craft:craft_leggings|
|pickaxe／石／皮镐|3.5|1／common|pickaxe／tool|0／0%|60|craft:craft_pickaxe|
|shortblade_2／金属短刀|2.8|2／common|shortblade／weapon|30／0%|100|craft:craft_shortblade_2|
|longblade_2／金属长刀|4.5|2／common|longblade／weapon|40.5／0%|80|craft:craft_longblade_2|
|spear_2／金属长枪|4|2／common|spear／weapon|34.5／0%|100|craft:craft_spear_2|
|blunt_2／金属战斧|4.2|2／common|blunt／weapon|45／0%|80|craft:craft_blunt_2|
|bow_2／金属猎弓|2.8|2／common|bow／ranged|37.5／0%|80|craft:craft_bow_2|
|crossbow_2／金属弩|5|2／common|crossbow／ranged|60／0%|60|craft:craft_crossbow_2|
|head_2／金属兜帽|1.3|2／common|head／head|0／15%|100|craft:craft_head_2|
|chest_2／金属胸甲|4.5|2／common|chest／chest|0／15%|100|craft:craft_chest_2|
|legs_2／金属护腿|2.5|2／common|legs／legs|0／15%|100|craft:craft_legs_2|
|feet_2／金属靴|1.7|2／common|feet／feet|0／15%|100|craft:craft_feet_2|
|shield_2／金属盾|3.8|2／common|shield／offhand|0／0%|80|craft:craft_shield_2|
|pickaxe_2／金属镐|4.5|2／common|pickaxe／tool|0／0%|60|craft:craft_pickaxe_2|
|shortblade_3／高级金属短刀|3.8|3／common|shortblade／weapon|45／0%|100|craft:craft_shortblade_3|
|longblade_3／高级金属长刀|5.5|3／common|longblade／weapon|60.75／0%|80|craft:craft_longblade_3|
|spear_3／高级金属长枪|5|3／common|spear／weapon|51.75／0%|100|craft:craft_spear_3|
|blunt_3／高级金属战斧|5.2|3／common|blunt／weapon|67.5／0%|80|craft:craft_blunt_3|
|bow_3／高级金属猎弓|3.8|3／common|bow／ranged|56.25／0%|80|craft:craft_bow_3|
|crossbow_3／高级金属弩|6|3／common|crossbow／ranged|90／0%|60|craft:craft_crossbow_3|
|head_3／高级金属兜帽|1.8|3／common|head／head|0／25%|100|craft:craft_head_3|
|chest_3／高级金属胸甲|5|3／common|chest／chest|0／25%|100|craft:craft_chest_3|
|legs_3／高级金属护腿|3|3／common|legs／legs|0／25%|100|craft:craft_legs_3|
|feet_3／高级金属靴|2.2|3／common|feet／feet|0／25%|100|craft:craft_feet_3|
|shield_3／高级金属盾|4.8|3／common|shield／offhand|0／0%|80|craft:craft_shield_3|
|pickaxe_3／高级金属镐|5.5|3／common|pickaxe／tool|0／0%|60|craft:craft_pickaxe_3|
|fishing_rod／钓竿|1.5|1／common|钓鱼，仅成功获得鱼磨损1；鱼种／鱼饵由048配表／tool|0／0%|40|craft:craft_fishing_rod|
|bow_rare／猎手长弓|2.8|2／rare|bow／ranged|41.25／0%|160|reward:rescue_05；craft:craft_bow_rare|
|blueprint_hunter_bow／猎手长弓图纸|0|2／rare|阅读永久解锁对应配方；不能从实物逆向学会／—|0／0%|0|claim:side_hunter_blueprint|
|hearth_blade／归火短刀|3.8|3／unique|shortblade／weapon|49.5／0%|400|claim:hometown_hearth_blade|

## 全配方

普通配方预先掌握；非known项必须持图学习。材料数为每批消耗，手工即时，后台沿046每批360工人W及设施效率。

|ID|材料|产出|设施／级|知识|
|---|---|---|---|---|
|rope|{'wood': 2}|{'rope': 1}|workbench／1|known|
|metal_ingot|{'ore': 2, 'wood': 1}|{'metal_ingot': 1}|smelter／1|known|
|steel_ingot|{'metal_ingot': 2, 'refined_ore': 1, 'wood': 2}|{'steel_ingot': 1}|smelter／2|known|
|cooked_food|{'wild_food': 2, 'wood': 1}|{'cooked_food': 1}|cooking／1|known|
|full_meal|{'wild_food': 3, 'wood': 1}|{'full_meal': 1}|cooking／2|known|
|arrows|{'wood': 1}|{'arrow': 4}|workbench／1|known|
|medicine|{'herb': 2}|{'medicine': 1}|workbench／1|known|
|roast|{'meat': 1, 'wood': 1}|{'roast': 1}|campfire／1|known|
|firepot|{'stone': 2, 'wood': 2, 'rope': 1}|{'firepot': 1}|workbench／1|known|
|craft_shortblade|{'wood': 10, 'stone': 15}|{'shortblade': 1}|workbench／1|known|
|craft_longblade|{'wood': 10, 'stone': 15}|{'longblade': 1}|workbench／1|known|
|craft_spear|{'wood': 10, 'stone': 15}|{'spear': 1}|workbench／1|known|
|stone_axe|{'wood': 10, 'stone': 15}|{'axe': 1}|workbench／1|known|
|craft_bow|{'wood': 10, 'rope': 10}|{'bow': 1}|workbench／1|known|
|craft_hood|{'hide': 5, 'rope': 5}|{'hood': 1}|workbench／1|known|
|craft_armor|{'hide': 15, 'rope': 5}|{'armor': 1}|workbench／1|known|
|craft_leggings|{'hide': 10, 'rope': 5}|{'leggings': 1}|workbench／1|known|
|craft_boots|{'hide': 5, 'rope': 5}|{'boots': 1}|workbench／1|known|
|craft_shield|{'wood': 15, 'rope': 5}|{'shield': 1}|workbench／1|known|
|craft_pickaxe|{'wood': 10, 'stone': 15}|{'pickaxe': 1}|workbench／1|known|
|craft_shortblade_2|{'wood': 10, 'metal_ingot': 15}|{'shortblade_2': 1}|forge／1|known|
|craft_longblade_2|{'wood': 10, 'metal_ingot': 15}|{'longblade_2': 1}|forge／1|known|
|craft_spear_2|{'wood': 10, 'metal_ingot': 15}|{'spear_2': 1}|forge／1|known|
|craft_blunt_2|{'wood': 10, 'metal_ingot': 15}|{'blunt_2': 1}|forge／1|known|
|craft_bow_2|{'wood': 10, 'rope': 10, 'metal_ingot': 5}|{'bow_2': 1}|forge／1|known|
|craft_crossbow_2|{'wood': 10, 'rope': 10, 'metal_ingot': 10}|{'crossbow_2': 1}|forge／1|known|
|craft_head_2|{'hide': 5, 'rope': 5, 'metal_ingot': 5}|{'head_2': 1}|forge／1|known|
|craft_chest_2|{'hide': 15, 'rope': 5, 'metal_ingot': 5}|{'chest_2': 1}|forge／1|known|
|craft_legs_2|{'hide': 10, 'rope': 5, 'metal_ingot': 5}|{'legs_2': 1}|forge／1|known|
|craft_feet_2|{'hide': 5, 'rope': 5, 'metal_ingot': 5}|{'feet_2': 1}|forge／1|known|
|craft_shield_2|{'wood': 15, 'rope': 5, 'metal_ingot': 5}|{'shield_2': 1}|forge／1|known|
|craft_pickaxe_2|{'wood': 10, 'metal_ingot': 15}|{'pickaxe_2': 1}|forge／1|known|
|craft_shortblade_3|{'wood': 10, 'steel_ingot': 15}|{'shortblade_3': 1}|forge／3|known|
|craft_longblade_3|{'wood': 10, 'steel_ingot': 15}|{'longblade_3': 1}|forge／3|known|
|craft_spear_3|{'wood': 10, 'steel_ingot': 15}|{'spear_3': 1}|forge／3|known|
|craft_blunt_3|{'wood': 10, 'steel_ingot': 15}|{'blunt_3': 1}|forge／3|known|
|craft_bow_3|{'wood': 10, 'rope': 10, 'steel_ingot': 5}|{'bow_3': 1}|forge／3|known|
|craft_crossbow_3|{'wood': 10, 'rope': 10, 'steel_ingot': 10}|{'crossbow_3': 1}|forge／3|known|
|craft_head_3|{'hide': 5, 'rope': 5, 'steel_ingot': 5}|{'head_3': 1}|forge／3|known|
|craft_chest_3|{'hide': 15, 'rope': 5, 'steel_ingot': 5}|{'chest_3': 1}|forge／3|known|
|craft_legs_3|{'hide': 10, 'rope': 5, 'steel_ingot': 5}|{'legs_3': 1}|forge／3|known|
|craft_feet_3|{'hide': 5, 'rope': 5, 'steel_ingot': 5}|{'feet_3': 1}|forge／3|known|
|craft_shield_3|{'wood': 15, 'rope': 5, 'steel_ingot': 5}|{'shield_3': 1}|forge／3|known|
|craft_pickaxe_3|{'wood': 10, 'steel_ingot': 15}|{'pickaxe_3': 1}|forge／3|known|
|craft_gloves|{'hide': 5, 'rope': 5}|{'gloves': 1}|workbench／1|known|
|craft_belt|{'hide': 5, 'rope': 5}|{'belt': 1}|workbench／1|known|
|craft_quiver|{'hide': 5, 'rope': 5}|{'quiver': 1}|workbench／1|known|
|craft_fishing_rod|{'wood': 10, 'rope': 5}|{'fishing_rod': 1}|workbench／1|known|
|craft_bow_rare|{'wood': 15, 'rope': 15, 'metal_ingot': 10}|{'bow_rare': 1}|forge／2|blueprint_hunter_bow|

## 装备维修基准

0→满时逐材料ceil(基准×20%)；局部维修见设计D4。同款物品修理自身实例。

|装备ID|制作／等值基准|全修材料|设施|
|---|---|---|---|
|axe|{'wood': 10, 'stone': 15}|{'wood': 2, 'stone': 3}|workbench 1|
|bow|{'wood': 10, 'rope': 10}|{'wood': 2, 'rope': 2}|workbench 1|
|hood|{'hide': 5, 'rope': 5}|{'hide': 1, 'rope': 1}|workbench 1|
|armor|{'hide': 15, 'rope': 5}|{'hide': 3, 'rope': 1}|workbench 1|
|boots|{'hide': 5, 'rope': 5}|{'hide': 1, 'rope': 1}|workbench 1|
|shield|{'wood': 15, 'rope': 5}|{'wood': 3, 'rope': 1}|workbench 1|
|shortblade|{'wood': 10, 'stone': 15}|{'wood': 2, 'stone': 3}|workbench 1|
|longblade|{'wood': 10, 'stone': 15}|{'wood': 2, 'stone': 3}|workbench 1|
|spear|{'wood': 10, 'stone': 15}|{'wood': 2, 'stone': 3}|workbench 1|
|leggings|{'hide': 10, 'rope': 5}|{'hide': 2, 'rope': 1}|workbench 1|
|pickaxe|{'wood': 10, 'stone': 15}|{'wood': 2, 'stone': 3}|workbench 1|
|shortblade_2|{'wood': 10, 'metal_ingot': 15}|{'wood': 2, 'metal_ingot': 3}|forge 1|
|longblade_2|{'wood': 10, 'metal_ingot': 15}|{'wood': 2, 'metal_ingot': 3}|forge 1|
|spear_2|{'wood': 10, 'metal_ingot': 15}|{'wood': 2, 'metal_ingot': 3}|forge 1|
|blunt_2|{'wood': 10, 'metal_ingot': 15}|{'wood': 2, 'metal_ingot': 3}|forge 1|
|bow_2|{'wood': 10, 'rope': 10, 'metal_ingot': 5}|{'wood': 2, 'rope': 2, 'metal_ingot': 1}|forge 1|
|crossbow_2|{'wood': 10, 'rope': 10, 'metal_ingot': 10}|{'wood': 2, 'rope': 2, 'metal_ingot': 2}|forge 1|
|head_2|{'hide': 5, 'rope': 5, 'metal_ingot': 5}|{'hide': 1, 'rope': 1, 'metal_ingot': 1}|forge 1|
|chest_2|{'hide': 15, 'rope': 5, 'metal_ingot': 5}|{'hide': 3, 'rope': 1, 'metal_ingot': 1}|forge 1|
|legs_2|{'hide': 10, 'rope': 5, 'metal_ingot': 5}|{'hide': 2, 'rope': 1, 'metal_ingot': 1}|forge 1|
|feet_2|{'hide': 5, 'rope': 5, 'metal_ingot': 5}|{'hide': 1, 'rope': 1, 'metal_ingot': 1}|forge 1|
|shield_2|{'wood': 15, 'rope': 5, 'metal_ingot': 5}|{'wood': 3, 'rope': 1, 'metal_ingot': 1}|forge 1|
|pickaxe_2|{'wood': 10, 'metal_ingot': 15}|{'wood': 2, 'metal_ingot': 3}|forge 1|
|shortblade_3|{'wood': 10, 'steel_ingot': 15}|{'wood': 2, 'steel_ingot': 3}|forge 3|
|longblade_3|{'wood': 10, 'steel_ingot': 15}|{'wood': 2, 'steel_ingot': 3}|forge 3|
|spear_3|{'wood': 10, 'steel_ingot': 15}|{'wood': 2, 'steel_ingot': 3}|forge 3|
|blunt_3|{'wood': 10, 'steel_ingot': 15}|{'wood': 2, 'steel_ingot': 3}|forge 3|
|bow_3|{'wood': 10, 'rope': 10, 'steel_ingot': 5}|{'wood': 2, 'rope': 2, 'steel_ingot': 1}|forge 3|
|crossbow_3|{'wood': 10, 'rope': 10, 'steel_ingot': 10}|{'wood': 2, 'rope': 2, 'steel_ingot': 2}|forge 3|
|head_3|{'hide': 5, 'rope': 5, 'steel_ingot': 5}|{'hide': 1, 'rope': 1, 'steel_ingot': 1}|forge 3|
|chest_3|{'hide': 15, 'rope': 5, 'steel_ingot': 5}|{'hide': 3, 'rope': 1, 'steel_ingot': 1}|forge 3|
|legs_3|{'hide': 10, 'rope': 5, 'steel_ingot': 5}|{'hide': 2, 'rope': 1, 'steel_ingot': 1}|forge 3|
|feet_3|{'hide': 5, 'rope': 5, 'steel_ingot': 5}|{'hide': 1, 'rope': 1, 'steel_ingot': 1}|forge 3|
|shield_3|{'wood': 15, 'rope': 5, 'steel_ingot': 5}|{'wood': 3, 'rope': 1, 'steel_ingot': 1}|forge 3|
|pickaxe_3|{'wood': 10, 'steel_ingot': 15}|{'wood': 2, 'steel_ingot': 3}|forge 3|
|fishing_rod|{'wood': 10, 'rope': 5}|{'wood': 2, 'rope': 1}|workbench 1|
|bow_rare|{'wood': 15, 'rope': 15, 'metal_ingot': 10}|{'wood': 3, 'rope': 3, 'metal_ingot': 2}|forge 2|
|hearth_blade|{'wood': 10, 'rope': 10, 'steel_ingot': 20}|{'wood': 2, 'rope': 2, 'steel_ingot': 4}|forge 3|
