# TASK-069 两项 bind 字号只读复核

2026-10-05；Root 集成树 `G:/GameFactory/Hearthward/.agent-local/task051`。本 Agent 仅读取当前源码、资源与既有截图，未改 Root 文件，未运行 UE 或 Git。两个 patch 均为候选，构建和 Native 结果为 NOT_RUN。

Inventory category 无实际字号缺口。`inventory.element.008` 虽在资源写 `font:16`，初始 `text:"全部"` 非空。`HearthwardScreenWidget.cpp:263` 在 `Resolve(category)` 前已把非空正文夹到 24 并乘 TextScale；Resolve 只换 E.Text，不换 E.Font。100% 实际字号为 24；125%/150%通用重排又在662行保证相同比例的最小值。把该资源16改24不会修复任何当前行为，候选保留该行。

Map exploration 的缺口属实。`map.element.007` 初始 text 为空，资源字号17，故100%实际 E.Font=17。125%/150%专用地图重排在604行把解析后的非空文字夹到24×scale，因此该缺口只出现在默认100%布局。候选只把这一资源字段17改24，无 Source 修复或全 bind clamp。

地图保留原文字 `[94,194,300,30]` 和进度条 `[94,223,280,4]`。只读参考：原生旧图 `Saved/Task064/native-widget-offscreen/map-initial-exploration.png` 的17字号标签较小；既有布局记录该条矩形。Pillow 以现有 LXGWWenKai 24像素读取最长合法“探索进度  100%”，未含 tracking 的字形宽173，字形 bbox `(0,3,173,26)`，该参考支持300宽度有余量。Pillow 字形框与 Slate 字体最大高度的定义不同，不能据此断言进度条无需挪动。

已有100% `Hearthward.Map064.ExplorationBoundaries` 只量描述与目标，不检验 exploration 的实际 E.Font；125%/150%的 `Hearthward.UI069.MapScaledGuidanceKeepsCompleteText` 也未断言这一行。旧 PASS 无法证明默认字号缺口已修复。

`map-exploration-native-existing-test-proposal.patch` 复用100%现成 Widget、布局、Typeface 与 FontMeasure。在现有测试的同一流程追加检查：根据公开 DescribeLayout 的 bind 找真实 exploration，按 parent/type 找真实侧栏进度条，使用实际 font/tracking测量完整文字，断言字号>=24、宽高容纳且字体框底不穿过进度条。未新增测试夹具、setter、API或固定 bar 坐标；该块依赖 Root 已加入 DescribeLayout 的实际 font/tracking/bind 诊断字段。补丁的字体构建只覆盖既有100%正文角色，Resource候选24对应现有 body Typeface。

建议 Root 先应用测试候选，用默认资源17得到针对真实字号的 RED；再应用仅一字段的资源候选复测。若 FontMeasure 的真实框底超过条Y223，再按测得结果局部下移条并确认与描述Y239之间留白。没有这项实际失败时，保留现几何。100%单项即可；125%/150%字体夹取结果保持30/36，若未出现其他变化不必重复全部九页。

测量依据参照 Epic 5.8 的 [FSlateFontMeasure::Measure](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/SlateCore/FSlateFontMeasure/Measure)：高度按所选字体及字号的最大字符高度返回。该 API 与当前 Paint 及 Map reflow 使用的测量服务一致。

候选产物为同目录的资源单字段 patch、现有 Native 局部 patch、事实 JSON。资源候选已解析并逐元素比较，只改变 Map exploration 一行的 font 字段，Inventory 保持。
