# npc_line-first：单变量实际诊断与撤回

2026-10-05接续，Root串行公开UEClient，先完成保留两正例/原Schema的CPU60。新候选仅把Schema中同一npc_line声明从末尾移到intent前；8字段、24分支、值域、required、Parser、System及冻结模型参数均不动。受测为未提交补丁；候选构建实际通过。

运行固定已有10条：C01/C08/C09/C18/C19/C27/C40/A01/U01/U03。完整JSON首键十条均为npc_line，变量确实生效；raw5/10、明确E2E4/7、执行3/6、歧义/越权raw1/3、额外物品0。原两正例固定10基线raw7/10、明确7/7、执行6/6。A01歧义分支改善，同时C01/C08/C18明确任务退化；未满足保留条件。

实际error=null，CLI exit1为业务阈值失败，stop_editor ok=true。独立完整报告、launch/stop和Saved目录使用 vulkan-npc-line-first-diagnostic-10 前缀。不把部分歧义改善外推完整60、CPU或Owner口吻。

已逐字节撤回语句移动，恢复原intent-first property顺序，保留全部之前GREEN的语义守卫。没有更改System示例来追调失败，没有第二次同候选运行。Schema字段次序可能影响生成的文献与源码假设在本地未取得可用整体收益；System示例顺序不一致也只记录为未隔离假设，不写作已确认原因。该实验结束。
