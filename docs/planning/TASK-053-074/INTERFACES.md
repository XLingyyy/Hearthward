# 053—074共享接口设计草案

批准入口仍为已有CT契约。本文件描述已批准施工的接入约束；不建立第二套公共结构，不提前写空接口。若实现需要改公共声明，所属任务先将准确字段／调用方补进原契约并登记Owner授权。

|边界|唯一提供者与消费者|后续接入要求|
|---|---|---|
|时间|WorldClock提供A／W／epoch，Survival、Camp、Nature、Campaign消费|实玩动作消费A，生产／成长消费W；暂停和加载都不推进。所有跳时走052协调器，不各自Tick补时间|
|动作|现有Interaction／Traversal／Combat／Survival提供，Input／UI／伙伴消费|开始时校验epoch、存活、距离、遮挡、状态、实际实例；完成前复核。伤害／取消／读档／旅行终止旧动作。高优先动作失败不掉进另一个动作扣费|
|导航|CompanionNavigation和CharacterMovement执行，Campaign／Nature／正式连接消费|导航查询只对已加载／已生成网格有效；待加载、不可达、被取消分开反馈。连接执行前复核落脚和胶囊，不直接瞬移伙伴|
|敌人|CombatTarget维护感知与伤害，Campaign维护EnemyId／Generation／区域及持久事实|演员卸载不能复活、重发经验或丢尸体；搜索只读最后已知位置。警报传播成功才提交尸体地区标记；击晕与击杀清敌等价|
|装备与产物|Inventory／Storage权威，Workshop／Camp／Nature／Campaign消费|InstanceId、来源ID、掉落／领取账本贯穿转交／存仓／放地／维修；多端原子结算，不用物品名字代表独立实例|
|营地|Camp提供人口、阶级、公共口粮；Building提供设施实例与投入账本|两地全局共享一次，设施／队列／生产区独立。一个PersonId只分配一次；五身体岗位，兄弟在真实劳动时工效3|
|生态与动作|Nature提供AnimalId／生命代次／状态／实际位移；AnimalMotion负责可见姿态|060明确动画驱动与生态移动的单一拥有者；捕捉／牵引／死亡都不运行第二个逃离移动器，演示模式保持独立|
|任务与知识|Campaign条件／奖账本、Progression知识事实，UI与伙伴消费|正式23任务与旧11任务命名空间分开；发现、激活、完成、已领奖分开；无真实事件不能推进条件|
|模型|LocalAI执行已有一次本机推理，AgentContract投影能力；UE校验和执行|保留知情范围、显式确认、约束版本、task／request／epoch。未安装／未就绪反馈可用性；旧回复永远不进入新世界|
|保存|Save聚合现有领域快照，所有领域提供restore／validate|保存前领域处于一致边界；恢复先废止旧epoch，再还原同一快照。不保存Actor指针、不推测离线产物、不删未知未来档|

共享写入窗口：gameplay.json、Save公共声明、输入设置、主地图／外部Actor及正式契约每次只有一个任务写。小组协作或锁的将来需求按WORKFLOW处理；施工共享窗口与资产锁按各任务JSON及实际LFS记录逐项登记，根Agent串行集成与运行引擎。

导航接入借鉴[Epic Navigation Invokers](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-navigation-invokers-in-unreal-engine)的已加载局部网格；[World Partition NavMesh](https://dev.epicgames.com/documentation/en-us/unreal-engine/world-partitioned-navigation-mesh)在当前官方文档标为Experimental，因此本设计保留既有方案。敌人感知参考[Epic AI Perception](https://dev.epicgames.com/documentation/unreal-engine/ai-perception-in-unreal-engine?lang=en-US)的刺激更新和观测记忆原则，复用当前组件，不默认改写为另一套AI框架。
