# TASK-012｜伙伴安全委托与多趟采集交付

更新：2026-09-18。Owner：XLingyyy；分支：`codex/TASK-012-companion-delivery`。
基线：`941323ef0d47df2a08cd711b814d5c9aa21a6f5a`。本单叠加011及README工作流调整，尚未合入main。
实施与提交推送已有用户授权；Issue、独立评审、契约及正式原型审批未补齐，流程保持Blocked。

## 已实现范围

结构化命令票据包含GUID、代次与世界epoch。请求/提交均检查30米距离和暂停；模型候选不能修改安全判定或资源数量。
执行器只接受一个物品/数量目标与collect→return→deposit三步。缺少目标/数量返回澄清；未知/危险步骤及超过三步拒绝。
新候选未批准时继续旧目标；接受新的对应目标才替换。取消与覆盖保留已经发生的入库和当前携带物资。
旧代次、重复候选、取消后/epoch变化后的迟到候选不能激活任务；当前动作也在每帧结算前复核epoch。

开发伙伴实际走到有限资源点，等待五秒后将资源原子转入独立携带容器，再实际移动到营地才调用010共享仓储入库。
交付进度只按Transfer返回的MovedCount增加。真实资源不足、目标失效、采集中断或去程受阻时返营；回程被挡则保留位置和物资，通路恢复后继续。
同一同步结算期间拒绝重入请求/取消；库存两端先提交，再广播事件。普通背包增加一个可信C++原子转移入口，未提供模型自由编辑世界接口。

## 原型边界

- 本单是原有任务明确要求的PROTOTYPE_ONLY隔离场景。开发命令运行期创建圆柱/方块，不改Content、地图、Config或004。
- 16份木材、每趟重量4、五秒采集、180cm/s直线碰撞移动、50cm到达距离均为测试参数。携带容器复用100容量宿主，不确立R14正式弟弟容量。
- 原生Sweep检查根胶囊碰撞；测试通路不包含全地图路径规划/坡地。安全由场景明确给出，未实现生产环境危险识别。
- 原话独立保存，绝不作为库存数值；未实现自然语言解释、角色认知记忆、两阶段检索、长期约定或完整对话界面。
- R18长期安排覆盖、R20迟到文字显示、R14容量保持OPEN。仅对本单同类采集目标执行覆盖，不定义长期规则优先级。
- epoch钩子只验证旧动作失效，未实现TASK-013存档/回档。返营/失败反馈只在30米内显示开发状态。

## 验证

环境：UE5.8.1、MSVC14.44.35228、WindowsSDK10.0.22621.0。通过GameFactory UEClient公开API构建、执行原生测试、启动/关闭编辑器。
最终实现提交：`6abcc11880dd8b28a251fbbdc893b37fcdfa28e8`。测试发生在该提交前的本单工作树；原生/45项PIE覆盖的执行器与库存源码随后未变，开发命令数量解析的最后修复有独立最终构建与定向复测，详见下文。
本次后续绑定提交仅更新README/交接/项目状态，不改变运行代码。

| 项目 | 实际结果 | 证据 |
|---|---|---|
| Development Editor / Win64 | PASS | [构建](../qa/evidence/TASK-012/build-final.json) |
| Hearthward.Companion | 2/2 PASS | [原生结果](../qa/evidence/TASK-012/Hearthward.Companion.json) |
| Hearthward.Inventory局部回归 | 4/4 PASS | [回归结果](../qa/evidence/TASK-012/Hearthward.Inventory.json) |
| 两轮真实PIE | 45/45 PASS | [报告](../qa/evidence/TASK-012/pie-results.json) / [脚本](../qa/evidence/TASK-012/verify_companion_pie.py) |
| 定向画面与开发数量入口 | 4项行为+6项输入 PASS，4张截图已核对 | [报告](../qa/evidence/TASK-012/visual-results.json) / [脚本](../qa/evidence/TASK-012/verify_companion_visual.py) |
| T-020 real-model | NOT_RUN | [分模式记录](../qa/evidence/TASK-012/model-evaluation.json) |

T-007：真实三趟4+4+2=10，携带4且尚未入库时进度为0；入库回调均处于营地到达范围。
T-008：玩家虚报100份不会生成物资；实际剩余6份的8份目标只交付6/8并返营等待。
T-009：去程碰撞、采集中安全失效、资源Actor销毁均返营；回程碰撞期间背包仍4、仓库不增加，通路恢复后真实入库。
T-010：重复/迟到/取消/覆盖/旧epoch候选拒绝，当前行动在epoch切换后停止；两轮PIE库存和票据隔离。
另验证暂停冻结、结算重入、无提前预留及取消保留已采物资。

数量开发入口的`1.5`曾实际被解析成1；已改为逐位检查正int32，不再使用会截断小数的LexTryParseString。
复现见[修复前记录](../qa/evidence/TASK-012/visual-probe-before-fix.json)，该记录的ok仅指四项画面行为，数量探针另列失败；后续探针初版比较方式的问题也明确记录。
此修复只改开发命令解析，6项原生测试与45项PIE的执行器/库存路径未变；最终构建和定向开发入口验证补测，未重复整套测试。

两个原生测试组各保留13条启动期Condition failed诊断；测试条目自身通过。没有宣称引擎日志无错误。
T-020缺少已锁定的模型标识、版本与本单推理端点；未下载模型或调用付费接口。fixture PASS不代表real-model通过。
打包、两机复现、完整M0与正式伙伴系统验收NOT_RUN。

## 最终画面

[去程](../qa/evidence/TASK-012/lane-outbound.png)、[携带4但入库0](../qa/evidence/TASK-012/lane-returning.png)、
[10/10交付完成](../qa/evidence/TASK-012/lane-completed.png)、[缺料6/8返营等待](../qa/evidence/TASK-012/lane-shortage.png)。
已检查1280×720原生HUD文字与伙伴测试通路；截图分辨率不等于物理窗口/DPI兼容验证。
初轮行为脚本视角被主角遮挡；定向镜头改为侧视，显式指定Rotator的pitch/yaw/roll后保留上述最终画面。

## 复现与工程参考

手动入口和测试命令见[BUILD_AND_TEST](../qa/BUILD_AND_TEST.md)。根README已按当前能力和限制同步替换。
工程参考：[Epic Actor移动API](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/AActor)；
[SetActorLocation的根组件Sweep语义](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/GameFramework/AActor/SetActorLocation)。
正式全地图移动可后续接[AI MoveTo](https://dev.epicgames.com/documentation/unreal-engine/BlueprintAPI/AI/AIMoveTo)，本单没有添加导航模块/路径资产。
