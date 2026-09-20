# 第三轮验证范围

第二轮源码差异绑定：eeadd455b16c6f35934de0f53d34c6151c92ec7d28330886ed053f717f5e1b1b。此前已完成原生、真实模型、三轮状态/故障、120秒超时、CPU和物理UI，原记录在round2/。

第三轮修改仅为：

- HearthwardLocalAISubsystem.cpp的库存只读重分类、回忆只读重分类所依赖别名，以及SendInference的明确采集正例。
- npc-agent.policy.json增加偏爱→喜欢别名。
- local-ai.lock.json更新提示版本至task025-agent-v2-6。

未改能力Schema、解析器、任务卡确认、结算/去重、材料与耐久、导航、存档格式、请求取消/超时处理或UI实现。模型原始结果仍单独保留，确定性纠正仍标deterministic_fallback。

按影响范围重跑：编译、项目原生、原025真实模型回归、多轮交互、开发60、已知失败7、新留出60及CPU实际模型路径。以第三轮各manifest为当前代码证据；早期状态、故障、玩家领域及物理UI报告保留其实际旧版本绑定，只证明其当时且本次未改的路径，不写成第三轮整版本重测。

新留出编号heldout3-*。第一、第二轮留出均已暴露并归档，不继续作为独立评分。独立Reviewer、人物自然度人工审查、Shipping和第二机器仍未完成。
