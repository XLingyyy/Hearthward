# TASK-048 内容表（候选）

所有新增规则待Owner确认，正式解释见[设计决定](../../design/DSGN-R15-nature-production.md)。本文件由本次candidate内容整理；游戏仍读取Resources/Data/gameplay.json，未读取本候选。

## 资源

| ID | 名称→产物 | 容量 | 工具／最低阶 | 耗尽后恢复 | 空间要求 |
|---|---|---:|---|---:|---|
| tree | 树木→wood | 12 | axe/1 | 2日 | 营地边缘及林地；单株间距≥4m |
| stone_outcrop | 石点→stone | 8 | pickaxe/1 | 2日 | 营地外缘和山坡；避开道路 |
| ore_vein | 普通矿点→ore | 12 | pickaxe/1 | 2日 | 首营外150—300m安全可达岩壁 |
| rich_ore_vein | 精矿点→refined_ore | 8 | pickaxe/2 | 4日 | 首营外400—800m高地；至少两处 |
| herb_patch | 药草→herb | 4 | hand/0 | 2日 | 安全林缘，首营至少4点 |
| food_patch | 可食植物→wild_food | 16 | hand/0 | 2日 | 首营20—45m内至少4组，共64份 |
| flower_patch | 山花→flower | 4 | hand/0 | 2日 | 沿探索路线，不设隐藏兑换币 |
| fallen_branches | 落枝堆→wood | 16 | hand/0 | 2日 | 首营20—45m内至少6堆，共96木 |
| loose_stones | 散石堆→stone | 8 | hand/0 | 2日 | 首营20—45m内至少4堆，共32石 |
| wild_seed_greens | 野菜种源→seed_greens | 4 | hand/0 | 2日 | 首营外50—250m安全探索点，每种至少2点；不随机抽空 |
| wild_seed_grain | 谷物种源→seed_grain | 4 | hand/0 | 2日 | 首营外50—250m安全探索点，每种至少2点；不随机抽空 |
| wild_seed_herb | 药草种源→seed_herb | 4 | hand/0 | 2日 | 首营外50—250m安全探索点，每种至少2点；不随机抽空 |

## 野生动物

固定伤害不随玩家等级变动；无护甲动物头部箭伤×3。组数量为每个已标记栖息地的初始配额，正式地图需落实出生槽。

| ID／物种 | 行为 | 血量 | 每击伤害／间隔秒 | 速度m/s | 发现／逃窜m | 每组 | 肉／皮 | 区域／活动 |
|---|---|---:|---|---:|---|---:|---|---|
| deer／鹿 | flee | 75 | 0.000000/1.4 | 7 | 22/70 | 2 | 6/3 | 林缘草地/day |
| hare／野兔 | flee | 20 | 0.000000/1.4 | 6 | 14/50 | 3 | 1/1 | 草地与灌木边/day |
| pheasant／雉鸡 | flee | 20 | 0.000000/1.4 | 5 | 12/50 | 3 | 1/0 | 草地，短距扑翼逃跑/day |
| ram／野羊 | retaliate | 90 | 15.037594/1.4 | 6 | 18/60 | 2 | 4/2 | 山坡开阔地/day |
| boar／野猪 | retaliate | 120 | 15.037594/1.4 | 7 | 18/70 | 2 | 5/2 | 低地密林/dawn_dusk |
| wolf／狼 | aggressive | 100 | 15.037594/1.4 | 8 | 24/80 | 2 | 4/2 | 远离营地的林道/night |
| black_bear／黑熊 | retaliate | 240 | 15.037594/1.0 | 6 | 18/70 | 1 | 8/4 | 远林，营地外≥300m/day |
| red_fox／赤狐 | flee | 40 | 0.000000/1.4 | 7 | 20/60 | 1 | 2/2 | 林缘与河岸/night |

每次狩猎30经验；动物死亡后2日再生新代次，掉落永久保存到实际领取；重复读取或击晕后再攻击不重复奖励。

## 家畜

| ID／物种 | 血量 | 日耗饲料（成体／幼体） | 成体／幼体肉 | 每对繁殖／幼体成年 | 野外来源 |
|---|---:|---|---|---|---|
| goat／山羊 | 60 | 2/1 | 4/1 | 2日1幼体／2日 | 2处×2，一次性 |
| pig／家猪 | 80 | 3/2 | 6/1 | 2日1幼体／2日 | 2处×2，一次性 |
| hen／家鸡 | 20 | 1/1 | 2/1 | 2日1幼体／2日 | 2处×2，一次性 |

| 栏级 | 营地门槛 | 容量 | 本次建造／升级费用 |
|---|---:|---:|---|
| 1 | 2 | 6 | {'wood': 60, 'stone': 20, 'rope': 10} |
| 2 | 4 | 10 | {'wood': 120, 'stone': 40, 'rope': 20} |
| 3 | 6 | 14 | {'wood': 180, 'stone': 60, 'rope': 30} |

一栏一种，幼体占一格；缺饲料停长、不死亡，满栏不积欠繁殖。只产肉，不产蛋奶毛皮。

