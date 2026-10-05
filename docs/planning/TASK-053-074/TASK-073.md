# TASK-073｜执行首版全流程试玩和最终平衡

2026-10-03，设计Approved／施工已授权，实施按依赖推进。原稿075，阶段D，P0；Owner／Reviewer：XLingyyy，无Issue。

依赖：TASK-060、TASK-061、TASK-062、TASK-066、TASK-067、TASK-068、TASK-069、TASK-070、TASK-071、TASK-072。基线、权限和共同规则见[BASELINE](BASELINE.md)、[DECISIONS](DECISIONS.md)、[INTERFACES](INTERFACES.md)、[ACCEPTANCE](ACCEPTANCE.md)。

## 入口与体验目标

使用已过存档／双后端性能／独立机器的同一RC来源，正常新游戏完成夜袭→首营→首救与升阶→生态和成长→四区→永久夺回→实际使用第二营地。3名新玩家，无作者提示，全员无阻塞完成；有效实玩中位数8—12小时，加载／暂停／真实休息单列。15支线由全部样本合计覆盖，不要求每人全清。

## 记录和评估

按探索、经营、战斗互斥分类记录3:3:4观察比例；另记迷路／菜单／等待／重试，不能藏进“经营”凑十小时。观察升级节奏、工具和装备更换、药食消耗、来源与工作量、钓种养价值、营救人口、四区潜入／正面选择、弟弟有效协作和模型等待占比。测试不同合法路线与任务先后，保留失败／退出和角色反馈。

核对所有主线与支线、两地建筑／分工、旧档继续、过期模型请求、真实危险和永久世界。玩家1v3与弟弟1v3两阶段真人样本、068表达集、069访问性和070实听未完成项由对应任务补齐，不用三名全篇玩家自动抵消这些门槛。

## 平衡改动原则

先列阻塞／误导／经济失衡／战斗压力／节奏问题及证据，定位参数、触发和实际供给，给最小调整建议、预计影响及原批准设计。材料／经验／药效／效率／2%宝藏／驻军／胜利阈值均有Owner基线，改数必须明确确认，不在验收中隐改。

修复功能缺陷回归该路径；调整共享经济／战斗参数才扩大关联流程。变更后保留原样本结果，不删除不达标样本；新版本受测SHA重新绑定。可接受的非阻塞瑕疵需Owner明确列入已知问题，P0／复制物品／丢档／无法完成不能带入RC。

## 出口

完整体验报告、任务覆盖表、阶段时间与资源统计、角色反馈、缺陷闭合及获批平衡diff、最终验收状态。录音暂缓如仍影响首版范围，按D06由Owner处理；无真人样本或时长门槛未达标，报告缺口，不能宣布首版验收完成。试玩不增加新玩法，不以重复采料或强制等待延长时长。

## 建议施工范围

- `docs/qa/TASK-073/`
- `docs/handoffs/TASK-073.md`
- `Resources/Data/gameplay.json（仅实测后Owner批准平衡项）`
- `发现的功能缺陷按所属任务精确登记路径`
- `README.md`
- `docs/PROJECT_STATE.md`

Owner已确认本方案并授权施工；上述范围在任务激活时按准确文件登记到allowed_paths。契约沿用：[CT-TASK-049-campaign](../../contracts/CT-TASK-049-campaign.md)、[CT-TASK-046-camp-economy](../../contracts/CT-TASK-046-camp-economy.md)、[CT-TASK-050-companion-activities](../../contracts/CT-TASK-050-companion-activities.md)、[CT-TASK-052-clock-refresh](../../contracts/CT-TASK-052-clock-refresh.md)。Shared Save／gameplay.json／主地图变更先在所属单登记准确边界；不因列入建议路径而自动授权。
