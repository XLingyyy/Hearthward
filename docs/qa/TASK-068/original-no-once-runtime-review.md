# 原话禁耗与本次材料例外回归

2026-10-04，Root 集成最小 Contract 守卫并通过 Editor Development 构建。

- 初始真实 RED：`Saved/Task053/revision071-blade055-green-no-once068-red/index.json`。模型遗漏原话“不要消耗木材”、以泛化“这次制作”授权 once:wood 各一处错误。
- 首轮修复后：`Saved/Task053/no-once068-green-suffix-red-diagnosis071/index.json` 20项中19项成功，0警告。上述原始断言及全部既有 NPC 合同／预算／数量检查通过；Once 测试新增的“木材粉／木材的替代物”两条负断言实际 RED。实际部分取料存档与命令版本诊断独立 Success，0错误／警告。
- 材料词项尾部收紧后：`Saved/Task053/no-once068-complete-material-boundary-green/index.json` 四项 Original 回归全部 Success，0错误／警告，含原话数量、预算、禁耗、材料明确本次授权。

守卫仅作用 craft／repair：保留原话明确 no 材料；每个 once 材料必须有句／字段边界起始的明确本次允许使用同一材料。支持已有别名、完整登记名称／ID及明确材料列表，保持 GoalText 多约束。否定授权、错材料、未知复合名称或尾部不授予例外；复杂未解析材料短语拒绝，未实现通用中文句法理解。普通仓库来源授权保持独立，长期规则记录不撤销，模型 raw 不改写。

这是公开 Contract 校验的定向证据。真实模型分类／完整60表达、正常UI／实际HTTP链与角色自然度各自登记，不由该守卫的通过替代。
