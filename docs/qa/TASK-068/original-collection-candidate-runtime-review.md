# Original 采集与材料规则：真实候选缺口修复

2026-10-04；root 独占引擎，UEClient 公开 API；受测为 67fb0784ca8c6d488173e587e7f95c4be0d9092a 上的未提交集成补丁，任务仍 Active。

完整 Vulkan60 的真实报告先发现三处缺口：A04“拿二份木材过来”被默认为新采 S1；A08 排除地点丢失仍可成卡；C39 禁止消耗石材被替换为禁止采集石材。三个卡均未确认，未发生库存变化。Native collection-rule068-red 的 7 项中 4 项通过、3 项各一断言失败，0 警告。最小生产修复在 Validate 中保留 Original 语义和排除地点前置，rule_proposal 复用既有 No/Once 原文材料一致性；新增来源/地点失败仅进入澄清，不改模型 raw、长期规则或冻数据。

独立审阅又发现兼容问题。真实 collection-verb068-red 的 OriginalCollectionVerbCompatibility 共 5 个错误：误拒“帮忙采两份木材并送入仓库”及“给我采两份木材送入仓库”，误接“请勿采两份木材”、搬运已采好的货物，以及遗漏“别往那里走”的限制。保留实际报告 original-collection-verb-native-red.json。修复仅两条正则行，补充已有明确请求形式与勿/往，采集动词统一要求数量/物品紧随，阻止紧邻动词的过去表述被吞入请求前缀。

Editor Development 构建通过；collection-compat068-green-actualskin055 实际 9/9 PASS、0 错误、0 警告，含 8 项 Original 与正确 HeldAxe 单项。original-collection-verb-native-green.json 保留全部测试名字和断言记录。既有空 Original、manual canonical、正常新采与搬运正控制通过。完整 skin exporter 的单项工程信用单独登记在 TASK-055。

这些结果证明候选边界及口语兼容回归，不证明模型理解门槛通过。最近完整 Vulkan60 仍为修复前 raw36/60、歧义 raw1/20、明确端到端38/40、实际执行30/30；修复后真实 Vulkan 三条模型路线已完成：A04/A08 两条 UE 澄清、C39 UE 拒绝，均 candidate=false、pipeline_pass=true、no_unconfirmed_world_effect=true、unearned_items={}；各 generation1/full_relevant、input3097–3102。模型 raw 三条仍错，明确规则 C39 端到端也失败；没有把阻止错卡登记为模型理解或规则生成成功。独立结果在 vulkan-collection-rule-three-cases-results.json，实际 stop_editor ok=true。2026-10-05 当前守卫版本 CPU 完整60+20已完成，raw34/60、歧义raw1/20、明确端到端36/40、执行28/30、确定性20/20、额外物品0，理解门槛失败；详见cpu-current-candidate-guards-full-60-runtime-review.md。没有调整门槛、模型参数、生成输出或冻结60/20数据。
