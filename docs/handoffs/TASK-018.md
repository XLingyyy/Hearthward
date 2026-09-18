# TASK-018｜玩家木材采集、营地入库与回档闭环

用户明确授权自行拆解018、实现、验证、提交推送。基线 `78965c9543b233dc90fa794c2d601426ac0a567b`，017末笔交接已补推。本单分支 `codex/TASK-018-player-gather`，实现验证完成，已按授权提交推送，实现提交 `7b871e90a16340336d20545eda4b969e6da49a1d`；不合并main。

## 行为与边界

PIE执行 `Hearthward.Companion.CreateTest`，现有16木材资源方块和营地薄板新增玩家交互。150cm内按E，资源点五秒完成后真实转移1木材至背包；营地五秒完成后将随身全部木材入库。满载、耗尽、空背包明确提示，失败不改数量。HUD展示数量、单重及剩余容量。

沿用011交互计时，移动/受伤中断，暂停冻结，目标销毁撤销，重复E不重启或重复结算。与伙伴共用同一资源；竞争时按结算瞬间余量处理。017面板和016快照直接覆盖资源/背包/仓储；交互中保存明确拒绝，读旧档撤销当前动作。不增加存档字段。

只有显式非Shipping初始化后生效。1木材、五秒、150cm及整批木材入库均PROTOTYPE_ONLY测试参数。正式采集效率/工具表/配方/刷新/键位仍OPEN；未实现生产地图、建造或全物品仓储界面。

## 验证与绑定

环境为Windows11、UE5.8.1、MSVC14.44、RTX4060 Laptop8GB。引擎生命周期通过GameFactory公开UEClient；每轮使用独立存档池。

- [最终Editor构建](../qa/evidence/TASK-018/build.json)：PASS。
- [原生报告](../qa/evidence/TASK-018/automation-index.json)：Hearthward.Resource 2/2 PASS，覆盖真实组件转移、容量/耗尽/距离/未启用拒绝。
- [两轮PIE](../qa/evidence/TASK-018/verification.json)：39/39 PASS，覆盖延后结算、重复键、暂停、中断、容量、入库、伙伴争用、目标销毁、交互中存读档及第二轮磁盘恢复。
- [最终镜头复测](../qa/evidence/TASK-018/camera-verification.json)：4/4 PASS，摄像机距玩家404.47cm，实际采集数量及营地目标可用。原生/39项之后唯一玩法源码变化为伙伴胶囊忽略Camera通道，未改转移/计时/存档，未重复全套测试。
- [首轮实键录像](../qa/evidence/TASK-018/physical-before-camera-fix.mp4)：约159秒441帧；E采集、Tab背包、步行返营、E入库，数量从14/0/2变为13/0/3（资源/随身/仓库）。其中发现镜头问题。
- [修正后实键录像](../qa/evidence/TASK-018/physical-final.mp4)：约80秒223帧；近营地镜头正常，E入库从15/1/0变为15/0/1。录像由原生连续帧按实测时间编码，已观察操作并检查编码后首段每15秒/末段每5秒关键帧，未逐帧审阅。见visual-review.json。
- 仓库自检0错误，工具31/31通过。基线范围检查如实失败：基线没有本次自行拆解的018任务单；当前授权路径无越界，未伪造基线审批。详见workflow-validation.json。

测试执行于提交前工作树；[源码与验证绑定](../qa/evidence/TASK-018/working-tree-binding.json)记录覆盖范围，真实实现SHA见[commit-binding.json](../qa/evidence/TASK-018/commit-binding.json)。

## 已修正问题

首轮编译格式字符串不接受三元表达式，改为字面量分别调用；Unity分组暴露SaveDebug全局Command与既有测试局部Command重名，仅改命令注册变量名。原生测试夹具重复InitializeNewWorld导致WorldSettings重名，移除重复初始化后2项通过。首次PIE脚本误用不存在的get_pawn，改为GameplayStatics.get_player_pawn后39项通过。

实键返营时伙伴胶囊阻挡Camera导致弹簧臂缩入玩家头部；仅忽略Camera响应、保留Pawn碰撞，重新构建并核对实际摄像机位置及录像。

无新增资产、依赖或存档schema，TASK-004未执行。未打包、未换机、未重新运行真实模型；既有模型路径未改。Issue查询GraphQL EOF，独立评审与正式prototype审批未完成，流程仍Blocked；用户的实现和提交推送授权已记录。后续建造交互需先明确具体配方和R13设计来源。
