# 制作查找与会话材料目标

事实来源：配方仍是 `Resources/Data/gameplay.json` 的58条原记录；筛选可制作使用 `UHearthwardBuildingComponent::CraftingStatus`；制造使用原 `Craft`/`Workshop`。本任务不新增制造真值。

材料投影只读：Needed=每批需求×目标批数，Available=背包Available＋（在营地内时的共享仓储Available），Missing=max(0,Needed−Available)。原预留物资不可用。离营时共享仓储仅作参考；材料齐备仍须满足实际设施、等级、状态和输出容量。

目标是当前会话的一项RecipeId/批数/epoch。明确点击追踪可替换，取消只清空材料目标。浏览另一个配方不替换目标；修改正在追踪配方的批数会更新目标。跨页面保留；换时间线清空；未增加持久化字段。

搜索和分类仅改变列表；选择的RecipeId可继续查看详情，空结果也有清空与取消追踪入口。没有压缩库存格子、自动排序或自动委托。
