# TASK-016 交接｜世界知识快照与回档隔离

用户授权实现、验证、提交并推送。任务分支 `codex/TASK-016-save-snapshots`，真实基线 `f2a8e3df41febcc4f4f0c4235fb477bdcb7e564e`，不合并main；004未执行。实现提交 `17f6008b55f102936aa6c6056b0bab5326088401`；[完整绑定](../qa/evidence/TASK-016/commit-binding.json)区分最终构建/原生验证与PIE覆盖范围。

## 当前实现

- 显式非Shipping伙伴夹具的完整已接入状态在同一游戏线程边界捕获；USaveGame内存序列化，CRC与长度校验，临时文件回读成功后Windows同目录原子替换。正常池为Saved/SaveGames/HearthwardPrototype/pool.hws，临时.pending不是用户节点。
- 全池共用最多50点；手动/锁定保护，只轮换最早未锁自动档。新进度同样需要一个空位或可轮换的自动节点，全保护时明确提示先腾位；没有第51个退出档。
- 自动保存默认10实玩分钟，配置1—60；暂停冻结，危险延后，严重饥饿可存并提示。生产危险系统缺失，六类危险输入来自明确夹具，未自行裁定R22交叉规则。
- 还原玩家位置/视角/背包/计时，世界累计时间、共享库存，伙伴/营地/资源位置、有限资源、伙伴携带物资、任务阶段/目标/已入库量与采集计时。恢复任务重新绑定新epoch和执行GUID；不回放物资结算事件。
- 原始玩家交流与NPC已输出台词随快照回退，知识修订与内容共同保存；近期最多8条（合计512字符预算）以不可信交流历史进入原有过滤上下文，实际世界事实继续由UE提供。没有外部向量库路径或跨进度共享记忆。
- 加载废止待处理HTTP、清空未来回复/候选/上下文，关闭旧对话草稿并恢复输入；旧库存epoch和结构化候选无法执行。恢复事件在全部状态赋值后发布，HUD消费事件，保存模块不依赖UMG。

## 使用

开发PIE先运行 `Hearthward.Companion.CreateTest`。准备明确初始位置/状态后执行 `Hearthward.Save enable`，再执行 `Hearthward.Save new`。初始快照取enable时的夹具状态，不定义正式开局地点。
`Hearthward.Save manual` 保存，`Hearthward.Save list` 在日志列出完整GUID、进度、时间、地点、阶段、类型和锁定；`Hearthward.Save load <GUID>` 加载；`lock/unlock/delete <GUID>` 显式管理节点。
同一测试地图重启PIE后重新创建伙伴夹具并enable，再读取旧节点；不自动切图，不自动重建任意实体。所有开发入口在Shipping关闭。

## 验证与绑定

环境：Windows 11，UE 5.8.1，MSVC14.44.35228，SDK10.0.22621.0，RTX4060 Laptop8GB。全部引擎构建/启动/测试由GameFactory公开UEClient执行，独立测试池GUID隔离人工档池。
实际结果：Editor最终构建PASS；Hearthward.Save原生2/2 PASS；两轮PIE 54/54 PASS；仓库结构0错误，工具自测31/31 PASS。原生启动仍记录此前已有13条Condition failed，本单未新增字段初始化诊断；用例结果为Success，不将启动诊断抹去。
文件证据：[build.json](../qa/evidence/TASK-016/build.json)、[原生报告](../qa/evidence/TASK-016/automation-index.json)、[PIE报告](../qa/evidence/TASK-016/verification.json)、[工作流检查](../qa/evidence/TASK-016/workflow-validation.json)。
最后只修改历史上下文预算与对应测试，未修改保存/恢复路径；最终构建和原生测试已覆盖该修改。PIE的54项在此前池规则和初始化修正之后运行，真实模型与回档边界已验证；没有将它伪称为预算修改后的完整PIE重跑。
基线范围检查如实失败：21个必要集成/文档路径不在原Backlog允许列表内；按用户本轮授权补齐的当前范围无越界。未更换base、未修改检查器。
测试脚本见 [verify_save_pie.py](../qa/evidence/TASK-016/verify_save_pie.py)，覆盖T-011与T-012已实现夹具范围。正式生命/战斗/建筑/旗帜/宝箱等范围仍NOT_RUN。

首轮原生测试暴露View未显式初始化，已改为ZeroRotator；首轮PIE通过后复核GDD，将“新进度满额一律先删除”修正为与全局轮换规则一致，避免追加设计没有规定的限制。最终报告必须来自这两处修正之后的构建。

## 限制与后续

- 这是schema 1开发格式，不承诺旧版本自动迁移。CRC用于意外损坏检测，无防篡改声明。整个池为一个原子文件，池损坏会明确拒绝读取和新增；没有备份恢复菜单。
- 仅覆盖已接入的伙伴独立夹具；生命/技能/敌人/建筑/地图解锁/旗帜/生产/宝箱等尚未实现，不用假字段声称完整GDD存档。进行中的独立交互不能保存，额外未接入容器不能保存；加载参与者缺失或地图不同会拒绝。
- 当前同步写入适合小型快照，尚未做正式大世界写盘性能验收。完整知识编辑/撤销、长期约定索引、Embedding、正式存档UI和退出提醒待后续任务。随机系统未实现，未声称验证未来随机结果变化。
- 初始位置、自动档延后交叉细节、生产危险来源按R22继续开放。TASK-004、main合并、发布、打包与第二台机器验收不在本单范围。
- Issue、独立评审和正式prototype审批仍缺失，流程状态Blocked；不将用户执行授权冒充独立审查通过。

工程参考：[Epic保存/加载文档](https://dev.epicgames.com/documentation/unreal-engine/saving-and-loading-your-game-in-unreal-engine)。复用USaveGame已有序列化机制；大规模游戏中自动存档的异步写入建议留待实际性能需求时接入。
