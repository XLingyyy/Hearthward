# TASK-068｜中性拒绝提示候选实测与撤回

2026-10-04，Vulkan/Qwen3.5-4B既有固定参数及完整目录，固定10项诊断子集。原始Saved/Task068/vulkan-neutral-refuse-diagnostic-10及QA同名launch/results/stop完整保留，实际编辑器已关闭。3328输入预算不变，最大实际输入3208 token，全部full_relevant、generation_calls1、dropped[]。

新增一个独立负数拒绝例后，raw7/10、明确任务端到端6/7、执行5/6、守卫raw1/3、额外物品0。U01真实负数refuse正确；C08 raw craft/arrows/3/bag却漏掉wood3消耗预算，limits[]，其npc_line仍复述预算，UE允许candidate。相对两正例候选的C08正向结果出现回归，总理解数未改善。A01仍臆造数量1，U03仍把33截为32；现有原话数量守卫保持两者不可执行。

C09在当前生产camp取料修复后，raw/candidate/实际执行完整通过：camp wood20→18、rope0→1；Brother原wood12保持12。来源修复的模型真实入口验证已GREEN。

root仅撤回第三拒绝例，保留两个完整JSON正例、完整能力目录、Schema与数量保真守卫。撤回后尚需与后续生产增量一起构建，不借用本轮DLL结果声称撤回候选已运行。限制漏投影将先核对原话独立守卫与真实业务RED，避免提示台词掩盖任务卡缺约束。完整60表达及其他后端门槛未通过。
