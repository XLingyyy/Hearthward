# 救援靠近批准记录

2026-10-08，Owner在当前会话明确选择“采用自动靠近后施救（推荐）”。

- 保留两米内的交互入口。
- 开始后自动靠近到约0.8米；进入施救阶段后才开始原有五秒计时。
- 靠近受阻、玩家主动移动、受伤、目标失效或时间线切换时取消。
- 保持原有倒地期限、到期优先和恢复10%最大生命规则，不修改保存结构。
- 实现复用UE CharacterMovement实际碰撞移动；不得以瞬移或纯视觉位移冒充靠近。

引擎接口参考：[UE 5.8 CharacterMovement](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UCharacterMovementComponent)。

实现与实机验证尚未完成，本记录不代表验收通过。
