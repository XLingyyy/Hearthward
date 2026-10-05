# TASK-069 实际地图字号与输入验证

2026-10-04，根未提交源码；branch codex/TASK-053-traversal，base 67fb0784ca8c6d488173e587e7f95c4be0d9092a，UE5.8.2 Editor Development。

真实 RED：map069-scaled-text-red 的原生实际 Widget 在125%/150%字号产生27个断言错误。描述高度、图例间距、剩余驻军提示及侧栏完整内容不足。原失败index保留于 Saved/Task053/map069-scaled-text-red；原同名PNG已被后续真实绿图替换，不把当前文件作为原失败图。

生产修复复用现有测量、TextScroll与裁切，侧栏全文可滚动到末尾，操作按钮/命中范围保持可达；地图画布滚轮继续缩放，侧栏滚轮只移动文案。保留全部文案。

真实 GREEN：restart071-real-movement-write-guard068-mapwheel069 中 Hearthward.UI069.MapScaledGuidanceKeepsCompleteText Success，0错误、1警告。验证125%/150%字量测高、不重叠、End/Home全文可达及坐标变换后真实FPointerEvent命中：侧栏滚轮不改变地图画布，画布滚轮不改变侧栏偏移。该警告为夹具LocalPlayer无有效PlayerInput，原始报告保留，不吞警告。既有100%独立ExplorationBoundaries在 map069-readable-green-restart071-mature-write 通过，亦1条同类警告。

实际offscreen PNG保存在 map-readable-green-125-150/。根已核看125%/150%初始和End状态：全文侧栏保持顺序，图例和营地操作可达；右侧地图、剩余驻军提示、三行目标与页脚无重叠。人工桌面操作及全九页仍未验收；本项是生产Widget/Slate定向工程证据。
