# TASK-071 重启存档夹具：同步 World Tick 的移动调度调查

本轮只读源码与既有 Native 报告，未运行 UE、构建或 Git，未写 root 源码。以下补丁是夹具草稿，实际落地与业务存档验收由根运行。

## 已有实际失败

根报告 `Saved/Task053/map069-readable-green-restart071-mature-write/index.json` 中 `Hearthward.Save.ActualLoadPointInvalidatesOldCommandTicket` 只有 1 项 Error、0 Warning。失败停在 `SaveTests.cpp:149` 的贴地前置：Hero `mode=3 z=99.694 active=1 tick=1 controller=1`；Brother `mode=1 z=82.150 active=1 tick=1 controller=1`。存档业务断言尚未执行，不能计作 TASK-071 业务 RED。

正常 Standalone GameInstance、LocalPlayer、InitializeActorsForPlay、NotifyBeginPlay、已注册 QueryOnly WorldStatic 地板均已存在。当前 80 次 `.025f` World Tick 都在同一个同步 `RunTest` 内完成。

## 本机 UE 5.8 源码确认

- `Runtime/Launch/Private/LaunchEngineLoop.cpp:6130–6131`：全局 `GFrameCounter` 每次引擎 Tick 递增一次。当前 RunTest 内调用 World Tick 不会交回引擎帧循环。
- `Runtime/Engine/Private/LevelTick.cpp:1502`：`UWorld::Tick` 内不递增 `GFrameCounter`；全文仅有该计数的统计日志引用。
- `Runtime/Engine/Private/TickTaskManager.cpp:622–625,1473–1487,2622–2631`：调度使用 `TickVisitedGFrameCounter == uint32(GFrameCounter)` 检查已访问的 TickFunction，首轮排队会写入此值；同一个帧号的后续 World Tick 跳过它。
- 同文件 `StartFrame:1072` 与 `EndFrame:1117,1652,2175` 清理调度队列、冷却和上下文，未重置 `TickVisitedGFrameCounter`。不能通过连续 World Tick 假定组件连续执行。
- `Runtime/Engine/Classes/GameFramework/CharacterMovementComponent.h:1300–1312`：`TickComponent` 是 public virtual。`PerformMovement` 在 `2283–2291` 的 protected 段，草稿不直接调用它。
- `Runtime/Engine/Private/Components/CharacterMovementComponent.cpp:1650–1799,6441–6458`：真实 TickComponent 仍检查有效数据、移动组件、物理模拟和本地控制条件，并在当前 Authority 角色中经 ControlledCharacterMove 进入 PerformMovement。主调用路径没有该帧号去重检查。`UActorComponent::TickComponent:1882–1903` 允许空 ThisTickFunction 参数，仍要求组件已注册。

首步重力位移 `0.5 * -980 cm/s² * (0.025 s)² = -0.30625 cm`，与 Hero 从 100 cm 变成 99.694 cm 精确匹配。源码机制和现有报告共同支持后续调度被同帧去重的判断；仍需要根运行补丁验证最终贴地。

## 项目调用链边界

`HearthwardCharacter.cpp:220–223` 的 Tick 调用 `PlayerSettings::ApplyTo`；该方法 (`Experience/HearthwardPlayerSettings.cpp:62–73`) 只修改摄像机 FOV 与后处理，不停止移动。

`Experience/HearthwardTraversalComponent.cpp:18–23` 把 Traversal 作为 Movement 的前置 Tick；`112–135` 在启用存活、涉水或攀越时管理模式。草稿保留首个完整 World Tick，由引擎正常执行组件前置关系。夹具包名 `Task071RestartFixture` 不匹配水域数据限定的 `L_HearthwardWilds`；静止角色没有 BeginVault 请求，Adventure 在贴地断言之后启用。后续只推进既有真实移动组件，未替换 Traversal 的业务状态。

Brother 的 Walking 与 82.150 cm 结果不能证明其移动执行了 80 帧。其胶囊半高 80 cm，UE 在进入 Walking 时会 FindFloor / AdjustFloorHeight / SetBaseFromFloor (`CharacterMovementComponent.cpp:1441–1452`)，典型脚底余量为 2.15 cm。确切发生于初始化还是首轮调度，当前报告没有逐步数据，本轮不作额外断言。

## 最小夹具草稿

`restart-fixture-movement-draft.patch` 只替换实际贴地循环：

1. 保留一次 `.025f` 正常 World Tick。
2. 余下最多 79 次对两个已注册角色的真实 `GetCharacterMovement()->TickComponent(.025f,LEVELTICK_All,nullptr)` 调用，保留原退出条件及贴地断言。
3. 继续原公共 EnableAdventure、地形查询与完整存档验证。

保留真实重力、胶囊扫掠、地板阻挡、控制器和落地模式转换。未直接写 MovementMode、Actor 坐标、WorldClock、GFrameCounter、HealthSafe 或身体状态；没有新增 setter、生产接口或依赖。

验证门槛：根按既有 phase=write 定向 Native 命令重跑。须先确认双胶囊自然落地，再评估首次出现的业务失败；需要分进程 read 原样保留。草稿不提供实际运行 PASS 信用，也不覆盖完整引擎帧/PIE 调度验收。

官方 API 与移动执行路径参考：[UCharacterMovementComponent::TickComponent](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UCharacterMovementComponent/TickComponent)，[Understanding Networked Movement](https://dev.epicgames.com/documentation/en-us/unreal-engine/understanding-networked-movement-in-the-character-movement-component-for-unreal-engine)。本机 UE 5.8 header 的实际访问级别优先。
