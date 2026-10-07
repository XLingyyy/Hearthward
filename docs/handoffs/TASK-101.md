# TASK-101 当前技术回归交接

[任务单](../tasks/TASK-101.md) · [报告](../qa/TASK-101/REPORT.md) · [字段矩阵](../qa/TASK-101/FIELD_MATRIX.md) · [最小运行集合](../qa/TASK-101/RUN.md)

实际根目录G:/GameFactory/Hearthward，分支codex/TASK-084-103-iteration，参考完整HEAD 6fcf5c22e965f0f7409438f19bc7b09e96ffb058 加084—103批次未提交改动。用户授权逐单本地实施，root委派独立技术回归并统一冻结/编译/运行UE；Owner XLingyyy，Reviewer未指派。当前candidate4/version.3已取得最新一个旧副本节点加载、隔离保存和跨进程明确Continue证据，整任务保持Active。

## 已完成的可执行准备

root已将原用户pool.hws只读复制至 .agent-local/qa/TASK-101/20261007-original-copy/ 并生成 originals.json，35262 bytes；本单要求的原件SHA已经一次登记，本代理不重复hash、不写原件、不删除档池。candidate4/version.3及candidate-2/version.1分别在只读copy的隔离再复制件上取得部分兼容证据；原件未写、只读copy大小/mtime未变，共享launcher profile未触。

只读核对当前Save schema9/NPCState3、实际旧包头迁移与未来/损坏拒绝。新增FIELD_MATRIX逐项绑定现有持久字段和本批暂态；新增RUN给出7条直接相关Save/Time/Input过滤器，预期发现7项，每轮新HearthwardSaveTestPool UUID。没有新增镜像实现的测试，没有修改任何Save/Time/UI源码、格式、版本或正式规则。

## candidate4/version.3 最新旧副本节点与跨进程Continue

最新安全摘要见[OS_CANDIDATE4_ORIGINAL_COPY_COMPATIBILITY.json](../qa/TASK-101/OS_CANDIDATE4_ORIGINAL_COPY_COMPATIBILITY.json)。实际Shipping标题.3；中文空格独立UserDir只接收readonlycopy的再复制件，原4旧节点可见，仅加载最新2026-09-27 15:07并正常确认。室外夜main02/HP100/food99/stamina100/弟弟等待/四快捷栏0/营地1243米实际可见；手动保存4→5，隔离Compatible-v8 pool为394263B，不据目录名推断schema。

第二独立public API launch回执成功，正常标题明确鼠标Continue恢复同场景/任务/生命/空快捷栏，F6仍5（01:26手动＋四旧节点），无新增新游戏自动档；两轮AltF4均确认child与bootstrap退出。首次launch已成但dict.to_dict回执处理失败，真实CIM+OS标题补证且未重复启动，原错误完整保留。

原始结果和当前实存6张截图留 `.agent-local/qa/TASK-101/os-original-copy-candidate4-20261007/`；docs仅安全summary，不复制个人save或raw profile。原件未写、immutable copy stat未变，无新hash，原UserSave和共享launcher profile均未触。只取得最新一个旧节点和可见字段子范围；4旧节点全加载/完整字段等值、4新自然路线checkpoint、实际HTTP跨Load与实体二机仍NOT_RUN。

## candidate-2/version.1 历史Shipping原档副本与明确Continue

root在已构建历史候选 `iteration-084-103-20261007-2`、标题版本 `0.2.0-preview.20261007.1` 上实际使用Windows Run直接exe与OS键鼠。隔离UserDir只接收先前只读pool的另一个复制件；用户原件及只读copy未写，不重复hash。[结果及12张实际游戏窗口截图索引](../qa/TASK-101/OS_ORIGINAL_COPY_COMPATIBILITY.json) 绑定 [原始result副本](../qa/TASK-101/os-original-copy-20261007/result.json)，私有原始结果仍留 `.agent-local/qa/TASK-101/os-original-copy-20261007/`。

- 列表显示原4个locked自动节点；只实际选取最新 `2026-09-27 15:07` 节点并明确同意丢弃未保存进度。恢复到室外夜间、主线02营地准备阶段，玩家与弟弟可见，生命100，快捷物品数量为空，营地目的地约1243米。没有对4个节点逐一做完整字段等值检查。
- F6“保存当前”新增手动节点，总数5；隔离legacy输入及其backup各35262 bytes，写出的隔离Compatible-v8目录pool为660502 bytes。原始保存/迁移代码未改，目录名不用于推断schema。
- 第二独立进程按Return命中标题默认“新游戏”，新增第6个序章自动节点。这是已保留的测试操作错误，撤回该操作的Continue声明；未作为存档缺陷处理。随后明确加载先前营地准备手动节点，确认恢复，再保存为最新营地手动节点（`2026-10-06 22:08 UTC`），总数7并正常退出。
- 第三独立进程在标题页明确鼠标点击“继续游戏”，恢复相同室外营地准备阶段、玩家/弟弟、100生命及空快捷物品；F6列表仍7个节点，没有新增新游戏自动节点。此操作才计作明确Continue的部分证据。

这组历史证据仅验证candidate-2/version.1的原档副本可读取、隔离保存和单一已加载阶段的跨进程Continue。该记录时version.2/candidate-3尚未构建，当前candidate4部分兼容证据已另列。四个新正常路线节点独立重启、全部原节点逐字段等值、实际HTTP跨Load晚回复、IME/其余菜单与完整新暂态组合仍NOT_RUN。

## 运行与剩余边界

本轮build期间保持编译源冻结；root最终整合build成功，7项实际同版Success、0错误，LoadingRestoresLatestPageInputMode有1条现有EnhancedInput警告；其余6条0警告，保留docs/qa/TASK-101/native-green-20261007.json。当前ActualLoad废弃旧Companion票据与材料结算、文件拒绝/兼容、世界域刷新和Loading当前模式可执行；正常路线四阶段独立进程重启、完整货物/人口/区域/两营地、其余真实OS设置/快捷菜单/IME和新暂态ActualLoad→UI组合仍NOT_RUN；上述F6/加载确认/明确Continue已部分实测。不能拿仅AdvanceTimeline或helper诊断替代真实Load/UI组合。

085—099局部结果保留各单原报告；新回归必须同整合版本绑定，不能拼不同版本最佳成绩。README由root统一收尾；完整集成、同版完整Shipping回归、二机、人工与发布未验收。未提交／未推送／未合并／未发布；任务Active。
