# TASK-069 地图125%／150%最小真实复现准备

2026-10-04。根已批准补充真实Widget测试；本阶段仅测试与QA，没有UE／build／Git或生产改动。原100% ExplorationBoundaries用例全文保留。

## 已确认的代码与数据事实

- 当前地图说明来自interface.json的map.element.008，完整文本为两条显式源行，位置239，宽290，高120。ComposeMap没有随Comfort.TextScale调整此说明和后续图例。
- 六条图例文本位于Y383／425／467／509／551／593，步距42，高30。真实地图框、footer及说明、图例ID通过DescribeLayout公开出口读取。当前Widget头文件没有GetElements公开API，Elements私有；本测试不新增接口、不访问private。
- Element把这些文本统一提升到24design pixels后乘TextScale。125%／150%分别为30／36；NativePaint在FontRole空且Font>=30时选择主题display字体NotoSerifCJKsc，而100%使用body字体LXGWWenKai。Slate字号取Round(Font*.75)，并消费主题tracking=120。新测试逐项读取实际theme定义、角色、tracking，沿这条现有分支构造真实FSlateFontInfo。
- main_05完整目标含三条显式源行；当前ComposeMap已用真实字体测量目标并贴map.canvas底部，100%已通过根Native及两张渲染。此既有用例不修改。
- 剩余驻军提示固定1150,790，框350×32；它位于FirstMapElement至MapClipped设置循环内，受到真实map.canvas底855裁切。ShowRemaining使用Cleared*100>Total*95。新fixture直接State.Initialize(true)读取现有80名base原始敌人，只保留其中一个的原健康值，其余base Health=0，未合成敌人；实际ShowRemaining须先通过。随后用现有Gameplay.Track跟踪可用main_05。

## 源字体辅助估计与待真实验证边界

使用现有Pillow只读载入仓内实际display字体，以72／96 DPI两种假设补充估计；没有把Pillow结果算作UE RED。125%说明为3／5行，对应生产1.6倍行距144／240；150%为4／5行，对应230.4／288。各结果超过固定120，支持运行本次实际复现的必要性。提示在96 DPI估计为两行，第二行顶部838／847.6，存在canvas底部裁切风险。实际UE DPI、字体fallback与舍入由根运行的FSlateFontMeasure结果确定。

新用例不会仅凭整个objective的910宽空白矩形与提示相交判失败：它使用生产换行算法和实际FSlateFontMeasure测量逐行宽／字体行高，保留1.6倍垂直行距，逐行检测重叠。六条图例也检查实测字体行框是否留在实际行矩形内、相邻文字行是否相交。FontMeasure返回字体行度量，此记录不声称逐字像素墨迹包围盒。

## 根执行方式与产物

- 独立测试：`Hearthward.UI069.MapScaledGuidanceKeepsCompleteText`。只运行125／150两个设置状态，不扩为全篇矩阵。
- 常规Native可用NullRHI，检查正常CreateWidget／OpenPage／Refresh和真实theme字形度量。
- 根以现有`-Map064Render`显式启用非NullRHI GPU同一用例；本次capture沿既有FWidgetRenderer→1696×954 RGBA8→PNG方式，单独目录保留原100%截图：
  - `Saved/Task069/native-map-scaled/map-long-objective-remaining-125.png`
  - `Saved/Task069/native-map-scaled/map-long-objective-remaining-150.png`
  - 对应`-layout.json`及`-method.json`记录准确textScale／方法。
- 每scale记录说明行数、所需行距高度、真实说明框高／firstLegendY、objective／remaining行数及提示最后实测行底。断言失败仍继续截图，缺真实Widget／theme／内容等前置才提前结束。
- 内存仅改Settings.Comfort.TextScale并调用Screen.Refresh；未Persist，不修改用户设置文件。

方法为native widget offscreen及明确C++ campaign fixture，不能给PIE正常输入、Owner视觉、分辨率全矩阵或IME验收信用。根构建／Native／GPU结果尚待执行，当前无实际RED或GREEN声明。

增量`map-text-scale-red.patch`只增加现MapGuidanceTests.cpp中的独立用例，原100%正文与所有include保持原样。没有SourceScreenContent／interface／Character变更，没有测试setter、新通用helper、依赖或反射API。

## 官方参考

[官方FSlateFontMeasure与FindLastWholeCharacterIndexBeforeOffset](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/SlateCore/FSlateFontMeasure/FindLastWholeCharacterIndexBefor-)说明公共逐行字符边界测量API。实现以本机UE5.8头文件及现NativePaint调用为准；沿用仓内已有字体测量和offscreen捕获路径。
