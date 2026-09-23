# TASK-027 人物资产与基础动作

Owner：XLingyyy。授权来源：2026-09-23用户要求同步main、接入已有角色并制作行走、冲刺、跳跃、攻击、挖掘动作，随后明确编号027并要求继续。

基线：`414dc70481746875c8bd2b36d8b6140b475fcace`。工作分支：`codex/TASK-027-character-animation`。用户已追加授权提交、推送TASK-027内容到任务分支；未授权合并。

## 实施范围

- 使用已入库的 `/Game/Characters/Hero/Tripo/SK_Hero_Tripo`、现有材质和骨架，替换玩家灰盒。原始Tripo包只读。
- 在 `/Game/Characters/Hero/Animation` 生成待机、行走、冲刺、起跳、下落、落地、挥击、挖掘八个独立动画。
- 按用户追加要求，待机、行走、冲刺采用 UE 安装包自带 ThirdPersonIdle / ThirdPersonWalk / ThirdPersonRun 重定向；重点检查右手穿身和站立脚掌竖起。跳跃、下落、落地、挥击、挖掘保留本地关键帧动作。
- 角色移动速度驱动步态与播放速度，空中状态来自CharacterMovement，资源动作来自正在运行的资源交互，攻击来自实际近战成功事件。
- 自然地图浏览模式支持Shift冲刺和左键动作预览，不新增伤害、采矿结算、工具或敌人。

## 验收

1. UE5.8.2 Editor构建；角色网格、材质、骨架和八个动画资产加载。
2. PIE验证实际关节运动、行走/冲刺速度与动作状态、起跳/下落/落地、攻击结束恢复、五秒资源动作及移动中断、暂停、死亡。
3. 保存渲染录像并检查朝向、比例、关节形变与动作切换；另外验证真实输入。
4. 更新README、单任务交接和来源记录。未运行的检查明确标记。

## 技术依据

采用UE原生动画节点与速度驱动混合，参见Epic的[移动动画混合](https://dev.epicgames.com/documentation/unreal-engine/locomotion-based-blending-in-unreal-engine)和[状态转换](https://dev.epicgames.com/documentation/unreal-engine/transition-rules-in-unreal-engine)。当前项目角色与输入主要由C++定义，本任务使用原生AnimInstance/Proxy组合节点，保留独立可编辑的AnimSequence资产。

具体写入边界、权限和任务状态见同目录TASK-027.json。Issue与独立审查人尚未分配，不据此宣称正式验收。
