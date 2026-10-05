# TASK-069 默认地图探索文字字号

2026-10-05，UE 5.8.2，现有Hearthward.Map064.ExplorationBoundaries定向Native测试读取实际DescribeLayout的font、tracking、fontRole、bind与实际进度条几何，使用同一Slate FontMeasure测量完整探索文本。

实际绑定后Map探索字号17，正文最小24断言真实失败；同项其余边界通过。准确生产修复只将Resources/UI/interface.json中map.element.007的font从17改为24，原[94,194,300,30]矩形与barY223保持。未修改125/150专用重排。Inventory分类的原模板非空，Element已将实际字号夹到24，无需修改；见remaining-bound-font-readonly-review.md。

复测1/1成功、0错误、1既有EnhancedInput夹具初始化警告；实际font24、tracking120、全文145×28，文字bottom222、进度条Y223，无交叠。见[Native报告](map-exploration-font-native-green.json)及[实际渲染截图](map-exploration-font-green.png)。此前混合报告的Map RED与Loading GREEN分别保留。

这是默认字号下实际Widget边界与渲染验证。正常序章OS地图、全部页面/分辨率/Owner与Shipping未由本项覆盖，既有125/150结果保持各自原证据。
