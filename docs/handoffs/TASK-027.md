# TASK-027 人物动作

用户于2026-09-23授权实施，并追加授权提交推送027内容到任务分支。基线 origin/main 414dc70。现有工作树改动已保存在 .agent-local/pre-task027-sync 与 pre-task027-tracked-local-changes stash。

## 任务分支交付

- 分支 `codex/TASK-027-character-animation`；已基于同步后的远端main完成实现；按用户授权提交推送本任务内容，未合并main。
- 玩家改用现有Tripo骨骼网格、纹理材质与骨架，按180 cm显示。八个独立动画位于 `Content/Characters/Hero/Animation`：Idle、Walk、Sprint、JumpStart、Fall、Land、Attack、Dig。
- 按用户追加反馈，Idle / Walk / Sprint 已改用UE安装包的 ThirdPersonIdle / ThirdPersonWalk / ThirdPersonRun。`retarget_template.py` 通过原生 IK Retargeter 烘焙到原角色骨架，`author_motion.py` 仅负责其余五段，避免再覆盖模板动作。
- 原生AnimInstance使用速度混合、同步步态与动作过渡；跳跃遵循真实运动状态，近战动作跟随接受的攻击事件，资源动作跟随真实交互计时与中断。连续有效攻击可重新触发动作。
- 源骨架根骨带约100倍缩放且朝向与模板相差90度；重定向使用独立归一化代理网格，导出后恢复原骨架单位/朝向。源Tripo网格与Skeleton未修改。原始模板与依赖位于 `Content/Mannequin`，重定向配置位于 `Content/Characters/Hero/Animation/Retarget`。
- 自然地图：WASD移动、左Shift冲刺、空格跳跃、鼠标左键挥击预览。已有开发采集夹具旁按E执行五秒资源动作。

## 验证与证据

UE5.8.2 Development Editor构建通过。PIE检查、按键观察、关键帧及录像见 [TASK-027证据](../qa/evidence/TASK-027/README.md)。功能检查覆盖网格与八段动画加载、真实关节变化、移动速度、跳跃各阶段、资源动作暂停/中断/单次结算、近战伤害与动作恢复、自然浏览模式冲刺输入和死亡清理。

仓库工具自测31项通过；仓库文档校验仍有5项流程元数据错误：TASK-026与TASK-027各缺Issue和reviewer，TASK-026另有未登记的required_tests引用。指定基线路径自检还提示TASK-027在main基线中尚无批准任务快照。未据此宣称正式审查或合并验收通过。

## 明确边界

移动部分来自UE模板，其他动作仍为基础关键帧版本；没有脚部IK、工具握持、布料模拟或正式采矿系统。Dig接在现有资源交互上，开发夹具产出仍是木材。没有修改地图、生成配置、源Tripo资产或存档格式。关节/衣物穿插与坡地脚部贴合仍需要后续美术精修。

来源记录见 [PROVENANCE](../assets/TASK-027/PROVENANCE.md)。八个动画、引入的模板依赖和重定向资产已取得XLingyyy名下LFS锁，交接时保留；记录见证据目录。同步前的本地修改、树木文件和其他未跟踪文件均已保留；恢复stash前先审查其与当前main的差异，避免直接覆盖同步后的文件。