## 鱼与钓点

| ID／物种 | 物品ID | 鱼种权重 | 有效拉扯秒 | 生食饱食 | 重量（百分之一单位） |
|---|---|---:|---:|---:|---:|
| carp／鲤鱼 | fish_carp | 35% | 6 | 20 | 50 |
| crucian_carp／鲫鱼 | fish_crucian_carp | 40% | 5 | 20 | 50 |
| catfish／鲶鱼 | fish_catfish | 15% | 7 | 20 | 50 |
| eel／鳗鱼 | fish_eel | 10% | 7 | 20 | 50 |

每点24条，耗尽后2日刷新；首营河岸4点。1饵／次实际抛出；成功磨竿1，失败不磨。完整张力／取消／溢出规则见设计D3。

## 作物

| ID／作物 | 种子 | 成熟 | 产物 | 无照料／一次／两次 | 返种 |
|---|---|---:|---|---|---:|
| greens／野菜 | seed_greens×1 | 2日 | wild_food | 4/5/6 | 1 |
| grain／谷物 | seed_grain×1 | 3日 | grain | 8/10/12 | 1 |
| herb／药草 | seed_herb×1 | 4日 | herb | 12/15/18 | 1 |

两项照料每轮共用标记，返种不乘加成；成熟待收获，不自动收／播。

## 新物品与配方

保留047的62个物品ID及49配方，本候选新增13个物品ID和9个配方；原配方不重定义。新谷物20饱食、鱼20饱食，均不腐败；地图零重不可销毁，其余无饱食的物品不可直接吃。

| 新ID | 名称 | 重量 |
|---|---|---:|
| grain | 谷物 | 50 |
| bait | 鱼饵 | 5 |
| feed | 饲料 | 25 |
| seed_greens | 野菜种子 | 5 |
| seed_grain | 谷物种子 | 5 |
| seed_herb | 药草种子 | 5 |
| fish_carp | 鲤鱼 | 50 |
| fish_crucian_carp | 鲫鱼 | 50 |
| fish_catfish | 鲶鱼 | 50 |
| fish_eel | 鳗鱼 | 50 |
| treasure_map_1 | 藏宝图1 | 0 |
| treasure_map_2 | 藏宝图2 | 0 |
| treasure_map_3 | 藏宝图3 | 0 |

| 配方ID | 设施等级 | 输入 | 输出 |
|---|---|---|---|
| bait_048 | workbench 1 | {'wild_food': 1} | {'bait': 8} |
| feed_048 | workbench 1 | {'wild_food': 2} | {'feed': 4} |
| feed_grain_048 | workbench 1 | {'grain': 2} | {'feed': 4} |
| grain_food_048 | cooking 1 | {'grain': 2, 'wood': 1} | {'cooked_food': 1} |
| grain_meal_048 | cooking 2 | {'grain': 3, 'wood': 1} | {'full_meal': 1} |
| cook_carp_048 | campfire 1 | {'fish_carp': 1, 'wood': 1} | {'cooked_food': 1} |
| cook_crucian_carp_048 | campfire 1 | {'fish_crucian_carp': 1, 'wood': 1} | {'cooked_food': 1} |
| cook_catfish_048 | campfire 1 | {'fish_catfish': 1, 'wood': 1} | {'cooked_food': 1} |
| cook_eel_048 | campfire 1 | {'fish_eel': 1, 'wood': 1} | {'cooked_food': 1} |

所有新配方默认已知；手动加工即时、后台加工使用046工人时间，不因配方表存在就开通自动操作权限。鱼料理在篝火手动完成，无后台自动钓鱼。

## 2%额外宝藏池

| 命中2%后奖励 | 条件权重 | 唯一事实／知识 | 图对应箱产物 |
|---|---:|---|---|
| treasure_map_1 | 35 | nature048_treasure_1 | {'bow_2': 1, 'metal_ingot': 8} |
| treasure_map_2 | 30 | nature048_treasure_2 | {'pickaxe_3': 1, 'steel_ingot': 4} |
| treasure_map_3 | 20 | nature048_treasure_3 | {'crossbow_3': 1, 'steel_ingot': 4} |
| blueprint_hunter_bow | 15 | blueprint_hunter_bow | 学习猎手长弓知识 |

重复结果给2绳＋2草药，不重抽。3张图首次取得／阅读／箱生成／开启保持同一稳定链；图先于箱，箱不刷新。猎手图纸沿用047知识ID，与049支线共享去重。
图1箱约首营200—400m安全探索支路；图2／3约400—800m高地或远岸，必须徒步可达、避开永久不可回访的剧情空间；最终实际坐标由049关卡稿落地。

## 验算与缺口

[候选审计脚本](audit.py)检查供给、依赖、头击、养殖投入、概率与跳时模型；[结果](../../qa/TASK-048/calculation.json)只表示纸面关系。动物UE适配与动作、自然点实际放置、真实钓鱼输入和所有游戏运行仍未执行。
