# TASK-010 共享仓储与真实转移交接

## 交付范围

一个Game/PIE WorldSubsystem持有唯一营地库存；仓储访问组件不保存第二份数据。
普通物品复用008定义，个人容量100保持；营地无负重限制，库存int32计数、int64累计重量，数量表达溢出明确拒绝。
整批转移先在两份临时状态验证，通过后提交两端。缺量、超重等失败均不部分执行，无库存预留。
每次操作携带operation GUID和timeline epoch。身份和容器有效的首次请求结果记录到epoch结束；相同请求重试实际移动0，不广播；不同载荷复用ID返回冲突。
事件只在提交两端及重试记录后发出；个人原有事件使移速和009 HUD读取真实新状态。
旧epoch请求在查去重记录前拒绝。AdvanceTimeline保留库存，只切换epoch/清空去重记录，不等同于读档。
访问点调用者负责未来营地解锁和正式交互规则；当前不接个人Tab远程仓储入口。

## 设计与流程边界

依据GDD v0.3第5/7章、Q049/Q215/Q226/Q228/Q252/Q253；没有修改设计、未定R项或004任务。
CT-001仍DRAFT；本单具体实现语义见[任务单](../tasks/TASK-010.md)，未自批跨系统契约。
个人容器GUID只标识当前世界实例；保存实体ID、存档、弟弟背包容量、生产/奖励消费者、口粮转换、装备、营地解锁及交互UI尚未实现。
实现扩展既有状态与结果枚举（原枚举值保持顺序），没有创建第二套个人库存或改变持久化格式。
操作日志仅驻内存，当前epoch不淘汰记录；之后接存档/长期会话时需由契约任务确定持久化及清理边界。

基线 `4a66538ba88b3f556d391bd79df2c6b272d54c2e`；分支 `codex/TASK-010-shared-storage`。
用户授权实施和提交推送；Issue、独立评审未落实，流程继续Blocked。未合并main。
当前证据来自本次实现工作树，提交后补完整实现SHA绑定。无Content修改、无新增资产锁。

## 验证

UE5.8.1 / Win64 / Development Editor，MSVC14.44.35228、SDK10.0.22621.0；引擎操作均经UEClient公开API。
- 最终Editor构建通过：[构建结果](../qa/evidence/TASK-010/build.json)。
- `Hearthward.Inventory` 4/4原生测试通过，包含008两项回归及转移/重试两项：[原始报告](../qa/evidence/TASK-010/automation-index.json)。
- 先前已记录的13条引擎启动期Condition failed仍保留：[结果摘要](../qa/evidence/TASK-010/automation-result.json)。测试结果Success、退出码0，不代表日志无错误。
- 两轮真实PIE共42/42检查通过：[结果](../qa/evidence/TASK-010/pie-results.json)。每轮2个Actor共用一个容器GUID，跨PIE容器身份不同；6次成功转移、8次个人库存事件（含2次明确授予），无失败/重试额外事件。
- 两张1280×720截图已检查：[取出50份](../qa/evidence/TASK-010/withdrawn-half.png)、[取出100份](../qa/evidence/TASK-010/withdrawn-full.png)，HUD实际负重同步；满载行走315cm/s。
- 仓库自检通过；[范围记录](../qa/evidence/TASK-010/scope-check.json)按用户授权下更新后的任务声明核对，不等同于原TribeGame占位任务的基线审批认证。

首次编译定位到重量返回类型扩为int64后，旧测试int字面量使TestEqual重载歧义；已显式匹配int64，未删减断言。
首次PIE夹具调用了Python未导出的Actor生成函数，尚未开始玩法测试即退出；改为非Shipping开发命令创建两个真实运行期访问Actor。
第二次PIE唯一失败为Python返回GUID结构体的直接比较；改用引擎GuidLibrary值比较并记录三方GUID文本，最终确认完全一致，未改游戏状态实现。
原生4项通过后只新增开发夹具命令，再构建并运行PIE；未改变已测转移/容器代码，因此未重复执行同一原生套件。
失败尝试保留于本机 `.agent-local/task010-output.txt` 指向的隔离输出目录。
验证未覆盖完整存档读回、正式营地解锁、交互距离或打包；T-004/T-005仅在当前库存基础范围验证。

## 复现

见[构建和测试入口](../qa/BUILD_AND_TEST.md)和[PIE脚本](../qa/evidence/TASK-010/verify_storage_pie.py)。
开发命令 `Hearthward.Storage.CreateTestAccess` 创建两个无外观访问Actor，不授予物品、不保存地图，Shipping不注册。
脚本明确授予测试木材120份，分两次存入，然后从另一个访问点取出；UI仍使用009背包面板。
正式玩家仓储操作界面需要后续任务接入营地规则，当前交付为可运行、已验证的库存基础接口。
