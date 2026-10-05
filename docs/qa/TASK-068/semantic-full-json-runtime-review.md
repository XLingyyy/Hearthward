# TASK-068 完整JSON候选实际结果

2026-10-04根：codex/TASK-053-traversal，base 67fb0784ca8c6d488173e587e7f95c4be0d9092a，当前未提交源码通过Editor Development。按固定原10表达 C01/C08/C09/C18/C19/C27/C40/A01/U01/U03，以公开UE生命周期实际Vulkan模型请求。保持4B Q4_K_M、16 GPU layers、4线程、3328输入/256输出、单并发、temperature0、完整24能力/158ID/507关系。

原始结果/launch/stop已独立保存：vulkan-full-json-diagnostic-10-results.json，对应Saved/Task068/vulkan-full-json-diagnostic-10。真实输入3016—3146，全部full_relevant，无dropped，每表达generation1；真实后台缓存开启不改冻结语句或expected。

原始模型正确7/10，7条clear全部正确；明确任务端到端6/7，执行5/6，3条guard原始正确0/3，但UE全部安全拒绝执行卡（candidate=false）。额外物品0，完整门仍未通过。诊断子集不替代60+20或暖p95样本。

两个旧语义错误已修复：C09制作rope1/camp不再自动捏造max:rope:1，C18玩家包→经弟弟→仓库选store/player_bag。C01/C08/C19/C27/C40保留正确。A01仍捏造quantity1，U01负3仍改成3，U03超限33仍截32：新UE原话数量校验阻断U03候选，不计raw通过。

真实C09执行业务RED：候选明确camp，确认成功却弟弟wood12→10、campwood20→20、camp rope0→1。原完整before/after记录保留。正在追既有WorkshopTick TakeMaterials仅按背包缺量补货导致来源忽略；修复、Native及后续真实模型执行结果另记。

本组基于新完整JSON System、Schema B、第一版数量守卫。发现“补充：4／四份。”合法澄清回归后另增目标测试并修复；该版本尚未验证，不能用本组替代后续变更后的全部通过。
