# TASK-045 文档验证

日期2026-09-26。基线7ce8262bd8721dc59fb0703d365dab2e10340f89；本单只改文档。最终提交以同分支Git记录为准。

核对来源：原稿TASK-047、审计战斗／潜入条目、GDD v0.3第7—8章和Q编号、043／044批准增量；检查Gameplay现有命中／护甲／投掷／重击实现。识别归档击晕苏醒规则已被用户决定覆盖，不恢复它。

设计检查：D1—D6全部标PROPOSED；C01—C20仅验收输入，运行NOT_RUN。候选参数、相对伤害标尺、后续正式装备经济表分开；未推断普通敌人固定100血。动作以A为时钟，生存伤害统一交给044；尸体奖励和043生命代次衔接。工具自测33/33 PASS；范围登记基线a9e0666f35521fbd02b59aab107a4fd41d9c31d1。初次自检因稀疏检出缺少原有链接文件、主干尚无045范围快照失败；补齐已有路径检出并提交范围登记后重检。

命令：

- `python -X utf8 scripts/validate_repo.py --task TASK-045 --base a9e0666f35521fbd02b59aab107a4fd41d9c31d1`
- `python -X utf8 -m unittest discover -s scripts/tests -v`
- `git diff --check`

UE构建／自动化／PIE／Shipping均NOT_RUN。无资产改写，无LFS锁变更。参考Epic官方AI Perception及Animation Notifies文档，链接在设计正文；只借鉴工程机制，不将引擎文档当成本游戏数值依据。

最终结果：范围／结构自检PASS，0错误；工具33/33 PASS；git diff --check PASS。检查范围内无Source／Content／Config／Resources改动。
