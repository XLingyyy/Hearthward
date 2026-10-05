# 原话材料预算与多材料恢复回归

2026-10-04，Root 串行实际 EditorDevelopment 构建与 Native 测试。

- 修复前 `Saved/Task053/budget068-recovery068-red/index.json`：OriginalMaterialBudget 两处错误（遗漏/放宽 max）；WarehouseMaterialsPresentation 五处错误，多材料正常预留解除后不能完成原委托。
- 修复后 `Saved/Task053/budget-recovery-green055-blade-red/index.json`：OriginalMaterialBudget、OriginalQuantityBoundary、WarehouseMaterialsPresentation 全部 Success。制作测试保留一条已有 EnhancedInput 夹具警告。该合并运行有另一个石斧预期 RED，因此总进程退出 255，不能把总运行称为全通过。
- Contract 仅核对原话完整材料预算是否保留，包含无标点、材料在上限前、多材料、中文一百/十万的兼容回归；不改模型 raw。已确认约定仍能供给原话未提及的上限。
- TakeMaterials 对当前操作精确 Command/Payload 领取回执跳过重复领料与重复承重检查；未领材料使用 Available 检查实际可用库存；保持仓库来源完整成本授权。
- 多材料的测试使用真实 firepot 成本与正常库存 Reserve/Release、Stage/Confirm/Resume/Tick。静态预留经预检不再发生半领取；保存 r>1 且真实半领取的跨 LoadPoint 路径在 071 独立真实 RED→GREEN 回归完成，详见 ../TASK-071/partial-command-revision-runtime-review.md；本条材料恢复运行本身不包含该覆盖。
- 最新定向真实 C08 已保留 max:wood:3 并执行通过，见下文。预算校验通过不等于模型理解门槛通过；完整 60 表达与两后端验收待执行。

## 实际模型复核

唯一证据目录 `Saved/Task068/vulkan-original-budget-guard-diagnostic-10`，进程正常关闭。冻结10项诊断：raw 7/10、明确任务端到端7/7、实际执行6/6、未授权物品0。C08实际保留max:wood:3并完成，C09实际用共享仓库材料完成；A01缺数量、U01负数、U03超量的raw仍错误，现有原话一致性guard均未创建可执行候选。最大真实input_tokens=3146，各项generation_calls=1、full_relevant且未删投影字段。两项JSON示例保留，失败的第三拒绝示例已回退。此为定向复核，CLI退出1保留完整批次门槛；不登记60项、双后端或角色语气通过。
