# CT-TASK-046｜营地经济、库存与保存

状态：随Owner批准D1—D6并授权046施工采用。单写者为本任务，Source/Hearthward、Resources/Data和现有UI为协调范围；不修改共享地图或二进制资产。

- 新增Camp领域状态与WorldSubsystem；人口、营地阶级、口粮、区域岗位、有限点位、生产批次、设施等级／实付账本、已救援／永久夺回事实只有一份。UI读该系统并提交命令，不自行计算产出。世界时钟在生存失败截断后的实际W区间推进经济；暂停与离线不计，睡眠不计兄弟劳动。
- 共享仓储继续使用StorageSubsystem。整批原子扣料／产出；建造预留对转移、加工和后台生产都可见。建造中保存拒绝，取消释放预留；完成同时发布设施账本和扣料。已开始生产投入已扣，取消明确确认损失；移动／升级暂停而不取消批次。
- BuildingComponent保留实体摆放与稳定建筑ID；Camp按同ID保存设施等级与累计投入。旧档已有建筑作为零投入迁移，不按新配方补记高价材料。新增建筑必须记录实际支付材料；拆返一次，向下取80%，共享库存不受拆除影响。
- 运行表由获批候选转为正式稳定ID；未批准的047高级配方／048物种与白名单不填假数。自然图已有树木可按043两日规则注册；其他点位要有明确配置。无源／缺料显示等待，禁止免费口粮产出。
- SaveGame schema5新增可选CampEconomy字符串快照。完整验证后恢复库存、时钟、Camp和建筑引用；缺字段旧档保留既有等级／建筑，初始化未分配人口和一次初始公共池，随后保存该状态，不反复赠送。新epoch拒绝旧UI命令。快照包括投入、余数、源点due、人物唯一岗位与事实回执，无现实时间补算。
- 模型和文本不能发“救援成功／故乡夺回”来改经济。任务／区域的实际完成回调提交稳定事实ID并去重；046提供与现有任务完成事件的结构化连接，不把Demo击杀计数当完整故乡胜利。
- 通过原生测试覆盖源点守恒、分段／跳时等价、劳动力、取消／退款、升级、旧档与畸形快照；真实PIE覆盖面板命令、建造／升级／移动、库存和跨PIE存读档。

实现参考：Epic [World Subsystems](https://dev.epicgames.com/documentation/unreal-engine/programming-subsystems-in-unreal-engine)与[SaveGame](https://dev.epicgames.com/documentation/en-us/unreal-engine/saving-and-loading-your-game-in-unreal-engine)。复用项目现有Subsystem和SaveGame边界，不新增第三方依赖。
