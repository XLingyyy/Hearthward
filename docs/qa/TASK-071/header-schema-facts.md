# TASK-071 Header/schema 已知事实

依据现有契约、已归档来源和现测试；不把 Magic 数字直接当作全部历史 schema 的一一映射。当前日期 2026-10-04，未执行新增 native 验证或修改 Decode。

| 外层 | 已有历史/测试支持事实 | 缺省 Schema 的现有兼容条件 | 当前需保护的边界 |
|---|---|---|---|
| HWS1 / 0x48575331 | CT-003/task016的 schema1；已有 task019/task025 HWS1 二进制归档；schema1→2 正式迁移 | 反序列化为当前9且有 NPCStateVersion==0 时置1；只允许该旧外层 | 不能误拒绝省略旧 class-default Schema 的真实归档 |
| HWS2 / 0x48575332 | CT-003明确 schema2 写新外层；task040 schema2→3 文件迁移沿同HWS2外层；SaveTests明确schema2与schema3旧自然池 | 当前9且所有 SurvivalVersion==0 时置3 | 外层2曾包含2、3，不能固定只认schema2；真实35,262字节pool副本native已确认省略Schema/default9且四点NPC版本2，必须走既有schema2认知迁移 |
| HWS5 / 0x48575335 | EquipmentProgressionTests明确schema5旧库存fixture，正式5→6迁移 | 当前9且无显式新Clock元数据时置5 | 明确其他新schema套该旧外层不能当正常旧档迁移 |
| HWS6 / 0x48575336 | CT-TASK047 schema6；NatureTests明确schema6落盘fixture，再迁移7 | 当前9且无显式新Clock元数据时置6 | 保留省略旧默认Schema兼容 |
| HWS7 / 0x48575337 | CT-TASK048 schema7；CampaignTests明确schema7→8 fixture | 当前9且无显式新Clock元数据时置7 | 明确schema8套HWS7新增两行独立RED；当前静态路径直接8→9，须native确认 |
| HWS8 / 0x48575338 | CT-TASK049 schema8；CT-TASK052明确schema8/HWS8；现Clock测试实际迁移正控制 | 当前9且无显式新Clock元数据时置8 | 显式ClockV1/schema9必须拒绝；HWS8原有保护保留 |
| HWS9 / 0x48575339 | CT-TASK052当前唯一写出9；ClockV1与Origin强校验 | 无旧默认降级；缺Clock元数据拒绝 | 旧schema不能用当前外层套壳后自动升级 |
| HWS4、未知/未来 | 052契约明示未集成HWS4及未来拒绝 | 无 | 原档保留，不虚构迁移 |

“缺省 Schema”与“明确 Schema”必须区分。现 FHWorldSave 的加载 sentinel 为 ClockVersion=0/InitialDay=0/InitialMinute=-1；旧无Clock metadata文件可采用既有缺省兼容。单凭加载的 Pool.Schema==9 不能确认历史文件明确写过9，因为旧class-default Schema被省略后会取当前默认。原生 UGameplayStatics 负责反序列化，禁止为兼容另造tag解析器。

当前 Decode 的迁移分支多使用 Header!=HWS9，而非每个明确历史 schema/header组合匹配，所以代码能够接受的组合比已有契约/来源支持范围更宽。该事实不意味着全部组合可立刻改拒绝；需要按真实RED、历史来源和兼容正控制限定修复。根本轮已最小保护显式schema9 Clock元数据与所有旧外层；不会据这一保护认定其余完整组合验证已完工。

下一独立RED是 `explicit-schema8-envelope-red.patch`：复用现 Clock9OriginAndLegacyBoundary 的合法schema8/HWS8正控制；保留同一payload、死亡due10080及Clock sentinel，只换HWS7头，断言拒绝。新增两行在due4000坏数据前，防止用另一失效domain产生伪RED。
