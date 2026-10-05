# TASK-069 加载结束恢复最新页面输入模式：单项 Native 提案

补丁：loading-page-input-test-proposal.patch，仅既有 ExperienceTests.cpp 添加 LoadingSubsystem include 与71行单项测试。自动化名：Hearthward.UI069.LoadingRestoresLatestPageInputMode。

复用既有真实 standalone Instance、UGameViewportClient/SViewport、LocalPlayer/PC、角色组件、Screen fixture。先确认有效viewport和实际Title UIOnly的IgnoreInput=true，随后通过公共 BeginLoading、真实 OpenPage 和 FinishSession(false)立即结束遮罩验证：

- Title→加载→HUD：加载期始终禁输入；结束恢复最新GameOnly=false。
- 同一遮罩内Title→HUD→Pause：两次页面变化仍禁输入；结束保留最后UIOnly=true。
- GameOnly→加载→结束，期间不切页：恢复原false，不改变travel语义。

不直接调用 InputModeChanged、不读取私有快照、不手改IgnoreInput，不新增框架或helper。Root已确认第七次实际OS RED为begin saved1、Hide before0/saved1，结束后Escape/Tab仍留HUD且时钟继续；此提案用于保护对应状态所有权及接入点。

尚未编译/执行；Root负责登记、Source集成、Native与OS验证。本代理未改Root或生产，未UE/build/Git。


## Root实际运行记录与fixture修订

首跑发生fixture CRASH，未验证目标输入断言：初稿使用ACharacter并注册Gameplay等组件，遗漏HUD实际依赖的Traversal，ComposeHUD在FindVault处解引用空组件。证据：Root docs/qa/TASK-069/loading-native-fixture-missing-traversal.json。Root仅在该测试注册真实UHearthwardTraversalComponent，不给生产增加fallback；本提案.patch保留初稿，最终执行版本以Root修订后的ExperienceTests.cpp为准。

Root补齐fixture后回传本项1/1 PASS。混合报告docs/qa/TASK-069/loading-green-map-exploration-font-red-native.json内，Loading为Success；另一个Map探索Font17用例为唯一RED，不能据Loading单项通过宣称该整份run全绿。本代理未运行UE，也未再发送Sourcepatch。
