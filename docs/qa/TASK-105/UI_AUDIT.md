# M0 UI 与变更通知审查

2026-10-11，本地 TASK-105 分支静态审查。此记录描述调用关系，不是耗时或 FPS 测量。

- `HearthwardScreenWidget.cpp:117`：LoadTheme 在 Widget 首次建立时读取主题、导入外部 PNG，并生成日记高亮蒙版。同一 Widget 内已有按文件去重；没有证据支持直接删除图片初始化或增加第二套缓存。
- `HearthwardScreenWidget.cpp:369`：Refresh 重建当前页的 Elements，并调用对应 Compose 函数。
- `HearthwardScreenWidget.cpp:541`：NativeTick 每 0.2 秒刷新页面；地图标记与 HUD 的 Paint 另外按帧失效。不能把“每帧绘制”误写成“每帧重建所有页面”。
- Gameplay OnChanged 的生产订阅者为主角 RefreshHeldTool 和 PresentationComponent::GameplayChanged。ScreenWidget 没有直接订阅此事件；删除广播不会消除其 5Hz Refresh，却会影响装备和成功音效。
- `HearthwardGameplayComponent.cpp:333`：库存变化同步装备实例状态，记录已获得物品，然后广播。目录索引保留原行对象，Survival 测试临时修改药品属性仍可见；不缓存可变玩法状态。

后续测量需区分 LoadTheme 冷初始化、各页面 Refresh/Compose、Paint、Game/Render/GPU、流送及模型请求区间。采用 Epic Unreal Insights，固定地图、画质和候选身份。未取得同口径耗时前，保留刷新频率、通知和画质；本单不宣称 UI 或帧率提升。

官方参考：[Unreal Insights](https://dev.epicgames.com/documentation/unreal-engine/unreal-insights-in-unreal-engine?lang=en-US)。
