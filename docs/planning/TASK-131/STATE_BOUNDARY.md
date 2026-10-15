# TASK-131 持久与暂态边界

2026-10-13，D112-06已由Owner批准。实际基线schema9，新增字段采用schema10（封装0x4857533A），沿用50节点原池与安全保存规则。

|域|持久类型|暂态|
|---|---|---|
|灯|实例ID、Lit|渲染组件|
|捷径|实例ID、Open|碰撞/动画引用|
|协作|实例ID、结果枚举|会话、动作票据、维持状态|
|人物|人物ID、登记的步骤/选择|语音队列、候选对话|
|天气|实例ID、种子、阶段、阶段起始世界分钟|粒子、音源|
|踪迹|稳定动物ID、代次、有界位置/世界分钟|实时目标指针|
|陈列|实例ID、营地、槽位、装备GUID或事实|预览/临时网格|

内容在读取存档前注册具名定义；Actor绑定与定义分离，卸载后的弱引用不能提交，读档清空绑定后由内容重新解析。生产目录目前无新增实例；七类测试实体明确使用PROTOTYPE_ONLY前缀，仅在原生测试作用域注册。后续内容单注册实际ID，不用任意字符串袋承载新状态。

事务沿UHearthwardSaveSubsystem与UHearthwardStorageSubsystem执行。票据包含操作ID、稳定目标ID、原仓储epoch和状态revision；原料/产物使用仓储现有整笔预检。失败不改变库存或持久状态，成功后递增revision并销毁票据。读档复用AdvanceTimeline并取消会话、清空弱引用，不继续旧导航/动画/模型回调。此接口只提交状态；表现消费者在成功返回后更新画面。

旧schema9新增状态为空，保留所有原有反射字段。写盘沿现有pending文件、回读验证、版本备份和原子替换。新版字段不参与旧版伪装封装；未知实例/选择/未来封装拒绝，原件保留。v0.3.0真实样本缺失仍单列，合成夹具不能替代。

技术依据：[Epic SaveGame](https://dev.epicgames.com/documentation/en-us/unreal-engine/saving-and-loading-your-game-in-unreal-engine)。本项目沿已有USaveGame和标记属性序列化扩展，没有新增独立档池。
