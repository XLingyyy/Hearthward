# TASK-071 保存迁移只读调查

2026-10-04。Owner/Reviewer XLingyyy。依据批准 TASK-071、根当前 Save/Clock/AI/Nature 实现及根实际 native 结果。子代理未执行 UE/build/Git、未写原档或计算 checksum。本文区分真实旧档、测试 fixture 与未验证候选。

## 来源与实际结果

原工作目录 `Saved/SaveGames/HearthwardPrototype/pool.hws` 是实际存在的非 `test-*` 历史主池，大小 35,262 字节、HWS2 头、声明 payload 35,250 字节、GVAS，mtime 2026-09-27 23:07:37.507690。头部来源检查没有打印私人 payload；首次有限 tag 检查未发现显式 Schema/WriterVersion。当前尚无证据将它标记为手工 Demo 录制档。原目录其余 122 个 `test-*` 池属于生成测试文件，不能作真人旧档来源。

已有仓库历史 fixture `docs/qa/evidence/TASK-020/legacy-task019.hws` 与 `docs/qa/evidence/TASK-025/rev2/legacy-v1.hws` 都是已有 HWS1 实际二进制归档，现有 NPCMemoryCompatibility 测试引用；这些来源独立于本次生成测试。S2To3RealFileMigration / NaturalLegacyFile 等名称中的 RealFile 指落盘测试 fixture，不能替代真实历史池来源。

根复制真实 HWS2 主池到 `Saved/Task071/actual-legacy-source/pool.hws` 后，以原生 `Hearthward.Save.LegacyCampMigration` 外部参数读取副本。`integrated069-baseline071/index.json` 确认 Read 失败，返回身份、任务状态或跨系统关联 generic error。测试保留原副本字节，没有成功写出 migrated 文件。不得称其文件损坏，也不能强行接受。

同轮 Clock9OriginAndLegacyBoundary 的新增单一回归确认：显式 schema9/ClockV1/Origin(4,1200) payload 套 HWS7 被错误接受。根已登记并将现有显式 schema9 clock 保护由 HWS8 推广至所有非 HWS9 头；仍保留旧无 schema/无 clock 元数据的默认兼容逻辑。根后续 `diagnose055071-baseline066-compact068/index.json` 的 Clock9OriginAndLegacyBoundary 已通过。

## 当前迁移及真实失败定位

Decode 使用 UGameplayStatics::LoadGameFromMemory 标准 GVAS 反序列化，链式迁移 schema1→2→3→5→6→7→8→9；schema4 本项目不支持。迁移先校验生命、库存实例化、营地/救援身份、旧 field refresh 死亡时间，再进入现有完整 Validate。最终失败即通过 Diagnose 产生上述 generic error。Diagnose 未指出 clock、camp、nature、campaign、gameplay、库存或 NPCMemory 自身错误；仍须检查未被 Diagnose 单独覆盖的字段版本、任务关系、GUID/时间/跨域状态。

根后续实际 native 元数据诊断确认四个点的 raw Schema=9/writerAbsent，全部 SurvivalVersion=0、NPCStateVersion=2、ClockVersion=0/Origin(0,-1)。身份、记忆 campaign/有效性、知识计数/修订、任务 acquired/carried 关系、安全与 transform 布尔均合法。这确认 HWS2 省略旧 Schema 后读取为当前默认9，而现 fallback 直接设3，跳过了 schema2→3 认知迁移，最终严格 NPCStateVersion==3 条件拒绝。没有证据指向个人文件损坏。

已提供 `actual-legacy-metadata-diagnostic.patch`，基于根已有 Clock 红测的 SaveTests.cpp。仅现 LegacyCampMigration 的外部副本 Read 失败分支再次标准反序列化；输出 raw schema、writerPresent、各点 survival/NPC/clock 版本、origin缺失标记、数值时间、自然模式、phase、知识计数及身份/任务关系布尔。不输出 GUID、NPC 原文、item名称、JSON、payload、昵称路径。主 Read/原档字节保持断言不变。根已执行并取得上述事实；测试诊断不能代替真实迁移验收。已授权提供 legacy-default-schema-fix.patch：既有 HWS2/defaultSchema/allSurvival0 fallback 仅非空且全部 NPCStateVersion==2 时走2，其余保留3，mixed仍拒绝。现有迁移负责认知转换；同时用临时 CDO2 真实标准序列化省略 Schema、退出 scope恢复CDO9的 CI 回归保护此边界。未提交私人档，补丁待根构建及真实副本/CI定向验收。

## 持久字段与回调边界

目前已整合修改中识别到的新持久域字段是 NatureAnimal.Threat，使用现有 Nature JSON V1，缺省为 NAME_None，允许 player/brother；老 JSON 缺字段由结构默认 None 接收，不触发离线推进或发奖。Survival 恢复设施/epoch、Campaign 5A deadline、导航状态和自然动画动作字段为瞬态，不能据任务编号升级 schema。后续新增真实持久字段仍需在最终集成检查。

Save Restore 在提交实际 inventory/clock/transform/domain 前调用 Storage.AdvanceTimeline、LocalAI.ResetForSnapshot；Clock.Install 清理 receipt cache并安装原 A/W/Origin；Companion 活跃委托保留 CommandId、重新绑定新 epoch。Nature.Cancel、building取消、interaction清空、StopNavigation、Survival.ResetTransient 已存在。LocalAI.ResetForSnapshot 调用 CancelPending，清掉 Ticket/Proposal/待确认；取消约定等提交接口同时检查 expected epoch/revision/command/candidate。当前无证据确认旧回调污染新档。

已有 WorldClockTests 的 epoch 用例直接 AdvanceTimeline，尚未替代一次真实 Save.LoadPoint 后旧回调到达的跨系统验收。最小候选后续回归可复用当前真实世界 fixture，取得旧请求 token→实际保存/恢复→提交同旧 token，断言新 epoch 与完整状态保持；仅在根确认范围后新增。

Header/schema 完整对应关系亦未被当前仅 schema9 Clock guard 全覆盖。例如显式 schema8 payload 与 HWS7 封装是否被接受，需要独立真实 RED；不得用本轮一个 guard 宣称 TASK-071 全部完成。
