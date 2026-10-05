# TASK-069 地图大字号生产最小修复

2026-10-04。根已执行真实Native RED，原始报告`Saved/Task053/map069-scaled-text-red/index.json`：27业务错误、1 Warning。125%说明4行需192，高度仍120；150%说明5行需288；六图例框高度30容不下真实字体，且相邻文字行交叉；150%remaining提示两行实测底899.6，超过canvas855。已只读核看根真实150% PNG，可见说明／图例交叉、remaining尾字裁切、营地侧栏动作文字进入预览及地图营地标签进入guidance。

## 生产文件与实际处理

`map-readable-production.patch`相对根最新3个cpp局部增量：

- **HearthwardScreenContent.cpp**：将现有objective字体测量提为ComposeMap内局部TextHeight，用于目标、remaining及地点标签；字体、完整文案、tracking和1.6倍行距保持。目标保留既有map.canvas底部锚点；remaining扩至600宽，靠地图右侧并移到目标上方12间距，避免把目标抬进营地图标。进入guidance区域的地点标签移到自身图标上方，原文字保留；原位置已经在canvas纵向范围外的标签不因此移入画布。大字号新增侧栏的“滚轮阅读侧栏”操作提示。
- **HearthwardScreenWidget.cpp**：复用ApplyReadableLayout，在map分支只重排实际map.sidebar元素；按原顺序排列标题、探索、说明、六图例、营地名称／按钮、预览。图例图标与文字保持配对，文字框使用真实显示字体、tracking和换行高度；探索这种后绑定文本也按用户字号至少24*scale显示。保留原图像、动作、字体角色及组件。总高度超出侧栏时使用现有TextScroll／TextScrollMaximum／TextScrollClipped；viewport采用实际sidebar上下边界加16／减8，当前为136至851。
- 大字号侧栏滚轮更新TextScroll，画布滚轮仍更新MapZoom；已有PageUp／PageDown／Home／End机制直接用于侧栏。键盘选择侧栏按钮时沿既有自动滚动机制将完整按钮带入视口；地图header／canvas按钮导航不会滚动侧栏。点击只接受侧栏可见区域中的真实按钮，已裁出的动作不能从header／footer坐标触发。
- 大字号footer保留全部现有控制文案，用footer实际宽度排版并垂直居中；不降低字号。更新既有LayoutBounds记录实际重排／滚动位置，使DescribeLayout对应真实绘制与命中坐标。
- **HearthwardScreenPaint.cpp**：大字号侧栏沿现有clip逻辑裁到实际sidebar viewport；背景、header、footer和地图继续使用自身区域。侧栏多行按钮按完整文字块高度做垂直居中，保留全字，不让后续行进入预览。

无Header、公共setter、通用框架、依赖、资产、theme或持久化格式改动。100%地图继续沿原静态侧栏路径；原100%回归测试全文保留。现有地图比例、探索半径、雾、缩放、平移、标记和任务入口语义保持。

## 滚动可达性增量与根验证

另附`map-sidebar-scroll-validation.patch`，相对根已集成的MapGuidanceTests.cpp仅扩展新增UI069用例。每scale先保存原head截图；以公开NativeOnKeyDown投递End，断言真实camp action完整进入sidebar并且ActionAt命中travel，保存`-sidebar-end`截图；Home恢复原说明位置和map页面。沿已有ExperienceTests的FGeometry／FKeyEvent公共范式，无测试setter。原100%正文和断言保持。

根需构建并运行：

- 原`Hearthward.Map064.ExplorationBoundaries`100%回归。
- `Hearthward.UI069.MapScaledGuidanceKeepsCompleteText`125／150完整文本、字体框、相邻行、guidance及End／Home可达性。
- 非NullRHI加现有`-Map064Render`，保留每scalehead PNG／layout／method，并新增`map-long-objective-remaining-{125,150}-sidebar-end.png`及对应记录。

本地只做局部源码／diff和结构检查，未执行UE／build／Git，未给GREEN或正式视觉信用。脚本直接调用公开Widget输入回调，仍属于明确fixture的native widget offscreen验证；实际键鼠与PIE验收由根后续执行。

## Warning边界

本轮真实唯一Warning为EnhancedInputLocalPlayerSubsystem缺有效PlayerInput以载入user settings。实际Engine PlayerController.cpp:5369–5377的SetPlayer已经正常调用InitInputSystem；插件EnhancedInputSubsystems.cpp:73–83实际要求Cast<UEnhancedPlayerInput>，91–119在该条件失败时记录此Warning。尚未获得本次具体触发时刻／PlayerControllerChanged参数及创建／销毁路径的运行证据，所以本补丁不追加重复InitInput，不设置editor script开关、不过滤Warning或日志。根重新执行应继续如实保留该独立fixture Warning状态。
