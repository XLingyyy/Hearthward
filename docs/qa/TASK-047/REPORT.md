# TASK-047 实现验证报告

2026-09-27，Windows / UE 5.8.2 / Editor Development。Owner、Reviewer：XLingyyy。Owner已批准D1—D6及沿047实施、提交推送；canonical047对应原稿049。

## 受测版本

- 主干基线：`dbba6d052b7e08e612d858ca07fc87e90dae5840`（046已集成）。
- 实施范围批准基线：`06dce4924dccd58f903cc76548a3a5dc2ea8e936`。
- **本轮全部最终UE证据绑定源码：`fa8df934f7ccc0912810812b74e186ff9cd3721e`**。
- 后续交付提交只整理文档、日志和媒体，不修改Source、Resources或PIE脚本；本报告不把任务分支测试宣称为main集成验证。

## 交付内容

60级成长、29技能各3级／满级52点、免费安全洗点；62物品、49制作配方、38件耐久装备；独立装备GUID、逐件损耗、四部位护甲、玩家／弟弟／仓储转移与明确穿戴；设施与图纸限制、25%／50%／全修、背包优先仓储补料且原子结算；初始装备一次领取、后勤员和双方各自付费的五阶背包。

schema6保存双方背包、仓储、地面装备与奖励事实。旧档按比例迁移耐久，确定性生成实例ID、返还旧技能，不补装备或回血；首次覆盖旧档前保留原字节到`.pre-schema6`。跨容器重复GUID、重复唯一装备和不支持的schema被拒绝。正式运行表为`Resources/Data/gameplay.json`；planning中的候选JSON保留批准时的历史提案。

## 实际结果

| 验证 | 结果 | 证据 |
|---|---|---|
| UEClient `build.project(target='HearthwardEditor', configuration='Development')` | PASS | [最终构建](build-final.json) |
| UEClient `testing.run_automation_tests(...)` | PASS，32/32，0 warnings | [命令与日志](native-final.json)、[逐项结果](native-report.json) |
| 真实Editor PIE、UI动作、存读档及重开PIE | PASS，64/64 | [启动](pie-final-launch.json)、[断言结果](pie-final.json)、[重放脚本](verify_equipment_pie.py) |
| 本任务UEClient关闭自己的Editor | PASS | [停止记录](editor-final-stop.json) |
| `python scripts/validate_repo.py` | PASS，0 errors | [仓库日志](repo.txt) |
| `python scripts/validate_repo.py --task TASK-047 --base 06dce4924dccd58f903cc76548a3a5dc2ea8e936` | PASS，0 errors | [范围日志](scope.txt) |
| `python -m unittest discover -s scripts/tests -v` | PASS，33/33 | [工具日志](tools-implementation.txt) |

原生过滤器为`Hearthward.Inventory+Hearthward.Storage+Hearthward.Save+Hearthward.Gameplay+Hearthward.Combat+Hearthward.Camp+Hearthward.Survival+Hearthward.Resource+Hearthward.NPCAgent.OwnBagWorkshopTransactions`。运行参数、平台和完整输出见JSON。

重点原生覆盖：同款装备独立耐久、低于一次耗损仍能完成当前动作、转移身份保持、重复交易与冲突、局部维修向上取整、背包优先仓储补料、材料不足／超重时整笔回滚、曲线与技能预算、旧档迁移及原路径覆盖前备份、跨容器／地面重复实例拒绝。

PIE在`/Game/Hearthward/Bootstrap/L_Bootstrap`中使用实际组件及UI动作。覆盖建工作台→制造第二把斧→只修指定副本→转交弟弟并明确装备→分别付费升级背包→掉落／拾取→存读档→重新进入PIE从磁盘继续。随后覆盖五次营救奖励、图纸奖励与学习、故乡唯一装备奖励、唯一装备放下确认与存读档去重。过期UI时间线写入被拒绝，洗点不补满状态。

复现时从上层GameFactory环境创建`UEClient(project_path=<本工作树/Hearthward.uproject>, ue_root=<UE_5.8>)`，按上述构建和过滤器运行原生测试；同一个client以`runtime.launch_editor`启动Bootstrap，传入独立`-HearthwardSaveTestPool=<uuid>`、有效`-HearthwardAIBundlePath=<LocalAI目录>`和`-ExecutePythonScript=<本工作树/docs/qa/TASK-047/verify_equipment_pie.py>`。等待`Saved/Task047/pie/results.json`生成后，用该client的`runtime.stop_editor`关闭自己的PID。完整启动参数见启动JSON；本机绝对路径需按环境替换。

## 画面证据

- [逐件维修](equipment-repair.png)：同款两把斧分别为50/80和70/80，维修报价与当前选择一致。
- [弟弟装备](brother-equipment.png)、[背包升级](backpacks.png)、[重开PIE恢复](disk-restored.png)。
- [唯一装备放下确认](unique-confirm.png)：真实确认弹窗和取消路径。
- [实际PIE采样录像](equipment-pie.mp4)：26帧、约16.56秒，按引擎Shot SHOWUI采样时刻生成可变帧率视频，包含编辑器窗口。它记录所采界面片段，不能用作连续键鼠操作、动作质量或性能证据。[编码记录](video.json)。

已检查维修与唯一装备确认截图：文本可读、按钮无重叠。临时地板和测试对象仅存在于未保存的测试世界；未修改Content资产。

## 失败尝试与修正

保留build-first至build-seventh、native-run/native-second和pie-first-failure/pie-second等中间记录，最终结论以上表为准。编译期修正头文件顺序、可见接口与返回类型、SpawnActor参数和JSON键构造；新测试修正对同一数组扩容时引用失效的问题；旧战斗／配方断言同步本次批准的斧伤害和正式配置标记，未删测试或跳过检查。

首次PIE脚本错误选择初始保存节点，改为实际最新节点后，同PIE与重开PIE恢复均通过。首次启动器未保留UEClient对象，新client不能关闭该PID；通过本任务窗口“不保存”关闭，随后所有运行保留同一个client至停止。最终停止成功。

## 未覆盖与交付边界

- 本次PIE通过组件／UI自动操作及明确资源、胜利夹具进行；未声称自然地图已经布置完整资源点、敌人、营救和故乡关卡。这些内容继续由048／049接入。
- 狩猎／钓鱼完整物种和自然源点、主支线布置、新装备专用视觉资产未在本单制作；现有模型复用。
- 新Shipping包、第二机器、全自然路线、十小时节奏／平衡、长期LLM表现及Owner亲自体验验收：NOT_RUN。
- 任务分支实现和定向验证完成；正式PR审查、main合并与集成验收由Owner处理。Owner／Reviewer同人为既有授权，Agent不将其记成独立审查通过。
