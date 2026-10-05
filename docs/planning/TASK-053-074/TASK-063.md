# TASK-063｜让伙伴接入正式关卡与新增生活能力

2026-10-03，设计Approved／施工已授权，实施按依赖推进。原稿065，阶段B，P1；Owner／Reviewer：XLingyyy，无Issue。

依赖：TASK-050、TASK-054、TASK-056、TASK-057、TASK-058、TASK-059。基线、权限和共同规则见[BASELINE](BASELINE.md)、[DECISIONS](DECISIONS.md)、[INTERFACES](INTERFACES.md)、[ACCEPTANCE](ACCEPTANCE.md)。

## 当前基础与目标

[050能力矩阵](../TASK-050/CAPABILITIES.md)已登记营地物流、照料、已知安全采集、有限生产、维修／制作和同行活动；029—040已有单次推理、执行器、记忆、主动交流和导航仲裁。本单接正式地点、物品／设施实例和可执行入口，补缺项，不重写Agent架构。

## 能力与流程

首批覆盖切片：按正常自由措辞请弟弟跟随／等待／协攻，给取存真实物资，已知安全采集返营，救援护送，工作台有限批次及真实维修。意图理解→可知目标投影→任务卡确认→真实到达→原子执行→交付回执保持已有路径。未知点、未发现目标、未建设施和不足材料不提供假选项。

独立权限：安全物流／采集、已授权照料、有限营地批次、按实际设施维修制作；同行权限：护送、狩猎、钓鱼、捕捉，始终处于玩家同场和050距离／安全边界。普通食物<25%、药品生命<35%沿050，稀有物逐项授权；弟弟不能独立探未知地图、建造／拆除／升营地／调人口或使用隐藏宝箱。

060—062交付后，再验自然照料、同行猎鱼畜完整能力。063首批是065前置，后续能力是068／073前置；不得先把未接入能力标PASS。一个current task，新委托明确确认替换，routine只在授权和空闲时恢复；暂停／取消／战斗／倒地／通信越界优先级沿现有仲裁。

## 权威、失败与保存

模型只提出高层结构化意图，实际数量、路线、材料、物品ID和权限由UE定。受阻分无路／流送等待／缺料／满包／越界，保留已经发生的真实部分产物，不能语句生成全额奖励。通信范围30m；重新规划不得超出accepted constraints，也不能靠选择别名获得新物品。

所有异步命令带request／task／epoch／约束版本，回档废止旧命令及旧导航／动画结果。长期约定与撤销、事实和记忆保留原批准格式；不扩大输入预算、不改后端、不把世界快照全量泄露给角色。

## 最小验证与出口

先复用050目标用例，针对真实地点增加运输守恒、多趟、缺料、不安全／不可达／取消、回档和过期权限。正式地图用真实CPU或Vulkan做完整自由表达→确认→到达→取得→交付，日志分原始理解、校验、执行、事实回执；假的模型测试只验证契约。首批与生态扩展分别记结果，最终模型60表达和角色体验由068。动态回复仅文本，录音暂缓。

## 建议施工范围

- `Source/Hearthward/AI/（限现有能力与执行器接入）`
- `Source/Hearthward/Companion/`
- `Source/Hearthward/Gameplay/HearthwardCompanionBehavior.cpp及Source/Hearthward/AI/HearthwardNPCContextProjection.cpp`
- `Source/Hearthward/Nature/（能力消费接口）`
- `Source/Hearthward/Building/HearthwardWorkshopService.cpp`
- `Source/Hearthward/UI/HearthwardDialogueWidget.cpp`
- `Source/Hearthward/Tests/（现有伙伴相关定向用例）`

Owner已确认本方案并授权施工；上述范围在任务激活时按准确文件登记到allowed_paths。契约沿用：[CT-002-companion-command](../../contracts/CT-002-companion-command.md)、[CT-TASK-050-companion-activities](../../contracts/CT-TASK-050-companion-activities.md)、[CT-TASK-052-clock-refresh](../../contracts/CT-TASK-052-clock-refresh.md)。Shared Save／gameplay.json／主地图变更先在所属单登记准确边界；不因列入建议路径而自动授权。
