# TASK-101 技术回归记录

当前为Active、部分技术回归已实际完成。参考完整HEAD `6fcf5c22e965f0f7409438f19bc7b09e96ffb058`，分支 `codex/TASK-084-103-iteration` 加084—103批次本地未提交修改；root统一冻结/编译本轮源并记录dirty清单。candidate4 `.20261007.3` 已实际完成原档副本的最新一个旧节点加载、隔离保存和第二独立进程明确Continue；正式四阶段正常路线尚未到达全部节点，不能声明完整集成通过。

## 原档保护

root已将用户原档只读复制到 `.agent-local/qa/TASK-101/20261007-original-copy/pool.hws`，清单为同目录 `originals.json`。原路径 `Saved/SaveGames/HearthwardPrototype/pool.hws`，35262 bytes；任务明确要求的SHA256已计算一次并登记 `93974876b7ef8347e79e743d9ce725367a001b04297c5b8290cdb0fc4454266a`，本代理没有重复hash/写原件/删除档池。原只读copy继续保留；candidate-2/version.1及当前candidate4/version.3分别在独立UserDir的再复制件上完成部分读取、保存和明确Continue，两个版本结果分别记录。candidate4原件未写、只读copy大小/mtime未变、共享launcher profile未触，无新hash。

当前源的写schema9/NPCState3以及实际历史包头边界已只读确认。本单未改Save源码、字段、版本、迁移政策、时钟规则或共享UI源。资产包/LFS与GGUF未写、模型指纹沿锁定记录，不重复重算模型hash。

## candidate4/version.3 最新旧节点的实际兼容与重启

Root实际运行 `iteration-084-103-20261007-4`，标题版本 `0.2.0-preview.20261007.3`；独立UserDir含中文和空格，只接收原只读copy的再复制件。公开归档为[安全结果摘要](OS_CANDIDATE4_ORIGINAL_COPY_COMPATIBILITY.json)，原始结果/进程/截图保留于 `.agent-local/qa/TASK-101/os-original-copy-candidate4-20261007/`。当前实际目录和原results均列6张游戏截图；未复制个人save、raw profile或完整启动命令到docs。

- 原4个旧自动节点可见，仅实际选择最新 `2026-09-27 15:07`，通过正常Load与明确确认恢复室外夜间、main02“把营地安顿下来”。观察生命100/100、饥饿99/100、体力100/100、弟弟原地等待，四快捷栏各0，营地指引约1243米。
- 实际手动保存使节点4→5，隔离Compatible-v8目录创建394263 bytes的pool。该目录名称和大小不独立证明schema；Save生产与迁移规则未改。
- 第二独立进程的公开 `runtime.launch_packaged` 回执为ok=true；显式鼠标“继续游戏”恢复同室外场景、任务、100生命和空快捷栏。F6仍为5节点（新增01:26手动节点与原4个日期），没有新增新游戏自动节点。两次AltF4后各自child与bootstrap均已退出。
- 首次公开launch已实际成功，回执处理调用dict.to_dict报错；Root以真实CIM进程与OS标题补证，没有为恢复元数据重复启动。这是保留的运行器回执错误，不当作存档或游戏启动失败抹去。

本轮只验证最新一个旧节点、保存后跨进程Continue和上述可见字段；全部4旧节点逐一加载、完整字段快照等值、四个新自然路线检查点、实际HTTP跨Load晚回复、实体二机和完整集成仍NOT_RUN。没有把同一物理机器的两个独立进程计作二机，也未触原UserSave或共享候选launcher profile。下节candidate-2历史结果继续绑定其原包，不与本轮节点或文件大小合并。

## candidate-2/version.1 历史Shipping原档副本兼容与明确Continue

root在已构建历史候选 `iteration-084-103-20261007-2`、标题版本 `0.2.0-preview.20261007.1` 上实际使用Windows Run直接exe与OS键鼠。隔离UserDir只接收先前只读pool的另一个复制件；用户原件及只读copy未写，不重复hash。[结果及12张实际游戏窗口截图索引](OS_ORIGINAL_COPY_COMPATIBILITY.json) 绑定 [原始result副本](os-original-copy-20261007/result.json)，私有原始结果仍留 `.agent-local/qa/TASK-101/os-original-copy-20261007/`。

- 列表显示原4个locked自动节点；只实际选取最新 `2026-09-27 15:07` 节点并明确同意丢弃未保存进度。恢复到室外夜间、主线02营地准备阶段，玩家与弟弟可见，生命100，快捷物品数量为空，营地目的地约1243米。没有对4个节点逐一做完整字段等值检查。
- F6“保存当前”新增手动节点，总数5；隔离legacy输入及其backup各35262 bytes，写出的隔离Compatible-v8目录pool为660502 bytes。原始保存/迁移代码未改，目录名不用于推断schema。
- 第二独立进程按Return命中标题默认“新游戏”，新增第6个序章自动节点。这是已保留的测试操作错误，撤回该操作的Continue声明；未作为存档缺陷处理。随后明确加载先前营地准备手动节点，确认恢复，再保存为最新营地手动节点（`2026-10-06 22:08 UTC`），总数7并正常退出。
- 第三独立进程在标题页明确鼠标点击“继续游戏”，恢复相同室外营地准备阶段、玩家/弟弟、100生命及空快捷物品；F6列表仍7个节点，没有新增新游戏自动节点。此操作才计作明确Continue的部分证据。

