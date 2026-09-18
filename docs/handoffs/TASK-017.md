# TASK-017 交接｜存档管理与退出确认界面

用户于2026-09-18授权接手推进017，并明确由Agent根据现有游戏设计自行拆解。已根据当前DOCX第13/14章完成存档管理UI，实现与验证位于任务分支 `codex/TASK-017-save-menu`。开发基线为 `7ef4abe6c89ce3043eb5a57c794eaec49939626c`；用户随后明确要求“提交并推送”，按该授权提交推送任务分支，不合并main。TASK-004未执行。

## 实现与使用

- `HearthwardSaveWidget` 原生UMG列表/详情展示全部进度共用50点、UTC时间、所属进度、地点、阶段、手动/自动、锁定/轮换保护；滚动列表支持全部50个实际节点。
- 手动保存、加载、新进度、锁定/解锁、删除和1—60分钟间隔直接调用016公开接口。保存模块新增只读启用状态与六类危险原因查询，并区分手动拒绝和自动延后提示；schema、存储文件和容量规则不变。
- 读取、新进度、删除、退出先确认。确认绑定原节点且屏蔽其他操作，取消无副作用；退出警告未保存进度丢失，不创建退出档。
- F6临时非Shipping入口；Esc/F6关闭，有确认时先取消；Tab切换背包。打开暂停，与背包/交流互斥；关闭仅解除本菜单取得的暂停，平衡移动/视角屏蔽并恢复光标。回档后菜单继续显示实际恢复结果。
- PIE先执行 `Hearthward.Companion.CreateTest`，按F6，首次点击“启用存档”，再新进度或加载。初始节点仍取启用时的夹具状态；没有自动切图/创建任意世界实体。

UI沿用015原生UMG样式，无新增资产、插件或依赖。实现主要位于 `Source/Hearthward/UI/HearthwardSaveWidget.*`、`HearthwardHUDSave.cpp`；既有HUD、DialogueWidget仅做互斥/入口接入。

## 验证

环境：Windows11，UE5.8.1，MSVC14.44.35228，SDK10.0.22621.0，RTX4060 Laptop8GB。引擎生命周期全部通过GameFactory公开UEClient，显式指定Hearthward；每次使用新GUID独立测试池。

- 最终Editor构建：PASS。[build.json](../qa/evidence/TASK-017/build.json)
- `Hearthward.Save` 原生2/2：PASS。[automation-index.json](../qa/evidence/TASK-017/automation-index.json)。此后仅修正UI排版；Save模块未变，最终UI由构建/PIE覆盖。已有13条引擎启动Condition failed仍记录，匹配的两项测试均Success。
- 最终两轮PIE48/48：PASS。[verification.json](../qa/evidence/TASK-017/verification.json)。覆盖空池/夹具缺失、确认/取消、实际磁盘写入与跨PIE回档、epoch变化、全50点保护、六类禁存及饥饿提示、间隔边界、损坏拒绝、菜单互斥与外部暂停保留。
- 实键与鼠标：已验证选择/滚动、锁定、读取确认/取消、保存、F6开关、Tab背包、Esc取消退出，以及关闭后W行走。[128秒录像](../qa/evidence/TASK-017/physical-input.mp4)、[状态轨迹](../qa/evidence/TASK-017/physical-trace.jsonl)。录像由原生Shot showui连续采集316帧，按实测时间间隔编码；已检查操作过程与编码后每10秒关键帧，未宣称逐帧审阅。
- 独立 `-game`：1280×720及1920×1080启动分辨率的面板检查通过；Windows150% DPI使窗口截图像素尺寸不同。[720窗口](../qa/evidence/TASK-017/standalone-720.png)、[1080窗口](../qa/evidence/TASK-017/standalone-1080.png)。未打包，运行的是Editor `-game`模式。
- 独立游戏确认退出后正常关闭，文件大小/修改时间均未变化。[退出检查](../qa/evidence/TASK-017/quit-check.json)、[退出日志](../qa/evidence/TASK-017/standalone-quit-log.txt)。
- 仓库自检0错误、工具自测31/31通过，当前授权路径无越界。基线范围命令如实失败：017由本轮首次拆解，基线不存在017任务单；未修改检查器或替换base冒充已审批。详见 [workflow-validation.json](../qa/evidence/TASK-017/workflow-validation.json)。

本次为未提交工作树验证，不能把基线SHA当作017实现SHA。[working-tree-binding.json](../qa/evidence/TASK-017/working-tree-binding.json)与源码快照保存本轮受测实现；提交后应追加真实提交绑定。

## 已修正与限制

首轮编译因局部Padding遮蔽UUserWidget成员失败，已改名。首轮48项功能通过后，截图发现间隔数字对比度不足、返回按钮折行，以及底层HUD干扰；修正SpinBox前景色、按钮不折行、全屏遮罩及菜单时隐藏HUD，并让列表滚动到选中节点。最终构建、第二次完整48项PIE及实键均在这些修正之后。

仅覆盖016已接入的非Shipping伙伴夹具；生产危险识别、完整世界恢复、主菜单、自动切图、任意实体重建、打包与第二台机器仍NOT_RUN。真实模型推理逻辑未变，本单未重新运行真实模型回档测试，保留016已有证据。不关闭R22/R23，不修改地图/设计原件，不解锁既有资产。

Issue查询gh认证401、匿名API限流403；独立评审和正式prototype审批未完成，流程仍Blocked。本次用户授权拆解/实现/验证，并随后明确授权提交推送017任务分支；README、任务单、项目状态与交接已同步当前成果。后续优先按M2缺口安排最小采集→入库→交互建造→存读档集成，正式建造配方与R项仍需设计决定。
