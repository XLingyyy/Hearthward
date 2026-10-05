# 统一验收口径

这些门槛继承Owner已批准DSGN-R23及各领域设计，新增任务不能悄悄放宽。设计包核查已完成；Owner已批准施工。工程结果按当前任务报告登记，真人与独立机器结果仍待实际执行。每单记录源码完整SHA、引擎／资产版本、命令、场景和原始结果；已有PASS保留其原SHA。

## 分层证据

设计一致性→相关原生测试→正常输入的正式地图PIE／Standalone→Owner体验→所需真人样本→Shipping／独立机器。只做所改功能的最小有意义验证；共享接口或保存变化再扩大消费者回归。Debug夹具可定位问题，不能证明正常新游戏完成内容。假模型、固定回复、给材料、直接设任务完成、杀敌控制台不进入玩家验收。

## 战斗和体验

- 玩家1v3与弟弟1v3各阶段I／II，每阶段5名真人×3次＝15次，至少12次成功。标准普通敌同场活动。玩家测试弟弟不攻击／不救援，给常规食物1和药1；弟弟测试玩家不攻击／不吃药代救。不能靠虚拟重复样本代替真人。
- 065：5名初次玩家，无作者提示，从夜袭后首次营地控制开始；至少4人完成救援1人、工作台I、营地S2及保存退出继续，完成者有效实玩中位数30—60分钟。记录迷路、重试和停顿；只扣加载、真实休息和暂停。夜袭单独验收。
- 073：3名新玩家，全员无阻塞到永久夺回且实际使用第二营地，有效实玩通关中位数8—12小时；15支线由全部样本合计覆盖。3:3:4是探索／经营／战斗的互斥分类观察目标，不强行凑配额。
- 人类体验未完成时，程序任务可标“实现＋定向验证完成”，整批体验门槛仍未完成；最终Acceptance由Owner决定。Owner和Reviewer同一人，不登记虚构独立审查。

## 伙伴与本地模型

068每个实际后端CPU／Vulkan各60表达＝40明确、10歧义、10越权／不支持。原始理解≥90%，澄清／拒绝≥90%，明确任务端到端≥90%；另外至少30个可执行行为≥95%正确，至少20边界响应正确，复制物品及旧epoch／旧约束越界结算为0。理解、拒绝、执行、角色表达单独报告，不用安全拒绝掩盖理解错误。

保留现有单次4B推理、3328输入／256输出预算、单并发及锁定版本；默认16 GPU层，任何覆盖明确记录，不自动更换模型或扩大上下文。角色台词必须合于兄弟身份、证据和当前情境；动态回复保持文本。

## 联合性能与平台

目标机Windows11x64、i7-13650HX、RTX4060 Laptop8GB、16GB内存、SSD，通电并记录驱动／功耗／温度。目标配置1080p、100%渲染、项目极高sg3、DX12SM6／Nanite／VSM／原生TAA；无动态分辨率、帧生成、垂直同步，帧率不限。它是目标配置，最低配置由072真实测试得出。

072分别测营地昼夜、序章、三人战斗、路线流送、经营面板，每场与真实CPU和Vulkan模型请求同时运行：p99帧时≤16.67ms、1%Low≥60FPS、>50ms帧占比≤0.1%。加载另计；固定视角、单后端或纯Editor采样不足以替代。记录UE／模型进程内存、显存、页文件；无OOM／进程崩溃，可用RAM<1GB登记容量风险。

暖启动完整UE校验后回复p95：Vulkan≤10秒、CPU≤30秒；冷启动模型ready：Vulkan≤60秒、CPU≤90秒；等待反馈≤0.2秒。含排队、上下文、HTTP及UI，不把promptMs当TTFT；当前非流式TTFT记NOT_RUN。业务超时120秒保持原规则，性能目标不自动修改它。

第二台独立机器运行Shipping、不安装UE／Python、不配置API密钥；验证干净安装、模型随包、存档／升级、断网、非管理员路径、中文／空格路径。仅测试实际支持的Windows键鼠；不新增手柄承诺。

性能采样参考[Epic Timing Insights](https://dev.epicgames.com/documentation/unreal-engine/timing-insights-in-unreal-engine)与[Memory Insights](https://dev.epicgames.com/documentation/en-us/unreal-engine/memory-insights-in-unreal-engine)，默认复用已有运行入口，遇到可复现瓶颈才添加定向跟踪。打包参考[Epic Build Operations](https://dev.epicgames.com/documentation/en-us/unreal-engine/build-operations-cooking-packaging-deploying-and-running-projects-in-unreal-engine)，继续由UEClient公开API及现有package_demo入口编排；不为规划安装新测试框架。