这组历史证据仅验证candidate-2/version.1的原档副本可读取、隔离保存和单一已加载阶段的跨进程Continue。记录当时AI/音频源码继续变化、version.2/candidate-3尚未构建；后续当前candidate4结果已另列。四个新正常路线节点独立重启、全部原节点逐字段等值、实际HTTP跨Load晚回复、IME/其余菜单与完整新暂态组合仍NOT_RUN。

## 最小可运行回归

7条既有实际Save/Time/Input过滤器及执行边界见 [RUN.md](RUN.md)。字段和显示暂态逐项见 [FIELD_MATRIX.md](FIELD_MATRIX.md)。没有因任务名而运行整个Save套件，也没有新增镜像实现的测试。root本轮构建开始后保持源冻结；root最终整合Development Editor build成功，同版7/7 Success、0错误，其中LoadingRestoresLatestPageInputMode有1条EnhancedInput警告，其他6条0警告。原联合index为 .agent-local/qa/TASK-088-101/native-integration-20261007/index.json，保留 [7项原生子集](native-green-20261007.json)。警告原文：UEnhancedInputLocalPlayerSubsystem for Local Player LocalPlayer_7 does not have a valid PlayerInput object. Failed to load user settings. 这是现有夹具记录，保留而未屏蔽，不计真实OS输入通过。

|用例|本轮可执行部分|当前结果|
|---|---|---|
|C01 字段恢复|原生已支持字段；candidate4 Shipping原4旧节点可见并实际加载最新一个|原生及历史包分别绑定原报告；candidate4营地准备/生命100/空快捷物品部分恢复及重启已实测，四个新阶段及完整字段等值NOT_RUN|
|C02 旧响应|ActualLoadPoint废弃真实旧Companion票据；087请求序号/取消夹具同版回归|原生旧票据Success；真实HTTP跨Load晚回复NOT_RUN|
|C03 货物生产|PartialMaterialSettlement真实读档与Camp睡眠分段/恢复|原生子范围Success；真实返营携货节点未录制|
|C04 输入恢复|UI069模式回归；candidate4 Shipping正常Load确认、F6保存和第二独立进程显式鼠标Continue|原生Success含1历史警告；candidate4上述OS部分已验，完整设置Apply/其余菜单/IME NOT_RUN|
|C05 时间推进|DomainPartitionRefreshAndEpoch真实子系统与Camp睡眠分段|原生分段/epoch Success；正常暂停/对话/设施8小时仍NOT_RUN|
|C06 重复事务|PartialSettlement/Domain requests及本批89/93/99消费者旧epoch局部回归|本单PartialSettlement/Domain子范围Success；093两项首轮Fail保留，修正夹具焦点后定向两项Success（合计2警告/0错误）；完整正常路线NOT_RUN|
|C07 兼容拒绝|FileIntegrity/Compatibility原生；candidate4原pool只读copy的隔离再复制件实际读取保存|原生及candidate-2历史结果保留；candidate4最新旧节点部分兼容已验，原件未写/只读copy stat未变，无新hash；全部旧节点/未来损坏档本包完整复验未完成|
|C08 暂态清理|源码只读确认Tracker、操作卡、回营反馈与音源不新增保存字段/读取epoch|ActualLoad→新UI组合集成NOT_RUN，不拿epoch诊断代替|

093独立回归补充：首轮LocalFacilitiesAndWork与UpgradePreviewAndReplay因非Focusable夹具焦点报错Fail，历史 `.agent-local/qa/TASK-088-101/native-integration-20261007/index.json` 保留。夹具补SetIsFocusable(true)后，主代理实际定向两项Success，合计2条Standalone EnhancedInput warning、0错误，最新原index `.agent-local/qa/TASK-090-100/native-red-20261007/index.json`（目录名不代表093结果），子集见 `docs/qa/TASK-093/native-correctedfixture-20261007.json`。此为093独立证据，不增加本单7条分母，不代表ActualLoad→新UI组合或四阶段完成。

原生通过后保留实际warning/error、完整原联合index与本单子集。candidate4与candidate-2 Shipping真实OS子范围分别单列；其余输入/渲染、实际HTTP跨Load、四阶段独立进程重启、同版完整Shipping集成、二机与人工继续单列，不由测试夹具或单一旧节点替代。

范围基线检查NOT_RUN：没有含本单获批快照的实际提交，当前HEAD为参考基线加dirty；不更改验证器。README由root串行收尾。未提交／未推送／未发布。
