# TASK-068｜验收伙伴角色表达和长程协作可靠性

2026-10-03，设计Approved／施工已授权，实施按依赖推进。原稿070，阶段C，P1；Owner／Reviewer：XLingyyy，无Issue。

依赖：TASK-050、TASK-060、TASK-061、TASK-062、TASK-063、TASK-065、TASK-066、TASK-067。基线、权限和共同规则见[BASELINE](BASELINE.md)、[DECISIONS](DECISIONS.md)、[INTERFACES](INTERFACES.md)、[ACCEPTANCE](ACCEPTANCE.md)。

## 当前基础与目标

050活动、029—040认知和执行已经接通；历史32表达与定向测试不能代替本轮全能力／双营地／全篇角色验收。目标是本机真实模型对正常中文、歧义、越权及长程事实的理解、执行和角色一致。

## 测试集与角色设计

CPU、Vulkan各固定60表达：40明确任务、10需澄清、10越权／不支持；覆盖数量、否定、换任务、别名、运输／采集／生产／照料、同行护送／猎鱼畜、已知与未知地点、双营地。请求文本、原始分类、可信证据、校验结论和实际世界结果分别保存，个人自由对话须经Owner同意后匿名，避免提交私密原文。

弟弟语言保持兄弟关系和生活语气，普通30—60字、复杂80—150字沿050，不用系统术语、无依据承诺、全知报点。记忆回答引用真实episode／证据，不将一次推断升级事实；亲见纠错、撤销约定、三日主动交流和通信距离按原批准规则。动态语音不纳入，文本角色质量由Owner审阅实际样本。

## 长程与边界

从首次采集委托、救援协作到种养／猎鱼、有限生产、四区夺回、两营地分工，保留真实任务链。穿插取消、覆盖、倒地、战斗、流送受阻、模型忙／失败、睡眠和保存退出回旧档。只用一个真实当前任务，已完成部分交付保持实际物资；拒绝恢复旧权限或跨epoch迟到结算。

保留锁定模型／运行库、3328／256预算、并发1和16层默认。先诊断检索、投影、模型分类、约束校验还是执行失败，再做所属模块最小修复；不以大模型／云服务／扩大token解决未经证实问题。

## 门槛与交付

沿ACCEPTANCE逐后端理解≥90%、澄清拒绝≥90%、明确端到端≥90%；另≥30可执行≥95%、≥20边界正确、复制物品及过期结算0。实际角色表达列通过／问题及Owner结论；不能用结构合法率代替角色自然度。报告暖／冷版本和模型可用性，性能完整采样归072。缺真人或任一后端结果时保留未完成，开发夹具与真实推理各自统计。

## 建议施工范围

- `docs/qa/TASK-068/`
- `docs/handoffs/TASK-068.md`
- `Source/Hearthward/AI/（有真实缺陷才登记具体文件）`
- `config/npc-agent.policy.json及Source/Hearthward/AI/HearthwardAgentContract.cpp（实际角色提示／能力）`
- `config/local-ai.lock.json（仅明确批准的版本修复）`
- `README.md`

Owner已确认本方案并授权施工；上述范围在任务激活时按准确文件登记到allowed_paths。契约沿用：[CT-002-companion-command](../../contracts/CT-002-companion-command.md)、[CT-TASK-050-companion-activities](../../contracts/CT-TASK-050-companion-activities.md)、[CT-TASK-052-clock-refresh](../../contracts/CT-TASK-052-clock-refresh.md)。Shared Save／gameplay.json／主地图变更先在所属单登记准确边界；不因列入建议路径而自动授权。
